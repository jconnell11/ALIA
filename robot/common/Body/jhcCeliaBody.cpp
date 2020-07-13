// jhcCeliaBody.cpp : controls basic I/O of Celia smart room
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 IBM Corporation
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

#include "Interface/jhcMessage.h"
#include "Interface/jms_x.h"
#include "Video/jhcKinVSrc.h"

#include "Body/jhcCeliaBody.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcCeliaBody::jhcCeliaBody ()
{
  // starting values
  iw = 640;
  ih = 480;
  cw = 1280;
  ch = 960;
  kin = 0;

  // where the depth image is valid (relative to small image)
  dbox.SetRoi(34, 7, 586, 430);

  // load parameter defaults
  Defaults();
}


//= Default destructor does necessary cleanup.

jhcCeliaBody::~jhcCeliaBody ()
{
  if (kin > 0)
    delete vid;
}


//= Bind an external video source to be used.
// deallocates Kinect driver if bound

void jhcCeliaBody::BindVideo (jhcVideoSrc *v)
{
  if (kin > 0)
    delete vid;
  kin = 0;
  vid = v;
  chk_vid();
}


//= Bind the Kinect depth sensor for obtain video and range.

int jhcCeliaBody::SetKinect (int noisy)
{
  jhcKinVSrc *k;

  // make sure not already bound
  if (kin > 0)
    return 1;

  // try connecting
  if (noisy > 0)
    jprintf("Initializing depth sensor ...\n");
  if ((k = new jhcKinVSrc("0.kin2")) == NULL)
  {
    if (noisy > 0)
      Complain("Could not communicate with Kinect");
    return 0;
  }

  // configure images
  if (noisy > 0)
    jprintf("    ** good **\n\n");
  BindVideo(k);
  kin = 1;
  return 1;
}


//= Reset state for the beginning of a sequence.
// needs to be called at least once before using robot
// if noisy > 0 then prints to log file

int jhcCeliaBody::Reset (int motors, int noisy) 
{
  if (motors > 0)
    NeckReset(noisy);
  chk_vid();
  tprev = 0;
  return CommOK();
}


//= Make sure receiving images are correct size.

void jhcCeliaBody::chk_vid ()
{
  // set image sizes 
  cw = vid->XDim();
  ch = vid->YDim();
//  iw = 640;
//  ih = 480;
  if ((iw = vid->XDim(1)) <= 0)
    iw = cw;
  if ((ih = vid->YDim(1)) <= 0)
    ih = ch;

  // set frame rate
  tstep = vid->StepTime();
  if (vid->Dual() > 0)
    tstep = vid->StepTime(1);

  // make up receiving images
  vid->SizeFor(col, 0);
  vid->SizeFor(rng, 1);
  if (cw > iw)
    col2.SetSize(iw, ih, 3);
}


///////////////////////////////////////////////////////////////////////////
//                           Kinect Image Access                         //
///////////////////////////////////////////////////////////////////////////

//= Get color image that matches the size of the depth image (640 480).
// will downsample if hi-res Kinect mode selected

int jhcCeliaBody::ImgSmall (jhcImg& dest) 
{
  if (!dest.SameFormat(col))
    return Smooth(dest, col);
  return dest.CopyArr(col);
}


//= Get color image in the highest resolution available.
// will upsample if low-res Kinect mode selected

int jhcCeliaBody::ImgBig (jhcImg& dest) 
{
  if (!dest.SameFormat(col))
    return Bicubic(dest, col);
  return dest.CopyArr(col); 
} 


//= Get depth image as an 8 bit gray scale rendering.
// generally better for display purposes

int jhcCeliaBody::Depth8 (jhcImg& dest) const 
{
  if (!rng.Valid())
    return dest.FillArr(0);
  if (!dest.Valid(2))
    return Night8(dest, rng, vid->Shift);
  return dest.CopyArr(rng);
}


//= Get depth image with full 16 bit resolution.
// upconverts if 8 bit gray scale version (e.g. from file)

int jhcCeliaBody::Depth16 (jhcImg& dest) const 
{
  if (!rng.Valid())
    return dest.FillArr(0);
  if (!dest.Valid(1))
    return Fog16(dest, rng);  
  return dest.CopyArr(rng);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Load new images from video source (e.g. Kinect).
// Note: BLOCKS until frame(s) become available

int jhcCeliaBody::UpdateImgs ()
{
  int ans;

  // do basic framegrabbing
  if (vid == NULL)
    return -1;
  if (vid->Dual() > 0)
    ans = vid->DualGet(col, rng);
  else
    ans = vid->Get(col);
  if (ans <= 0)
    return ans;

  // possibly flip images
  if (Flipped() > 0)
  {
    UpsideDown(col);
    UpsideDown(rng);
  }

  // make small version of hi-res color to match depth
  if (cw > iw)
    ForceSize(col2, col, 1);
  return ans;
}


//= Load in fresh configuration data from all mechanical elements.

int jhcCeliaBody::Update ()
{
  int ans;

  ans = UpdatePose();                    // read AX-12 servo data
  if (UpdateImgs() <= 0)     
    return 0;
  return ans;
}


//= Have all mechanical elements move now that command arbitration is done.

int jhcCeliaBody::Issue ()
{
  DWORD tnow = jms_now();
  double tvid = 0.001 * tstep, tupd = tvid;

  // determine actual update rate
  if (tprev != 0)
  {
    tupd = 0.001 * (tnow - tprev);
    tupd = __max(tvid, __min(tupd, 0.5));
  }
  tprev = tnow;

  // tell components to issue their commands
  NeckIssue(tupd);
  return CommOK();
}


