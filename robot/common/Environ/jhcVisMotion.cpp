// jhcVisMotion.cpp : look for areas of visual motion in color image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2026 Etaoin Systems
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

#include <math.h>

#include "Interface/jms_x.h"           // common video

#include "Environ/jhcVisMotion.h"

#include "Processing/jhcStats.h"
///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcVisMotion::~jhcVisMotion ()
{
}


//= Default constructor initializes certain values.

jhcVisMotion::jhcVisMotion ()
{
  // max number of motion regions
  blob.SetSize(100);         

  // initialize ping-pong buffers and record focal length
  prev = &gray0;
  mono = &gray1;

  // set processing values and state
  Defaults();
  Reset();
}


//= Set size of internal images (half of normal color).
// Note: MUST be called before Analyze()

void jhcVisMotion::SetSize (int iw, int ih, double flen)
{
  // images for analysis 
  sm.SetSize(iw / 2, ih / 2, 3);
  diff.SetSize(sm, 1);
  gray0.SetSize(diff);
  gray1.SetSize(diff);
  acc.SetSize(diff);
  sal.SetSize(diff);
  cc.SetSize(diff, 2);

  // save focal length for angle calculation
  cf = 0.5 * flen;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters controlling detection of motion regions.

int jhcVisMotion::detect_params (const char *fname)
{
  jhcParam *ps = &dps;
  int ok;

  ps->SetTag("vmot_det", 0);
  ps->NextSpecF( &stare,   0.3, "Stable for detect (sec)");
  ps->NextSpec4( &big,    50,   "Min intensity change (x2)");
  ps->NextSpecF( &barf,    0.1, "Max frac image motion");
  ps->NextSpecF( &fade,    0.8, "Motion history fading");  
  ps->NextSpec4( &sc,     33,   "Edge aggregation scale");
  ps->NextSpec4( &amin,  200,   "Min motion area (pel)");

  ps->NextSpec4( &sure,   15,   "Detections to trigger");
  ps->NextSpecF( &seek,    1.0, "Target timeout (sec)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling gaze angle normalization.

int jhcVisMotion::askew_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("vmot_aim", 0);
  ps->NextSpecF( &pdef,     0.0,  "Nominal pan (deg)");
  ps->NextSpecF( &tdef,     0.0,  "Nominal tilt (deg)");
  ps->NextSpecF( &atol,     2.0,  "Aim tolerance (deg)");  
  ps->NextSpecF( &park,     0.07, "Stable for parked (sec)");  // was 1 cyc
  ps->Skip(2);

  ps->NextSpecF( &annoy,   30.0, "Restore holdoff (sec)");           
  ps->NextSpecF( &restore,  1.5, "Restore timeout (sec)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcVisMotion::Defaults (const char *fname)
{
  int ok = 1;

  ok &= detect_params(fname);
  ok &= askew_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcVisMotion::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= dps.SaveVals(fname);
  ok &= aps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcVisMotion::Reset ()
{
  // no motion regions currently
  clr_acc();
  itch = 0;
  seek = 0; 

  // no need to straighten head
  crink = 0;
  fix = 0;
  wait = jms_now();
}


//= Clear motion history due to camera movement.
// returns 0 always for convenience

int jhcVisMotion::clr_acc ()
{
  prev->FillArr(0);          // forces high change
  acc.FillArr(0);                  
  sal.FillArr(0);            // for display
  cnt = 0;
  return 0;
}


//= Calculate rough direction toward an image region with substantial motion.
// also monitors long-term deflection of neck away from nominal direction

void jhcVisMotion::Analyze (const jhcImg& col, double pan, double tilt, double stable)
{
  // always check aim direction  
  chk_nominal(pan, tilt, stable);

  // only detect motion if parked
  if (stable < stare)
    clr_acc();              
  else
  {                 
    // run detection on half-sized monochrome version of input
    Decimate(sm, col, 2);
    MonoAvg(*mono, sm);
    BoxAvg(*mono, *mono, 3);           // camera vibration
    find_regions(pan, tilt);           
    swap_bufs();
  }

  // clear flag if no motion region detected for a while
  if ((itch > 0) && (jms_elapsed(targ) > seek))
    itch = 0;
}


//= Compare two parked frames to find best region of high change.
// returns 0 if nothing, 1 if something, 2 if solid detection

int jhcVisMotion::find_regions (double pan, double tilt)
{
  int win;

  // get significant pixel changes between frames
  AbsDiff(diff, *mono, *prev, 2.0);
  Squelch(diff, diff, big);                      // small noise
  if (FracOver(diff) > barf)                     // too much motion
    return clr_acc();                  

  // decay old motion then add in new differences
  ClipScale(acc, acc, fade);
  Offset(acc, acc, -1);
  ClipSum(acc, acc, diff);

  // aggregate over nearby edges and find regions
  BoxAvg(sal, acc, sc, sc, 4.0);
  CComps4(cc, sal, amin, 128);
  blob.FindParams(cc);

  // await several consecutive detections
  if ((win = blob.Biggest()) <= 0)
  {
    cnt = 0;
    return 0;
  }
  if (++cnt < sure)          
    return 1;

  // convert center pixel offset to full angles
  blob.BlobCentroid(&mx, &my, win);
  pmot = pan  - R2D * atan2(mx - sm.RoiMidX(), cf);
  tmot = tilt + R2D * atan2(my - sm.RoiMidY(), cf);

  // set current solid detection flag 
  itch = 1;
  targ = jms_now();
  return 2;
}


//= Swap small monochrome ping-pong buffers for next cycle.

void jhcVisMotion::swap_bufs ()
{
  if (mono == &gray0)
  {
    prev = &gray0;
    mono = &gray1;
  }
  else
  {
    prev = &gray1;
    mono = &gray0;
  }
}


//= Ask for gaze restoration if camera parked off-nominal for too long.

void jhcVisMotion::chk_nominal (double pan, double tilt, double stable)
{
  UL32 now = jms_now();

  if (stable >= park)        // parked
  {
    if ((fabs(pan - pdef) <= atol) && (fabs(tilt - tdef) <= atol))
    {
      crink = 0;             // nominal           
      wait = now;
    }
    else if ((crink <= 0) && (jms_elapsed(wait) > annoy))
      crink = 1;             // request restoration 
    fix = now;
  }    
  else                       // moving
  { 
    if ((crink > 0) && (jms_elapsed(fix) > restore))  
      crink = 0;             // give up 
    wait = now;
  }
}

