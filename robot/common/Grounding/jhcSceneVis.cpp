// jhcSceneVis.cpp : interface to ELI visual behavior kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2021 Etaoin Systems
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

#include "Language/jhcMorphTags.h"     // common audio
#include "Semantic/jhcNetNode.h"  

#include "Grounding/jhcSceneVis.h"


///////////////////////////////////////////////////////////////////////////
//                             Terminology                               //
///////////////////////////////////////////////////////////////////////////

//= Range categories with high and low value terms and superlatives (get from jhcMorphFcns?).
//                                            0           1        2         3          4           5

const char * const jhcSceneVis::rng[]  = {"distance", "size",  "length",  "width",  "thickness", "height"};
const char * const jhcSceneVis::rng0[] = {"close",    "small", "compact", "narrow", "thin",      "short" };
const char * const jhcSceneVis::rng1[] = {"far",      "big",   "long",    "wide",   "thick",     "tall"  };


//= Color description terms with superlatives.
//                                           6       7         8        9       10       11       12       13      14

const char * const jhcSceneVis::col[]  = {"red", "orange", "yellow", "green", "blue", "purple", "black", "gray", "white"};
                             

//= Position description with superlatives for lateral locations.
//                                           0             1                 2                3             4         5        6

const char * const jhcSceneVis::loc[]  = {"between", "to the left of", "to the right of", "in front of", "behind", "near", "next to"};
const char * const jhcSceneVis::sloc[] = {"middle",  "leftmost",       "rightmost",       "tween",       "side",   "prox"};


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
  ver = 1.40;
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
    body = rwi->body;
  }
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for qualitative description of distance and size.

int jhcSceneVis::rng_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("svis_rng", 0);
  ps->NextSpecF( &dist3, 60.0, "Very far distance (in)");
  ps->NextSpecF( &dist2, 36.0, "Far distance (in)");  
  ps->NextSpecF( &dist1, 24.0, "Close distance (in)");
  ps->NextSpecF( &dist0, 18.0, "Very close distance (in)");
  ps->Skip();
  ps->NextSpecF( &dvar,   1.0, "Alert dist hysteresis (in)"); 
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for qualitative description of object shape.

int jhcSceneVis::shape_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("svis_shape", 0);
  ps->NextSpecF( &len3, 4.0,  "Very long ratio (hi / mid)");
  ps->NextSpecF( &len2, 1.8,  "Long ratio (hi / mid)");
  ps->NextSpecF( &len1, 1.25, "Compact ratio (hi / mid)");
  ps->NextSpecF( &len0, 1.15, "Very compact ratio (hi / mid)");
  ps->NextSpecF( &thk3, 0.85, "Very thick ratio (lo / mid)");  
  ps->NextSpecF( &thk2, 0.6,  "Thick ratio (lo / mid)");  

  ps->NextSpecF( &thk1, 0.4,  "Thin ratio (lo / mid)");
  ps->NextSpecF( &thk0, 0.1,  "Very thin ratio (lo / mid)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for qualitative description of object dimensions.

int jhcSceneVis::dims_params (const char *fname)
{
  jhcParam *ps = &dps;
  int ok;

  ps->SetTag("svis_dims", 0);
  ps->NextSpecF( &sz2,  5.0, "Big square (in)");  
  ps->NextSpecF( &sz1,  2.0, "Small square (in)");
  ps->NextSpecF( &wid2, 2.5, "Wide threshold (in)");
  ps->NextSpecF( &wid1, 1.5, "Narrow threshold (in)");  
  ps->NextSpecF( &ht2,  4.0, "Tall threshold (in)");
  ps->NextSpecF( &ht1,  1.5, "Short threshold (in)"); 
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for comparisons and spatial locations.

int jhcSceneVis::comp_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("svis_comp", 0);
  ps->NextSpecF( &rdom,   0.1, "Measurement greater fraction");
  ps->NextSpecF( &cdom,   0.1, "Color band greater fraction");
  ps->NextSpecF( &tween,  0.3, "Between fraction from middle");
  ps->NextSpecF( &sdev,  30.0, "Side zone deviation (deg)");  
  ps->NextSpecF( &buddy,  1.5, "Adjacent distance wrt size");  
  ps->NextSpecF( &hood,   3.0, "Near distance wrt size");

  ps->NextSpec4( &cmax,   7,   "Max subit count (else \"lots\")"); 
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.
// only concerned with interpretation values, not control values

int jhcSceneVis::Defaults (const char *fname)
{
  int ok = 1;
  
  ok &= rng_params(fname);
  ok &= shape_params(fname);
  ok &= dims_params(fname);
  ok &= comp_params(fname);
  return ok;
}


//= Write current processing variable values to a file.
// only concerned with interpretation values, not control values

int jhcSceneVis::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= rps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= dps.SaveVals(fname);
  ok &= cps.SaveVals(fname);
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
  noisy = 0;

  // state variables
  nearby = -1;
  some = 0;
  close = 0;

  // assume robot has been bound and reset already
  if (body == NULL)
    return;
  src = body->View();
  bin.SetSize(*src, 1);
}


//= Post any spontaneous observations to attention queue.

void jhcSceneVis::local_volunteer ()
{
  alert_any();
  alert_close();
  mark_attn();
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcSceneVis::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(vis_value);
  JCMD_SET(vis_val_ok);
  JCMD_SET(vis_color);
  JCMD_SET(vis_col_ok);
  JCMD_SET(vis_position);
  JCMD_SET(vis_pos_ok);
  JCMD_SET(vis_comp_ok);
  JCMD_SET(vis_subit);
  JCMD_SET(vis_enum);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcSceneVis::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(vis_value);
  JCMD_CHK(vis_val_ok);
  JCMD_CHK(vis_color);
  JCMD_CHK(vis_col_ok);
  JCMD_CHK(vis_position);
  JCMD_CHK(vis_pos_ok);
  JCMD_CHK(vis_comp_ok);
  JCMD_CHK(vis_subit);
  JCMD_CHK(vis_enum);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                            Event Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Generate spontaneous message if robot starts seeing objects.
// only signals first of however many until there are none again
// <pre>
//    hq-1 -lex-  visible
//         -obj-> obj-N
//   ako-1 -lex-  object
//         -ako-> obj-N
// </pre>

void jhcSceneVis::alert_any ()
{
  double xy;
  int item;

  // wait for next sensor cycle then lock visual data
  if ((rwi == NULL) || (rpt == NULL) || !rwi->Readable())
    return;
  item = seg->Closest();

  // see if newly close (use hysteresis)
  if (item < 0)
    some = 0;
  else
  {
    xy = seg->DistXY(item);
    if (xy > (dist2 + dvar))
      some = 0;                        // no longer anything
    else if (xy <= (dist2 - dvar))
    {
      if (some <= 0)
        some = 2;                      // new object(s)
      else 
        some = 1;                      // still some object(s)
    }
  }

  // post message to reasoner if needed 
  if (some >= 2) 
  {
    jprintf(1, noisy, "vis_alert @ %4.2f\"\n", xy);
    rpt->StartNote();
    obj_node(item);
    rpt->FinishNote();
  }
  rwi->ReadDone();                     // release visual data
}


//= Generate spontaneous message if some object gets close to robot.
// <pre>
//   hq-1 -lex- close
//        -hq-> obj-N
// </pre>

void jhcSceneVis::alert_close ()
{
  double dist;
  int prev = nearby;

  // wait for next sensor cycle then lock visual data
  if ((rwi == NULL) || (rpt == NULL) || !rwi->Readable())
    return;
  nearby = seg->Closest();
  if (nearby != prev)
    close = 0;                         // reset if different object

  // see if newly close (use hysteresis)
  if (nearby < 0)
    close = 0;
  else
  {
    dist = seg->DistXY(nearby);
    if (dist > (dist1 + dvar))
      close = 0;                       // no longer close
    else if (dist <= (dist1 - dvar))
    {
      if (close <= 0)
        close = 2;                     // newly close
      else 
        close = 1;                     // still close
    }
  }

  // post message to reasoner if needed (OK to repeat)
  if (close >= 2) 
  {
    jprintf(1, noisy, "vis_close @ %4.2f\"\n", dist);
    rpt->StartNote();
    rpt->NewDeg(obj_node(nearby), "hq", "close", ((dist < dist0) ? "very" : NULL));  
    rpt->FinishNote();
  }
  rwi->ReadDone();                     // release visual data
}


//= If anything newly marked (2) reset older marks to zero.
// volunteered items are marked right away, others delayed one cycle

void jhcSceneVis::mark_attn ()
{
  const char *label;
  int t, nt, st, any = 0;

  // wait for tracker to be stable then lock visual data
  if ((rwi == NULL) || !rwi->Readable())
    return;

  // see if any associated nodes have disappeared
  nt = seg->ObjLimit(1);
  for (t = 0; t < nt; t++)
    if (seg->ObjOK(t))
    {
      label = seg->GetTag(t);
      if ((*label != '\0') && (trk2node(t) == NULL))
        seg->SetTag(t, "");
      if (seg->GetState(t) >= 2)       // newly marked
        any = 1;
    }

  // possibly change all state 1->0 and state 2->1
  if (any > 0)
    for (t = 0; t < nt; t++)
      if (seg->ObjOK(t))
        if ((st = seg->GetState(t)) > 0)
          seg->SetState(t, st - 1);
  rwi->ReadDone();                     // release visual data
}


///////////////////////////////////////////////////////////////////////////
//                             Value Ranges                              //
///////////////////////////////////////////////////////////////////////////

//= First call to measurement analyzer but not allowed to fail.
// answers "What distance/size/height is X?"
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_value0 (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *hq;

  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((hq = desc->Val("arg")) == NULL)
    return -1;
  if (hq->Val("hq") == NULL)
    return -1;
  if ((cst[i] = net2rng(hq)) < 0)
    return -1;
  return 1;
}


//= Determine category of information requested from HQ type.
// returns 0-5, negative if unknown

int jhcSceneVis::net2rng (const jhcAliaDesc *hq) const
{
  const jhcAliaDesc *ako;
  int cat, i = 0;

  while ((ako = hq->Fact("ako", i++)) != NULL)
    if (ako->Visible() && (ako->Neg() <= 0))
      for (cat = 0; cat <= 5; cat++)
        if (strcmp(ako->Lex(), rng[cat]) == 0)
          return cat;
  return -1;
}


//= Basic call to measurement analyzer always finishes in one step.
// gives qualitative value for features, rules can lead to negation
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_value (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *hq = desc->Val("arg");
  jhcAliaDesc *obj = hq->Val("hq");
  int t, cat = cst[i];

  // find the focus object 
  if (!rwi->Readable())
    return 0;
  if ((t = node2trk(obj)) < 0)
    return rwi->ReadDone(-1);
  seg->SetState(t, 2);                 // display as green

  // compute the desired property and assert it in net
  rpt->StartNote();
  rng2net(obj, cat, trk2rng(cat, t));
  rpt->FinishNote();
  return rwi->ReadDone(1);
}


//= First call to measurement verifier but not allowed to fail.
// answers "Is X close/big/wide?" although mutex rules may also cover this
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_val_ok0 (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *hq;
  int des;

  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((hq = desc->Val("arg")) == NULL)
    return -1;
  if (hq->Val("hq") == NULL)
    return -1;
  if ((cst[i] = net2des(des, hq)) < 0)
    return -1;
  camt[i] = des;
  return 1;
}


//= Determines category and desired range value from semantic net.
// "very long" -> cat = 2 and des = 5, "medium thick" -> cat = 4 and des = 3
// saves target (1-5) in "des" and returns category (0-5), negative for problem

int jhcSceneVis::net2des (int& des, const jhcAliaDesc *hq) const
{ 
  char mid[40];
  const jhcAliaDesc *mod;
  const char *val = hq->Lex();
  int cat, i;

  // see default value then scan kinds of measurement
  des = 0;
  for (cat = 0; cat <= 5; cat++)
  {
    // look for basic HQ of low, medium, or high in this category
    if (strcmp(val, rng0[cat]) == 0)  
      des = 2;
    else if (strcmp(val, rng1[cat]) == 0)
      des = 4;
    else
    {
      // use "medium close" not "medium far" for distance
      sprintf_s(mid, "medium %s", ((cat <= 0) ? rng0[cat] : rng1[cat]));
      if (strcmp(val, mid) == 0)
        des = 3;
    }

    // see if this category was pertinent
    if (des != 0)
    {
      i = 0;
      if ((des == 2) || (des == 4))
        while ((mod = hq->Fact("deg", i++)) != NULL)
          if (mod->Visible() && (mod->Neg() <= 0) && mod->LexMatch("very"))
          {
            // "very" amplifies desired value
            des = ((des > 3) ? 5 : 1);
            break;
          }
      return cat;
    }
  }
  return -1;
}


//= Basic call to measurement verifier always finishes in one step.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_val_ok (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *hq = desc->Val("arg");
  jhcAliaDesc *obj = hq->Val("hq");
  int t, cat = cst[i], des = (int) camt[i];

  // find the focus object 
  if (!rwi->Readable())
    return 0;
  if ((t = node2trk(obj)) < 0)
    return rwi->ReadDone(-1);
  seg->SetState(t, 2);                 // display as green

  // compute the desired property and assert or deny it in net
  rpt->StartNote();
  des2net(obj, cat, des, trk2rng(cat, t));
  rpt->FinishNote();
  return rwi->ReadDone(1);
}


///////////////////////////////////////////////////////////////////////////
//                               Color                                   //
///////////////////////////////////////////////////////////////////////////

//= First call to color analyzer but not allowed to fail.
// answers "What color is X?"
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_color0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  return 1;
}


//= Basic call to color analyzer always finishes in one step.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_color (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *hq, *obj = desc->Val("arg"), *mix = NULL;
  int t, cnum, n = 0;

  // find the referenced object and analyze its color
  if (!rwi->Readable())
    return 0;
  if ((t = node2trk(obj)) < 0)
    return rwi->ReadDone(-1);
  seg->SetState(t, 2);                                     // display as green
  n = seg->Spectralize(body->Color(), body->Range(), t);
  if (n <= 0)
    return rwi->ReadDone(1);

  // assert values in net (add linking "and" node if multiple colors)
  rpt->StartNote();
  if (n > 1)
    mix = rpt->NewNode("conj", "and");
  for (cnum = 0; cnum <= 8; cnum++)
    if (seg->DegColor(t, cnum) >= 2)
    {
      hq = rpt->NewProp(obj, "hq", col[cnum], 0, 1.0, 1);           
      if (mix != NULL) 
        rpt->AddArg(mix, "conj", hq);
    }
  rpt->FinishNote();
  return rwi->ReadDone(1);
}


//= First call to color verifier but not allowed to fail.
// answers "Is X red/blue/white?"
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_col_ok0 (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *hq;

  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((hq = desc->Val("arg")) == NULL)
    return -1;
  if (hq->Val("hq") == NULL)
    return -1;
  if ((cst[i] = txt2cnum(hq->Lex())) < 0)
    return -1;
  return 1;
}


//= Determine which color is being asked about.
// returns 0-8 (not category), negative if unknown

int jhcSceneVis::txt2cnum (const char *txt) const
{
  int cnum;

  for (cnum = 0; cnum <= 8; cnum++)
    if (strcmp(txt, col[cnum]) == 0)
      return cnum;
  return -1;
}


//= Basic call to color verifier returns true or false assertion.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_col_ok (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *hq = desc->Val("arg");
  jhcAliaDesc *obj = hq->Val("hq");
  int t, n, cnum = cst[i];

  // find the referenced object and analyze its color
  if (!rwi->Readable())
    return 0;
  if ((t = node2trk(obj)) < 0)
    return rwi->ReadDone(-1);
  seg->SetState(t, 2);                                       // display as green
  n = seg->Spectralize(body->Color(), body->Range(), t);

  // directly assert or refute in net
  rpt->StartNote();
  if (seg->DegColor(t, cnum) < 2)
    rpt->NewProp(obj, "hq", col[cnum], 1, 1.0, 1);           // "not red"
  else if (n > 1)
    rpt->NewDeg(obj, "hq", col[cnum], "partly", 0, 1.0, 1);  // missing "and" node
  else 
    rpt->NewProp(obj, "hq", col[cnum], 0, 1.0, 1);           // just "red"
  rpt->FinishNote();
  return rwi->ReadDone(1);
}


///////////////////////////////////////////////////////////////////////////
//                           Spatial Location                            //
///////////////////////////////////////////////////////////////////////////

//= First call to location finder but not allowed to fail.
// answers "Where is X?" in relation to know objects
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_position0 (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *pos;

  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((pos = desc->Val("arg")) == NULL)
    return -1;
  if (pos->Val("loc") == NULL) 
    return -1;
  return 1;
}


//= Basic call to location finder returns spatial assertion.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_position (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *ref, *pos = desc->Val("arg");
  jhcAliaDesc *hq, *obj = pos->Val("loc");
  const char *lex;
  double dx, dy, dist, best;
  int t, r, nt, side, prox, anchor = -1;

  // find the referenced objects and possibly analyze their color
  if (!rwi->Readable())
    return 0;
  if ((t = node2trk(obj)) < 0)
    return rwi->ReadDone(-1);

  // scan through other objects that have a node used in conversation
  nt = seg->ObjLimit();
  for (r = 0; r < nt; r++)
    if ((r != t) && seg->ObjOK(r))
      if ((ref = trk2node(r)) != NULL)
        if (ref->LastRef() > 0)                  // user knows about
        {
          // find distance to query object
          dx = seg->PosX(t) - seg->PosX(r);
          dy = seg->PosY(t) - seg->PosY(r);
          dist = sqrt(dx * dx + dy * dy);
 
          // save the closest thing as a spatial anchor
          if ((anchor < 0) || (dist < best))
          {
            anchor = r;
            best = dist;
          }
        }

  // choose most meaningful spatial relation
  if (anchor < 0)
    return rwi->ReadDone(-1);    
  if ((side = side_of(t, anchor)) > 0)
    lex = loc[side];                             // left of/right of/in front of/behind
  else if ((prox = near_to(t, anchor)) > 0)
    lex = loc[prox + 4];                         // near/next to
  else 
    return rwi->ReadDone(-1);         
   
  // directly assert or refute in net
  rpt->StartNote();
  hq = rpt->NewProp(obj, "loc", lex, 0, 1.0, 1, 2);
  rpt->AddArg(hq, "wrt", trk2node(anchor));  
  rpt->FinishNote();
  return rwi->ReadDone(1);
}


//= First call to location check but not allowed to fail.
// answers "Is X behind/next to/to the left of Y?
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_pos_ok0 (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *pos;

  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((pos = desc->Val("arg")) == NULL)
    return -1;
  if ((pos->Val("loc") == NULL) || (pos->Val("wrt") == NULL))
    return -1;
  if ((cst[i] = txt2pos(pos->Lex())) < 0)
    return -1;
  if ((cst[i] == 0) && (pos->Val("wrt", 1) == NULL))       // between
    return -1;
  return 1;
}


//= Determine which spatial relation is being asked about.
// returns 0-6 (not category), negative if unknown

int jhcSceneVis::txt2pos (const char *txt) const
{
  int rel;

  // make sure "left of" matches "to the left of" using loc[] + 7
  for (rel = 0; rel <= 6; rel++)
    if ((strcmp(loc[rel], txt) == 0) ||
        ((rel >= 1) && (rel <= 2) && (strcmp(loc[rel] + 7, txt) == 0)))
      return rel;
  return -1;
}


//= Basic call to location check returns true or false assertion.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_pos_ok (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *pos = desc->Val("arg"), *obj = pos->Val("loc");
  jhcAliaDesc *ref = pos->Val("wrt"), *ref2 = pos->Val("wrt", 1);
  int t, r, r2, rel = cst[i], neg = 1;

  // find the referenced objects and possibly analyze their color
  if (!rwi->Readable())
    return 0;
  if ((t = node2trk(obj)) < 0)
    return rwi->ReadDone(-1);
  if ((r = node2trk(ref)) < 0)
    return rwi->ReadDone(-1);
  r2 = node2trk(ref2);
  seg->SetState(t, 2);                                     // display as green

  // check if selected spatial relation holds
  if (rel == 0)                                            // between
    neg = ((twixt(t, r, r2) > 0) ? 0 : 1);
  else if ((rel >= 1) && (rel <= 4))                       // left of/right of/in front of/behind
    neg = ((side_of(t, r) == rel) ? 0 : 1);
  else if ((rel >= 5) && (rel <= 6))                       // next to/near
    neg = ((near_to(t, r) == (rel - 4)) ? 0 : 1);

  // directly assert or refute in net
  rpt->StartNote();
  pos = rpt->NewProp(obj, "loc", loc[rel], neg, 1.0, 1, 2);
  rpt->AddArg(pos, "wrt", ref);
  if (ref2 != NULL)
    rpt->AddArg(pos, "wrt", ref2);
  rpt->FinishNote();
  return rwi->ReadDone(1);
}


///////////////////////////////////////////////////////////////////////////
//                         Property Comparisons                          //
///////////////////////////////////////////////////////////////////////////

//= First call to feature comparison but not allowed to fail.
// answers "Is X closer/wider/greener than Y?"
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_comp_ok0 (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *hq;

  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((hq = desc->Val("arg")) == NULL)
    return -1;
  if ((hq->Val("hq") == NULL) || (hq->Val("wrt") == NULL))
    return -1;
  if ((cst[i] = txt2comp(hq->Lex())) == 0)
    return -1;
  return 1;
}


//= Determine category of information requested to be compared.
// returns base category + 1 for greater than, negated for less than, zero for unknown

int jhcSceneVis::txt2comp (const char *txt) const
{
  int cat;

  // try "less than" with range values
  for (cat = 0; cat <= 5; cat++)
    if (strcmp(txt, rng0[cat]) == 0)
    return -(cat + 1);

  // try "greater than" with range values
  for (cat = 0; cat <= 5; cat++)
    if (strcmp(txt, rng1[cat]) == 0)
      return(cat + 1);

  // try colors (e.g. "redder")
  for (cat = 6; cat <= 14; cat++)
    if (strcmp(txt, col[cat - 6]) == 0)
      return(cat + 1);
  return 0;
}


//= Basic call to feature comparison returns true or false assertion.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_comp_ok (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *hq = desc->Val("arg");
  jhcAliaDesc *hq2, *obj = hq->Val("hq"), *ref = hq->Val("wrt");
  const char *lex = NULL;
  int t, r, val, comp = cst[i], cat = abs(comp) - 1, neg = 1;

  // find the referenced objects and possibly analyze their color
  if (!rwi->Readable())
    return 0;
  if ((t = node2trk(obj)) < 0)
    return rwi->ReadDone(-1);
  if ((r = node2trk(ref)) < 0)
    return rwi->ReadDone(-1);
  seg->SetState(t, 2);                     // display as green

  // get relevant raw values and test 
  if ((cat >= 6) && (cat <= 14))
  {
    seg->Spectralize(body->Color(), body->Range(), t);
    seg->Spectralize(body->Color(), body->Range(), r);
    val = trk2ccomp(cat + 18, t, r);       // redder (6) -> cat = 24
  }
  else 
    val = trk2rcomp(cat + 18, t, r);
  if (((comp < 0) && (val == 1)) || ((comp > 0) && (val == 2)))
    neg = 0;

  // find property comparative term 
  if ((comp <= -1) && (comp >= -6))        // minimum in range
    lex = rng0[-comp - 1];
  else if ((comp >= 1) && (comp <= 6))     // maximum in range
    lex = rng1[comp - 1];
  else if ((comp >= 7) && (comp <= 15))    // colors and intensities
    lex = col[comp - 7];

  // directly assert or refute in net
  rpt->StartNote();
  hq2 = rpt->NewProp(obj, "hq", lex, neg, 1.0, 1, 2);
  rpt->AddArg(hq2, "wrt", ref);
  rpt->GramTag(hq2, JTAG_ACOMP);
  rpt->FinishNote();
  return rwi->ReadDone(1);
}


///////////////////////////////////////////////////////////////////////////
//                           Object Counting                             //
///////////////////////////////////////////////////////////////////////////

//= First call to object counter but not allowed to fail.
// can take restrictions on size, width, height, and color (barf if others)
// answers "How many big red things to the right of Y are there?"
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_subit0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  return 1;
}


//= Basic call to object counter returns number of matching valid tracks.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_subit (const jhcAliaDesc *desc, int i)
{
  char nums[13][20] = {"none", "one", "two", "three", "four", "five", "six",
                       "seven", "eight", "nine", "ten", "eleven", "twelve"};
  int props[33];
  jhcAliaDesc *obj = desc->Val("arg"), *ref, *ref2;
  int cc, r, r2, t, nt, cnt = 0;

  // determine selection criteria for objects based on query node properties
  if (!rwi->Readable())
    return 0;
  cc = obj_specs(props, &ref, &ref2, obj);
  if (cvt_refs(r, r2, ref, ref2) <= 0)
    return rwi->ReadDone(-1);
  if ((r >= 0) && (cc > 0))
    seg->Spectralize(body->Color(), body->Range(), r);

  // count objects matching description and mark them for display
  nt = seg->ObjLimit();
  for (t = 0; t < nt; t++)
    if (suitable(props, t, r, r2) > 0) 
    {
      cnt++;
      seg->SetState(t, 2);                       // display as green
    }

  // report resulting count
  rpt->StartNote();
  obj = rpt->NewNode("obj", NULL, 0, 0.0);       // does not exist
  rpt->NewProp(obj, "cnt", ((cnt <= cmax) ? nums[cnt] : "lots"));
  prop2net(obj, props, ref, ref2);
  rpt->NewProp(obj, "ako", "object");
  rpt->FinishNote();
  return rwi->ReadDone(1);
}


///////////////////////////////////////////////////////////////////////////
//                           Object Finding                              //
///////////////////////////////////////////////////////////////////////////

//= First call to object detector but not allowed to fail.
// can take restrictions on size, width, height, and color (barf if others)
// answers "Find a thin black thing near Y" repeatedly
// returns 1 if okay, -1 for interpretation error

int jhcSceneVis::vis_enum0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  cst[i] = 0;                          // nothing reported yet
  return 1;
}


//= Basic call to object detector returns one new object matching description each step.
// enumeration limits kept in cst[i] and camt[i], irrelevant if a superlative was used
// returns 1 if done, 0 if still working, -1 for failure

int jhcSceneVis::vis_enum (const jhcAliaDesc *desc, int i)
{
  int props[33];
  jhcAliaDesc *ref, *ref2, *obj = desc->Val("arg");
  int cc, r, r2, sel, t, nt, win, pref, cnt = 0;

  // determine selection criteria for objects based on query node properties
  if (!rwi->Readable())
    return 0;
  cc = obj_specs(props, &ref, &ref2, obj);
  if (cvt_refs(r, r2, ref, ref2) <= 0)
    return rwi->ReadDone(-1);
  sel = net2super(obj);
  if ((r >= 0) && ((cc > 0) || ((sel >= 7) && (sel <= 15))))
    seg->Spectralize(body->Color(), body->Range(), r);

  // mark previously unreported objects that pass all criteria 
  nt = seg->ObjLimit();
  for (t = 0; t < nt; t++)
  {
    seg->SetState(t, 0);
    if (seg->ObjOK(t) && !already(i, t))
      if (seg->SetState(t, suitable(props, t, r, r2)) > 0)
      {
        win = t;                       // in case only one
        cnt++;
      }
  }
  if (cnt <= 0)
  {
    jprintf(1, noisy, "vis_enum %d ==> nothing\n", cst[i]);
    return rwi->ReadDone(1);
  }

  // choose among alternatives either by superlative or innate preference
  if (cnt > 1)
  {
    pref = ((sel != 0) ? sel : pref_prop(props));
    if (pref == 0)
      win = pick_num();
    else if (pref == -100)             // beyond props[] (middle)
      win = pick_mid();
    else
    {
      cache_color(pref, props);     
      win = pick_best(pref, r, r2);    // including +/-35 (leftmost/rightmost)
    }
  }

  // clean up object marks and add to NRI list
  for (t = 0; t < nt; t++)
    seg->SetState(t, 0);
  if (win < 0)
    return rwi->ReadDone(1);
  record(i, win);

  // get semantic network node for object and assert that it meets all criteria
  rpt->StartNote();                      
  obj = obj_node(win);                 // either track node or new one 
  jprintf(1, noisy, "vis_enum %d ==> %s\n", cst[i], obj->Nick());
  prop2net(obj, props, ref, ref2);
  super2net(obj, sel);
  rpt->FinishNote();

  // more objects might appear by next call if just enumerating
  return rwi->ReadDone(((sel != 0) || (cst[i] >= 8)) ? 1 : 0); 
}


//= Pick some criterion to maximize or minimize.
// returns category + 1, negative to minimize

int jhcSceneVis::pref_prop (int props[]) const
{
  int cat;

  // prefer to maximize a hue else an intensity
  for (cat = 6; cat <= 14; cat++)
    if (props[cat] > 0)
      return(cat + 1);

  // prefer range properties in order listed
  for (cat = 0; cat <= 5; cat++)
    if ((props[cat] > 0) && (props[cat] != 3))
      return((props[cat] > 3) ? (cat + 1) : -(cat + 1));

  // pick a relative spatial location, dimension, or color
  for (cat = 15; cat <= 32; cat++)
    if (props[cat] > 0)
      return -(cat + 1);
  return 0;                           
}


//= See if a particular track ID has already been reported.
// cst[i] holds how many returned, old ids stored in vectors cpos[i] and cdir[i]

bool jhcSceneVis::already (int i, int t) const
{
  int j, nr = __min(8, cst[i]), id = seg->ObjID(t);

  for (j = 0; j < nr; j++)
    if (((j < 4)  && (cpos[i].VRef(j) == id)) ||
        ((j >= 4) && (cdir[i].VRef(j - 4) == id)))
      return true;
  return false;
}


//= Save a reported track ID so that it is not selected again.
// cst[i] holds how many returned, old ids stored in vectors cpos[i] and cdir[i]

void jhcSceneVis::record (int i, int t)
{
  int j = cst[i], id = seg->ObjID(t);

  if ((j < 0) || (j >= 8))
    return;
  if (j < 4)
    cpos[i].VSet(j, id);
  else 
    cdir[i].VSet(j - 4, id);
  cst[i]++;
}


//= Pick either highest associated node ID or highest track ID.
// assumes all tracks already marked as to suitability wrt criteria

int jhcSceneVis::pick_num () const
{
  const jhcAliaDesc *n;
  int t, id, nt = seg->ObjLimit(), hi = -1, win = -1;

  // look for already nodified object with highest node id
  for (t = 0; t < nt; t++)
    if (seg->GetState(t) > 0) 
      if ((n = trk2node(t)) != NULL)
      {
        id = n->Inst();
        if (id > hi)   
        {
          win = t;
          hi = id;
        }
      }
  if (win >= 0)
    return win;

  // look for object with highest tracking ID (most recent)
  for (t = 0; t < nt; t++)
    if (seg->GetState(t) > 0) 
    {
      id = seg->ObjID(t);
      if (id > hi)   
      {
        win = t;
        hi = id;
      }
    }
  return win;
}


//= Pick object closest to lateral center of group.
// assumes all tracks already marked as to suitability wrt criteria

int jhcSceneVis::pick_mid () const
{
  double x, y, lf, rt, bot, top, mx, my, d2, best;
  int t, nt = seg->ObjLimit(), any = 0, win = -1;

  // find span of suitable objects using lateral position
  for (t = 0; t < nt; t++)
    if (seg->GetState(t) > 0) 
    {
      x = seg->PosX(t);
      y = seg->PosY(t);
      if (any <= 0) 
      {
        lf  = x;
        rt  = x;
        bot = y;
        top = y;
        any = 1;
      }
      else
      {
        lf  = __min(lf, x);
        rt  = __max(rt, x);
        bot = __min(bot, y);
        top = __max(top, y);
      }
    }

  // find suitable object closest to middle of span
  mx = 0.5 * (lf + rt);
  my = 0.5 * (bot + top);
  for (t = 0; t < nt; t++)
    if (seg->GetState(t) > 0) 
    {
      x = seg->PosX(t) - mx;
      y = seg->PosY(t) - my;
      d2 = x * x + y * y;
      if ((win < 0) || (d2 < best))
      {
        win = t;
        best = d2;
      }
    }
  return win;
}


//= Compute all object colors if preference needs color but no requirement called for it.

void jhcSceneVis::cache_color (int pref, int props[])
{
  int cat, t, nt = seg->ObjLimit();

  // see if preference is "the reddest" or equivalent
  if ((pref < 7) || (pref > 15))                 
    return;

  // objects that passed all props might have color already computed 
  for (cat = 6; cat <= 14; cat++)      
    if (props[cat] > 0)                // known to have some color
      return;
  for (cat = 24; cat <= 32; cat++)     
    if (props[cat] > 0)                // known to be more colorful than ref
      return;  

  // color not needed up to now so compute for all passed objects
  for (t = 0; t < nt; t++)
    if (seg->GetState(t) > 0)
      seg->Spectralize(body->Color(), body->Range(), t);
}


//= Select suitable track with maximum or minimum of some value.
// pref = category + 1 with positive being max and negative being min
// assumes all tracks already marked as to suitability wrt criteria

int jhcSceneVis::pick_best (int pref, int r, int r2) const
{
  double v, best;
  int t, nt = seg->ObjLimit(), cat = abs(pref) - 1, win = -1;

  for (t = 0; t < nt; t++)
    if (seg->GetState(t) > 0) 
    {
      v = pref_val(cat, t, r, r2);
      if ((win < 0) || ((pref > 0) && (v > best)) || ((pref < 0) && (v < best)))
      {
        win = t;
        best = v;
      }
    } 
  return win;
}


///////////////////////////////////////////////////////////////////////////
//                            Track Testing                              //
///////////////////////////////////////////////////////////////////////////

//= Determine if some track matches all relevant properties and relations in vector.
// properties 0-31 = DSLWTH-ROYGBP-KXW-msp-dslwth-roygbp-kxw
// assumes color has already been found for reference if color comparison needed
// returns 1 if ok, 0 if not

int jhcSceneVis::suitable (int props[], int t, int r, int r2) const
{
  if (rng_test(props, t, r) > 0)
    if (loc_test(props, t, r, r2) > 0)
      return col_test(props, t, r);
  return 0;
}


//= Test range measurements to see if they are compatible with desires.
// 
//   category:     0      1     2     3       4       5
//   property: distance size length width thickness height
//
//   category:   18      19     20    21     22     23
//   relation: farther bigger longer wider thicker taller
// 
// for properties des = very low(1), low(2), medium(3), hi(4), very hi(5)
// for relations des = t smaller than r(1), t bigger than r(2)
// returns 1 if ok, 0 if not

int jhcSceneVis::rng_test (int props[], int t, int r) const
{
  int cat, des, bin;

  // test absolute range measurements
  for (cat = 0; cat <= 5; cat++)
    if ((des = props[cat]) > 0)
    {
      // infers that des = 4 (big) is satisfied by bin = 5 (very big) 
      bin = trk2rng(cat, t);
      if (((des >= 3) && (bin < des)) ||
          ((des <= 3) && (bin > des)))
        return 0;
     }

  // test range values relative to reference object (des = 0-2)
  for (cat = 18; cat <= 23; cat++)
    if ((des = props[cat]) > 0)
      if (trk2rcomp(cat, t, r) != des)
        return 0;
  return 1;
}


//= Test if locations of object track are compatible with desires. 
// 
//   category:  15    16   17  
//   relation: tween side prox 
// 
// tween: holds(1), side: left(1) right(2), prox: near(1) next to(2) 
// infers that des = 1 (near) is satisfied by val = 2 (next to) 
// returns 1 if ok, 0 if not

int jhcSceneVis::loc_test (int props[], int t, int r, int r2) const
{
  int des;

  if (props[15] > 0)    
    if ((r < 0) || (r2 < 0) || (twixt(t, r, r2) <= 0))
      return 0;
  if ((des = props[16]) > 0) 
    if ((r < 0) || (side_of(t, r) != des))
      return 0;
  if ((des = props[17]) > 0) 
    if ((r < 0) || (near_to(t, r) < des))
      return 0; 
  return 1;
}


//= See if color values are required for this track.
//
//   category:  6     7      8     9    10    11     12   13   14
//   property: red orange yellow green blue purple black gray white
//
//   category:   24     25       26      27     28     29      30      31    32
//   relation: redder oranger yellower greener bluer purpler blacker grayer whiter
//
// returns 1 and analyzes color if true, 0 if not needed

int jhcSceneVis::col_test (int props[], int t, int r) const
{
  int cnum, des;

  // see if any color values are required for this track
  for (cnum = 0; cnum <= 8; cnum++)
    if ((props[cnum + 6] > 0) || (props[cnum + 24] > 0)) 
      break;
  if (cnum > 8)
    return 1;
  seg->Spectralize(body->Color(), body->Range(), t);

  // test if desired colors for object track are present or not
  for (cnum = 0; cnum <= 8; cnum++)
    if (props[cnum + 6] > 0)         
      if (seg->DegColor(t, cnum) < 2)   
        return 0;

  // test if comparison of color percentages between track and reference is as desired
  for (cnum = 0; cnum <= 8; cnum++)
    if ((des = props[cnum + 24]) > 0)         
      if (trk2ccomp(cnum, t, r) != des)
        return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Track Properties                            //
///////////////////////////////////////////////////////////////////////////

//= Determine the value of some range category for a specific visual track.
// 
//   category:     0      1     2     3       4       5
//   property: distance size length width thickness height
//
// returns 0-5 for the particular measurement requested

int jhcSceneVis::trk2rng (int cat, int t) const
{
  double val;

  // sanity check
  if ((cat < 0) || (cat > 5))
    return 0;
  val = rng_val(cat, t);

  // quantize based on different thresholds
  if (cat == 0)                                  
    return quantize(dist0, dist1, dist2, dist3, val);  // distance
  if (cat == 1)
    return quantize( 0.0, sz1,  sz2,   0.0, val);      // size
  if (cat == 2)
    return quantize(len0, len1, len2, len3, val);      // length (ratio)
  if (cat == 3)
    return quantize( 0.0, wid1, wid2,  0.0, val);      // width
  if (cat == 4)
    return quantize(thk0, thk1, thk2, thk3, val);      // thickness (ratio)
  if (cat == 5)
    return quantize( 0.0, ht1,  ht2,   0.0, val);      // height
  return 0;
}


//= Compare some object track to a reference using a range value;
//
//   category:   18      19     20    21     22     23
//   relation: farther bigger longer wider thicker taller
// 
// returns 2 if t > r (e.g. taller), 1 if t < r (e.g. shorter), 0 if similar 

int jhcSceneVis::trk2rcomp (int cat, int t, int r) const
{
  double vt, vr, f = 1.0 + rdom;

  // sanity check
  if ((cat < 18) || (cat > 23))
    return 0; 
  vt = rng_val(cat - 18, t);
  vr = rng_val(cat - 18, r);

  // significance comparison (absolute measurements)
  if (vt > (f * vr))
    return 2;
  if ((f * vt) < vr)
    return 1;
  return 0;
}


//= Get the raw numerical value of some range value property.
// handles categories 0-5 (DSLWTH) only
// returns relevant raw value, negative if unknown category

double jhcSceneVis::rng_val (int cat, int t) const
{
  double maj, min, ht, lo, hi, vol, mid;

  // handle non-dimension 
  if (cat == 0)                 // distance
    return seg->DistXY(t);     

  // get some basic object measurements
  maj = seg->Major(t);
  min = seg->Minor(t); 
  ht  = seg->SizeZ(t);
  lo  = __min(ht, __min(maj, min));
  hi  = __max(ht, __max(maj, min));
  vol = maj * min * ht;
  mid = vol / (hi * lo);

  // select or compute appropriate dimension
  if (cat == 1)
    return sqrt(vol / lo);     // size = abs
  if (cat == 2)
    return hi / mid;           // length = ratio
  if (cat == 3)
    return min;                // width = abs min(x y)
  if (cat == 4)
    return lo / mid;           // thickness = ratio
  if (cat == 5)
    return ht;                 // height = abs z
  return -1.0;
}


//= Classify value into one of five bins based on set of thresholds.
// returns 1-5 (e.g. very small, small, medium big, big, very big)

int jhcSceneVis::quantize (double v0, double v1, double v2, double v3, double val) const
{
  double v00 = ((v0 > 0.0) ? v0 : v1 * v1 / v2), v33 = ((v3 > 0.0) ? v3 : v2 * v2 / v1);

  if (val >= v33)
    return 5;
  if (val >= v2)
    return 4;
  if (val > v1)
    return 3;
  if (val > v00)
    return 2;
  return 1;
}


//= Determine the value of some location for a track relative to a reference.
// 
//   category:  15    16   17  
//   relation: tween side prox 
// 
// returns 0-4 for spatial relation requested

int jhcSceneVis::trk2loc (int cat, int t, int r, int r2) const
{
  if ((cat == 15) && (r >= 0) && (r2 >= 0))      // between (0-1)
    return twixt(t, r, r2);
  if ((cat == 16) && (r >= 0))                   // front/left/right/behind (0-4)
    return side_of(t, r);            
  if ((cat == 17) && (r >= 0))                   // near (0-2)
    return near_to(t, r);
  return 0;
}


//= Decide whether object is between two reference objects based on line between them.
// returns 1 or 0

int jhcSceneVis::twixt (int t, int r, int r2) const
{
  double rx = seg->PosX(r), ry = seg->PosY(r), r2x = seg->PosX(r2), r2y = seg->PosY(r2);
  double sx = rx - r2x, sy = ry - r2y, len = sqrt(sx * sx + sy * sy);
  double dx = seg->PosX(t) - 0.5 * (rx + r2x), dy = seg->PosY(t) - 0.5 * (ry + r2y);

  if ((t == r) || (t == r2))
    return 0;
  return((sqrt(dx * dx + dy * dy) < (tween * len)) ? 1 : 0);
}    


//= Decides whether object is left/right of reference object based on reference endpoints.
// returns 1 (left), 2 (right), 3 (front), 4 (behind), or diagonal (0)

int jhcSceneVis::side_of (int t, int r) const
{
  jhcMatrix tloc(4), rloc(4);
  double ang;

  // get direction of focus object (-180 to 180) where Y is forward
  if (t == r)
    return 0;
  seg->World(tloc, t);
  seg->World(rloc, r);
  ang = R2D * atan2(tloc.Y() - rloc.Y(), tloc.X() - rloc.X());

  // resolve into one of four zones (or none)
  if ((ang <= (-180.0 + sdev)) || (ang >= (180.0 - sdev)))     // lower wx
    return 1;
  if ((ang <= sdev) && (ang >= -sdev))                         // higher wx
    return 2;
  if ((ang <= (-90.0 + sdev)) && (ang >= (-90.0 - sdev)))      // lower wy
    return 3;
  if ((ang <= (90.0 + sdev)) && (ang >= (90.0 - sdev)))        // higher wy
    return 4;
  return 0;                                                    // diagonal
}


//= Decides whether object is close to reference object based on size of reference.
// returns 1 (near), 2 (next to), or 0 (far)

int jhcSceneVis::near_to (int t, int r) const
{
  double maj = seg->Major(r), min = seg->Minor(r), ht = seg->SizeZ(r);
  double dx = seg->PosX(t) - seg->PosX(r), dy = seg->PosY(t) - seg->PosY(r);
  double dim = __max(__max(maj, min), ht), dist = sqrt(dx * dx + dy * dy);

  if (t == r)
    return 0;
  if (dist < (buddy * dim))
    return 2;
  if (dist < (hood * dim))
    return 1;
  return 0;
}


//= Compare the percentage of some color in two objects given track numbers.
//
//   category:   24     25       26      27     28     29      30      31    32
//   relation: redder oranger yellower greener bluer purpler blacker grayer whiter
//
// returns 2 if t > r (e.g. redder), 1 if t < r (e.g. less red), 0 if fairly similar

int jhcSceneVis::trk2ccomp (int cat, int t, int r) const
{
  double vt, vr;

  // sanity check
  if ((cat < 24) || (cat > 32))
    return 0;
  vt = seg->AmtColor(t, cat - 24);
  vr = seg->AmtColor(r, cat - 24);

  // significance comparison (already in fractions of area)
  if (vt > (vr + cdom))
    return 2;
  if ((vt + cdom) < vr)
    return 1;
  return 0;
}


//= Find a scalar value for the (relative) measurement underlying some category
// assumes r (and r2) are bound properly for cat = 15-32

double jhcSceneVis::pref_val (int cat, int t, int r, int r2) const
{
  jhcMatrix loc(4);
  double dx, dy;

  // simple range values and color fractions
  if ((cat >= 0) && (cat <= 5))
    return rng_val(cat, t);
  if ((cat >= 6) && (cat <= 14)) 
    return seg->AmtColor(t, cat - 6);
  
  // locations relative to reference(s)
  if (cat == 15)
  {
    // best tween = distance to middle (always min)
    dx = seg->PosX(t) - 0.5 * (seg->PosX(r) + seg->PosX(r2));
    dy = seg->PosY(t) - 0.5 * (seg->PosY(r) + seg->PosY(r2));
    return sqrt(dx * dx + dy * dy);    
  }
  if (cat == 16)
  {
    // best side = robot-based X coordinate (either max or min)
    seg->World(loc, t);
    return loc.X();
  }
  if (cat == 17)
  {
    // best prox = simple distance (always min)
    dx = seg->PosX(t) - seg->PosX(r);
    dy = seg->PosY(t) - seg->PosY(r);
    return sqrt(dx * dx + dy * dy);
  }

  // range values relative to reference (never reported in net)
  if ((cat >= 18) && (cat <= 23))
    return rng_val(cat, t);  

  // color values relative to reference (never reported in net)
  if ((cat >= 24) && (cat <= 32)) 
    return seg->AmtColor(t, cat - 24);

  // special naked superlative (leftmost / rightmost)
  if (cat == 100)                       
  {
    seg->World(loc, t);
    return loc.X();
  }
  return -1.0;
}


///////////////////////////////////////////////////////////////////////////
//                            Net Properties                             //
///////////////////////////////////////////////////////////////////////////

//= Determine the required values for 33 properties based on semantic network for object.
// properties: DSLWTH-ROYGBP-KXW-psn-dslwth-roygbp-kxw where lowercase are relative to reference
// will only record one binding for ref (first one), ref2 is for "between"
// returns 1 if some color comparison needed, 0 if none (so ref needs no color analysis)

int jhcSceneVis::obj_specs (int props[], jhcAliaDesc **ref, jhcAliaDesc **ref2, const jhcAliaDesc *obj) const
{
  int cat;

  // default to no reference objects
  *ref = NULL;
  *ref2 = NULL;

  // find desired range values and colors
  for (cat = 0; cat <= 5; cat++)
    props[cat] = net2val(obj, cat);
  for (cat = 6; cat <= 14; cat++)
    props[cat] = net2col(obj, cat - 6);

  // find desired location, range value, or color relative to some reference(s)
  for (cat = 15; cat <= 17; cat++)
    props[cat] = net2pos(ref, ref2, obj, cat);
  for (cat = 18; cat <= 32; cat++)
    props[cat] = net2comp(ref, obj, cat);

  // check for any comparisions to color of reference
  for (cat = 24; cat <= 32; cat++)
    if (props[cat] > 0)
      return 1;
  return 0;
}


//= Get putative quantized value for some property category given a semantic network node.
// only responds to categories 0-5 (DSLWTH)
// returns 1-5 typically, 0 if no requirement for this category

int jhcSceneVis::net2val (const jhcAliaDesc *obj, int cat) const
{
  char mid[40];
  const jhcAliaDesc *p, *d;
  int i = 0, j = 0;

  // sanity check
  if ((obj == NULL) || (cat < 0) || (cat > 5))
    return 0;

  // search for positive HQ facts
  while ((p = obj->Fact("hq", i++)) != NULL)
    if (p->Visible() && (p->Neg() <= 0) && (p->Val("wrt") == NULL))         
    {
      // value ranges 1-5: distance -> "medium close" not "medium far"
      if (p->LexMatch(rng0[cat]))
      {
        // check for "very" modifier
        while ((d = p->Fact("deg", j++)) != NULL)
          if (d->LexMatch("very"))
            return 1;
        return 2;
      }
      if (p->LexMatch(rng1[cat]))
      {
        // check for "very" modifier
        while ((d = p->Fact("deg", j++)) != NULL)
          if (d->LexMatch("very"))
            return 5;
        return 4;
      }
      sprintf_s(mid, "medium %s", ((cat <= 0) ? rng0[cat] : rng1[cat]));
      if (p->LexMatch(mid))
        return 3;
    }
      else if ((cat >= 6) && (cat <= 14))
        if (p->LexMatch(col[cat - 6]))             // color: only 0 or 3
          return 3;
   return 0;
}


//= Determine whether an object should have some particular color.
// returns 0 or 3

int jhcSceneVis::net2col (const jhcAliaDesc *obj, int cnum) const
{
  const jhcAliaDesc *p; 
  int i = 0;

  // sanity check
  if ((obj == NULL) || (cnum < 0) || (cnum > 8))
    return 0;

  // search for positive HQ facts
  while ((p = obj->Fact("hq", i++)) != NULL)
    if (p->Visible() && (p->Neg() <= 0) && (p->Val("wrt") == NULL))         
      if (p->LexMatch(col[cnum]))             // color: only 0 or 3
        return 3;
  return 0;
}


//= Look at spatial relations of object in semantic network to get value for category.
// cat: 15 = between, 16 = left (1), right(2), front(3), behind(4), 17 = near(1), next to(2) 
// return value for given category, 0 if no constraint (or if ref already bound) 

int jhcSceneVis::net2pos (jhcAliaDesc **ref, jhcAliaDesc **ref2, const jhcAliaDesc *obj, int cat) const
{
  const jhcAliaDesc *p;
  jhcAliaDesc *wrt2, *wrt = NULL;
  int j, prox = 0, i = 0;
 
  // scan through relative spatial locations for object
  while ((p = obj->Fact("loc", i++)) != NULL)
    if (p->Visible() && (p->Neg() <= 0) && ((wrt = p->Val("wrt")) != NULL))
      if (((*ref == NULL) || (wrt == *ref)) && !wrt->LexMatch("all"))
      {
        if (cat == 15)                 
        {
          // "between"
          if ((wrt2 = p->Val("wrt", 1)) != NULL)
            if (((*ref2 == NULL) || (wrt2 == *ref2)) && p->LexMatch(loc[0]))
            {
              *ref = wrt;
              *ref2 = wrt2;
              return 1;
            }
        }
        else if (cat == 16)            
        {
          // "left of/right of/in front of/behind"
          for (j = 1; j <= 4; j++)
            if (p->LexMatch(loc[j]) || ((j <= 2) && p->LexMatch(loc[j] + 7)))
            {
              *ref = wrt;
              return j;
            }
        }
        else if (cat == 17)            
        {
          // "near/next to" (may be both!)
          if (p->LexMatch(loc[5]))
            prox = __max(1, prox);       
          if (p->LexMatch(loc[6]))
            prox = 2;
        }
      }

  // check for some combined "next to" and "near" value
  if (prox <= 0)
    return 0;
  *ref = wrt;
  return prox;
}


//= For range values and colors determine if object should be more or less than reference.
// only responds for cat = 18 to 32, binds ref to reference object (can only be one)

int jhcSceneVis::net2comp (jhcAliaDesc **ref, const jhcAliaDesc *obj, int cat) const
{
  const jhcAliaDesc *p;
  jhcAliaDesc *wrt;
  int val, i = 0;

  // sanity check
  if ((cat < 18) || (cat > 32))
    return 0;

  // look at all positive HQ assertions that are relative
  while ((p = obj->Fact("hq", i++)) != NULL)
    if (p->Visible() && (p->Neg() <= 0) && ((wrt = p->Val("wrt")) != NULL))
      if (((*ref == NULL) || (wrt == *ref)) && !wrt->LexMatch("all"))
      {
        // test for max or min of some range value (dslwth)
        if ((cat >= 18) && (cat <= 23))
        {
          if (p->LexMatch(rng0[cat - 18]))
            val = 1;
          else if (p->LexMatch(rng1[cat - 18]))
            val = 2;
          else
            continue;
          *ref = wrt;
          return val;
        }

        // test for max of some color (roygbp-kxw)
        if ((cat >= 24) && (cat <= 32))
        {
          if (!p->LexMatch(col[cat - 24]))
            continue;
          *ref = wrt;
          return 2;
        }
      }
  return 0;
}


//= Determines if object is described with some superlative like "biggest".
// returns 0 if none else category + 1 with positive being max and negative being min

int jhcSceneVis::net2super (const jhcAliaDesc *obj) const
{
  const jhcAliaDesc *p, *r;
  int cat, i = 0;

  // look at all positive HQ assertions
  while ((p = obj->Fact("hq", i++)) != NULL)
    if (p->Visible() && (p->Neg() <= 0))
    {
      // location (leftmost, rightmost, middle)
      if (p->LexMatch(sloc[0]))
        return -100;                   // after all props[]
      if (p->LexMatch(sloc[1]))
        return -101;
      if (p->LexMatch(sloc[2]))
        return 101;

      // check for correct reference ("all")
      if ((r = p->Val("wrt")) == NULL)
        continue;
      if (!r->LexMatch("all"))
        continue;

      // value ranges 
      for (cat = 0; cat <= 5; cat++)
        if (p->LexMatch(rng0[cat]))
          return -(cat + 1);
        else if (p->LexMatch(rng1[cat]))
          return(cat + 1);

      // colors 
      for (cat = 6; cat <= 14; cat++)
        if (p->LexMatch(col[cat - 6]))
          return(cat + 1);
    }
  return 0;
}


//= Make sure reference objects refer to current visual tracks.
// assigns track numbers r and r2 and returns 1 if okay, 0 if problem

int jhcSceneVis::cvt_refs (int& r, int& r2, const jhcAliaDesc *ref, const jhcAliaDesc *ref2) const
{
  // setup defaults
  r = -1;
  r2 = -1;
  
  // look for main reference object
  if (ref == NULL)
    return 1;
  if ((r = node2trk(ref)) < 0)
    return 0;

  // look for possible secondary reference (for "between")
  if (ref2 == NULL)
    return 1;
  if ((r2 = node2trk(ref2)) < 0)
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Net Assertions                              //
///////////////////////////////////////////////////////////////////////////

//= See if node already assigned to visual object, else create new one.
// should be called inside rpt->StartNote to include "ako object" fact

jhcAliaDesc *jhcSceneVis::obj_node (int t)
{
  jhcAliaDesc *obj = trk2node(t);

  if (obj == NULL)
  {
    obj = rpt->NewNode("obj");
    rpt->NewProp(obj, "ako", "object", 0, 1.0, 1);
    rpt->NewProp(obj, "hq", "visible", 0, 1.0, 1);
    rpt->VisAssoc(seg->ObjID(t), obj);
    seg->SetTag(t, obj->Nick());
  }
  rpt->NewFound(obj);                  // make eligible for FIND
  seg->SetState(t, 2);                 // display as green
  return obj;
}


//= Make assertions in semantic net that object has all the properties in array.

void jhcSceneVis::prop2net (jhcAliaDesc *obj, int props[], jhcAliaDesc *ref, jhcAliaDesc *ref2)
{
  jhcAliaDesc *p;
  const char *lex;
  int cat, val;

  // basic range values and colors
  for (cat = 0; cat <= 5; cat++)
    rng2net(obj, cat, props[cat]);
  for (cat = 6; cat <= 14; cat++)
    if (props[cat] > 0)
      rpt->NewProp(obj, "hq", col[cat - 6], 0, 1.0, 1);

  // relative locations, range values, and colors 
  for (cat = 15; cat <= 32; cat++)
    if ((val = props[cat]) > 0)
    {
      // determine relationship term
      if (cat == 15) 
        lex = loc[0];                  // between
      else if (cat == 16) 
        lex = loc[val];                // left of/right of/in front of/behind
      else if (cat == 17) 
        lex = loc[val + 4];            // near/next to
      else if (cat <= 23)
        lex = ((val <= 1) ? rng0[cat - 18] : rng1[cat - 18]);
      else
        lex = col[cat - 24];

      // add relationship with reference(s)
      p = rpt->NewProp(obj, ((cat <= 17) ? "loc" : "hq"), lex, 0, 1.0, 1, 2);
      rpt->AddArg(p, "wrt", ref);
      if (cat == 15)
        rpt->AddArg(p, "wrt", ref2);
    }
} 


//= Make assertion about some range value of object.
// 
//   category:     0      1     2     3       4       5
//   property: distance size length width thickness height
//
// handles val = 1-5 (e.g. very small, small, medium big, big, very big)

void jhcSceneVis::rng2net (jhcAliaDesc *obj, int cat, int val)
{
  char mid[40];

  // see if property was required
  if ((val <= 0) || (cat < 0) || (cat > 5))
    return;

  // value ranges 1-5: distance -> "medium close" not "medium far"
  if (val == 1) 
    rpt->NewDeg(obj, "hq", rng0[cat], "very", 0, 1.0, 1);
  else if (val == 2)
    rpt->NewProp(obj, "hq", rng0[cat], 0, 1.0, 1);
  else if (val == 3)
  {
    sprintf_s(mid, "medium %s", ((cat <= 0) ? rng0[cat] : rng1[cat]));
    rpt->NewProp(obj, "hq", mid, 0, 1.0, 1);
  }
  else if (val == 4)
    rpt->NewProp(obj, "hq", rng1[cat], 0, 1.0, 1);
  else if (val == 5)
    rpt->NewDeg(obj, "hq", rng1[cat], "very", 0, 1.0, 1);
}


//= Affirm or deny that the object has the desired value of some measurement.
// 
//   category:     0      1     2     3       4       5
//   property: distance size length width thickness height
//
// des and val = 1-5 (e.g. very small, small, medium big, big, very big)
// handles case of des = "very big" and val = "big" (just not "very")

void jhcSceneVis::des2net (jhcAliaDesc *obj, int cat, int des, int val)
{
  jhcAliaDesc *hq;
  char mid[40];

  // sanity check
  if ((des == 0) || (val == 0) || (cat < 0) || (cat > 5))
    return;

  // deny "is it very small?" -> "not very small" or "not very not small" 
  if (des <= 2)
  {
    hq = rpt->NewProp(obj, "hq", rng0[cat], ((val <= 2) ? 0 : 1), 1.0, 1);     
    if (des <= 1)
      rpt->NewProp(hq, "deg", "very", ((val <= 1) ? 0 : 1), 1.0, 1);
  }
  else if (des == 3)
  {
    sprintf_s(mid, "medium %s", ((cat <= 0) ? rng0[cat] : rng1[cat]));   // for "close"
    rpt->NewProp(obj, "hq", mid, ((val == des) ? 0 : 1), 1.0, 1);
  }
  else if (des >= 4)
  {
    hq = rpt->NewProp(obj, "hq", rng1[cat], ((val >= 2) ? 0 : 1), 1.0, 1);     
    if (des >= 5)
      rpt->NewProp(hq, "deg", "very", ((val >= 5) ? 0 : 1), 1.0, 1);
  }
}


//= Make superlative assertion about object in semantic network.
// abs(sel) = category + 1 while sign gives maximum or minimum

void jhcSceneVis::super2net (jhcAliaDesc *obj, int sel) const
{
  jhcAliaDesc *hq;
  const char *val = NULL;

  // sanity check
  if (sel == 0)
    return;

  // find property superlative term 
  if ((sel <= -1) && (sel >= -6))          // minimum in range
    val = rng0[-sel - 1];
  else if ((sel >= 1) && (sel <= 6))       // maximum in range
    val = rng1[sel - 1];
  else if ((sel >= 7) && (sel <= 15))      // colors and intensities
    val = col[sel - 7];
  else if (sel == -100)                    // spatial position
    val = sloc[0];
  else if (sel == -101)                     
    val = sloc[1];
  else if (sel == 101)
    val = sloc[2];

  // make basic assertion and check for naked superlative
  if (val == NULL)
    return;
  if (abs(sel) >= 100)
    hq = rpt->NewProp(obj, "hq", val, 0, 1.0, 1);
  else
  {
    // add reference object for most 
    hq = rpt->NewProp(obj, "hq", val, 0, 1.0, 1, 2);
    rpt->AddArg(hq, "wrt", rpt->NewNode("obj", "all"));
    rpt->GramTag(hq, JTAG_ASUP);
  }
}


///////////////////////////////////////////////////////////////////////////
//                               Debugging                               //
///////////////////////////////////////////////////////////////////////////

//= Convert a property category into a text name.

const char *jhcSceneVis::cat2txt (int cat) const
{
  if ((cat >= 0) && (cat <= 5))
    return rng[cat];
  if ((cat >= 6) && (cat <= 14))
    return col[cat - 6];
  if ((cat >= 15) && (cat <= 17))
    return sloc[cat - 12];             // added to array for convenience
  return NULL;
}

