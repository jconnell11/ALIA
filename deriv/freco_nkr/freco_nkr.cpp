// freco_nkr.cpp : get a 256 feature vector from a 100x100 RGB face
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2025 Etaoin Systems
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
///////////////////////////////////////////////////////////////////////////

#ifndef __linux__
  #include <windows.h>                 // needed for resource access
  #include "resource.h"                // needed for IDR_FRECO_NET and IDR_FRECO_WTS

  // In Project/Properties/Linker/Additional Library Directories:
  //   add "../../OpenCV 4.10.0/build/x64/vc16/lib"
  // NOTE: project should be compiled as /MT (multi-threaded) with MFC in a static lib also
  #pragma comment(lib, "opencv_world4100.lib")

#else
  #include <stdint.h>

  // NOTE: compile remotely on various platforms since different versions of OpenCV !
  // Use Ubuntu command line box (WSL) to add proper ELF header (cached copies in res/):
  //   cd /usr/share
  //   sudo cp /mnt/c/user/code/deriv/freco_nkr/res/freco_nkr.net freco.net
  //   sudo cp /mnt/c/user/code/deriv/freco_nkr/res/freco_nkr.wts freco.wts
  //   sudo ld -r -b binary -o freco.net.x64 freco.net
  //   sudo ld -r -b binary -o freco.wts.x64 freco.wts
  //   sudo aarch64-linux-gnu-ld -r -b binary -o freco.net.arm64 freco.net
  //   sudo aarch64-linux-gnu-ld -r -b binary -o freco.wts.arm64 freco.wts
  //   cp freco.*.* /mnt/c/user/code/deriv/freco_nkr/res
  extern const uint8_t ndata[]  asm("_binary_freco_net_start");
  extern const uint8_t nstop[]  asm("_binary_freco_net_end");
  extern const uint8_t wdata[]  asm("_binary_freco_wts_start");
  extern const uint8_t wstop[]  asm("_binary_freco_wts_end");

#endif

#include <stdio.h>

#include <opencv2/dnn.hpp>             // ignore any conversion warnings
#include <opencv2/core/cuda.hpp>

#include "jhc_pthread.h"               

#include "Data/jhcImg.h"               // common video
#include "Processing/jhcResize.h"

#include "Face/freco_nkr.h"


// local function prototypes
pthread_ret get_feat (void *arg =NULL);
void extract_network ();


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= Version information string.

static char info[80];


//= Handle for background thread.

static pthread_t backg;


//= Whether background thread is active.

static int running = 0;


///////////////////////////////////////////////////////////////////////////

//= DNN that will be constructed.

static cv::dnn::Net DNN;


//= DNN loaded status (2 = GPU mode).

static int nok = -1;


//= Square 100x100 image forming actual input to DNN.

static cv::Mat sq100 = cv::Mat(100, 100, CV_8UC3, cv::Scalar(0, 0, 0));


//= Most recent 256 element feature vector from network.

static cv::Mat output = cv::Mat(1, 256, CV_32F); 


///////////////////////////////////////////////////////////////////////////

//= Most recent cropped face image for DNN and mugshot.

static jhcImg crop, crop2;


//= Utility functions for downsampling face images.

static jhcResize rsz;


//= Amount to add to each side of the face bounding box in mugshot.

static double pad = 0.25;


///////////////////////////////////////////////////////////////////////////
//                      Initialization and Locking                       //
///////////////////////////////////////////////////////////////////////////

#ifndef __linux__

//= Global variable with handle to current module.

static void *mod;


//= Entry point for DLL setup.

BOOL APIENTRY DllMain (HANDLE hModule,
                       DWORD ul_reason_for_call, 
                       LPVOID lpReserved)
{
  // clean up on exit
  if (ul_reason_for_call == DLL_PROCESS_DETACH)
    return TRUE;
  if (ul_reason_for_call != DLL_PROCESS_ATTACH)
    return TRUE;

  // system initialization
  mod = (void *) hModule;
  return TRUE;
}

#endif

///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= String with version number of DLL and possibly other information.

extern "C" DEXP_R const char *freco_version ()
{
  sprintf_s(info, "freco_nkr %4.2f - using OpenCV 4.10.0", 3.00);
  return info;
}


//= Sets up DNN for processing a series of cropped mugshots.
// if file name is NULL then default values will be used
// can request either main CPU or GPU (if compatible)
// returns 2 if running GPU mode, 1 for CPU mode, 0 or negative for failure

extern "C" DEXP_R int freco_setup (const char *fname, int gpu)
{
  // only initialize once (cannot change DNN)
  if (nok > 0)
    return nok;

  // unpack resources to standard files then use to configure DNN
  nok = 0;
  extract_network();
  DNN = cv::dnn::readNetFromCaffe("freco_nkr.net", "freco_nkr.wts");
  remove("freco_nkr.net");
  remove("freco_nkr.wts");
  if (DNN.empty())
    return nok;

  // enable GPU (if available) and configure for CUDA
  nok = 1;
  if ((gpu > 0) && (cv::cuda::getCudaEnabledDeviceCount() > 0))
  {
    DNN.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
    DNN.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    nok = 2;
  }
  return nok;
}


//= Loads comparison metric derived from training samples.
// returns 1 if successful, 0 or negative for error 

extern "C" DEXP_R int freco_metric (const char *fname)
{
  // fixed comparison metric for DNN
  return 0;
}


//= Start the face recognition system for training or identification.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (not required)
// does the time-consuming loading of GPU once ahead of time
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP_R int freco_start (int level, const char *log_file)
{
  if (nok >= 2)
  {
    printf("freco_nkr: Initializing DNN (can be slow) ...\n");
    cv::Mat blob = cv::dnn::blobFromImage(sq100);
    DNN.setInput(blob, "data");
    output = DNN.forward();
    printf("freco_nkr: Face recognition ready\n");
  }
  return 1;
}


//= Call at end of run to close log file or before loading a new configuration.

extern "C" DEXP_R void freco_done ()
{
  // ignored
}


//= Releases any allocated resources (call at end of run).

extern "C" DEXP_R void freco_cleanup ()
{
  abstime_t one_sec;

  // stop any background analysis in progress
  if (running > 0)
    pthread_timedjoin_np(backg, 0, abstime_wait(&one_sec, 1000));
  running = 0;
  pthread_detach(backg);

  // deallocate DNN (important if GPU-based)
  DNN = cv::dnn::Net();
}


///////////////////////////////////////////////////////////////////////////
//                          Mugshot Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Returns width of representative color face image.

extern "C" DEXP_R int freco_mug_w ()
{
  return ROUND((1.0 + 2.0 * pad) * 100.0);
}


//= Returns height of representative color face image.

extern "C" DEXP_R int freco_mug_h ()
{
  return ROUND((1.0 + 2.0 * pad) * 100.0);
}


//= Convert input region of interest into normalized color face image.
// mug is pointer to output image buffer of size freco_mug_w x freco_mug_h x 3
// src is bottom-up BGR image with lines padded to 4 byte boundaries
// iw is image width, ih is image height, assumes 3 fields (color)
// face bounding box has lf <= x <= rt and bot <= y <= top in pixels
// potentially rotates and enhances some portion of input image
// returns 1 if successful, 0 or negative if error

extern "C" DEXP_R int freco_mug (unsigned char *mug, 
                                 const unsigned char *src, int iw, int ih, 
                                 int lf, int rt, int bot, int top)
{
  jhcImg full, face;
  int w = rt - lf + 1, h = top - bot + 1;

  // sanity check
  if ((src == NULL) || (iw <= 0) || (ih <= 0) || (lf < 0) || (bot < 0) || 
      (rt < lf) || (rt >= iw) || (top < bot) || (top >= ih))
    return 0;

  // get cropped version of face then resample into mugshot
  full.Wrap((UC8 *) src, iw, ih, 3);
  crop2.SetSize(ROUND(w + 2.0 * pad * w), ROUND(h + 2.0 * pad * h), 3);
  rsz.Extract(crop2, full, ROUND(lf - pad * w), ROUND(bot - pad * h));

  // resample into mugshot
  face.Wrap((UC8 *) mug, freco_mug_w(), freco_mug_h(), 3);
  rsz.Bicubic(face, crop2, 1);
  return 1;
}


//= Reports coordinates of left eye (wrt person) in original input image.
// returns 1 if found, 0 if missing or not computed

extern "C" DEXP_R int freco_eye_lf (double& x, double& y)
{
  // not needed for DNN
  return 0;
}


//= Reports coordinates of right eye (wrt person) in original input image.
// returns 1 if found, 0 if missing or not computed

extern "C" DEXP_R int freco_eye_rt (double& x, double& y)
{
  // not needed for DNN
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                         Signature Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Returns the number of elements in an example signature vector. 

extern "C" DEXP_R int freco_vsize ()
{
  // output size of DNN
  return 256;
}


//= Analyze "src" image to generate "feat" vector (blocks until complete).
// source image is bottom-up 8 bit BGR order with width = iw, height = ih
// face bounding box has lf <= x <= rt and bot <= y <= top in pixels
// passed feature array must have 256 elements for DNN output
// must call freco_setup before (even with default parameters)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP_R int freco_vect (double *feat, const unsigned char *src, 
                                  int iw, int ih, int lf, int rt, int bot, int top)
{
  jhcImg full;
  int i;

  // sanity check
  if (DNN.empty())
    return -4;
  if ((src == NULL) || (iw <= 0) || (ih <= 0) || (lf < 0) || (bot < 0) || 
      (rt < lf) || (rt >= iw) || (top < bot) || (top >= ih))
    return -3;
  if (feat == NULL)
    return -2;

  // get cropped version of face for DNN input
  full.Wrap((UC8 *) src, iw, ih, 3);
  crop.SetSize(rt - lf + 1, top - bot + 1, 3);
  rsz.Extract(crop, full, lf, bot);

  // run DNN to get output vector then copy to supplied array
  get_feat();
  for (i = 0; i < 256; i++)
    feat[i] = (double) output.at<float>(i);
  return 1;
}


//= Request that a face in an image be analyzed in the background.
// source image is bottom-up 8 bit BGR order with width = iw, height = ih
// face bounding box has lf <= x <= rt and bot <= y <= top in pixels
// will cancel any other request that might be in progress
// must call freco_setup before (even with default parameters)
// typically 0.3 ms on 2.7 GHz CPU - src image can change after return
// returns 1 if successfully queued, 0 or negative for some error

extern "C" DEXP_R int freco_submit (const unsigned char *src, int iw, int ih, 
                                    int lf, int rt, int bot, int top)
{  
  jhcImg full;

  // make sure nothing in progress
  if (running > 0)
    if (pthread_busy(backg))
      return 0;

  // get cropped version of face for DNN input
  full.Wrap((UC8 *) src, iw, ih, 3);
  crop.SetSize(rt - lf + 1, top - bot + 1, 3);
  rsz.Extract(crop, full, lf, bot);

  // start DNN in background thread
  running = 1;
  pthread_create(&backg, 0, get_feat, NULL);
  return 1;
}


//= Wait a certain amount to see if face analysis request has completed.
// passed feature array must have 256 elements for DNN output
// returns 1 if done (and sets vector), 0 if not ready yet, negative for error

extern "C" DEXP_R int freco_check (double *feat, int ms)
{
  abstime_t wait;
  int i;

  // see if background thread is finished
  if (running > 0)
    if (pthread_timedjoin_np(backg, 0, abstime_wait(&wait, ms)) != 0)
      return 0;
  running = 0;

  // copy output vector to supplied array
  for (i = 0; i < 256; i++)
    feat[i] = (double) output.at<float>(i);
  return 1;
}


//= Computes a distance between two signature vectors (small is good).
// based on average of cosine distance and normalized correlation
// passed arrays must have 256 elements each
// return value between 0 and 1 (ID threshold around 0.2)

extern "C" DEXP_R double freco_dist (const double *f1, const double *f2)
{
  double sum12 = 0.0, sum11 = 0.0, sum22 = 0.0, sum1 = 0.0, sum2 = 0.0;
  double v1, v2, nc, cd, n = 256.0;
  int i;

  // sanity check
  if ((f1 == NULL) || (f2 == NULL))
    return 0.0;

  // accumulate sums for moments
  for (i = 0; i < 256; i++)
  {
    v1 = f1[i];
    v2 = f2[i];
    sum1  += v1;
    sum2  += v2;
    sum11 += (v1 * v1);
    sum22 += (v2 * v2);
    sum12 += (v1 * v2);
  }

  // combine cosine distance and normalized correlation
  cd = sum12 / sqrt(sum11 * sum22);
  cd = __max(-1.0, __min(cd, 1.0));
  nc = (n * sum12 - sum1 * sum2) / sqrt((n * sum11 - sum1 * sum1) * (n * sum22 - sum2 * sum2));
  nc = __max(-1.0, __min(nc, 1.0));
  return(0.5 - 0.25 * (cd + nc));
}


///////////////////////////////////////////////////////////////////////////
//                          Background Thread                            //
///////////////////////////////////////////////////////////////////////////

//= Process "crop" image to generate "output" feature vector

pthread_ret get_feat (void *arg)
{
  jhcImg sqr;
  cv::Mat blob;

  // resize cropped face image to OpenCV 100x100 BGR
  sqr.Wrap(sq100.data, 100, 100, 3);
  rsz.Bicubic(sqr, crop, 1);
  rsz.FlipV(sqr);

  // run sq100 thru net as "blob" to get output 
  blob = cv::dnn::blobFromImage(sq100);
  DNN.setInput(blob, "data");
  output = DNN.forward();
  return 0;
}


#ifndef __linux__

//= Retrieve ".caffemodel" to "freco_nkr.net" and ".prototxt" to "freco_nkr.wts".

void extract_network ()
{
  HMODULE hmod = (HMODULE) mod;
  HRSRC rsrc;
  HGLOBAL hres;
  FILE *out;

  // get network structure definition
  rsrc = FindResource(hmod, MAKEINTRESOURCE(IDR_FACE_NET), RT_RCDATA);
  if (rsrc != NULL)
    if ((hres = LoadResource(hmod, rsrc)) != NULL)
      if (fopen_s(&out, "freco_nkr.net", "w") == 0)
      {
        fwrite(LockResource(hres), 1, SizeofResource(hmod, rsrc), out);
        fclose(out);
      }

  // get weights for network
  rsrc = FindResource(hmod, MAKEINTRESOURCE(IDR_FACE_WTS), RT_RCDATA);
  if (rsrc != NULL)
    if ((hres = LoadResource(hmod, rsrc)) != NULL)
      if (fopen_s(&out, "freco_nkr.wts", "wb") == 0)
      {
        fwrite(LockResource(hres), 1, SizeofResource(hmod, rsrc), out);
        fclose(out);
      }
}

#else

//= Retrieve ".caffemodel" to "freco_nkr.net" and ".prototxt" to "freco_nkr.wts".

void extract_network ()
{
  FILE *out;

  // get network structure defintion
  if ((out = fopen("freco_nkr.net", "wb")) != NULL)
  {
    fwrite(ndata, 1, nstop - ndata, out);
    fclose(out);
  }

  // get weights for network
  if ((out = fopen("freco_nkr.wts", "wb")) != NULL)
  {
    fwrite(wdata, 1, wstop - wdata, out);
    fclose(out);
  }
}

#endif
