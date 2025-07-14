// jhcVisCoord.cpp : language, perceptions, learning, and control for external robot
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
  #include <direct.h>                  // for _mkdir
#endif

#include "Data/jhcImgIO.h"             // common video
#include "Interface/jms_x.h"           
#include "Interface/jtimer.h"

#include "RWI/jhcVisCoord.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcVisCoord::~jhcVisCoord ()
{
  // for debugging - only happens when program closes
  DumpAll();
}


//= Default constructor initializes certain values.

jhcVisCoord::jhcVisCoord ()
{
  // bind sub-mechanisms into overall body 
  rwi.body = &body0;
  rwi.neck = &(body0.neck0);
  rwi.arm  = &(body0.arm0);
  rwi.lift = &(body0.lift0);
  rwi.base = &(body0.base0);

  // connect microphone (if any) to person finder
  rwi.mic = &mic0;              
  (rwi.tk).RemoteMic(&mic0);

  // bind actual images into background processing
  rwi.rng = &(body0.img_r);
  rwi.col = &(body0.img_c);
  rwi.aux = &(body0.img_a);
  rwi.raw = &(body0.raw);

  // attach grounding kernels
  kern.AddFcns(ball); 
  kern.AddFcns(svis);
  kern.AddFcns(sup);
  kern.AddFcns(soc);
  kern.AddFcns(man);
  kern.Platform(&rwi, "jhcVisGrok");

  // default processing parameters and state
  noisy = 1;
  Defaults();
  up = 0;                    // needs reset
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for overall control of timing.
// this should be called in Defaults and tps used in SaveVals

int jhcVisCoord::kern_params (const char *fname)
{
  jhcParam *ps = &kps;
  int ok;

  ps->SetTag("coord_kern", 0);
  ps->NextSpec4( &(svis.dbg),    2, "SceneVis objects (std = 2)");
  ps->NextSpec4( &(sup.dbg),     2, "Support surfaces (std = 2)");
  ps->NextSpec4( &(soc.dbg),     2, "Social agents (std = 2)");
  ps->Skip();
  ps->NextSpec4( &(ball.dbg),    1, "Ballistic body (std = 1)");
  ps->NextSpec4( &(man.dbg),     1, "Manipulation arm (std = 1)");

  ps->NextSpec4( &(dmem.enc),    0, "LTM encoding (dbg = 3)");
  ps->NextSpec4( &(dmem.detail), 0, "LTM retrieval for node");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcVisCoord::Defaults (const char *fname)
{
  int ok = 1;

  // local parameters
  ok &= time_params(fname);            // jhcAliaSpeech
  ok &= kern_params(fname);
  ok &= jhcAliaCore::Defaults(fname);

  // kernel parameters
  ok &= ball.Defaults(fname);
  ok &= svis.Defaults(fname);
  ok &= sup.Defaults(fname);
  ok &= soc.Defaults(fname);
  ok &= man.Defaults(fname);

  // component parameters
  ok &= rwi.Defaults(fname);
  ok &= body0.Defaults(fname);
  ok &= mic0.Defaults(fname, 1);
  return ok;
}


//= Write current processing variable values to a file.

int jhcVisCoord::SaveVals (const char *fname) 
{
  int ok = 1;

  // local parameters
  ok &= tps.SaveVals(fname);
  ok &= kps.SaveVals(fname);
  ok &= jhcAliaCore::SaveVals(fname);

  // kernel parameters
  ok &= ball.SaveVals(fname);
  ok &= svis.SaveVals(fname);
  ok &= sup.SaveVals(fname);
  ok &= soc.SaveVals(fname);
  ok &= man.SaveVals(fname);

  // component parameters
  ok &= rwi.SaveVals(fname);
  ok &= body0.SaveVals(fname);
  ok &= mic0.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// dir = working directory for language, KBx, GND, config, calib, log, dump
// rname = particular robot, last name encodes body type (e.g. "Ivy Banzai")
// returns 2 if robot ready, 1 if ready but no robot, 0 or negative for error

int jhcVisCoord::Reset (const char *dir, const char *rname, int prt)
{
  char ifile[200] = "config/robot_vals.ini";
  const char *prog;

  // pass on working directory to ALIA 
  SetDir(dir);

  // create information logging directories if needed (octal mode)
  _mkdir(wrt("log"));
  _mkdir(wrt("timing"));
  _mkdir(wrt("KB"));
  _mkdir(wrt("dump"));

  // load configuration parameters based on robot last name
  if (rname != NULL) 
    if ((prog = strchr(rname, ' ')) != NULL)
      sprintf_s(ifile, "config/%s_vals.ini", prog + 1);
  Defaults(wrt(ifile));

  // initialize hardware subsystems
  body0.Reset();

  // start background processing of video
  jtimer_clr();
  rwi.Reset(1, 1, 0);

  // initialize speech and reasoning and add user faces
  if (jhcAliaSpeech::Reset(rname, prt, 0) <= 0)
    return 0;
  ((rwi.fn).fr).SetDir("%sfaces", Dir());
  ((rwi.fn).fr).LoadDB(wrt("config/VIPs.txt"), 0);
  up = 1;                    // reset accomplished
  return 1;
}


//= Do basic reasoning then muck with status images.

void jhcVisCoord::Respond ()
{
  if (rwi.Update(SpeechRC(), NextSense()) <= 0)
    return;
  Consider();
  rwi.Issue();
}


//= Call at end of run to put robot in stable state and possibly save knowledge.

int jhcVisCoord::Done (int face, int batt)
{
  jhcImgIO jio;
  char aux[80], fname[80];
  const char *me;

  // skip if system never reset
  if (up <= 0)
    return 0;
 
  // save info from run
  DumpSession();                       // brand new rules and ops
  jhcAliaSpeech::Done(1, batt);        // incl. accumulated knowledge
  rwi.DumpImages(Dir());               // input and output images

  // save call profiling
  me = Name();
  sprintf_s(fname, "timing/%s_%s.prof", ((me != NULL) ? me : "timing"), jms_date(aux));
  jtimer_rpt(1, wrt(fname), 1);

  // needs new reset
  up = 0;                              
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Output Images                            //
///////////////////////////////////////////////////////////////////////////

//= Dump object detection view into a pixel buffer in some format.
// image is normally 640 x 480, fmt <= 0 means not needed
// returns 0 if not written, 1 if okay 

int jhcVisCoord::GetView (void *pels, int fmt) const
{
  const jhcImg *src = rwi.HeadView();
  int ok = 0;

  // see if desired
  if ((fmt <= 0) || (pels == NULL))
    return 0;

  // choose between some common input formats
  if (fmt == 1)
    ok = get_bgr_bot(pels, *src);      // Banzai
  else if (fmt == 2)
    ok = get_bgr_top(pels, *src);      // Ganbei + Wansui
  return ok;
}


//= Dump overhead obstacle map into a pixel buffer in some format.
// image is normally 640 x 640 color, fmt <= 0 means not needed
// returns 0 if not written, 1 if okay 

int jhcVisCoord::GetMap (void *pels, int fmt)
{
  const jhcImg *src = rwi.MapView();
  int ok = 0;

  // see if desired 
  if ((fmt <= 0) || (pels == NULL))
    return 0;

  // choose between some common output image formats
  if (fmt == 1)
    ok = get_bgr_bot(pels, *src);      // Banzai
  else if (fmt == 2)
    ok = get_bgr_top(pels, *src);      // Ganbei + Wansui
  return ok;
}


//= Dump color image to DIB format buffer in BGR bottom-up order.

int jhcVisCoord::get_bgr_bot (void *pels, const jhcImg& src) const 
{
  int y, w3 = 3 * src.XDim(), sln = src.Line();
  const UC8 *s = src.PxlSrc();
  UC8 *d = (UC8 *) pels;

  if ((src.Fields() != 3) || (pels == NULL))
    return 0;
  for (y = src.YDim(); y > 0; y--, d += w3, s += sln)
    memcpy(d, s, w3);
  return 1;
}


//= Dump color image to OpenCV format buffer in BGR top-down order.

int jhcVisCoord::get_bgr_top (void *pels, const jhcImg& src) const
{
  // sanity check
  if ((src.Fields() != 3) || (pels == NULL))
    return 0;

  // full image
  int y, h = src.YDim(), ln = src.Line();
  const UC8 *s = src.PxlSrc() + ln * (h - 1);
  UC8 *d = (UC8 *) pels;

  // change from bottom-up to top-down
  for (y = h; y > 0; y--, s -= ln, d += ln)
    memcpy(d, s, ln);
  return 1;
}

