// jhcFFindOCV.cpp : wrapper for OpenCV face finding
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2018 IBM Corporation
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

// Uses OpenCV 2.4.5 (set in Properties: C++/Additional Dirs and Linker/Additional Dirs)
// Build.Settings.Link.General must have OpenCV libraries core, imgproc, objdetect, and zlib
// NOTE: project must be compiled as /MT (multi-threaded) with MFC in a static lib also
#ifdef _DEBUG
  #pragma comment(lib, "zlibd.lib")
  #pragma comment(lib, "opencv_core245d.lib")
  #pragma comment(lib, "opencv_imgproc245d.lib")
  #pragma comment(lib, "opencv_objdetect245d.lib")
#else
  #pragma comment(lib, "zlib.lib")
  #pragma comment(lib, "opencv_core245.lib")
  #pragma comment(lib, "opencv_imgproc245.lib")
  #pragma comment(lib, "opencv_objdetect245.lib")
#endif


#include <windows.h>         // needed for resource access
#include "resource.h"        // needed for IDR_FACE_XML

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "Face/jhcFFindOCV.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Destruction                        //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcFFindOCV::jhcFFindOCV ()
{
  cv::CascadeClassifier *cascade;

  // current code version
  ver = 1.02;

  // assume module is current executable
  hmod = NULL;

  // create the classifier object
  cascade = new cv::CascadeClassifier;
  cc = (void * ) cascade;
  *cname = '\0';
  ok = 0;

  // set processing parameters
  SetCascade(IDR_FACE_XML);         // omit if cascade not a resource
  Defaults();
  nface = 0;
}


//= Default destructor does necessary cleanup.

jhcFFindOCV::~jhcFFindOCV ()
{
  cv::CascadeClassifier *cascade = (cv::CascadeClassifier *) cc;

  if (cascade != NULL)
    delete cascade;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.
// also need to call SetCascade before system is operational

int jhcFFindOCV::Defaults (const char *fname)
{
  int ok = 1;

  ok &= find_params(fname);
  ok &= fps.LoadText(cname, fname, "face_casc", NULL, sizeof cname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcFFindOCV::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= fps.SaveVals(fname);
  ok &= fps.SaveText(fname, "face_casc", cname);
  return ok;
}


//= Parameters used for computing LBP face representation vector.
// set_sizes() should be called any time basic parameters change

int jhcFFindOCV::find_params (const char *fname)
{
  jhcParam *ps = &fps;
  int ok;

  ps->SetTag("face_find", 0);
  ps->NextSpecF( &pyr,   1.1, "Pyramid shrink step");    // was 1.2 (faster)
  ps->NextSpec4( &pals,  2,   "Neighbors needed");  
  ps->NextSpec4( &wlim, 20,   "Smallest face (pels)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Load face finder window scoring function from an XML file.
// called with no arguments (fname = NULL) defaults to loaded cascade name

int jhcFFindOCV::SetCascade (const char *fname)
{
  cv::CascadeClassifier *cascade = (cv::CascadeClassifier *) cc;
  const char *src = ((fname != NULL) ? fname : cname); 

  ok = 0;
  if (cascade->load(src)) 
    ok = 1;
  return ok;
}


//= Load face finder window scoring function from a resource.
// res_id is usually IDR_FACE_XML (depends on resource.h file)

int jhcFFindOCV::SetCascade (int res_id)
{
  char cfg[200] = "jhc_temp.txt";
  ok = 0;

  if (extract_xml(cfg, res_id, 200) <= 0)
    return ok;
  return SetCascade(cfg);
}


//= Attempt to extract cascade definition XML file from a resource.
// In Resource View do "Add Resource" of type RCDATA called IDR_FACE_XML
// then in Properties bind filename in "res" folder and load data
// returns name of temporary file written to (use with ffind_setup)

int jhcFFindOCV::extract_xml (char *fname, int res_id, int ssz) const
{
  HMODULE m = (HMODULE) hmod;
  HRSRC rsrc;
  HGLOBAL hres = NULL;
  char *data;
  FILE *out;
  DWORD sz, n;

  // try to locate embedded resource (hmod must be explictly set for DLL)
  if (hmod != NULL)
    rsrc = FindResource(m, MAKEINTRESOURCE(res_id), RT_RCDATA);
  else
    rsrc = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(res_id), RT_RCDATA);
  if (rsrc == NULL)
    return -2;
  if ((hres = LoadResource(m, rsrc)) == NULL)
    return -2;

  // open a temporary file to dump XML to (tmpnam requires admin!)
  if (fopen_s(&out, fname, "w") != 0)
    return -1;

  // copy all data out to temporary file
  data = (char *) LockResource(hres);
  sz = SizeofResource(m, rsrc);
  n = (int) fwrite(data, 1, sz, out);

  // clean up and check if successful
  fclose(out);
  if (n < sz)
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                 Low Level DLL Configuration Functions                 //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL.
// returns pointer to input string for convenience

const char *jhcFFindOCV::ffind_version (char *spec, int ssz) const
{
  sprintf_s(spec, ssz, "OpenCV 2.4.5 LBP face finder %4.2f", ver);
  return spec;
}


//= Loads all configuration and calibration data from a file.
// if this function is not called, default values will be used for all parameters
// returns positive if successful, 0 or negative for failure

int jhcFFindOCV::ffind_setup (const char *fname) 
{
  return Defaults(fname);
}


//= Start the face finder system running and await input.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcFFindOCV::ffind_start (int level, const char *log_file)
{
  if (*cname != '\0')
    return SetCascade(cname);
  return SetCascade(IDR_FACE_XML);
}


///////////////////////////////////////////////////////////////////////////
//                   Low Level DLL Analysis Functions                    //
///////////////////////////////////////////////////////////////////////////

//= Do face finding in a region of interest with lower left corner (rx ry).
// img is bottom-up BGR image with lines padded to 4 byte boundaries
// w is image width, h is image height, f is number of fields (3 = color)
// wmin is the minimum face width to look for (in pixels) 
// wmax is biggest to look for (0 = whole image is okay)
// sc is the minimum face detection score to accept
// returns number of face detections for later examination (i = 0 to n-1)
// Note: Can pass a subimage based on other properties like skintone or depth

int jhcFFindOCV::ffind_roi (const unsigned char *img, int w, int h, int f, 
                            int rx, int ry, int rw, int rh,
                            int wmin, int wmax, double sc)
{
  cv::CascadeClassifier *cascade = (cv::CascadeClassifier *) cc;
  std::vector<cv::Rect> faces;
  cv::Mat gray;
  jhcRoi area;
  int w0 = __max(wlim, wmin), w1 = ((wmax > 0) ? wmax : __min(w, h));
  int i, n;

  // check if cascade loaded then test arguments
  if (ok <= 0)
    return -1;
  if ((f != 3) || ((w % 4) != 0))
    return -2;
  area.SetRoi(rx, ry, rw, rh);
  area.RoiClip(w, h);

  // ingest jhcImg and crop to some subregion
  cv::Mat frame(h, w, CV_8UC3, (void *) img);
  cv::Rect r(area.RoiX(), area.RoiY(), area.RoiW(), area.RoiH());
  cv::Mat crop = frame(r);

  // make nice gray scale version (upside down)
  cv::cvtColor(crop, gray, CV_BGR2GRAY);
  cv::flip(gray, gray, 0);
  cv::equalizeHist(gray, gray);

  // run core face finder 
  cascade->detectMultiScale(gray, faces, pyr, pals, 0 | CV_HAAR_SCALE_IMAGE, 
                            cv::Size(w0, w0), cv::Size(w1, w1));

  // load results into ROI objects
  n = (int) faces.size();
  nface = __min(n, 100);
  for (i = 0; i < nface; i++)
    fbox[i].SetRoi(r.x + faces[i].x, (r.y + r.height) - (faces[i].y + faces[i].height), 
                   faces[i].width, faces[i].height);
  return nface;   
}


//= Extract bounding box lower left corner and dimensions for some face.
// returns negative if bad index, else score for this face

double jhcFFindOCV::ffind_box (int& x, int& y, int& w, int &h, int i) const
{
  if ((i < 0) || (i >= nface)) 
    return -1.0; 
  x = fbox[i].RoiX();
  y = fbox[i].RoiY(); 
  w = fbox[i].RoiW(); 
  h = fbox[i].RoiH(); 
  return 1.0;
}





