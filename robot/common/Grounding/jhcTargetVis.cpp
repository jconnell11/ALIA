// jhcTargetVis.cpp : interface to Manus visual behavior kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include "Grounding/jhcTargetVis.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcTargetVis::~jhcTargetVis ()
{
}


//= Default constructor initializes certain values.

jhcTargetVis::jhcTargetVis ()
{
  ver = 1.35;
  strcpy_s(tag, "TargetVis");
  Platform(NULL);
  rpt = NULL;
}


//= Attach physical enhanced body and make pointers to some pieces.

void jhcTargetVis::Platform (jhcManusRWI *io) 
{
  rwi = io;
  seg = NULL;
  ext = NULL;
  body = NULL;
  if (rwi != NULL)
  {
    seg = rwi->seg;
    ext = rwi->ext;
    body = rwi->body;
  }
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.
// only concerned with interpretation values, not control values

int jhcTargetVis::Defaults (const char *fname)
{
  int ok = 1;

  return ok;
}


//= Write current processing variable values to a file.
// only concerned with interpretation values, not control values

int jhcTargetVis::SaveVals (const char *fname) const
{
  int ok = 1;

  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Set up for new run of system.

void jhcTargetVis::local_reset (jhcAliaNote *top)
{
  const jhcImg *src;

  // noisy messages
  rpt = top;
  dbg = 1;

  // status
  focus = 0;
  close = 0;

  // assume robot has been bound and reset already
  if (body == NULL)
    return;
  src = body->View();
  bin.SetSize(*src, 1);
}


//= Post any spontaneous observations to attention queue.

void jhcTargetVis::local_volunteer ()
{
  alert_close();
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcTargetVis::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(class_color);
  JCMD_SET(class_size);
  JCMD_SET(class_width);
  JCMD_SET(det_texture);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcTargetVis::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(class_color);
  JCMD_CHK(class_size);
  JCMD_CHK(class_width);
  JCMD_CHK(det_texture);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                            Event Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Generate spontaneous message if object gets close to robot.
// use to be "close" now essentially "seen"
// <pre>
//   act-1 -lex-  see
//         -agt-> self-1
//         -obj-> obj-N
// </pre>

void jhcTargetVis::alert_close ()
{
  jhcAliaDesc *obj;
  int y, mid = 320, h0 = 150, y0 = 175, y1 = 200;   // was 150-200
 
  // wait for next sensor cycle then pick most central object
  if ((rwi == NULL) || (rpt == NULL) || !rwi->Accepting())
    return;
  focus = seg->CloseAbove(mid, h0);

  // see if newly close (use hysteresis)
  if (focus <= 0)
    close = 0;
  else
  {
    y = seg->Bottom(focus);
    if (y > y1)
      close = 0;
    else if (y <= y0)
    {
      if (close <= 0)
        close = 2;
      else 
        close = 1;
    }
  }

  // post message to reasoner if needed (possibly with features)
  if (close != 2) 
    return;
  rpt->StartNote();
  obj = obj_seen();
  add_size(obj, focus, 0);         // optional
  add_width(obj, focus, 0);        // optional
  add_colors(obj, focus, 0);       // optional
  rpt->FinishNote();
}


//= Make up a new object node and say the robot currently sees it.
// assume new NOTE already started, need to finish it later

jhcAliaDesc *jhcTargetVis::obj_seen () const
{
  jhcAliaDesc *act, *obj;

  act = rpt->NewNode("act", "see", 0, 0.0);
  rpt->AddArg(act, "agt", rpt->Self());
  obj = rpt->NewNode("obj", NULL, 0, 0.0);
  rpt->AddArg(act, "obj", obj);
  rpt->NewProp(obj, "ako", "object");
  return obj;
}


///////////////////////////////////////////////////////////////////////////
//                            Color Analysis                             //
///////////////////////////////////////////////////////////////////////////

//= First call to color analyzer but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcTargetVis::class_color_set (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  return 1;
}


//= Basic call to color analyzer always finishes in one step.
// does not actually look for requested object but uses focus (if any)
// returns 1 if done, 0 if still working, -1 for failure

int jhcTargetVis::class_color_chk (const jhcAliaDesc *desc, int i)
{
  if (rwi->Readable())
    return rwi->ReadDone(add_colors(desc->Val("arg"), focus, 1));
  return 0;
}


//= Anaylze object then generate semantic network assertions about color(s).
// makes and then close new NOTE if "only" > 0, otherwise mixes with existing 
// returns 1 for success, -1 for problem

int jhcTargetVis::add_colors (jhcAliaDesc *obj, int id, int only) 
{
  const jhcImg *clean;
  const char *col;
  int n = 0;

  // find colors in object mask
  if (id <= 0) 
    return -1;
  clean = seg->Clean();
  seg->PadMask(bin, id, 0);
  ext->FindColors(bin, *clean);

  // generate one or more color facts
  if (only > 0)
    rpt->StartNote();
  while ((col = ext->ColorN(n++)) != NULL) 
    rpt->NewProp(obj, "hq", col);
  if (only > 0)
    rpt->FinishNote();
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                             Size Analysis                             //
///////////////////////////////////////////////////////////////////////////

//= First call to size analyzer but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcTargetVis::class_size_set (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  return 1;
}


//= Basic call to size analyzer always finishes in one step.
// does not actually look for requested object but uses focus (if any)
// returns 1 if done, 0 if still working, -1 for failure

int jhcTargetVis::class_size_chk (const jhcAliaDesc *desc, int i)
{
  if (rwi->Readable())
    return rwi->ReadDone(add_size(desc->Val("arg"), focus, 1));
  return 0;
}


//= Anaylze object then possibly generate semantic network assertion about size.
// makes and then close new NOTE if "only" > 0, otherwise mixes with existing 
// returns 1 for success, -1 for problem

int jhcTargetVis::add_size (jhcAliaDesc *obj, int id, int only) 
{
  int sz;

  // find size of focus object and check if normal
  if (id <= 0) 
    return -1;
  sz = ext->SizeClass(seg->AreaPels(id), seg->BotScale(id));
  if (sz == 1)                                              
    return 1;

  // create network assertion (if not medium)
  if (only > 0)
    rpt->StartNote();
  rpt->NewProp(obj, "hq", ((sz <= 0) ? "small" : "big"));  
  if (only > 0)
    rpt->FinishNote();
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Width Analysis                             //
///////////////////////////////////////////////////////////////////////////

//= First call to width analyzer but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcTargetVis::class_width_set (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  return 1;
}


//= Basic call to width analyzer always finishes in one step.
// does not actually look for requested object but uses focus (if any)
// returns 1 if done, 0 if still working, -1 for failure

int jhcTargetVis::class_width_chk (const jhcAliaDesc *desc, int i)
{
  if (rwi->Readable())
    return rwi->ReadDone(add_width(desc->Val("arg"), focus, 1));
  return 0;
}


//= Anaylze object then possibly generate semantic network assertion about width.
// makes and then close new NOTE if "only" > 0, otherwise mixes with existing 
// returns 1 for success, -1 for problem

int jhcTargetVis::add_width (jhcAliaDesc *obj, int id, int only) 
{
  int wc;

  // find width of focus object and check if normal
  if (id <= 0)
    return -1;
  wc = ext->WidthClass(seg->WidthX(id), seg->HeightY(id));
  if (wc == 1)                                     
    return 1;

  // create network assertion (if not medium)
  if (only > 0)
    rpt->StartNote();
  rpt->NewProp(obj, "hq", ((wc <= 0) ? "narrow" : "wide"));  
  if (only > 0)
    rpt->FinishNote();
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Texture Analysis                            //
///////////////////////////////////////////////////////////////////////////

//= First call to stripedness analyzer but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcTargetVis::det_texture_set (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  return 1;
}


//= Basic call to stripedness analyzer always finishes in one step.
// does not actually look for requested object but uses focus (if any)
// returns 1 if done, 0 if still working, -1 for failure

int jhcTargetVis::det_texture_chk (const jhcAliaDesc *desc, int i)
{
  if (rwi->Readable())
    return rwi->ReadDone(add_striped(desc->Val("arg"), focus, 1));
  return 0;
}


//= Analyze object then generate semantic network assertion about truth of stripedness.
// makes and then close new NOTE if "only" > 0, otherwise mixes with existing 
// returns 1 for success, -1 for problem

int jhcTargetVis::add_striped (jhcAliaDesc *obj, int id, int only)
{
  const jhcImg *wk;
  int neg = 0;

  // analyze edges in monochrome image around focus object
  if (id <= 0) 
    return -1;
  seg->PadMask(bin, id, 0);
  wk = seg->Mono();
  if (ext->Striped(bin, *wk) <= 0)
    neg = 1;

  // create network assertion (always)
  if (only > 0)
    rpt->StartNote();
  rpt->NewProp(obj, "hq", "striped", neg);
  if (only > 0)
    rpt->FinishNote();
  return 1;
}

