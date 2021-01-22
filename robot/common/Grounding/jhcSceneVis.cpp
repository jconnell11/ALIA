// jhcSceneVis.cpp : interface to ELI visual behavior kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Etaoin Systems
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

#include "Grounding/jhcSceneVis.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSceneVis::~jhcSceneVis ()
{
}


//= Default constructor initializes certain values.

jhcSceneVis::jhcSceneVis ()
{
  ver = 1.00;
  strcpy_s(tag, "SceneVis");
  Platform(NULL);
  rpt = NULL;
}


//= Attach physical enhanced body and make pointers to some pieces.

void jhcSceneVis::Platform (jhcEliGrok *io) 
{
  rwi = io;
  seg = NULL;
  body = NULL;
  if (rwi != NULL)
  {
    seg = &(rwi->tab);
    ext = &(rwi->ext);
    body = rwi->body;
  }
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.
// only concerned with interpretation values, not control values

int jhcSceneVis::Defaults (const char *fname)
{
  int ok = 1;

  return ok;
}


//= Write current processing variable values to a file.
// only concerned with interpretation values, not control values

int jhcSceneVis::SaveVals (const char *fname) const
{
  int ok = 1;

  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Set up for new run of system.

void jhcSceneVis::local_reset (jhcAliaNote *top)
{
  const jhcImg *src;

  // noisy messages
  rpt = top;
  dbg = 1;

ocnt = 0;
rcnt = 0;
ccnt = 0;

  // assume robot has been bound and reset already
  if (body == NULL)
    return;
  src = body->View();
  bin.SetSize(*src, 1);
}


//= Post any spontaneous observations to attention queue.

void jhcSceneVis::local_volunteer ()
{

}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcSceneVis::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(vis_enum);
//  JCMD_SET(vis_color);
  JCMD_SET(vis_col_ok);
  JCMD_SET(vis_shp_ok);
//  JCMD_SET(vis_size);
//  JCMD_SET(vis_width);
//  JCMD_SET(vis_height);
  JCMD_SET(vis_layout);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcSceneVis::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(vis_enum);
//  JCMD_CHK(vis_color);
  JCMD_CHK(vis_col_ok);
  JCMD_CHK(vis_shp_ok);
//  JCMD_CHK(vis_size);
//  JCMD_CHK(vis_width);
//  JCMD_CHK(vis_height);
  JCMD_CHK(vis_layout);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                            Event Functions                            //
///////////////////////////////////////////////////////////////////////////
/*
//= Generate spontaneous message if object gets close to robot.
// use to be "close" now essentially "seen"
// <pre>
//   act-1 -lex-  see
//         -agt-> self-1
//         -obj-> obj-N
// </pre>

void jhcSceneVis::alert_close ()
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

jhcAliaDesc *jhcSceneVis::obj_seen () const
{
  jhcAliaDesc *act, *obj;

  act = rpt->NewNode("act", "see", 0, 0.0);
  rpt->AddArg(act, "agt", rpt->Self());
  obj = rpt->NewNode("obj", NULL, 0, 0.0);
  rpt->AddArg(act, "obj", obj);
  rpt->NewProp(obj, "ako", "object");
  return obj;
}
*/

///////////////////////////////////////////////////////////////////////////
//                            Object Detection                           //
///////////////////////////////////////////////////////////////////////////

//= First call to object detector but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_enum_set (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  return 1;
}


//= Basic call to object detector returns one new object each step.
// makes up 3 objects followed by 1 punt, then repeats
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_enum_chk (const jhcAliaDesc *desc, int i)
{
//  if ((++ocnt % 4) == 0)
  if (++ocnt >= 4)
    return -1;

  rpt->StartNote();
  rpt->NewProp(rpt->NewNode("obj"), "hq", "visible");
  rpt->FinishNote();

  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                            Color Analysis                             //
///////////////////////////////////////////////////////////////////////////
/*
//= First call to color analyzer but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_color_set (const jhcAliaDesc *desc, int i)
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

int jhcSceneVis::vis_color_chk (const jhcAliaDesc *desc, int i)
{
  if (rwi->Readable())
    return rwi->ReadDone(add_colors(desc->Val("arg"), 1));
  return 0;
}


//= Anaylze object then generate semantic network assertions about color(s).
// makes and then close new NOTE if "only" > 0, otherwise mixes with existing 
// returns 1 for success, -1 for problem

int jhcSceneVis::add_colors (jhcAliaDesc *obj, int only) 
{
  const char *col;
  int i = seg->NodeIndex(obj), n = 0;

  // find colors in object mask
  if (i < 0) 
    return -1;
  seg->ObjMask(bin, i, 0);
  ext->FindColors(bin, body->Color());

  // generate one or more color facts
  if (only > 0)
    rpt->StartNote();
  while ((col = ext->ColorN(n++)) != NULL) 
    rpt->NewProp(obj, "hq", col);
  if (only > 0)
    rpt->FinishNote();
  return 1;
}
*/

//= First call to color verifier but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_col_ok_set (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;

  cst[i] = 0;
  return 1;
}


//= Basic call to color verifier returns true or false assertion.
// alternates saying "yes" and "no" to whatever color is posited
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_col_ok_chk (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *col = desc->Val("arg");

  // say definitively "yes" or "no" to conjecture
if (cst[i] <= 0)
{
  rpt->StartNote();
  rpt->NewProp(col->Val("hq"), "hq", col->Word(), (++ccnt % 2)); 
  rpt->FinishNote();
}

//  if (cst[i]++ < 2)
//    return 0;
  return 1;
}


//= First call to shape verifier but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_shp_ok_set (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;

  return 1;
}


//= Basic call to shape verifier returns true or false assertion.
// always says "yes" to box, "no" to anything else
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_shp_ok_chk (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *shp = desc->Val("arg");
  const char *wd = shp->Word();

  // say definitively "yes" or "no" to conjecture
  rpt->StartNote();
  rpt->NewProp(shp->Val("ako"), "ako", wd, ((strcmp(wd, "box") == 0) ? 0 : 1));
  rpt->FinishNote();

  return 1;
}



///////////////////////////////////////////////////////////////////////////
//                             Size Analysis                             //
///////////////////////////////////////////////////////////////////////////
/*
//= First call to size analyzer but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_size_set (const jhcAliaDesc *desc, int i)
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

int jhcSceneVis::vis_size_chk (const jhcAliaDesc *desc, int i)
{
  if (rwi->Readable())
    return rwi->ReadDone(add_size(desc->Val("arg"), 1));
  return 0;
}


//= Anaylze object then possibly generate semantic network assertion about size.
// makes and then close new NOTE if "only" > 0, otherwise mixes with existing 
// returns 1 for success, -1 for problem

int jhcSceneVis::add_size (jhcAliaDesc *obj, int only) 
{
  int sz, i = seg->NodeIndex(obj);

  // find colors in object mask
  // find size of focus object and check if normal
  if (i < 0) 
    return -1;
  sz = ext->SizeClass(seg->MaxDim(i));
  if (sz == 1)                                              
    return 1;

  // create network assertion (if not medium)
  if (only > 0)
    rpt->StartNote();
  rpt->NewProp(obj, "hq", ((sz <= 0) ? "short" : "tall"));  
  if (only > 0)
    rpt->FinishNote();
  return 1;
}
*/
/*
///////////////////////////////////////////////////////////////////////////
//                            Width Analysis                             //
///////////////////////////////////////////////////////////////////////////

//= First call to width analyzer but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::class_width_set (const jhcAliaDesc *desc, int i)
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

int jhcSceneVis::class_width_chk (const jhcAliaDesc *desc, int i)
{
  if (rwi->Readable())
    return rwi->ReadDone(add_width(desc->Val("arg"), focus, 1));
  return 0;
}


//= Anaylze object then possibly generate semantic network assertion about width.
// makes and then close new NOTE if "only" > 0, otherwise mixes with existing 
// returns 1 for success, -1 for problem

int jhcSceneVis::add_width (jhcAliaDesc *obj, int id, int only) 
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

int jhcSceneVis::det_texture_set (const jhcAliaDesc *desc, int i)
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

int jhcSceneVis::det_texture_chk (const jhcAliaDesc *desc, int i)
{
  if (rwi->Readable())
    return rwi->ReadDone(add_striped(desc->Val("arg"), focus, 1));
  return 0;
}


//= Analyze object then generate semantic network assertion about truth of stripedness.
// makes and then close new NOTE if "only" > 0, otherwise mixes with existing 
// returns 1 for success, -1 for problem

int jhcSceneVis::add_striped (jhcAliaDesc *obj, int id, int only)
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
*/


///////////////////////////////////////////////////////////////////////////
//                          Spatial Arrangement                          //
///////////////////////////////////////////////////////////////////////////

//= First call to relation assessor but not allowed to fail.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_layout_set (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  cst[i] = 0;                          // for possible delayed return
  return 1;
}


//= Basic call to relation assessor evaluates whether posited arrangement holds.
// generates 3 "no" followed by 1 "yes" and repeats
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_layout_chk (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *r2, *rel = desc->Val("arg");


  // say definitively "yes" or "no" to conjecture
  if (cst[i] == 0)
  {
    rpt->StartNote();
    r2 = rpt->NewNode("loc", rel->Word(), ((++rcnt == 4) ? 0 : 1));
    rpt->AddArg(r2, "loc", rel->Val("loc"));
    rpt->AddArg(r2, "wrt", rel->Val("wrt"));
    rpt->FinishNote();
  }

  // possibly add dead cycles at end to allow for propagation
  cst[i] += 1;
//  if (cst[i] < 2)
//    return 0;
  return 1;
 }
