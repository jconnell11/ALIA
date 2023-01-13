// jhcSupport.cpp : interface to ELI surface finder kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2023 Etaoin Systems
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

#include "Interface/jms_x.h"           // common video

#include "Grounding/jhcSupport.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSupport::~jhcSupport ()
{
}


//= Default constructor initializes certain values.

jhcSupport::jhcSupport ()
{
  int i;

  // instance identification
  ver = 1.65;
  strcpy_s(tag, "Support");

  // set size of stored positions
  for (i = 0; i < smax; i++)
    saved[i].SetSize(4);

  // set up state
  Platform(NULL);
  rpt = NULL;

  // processing parameters
  Defaults();
}


//= Attach physical enhanced body and make pointers to some pieces.

void jhcSupport::Platform (jhcEliGrok *io) 
{
  rwi = io;
  tab = NULL;
  neck = NULL;
  lift = NULL;
  if (rwi != NULL)
  {
    tab  = &(rwi->tab);
    neck = rwi->neck;
    lift = rwi->lift;
  }
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for quantizing height and distance.

int jhcSupport::height_params (const char *fname)
{
  jhcParam *ps = &hps;
  int ok;

  ps->SetTag("sup_ht", 0);
  ps->NextSpecF( &hmax, 48.0, "Max surface height (in)");
  ps->NextSpecF( &havg, 36.0, "High avg height (in)");
  ps->NextSpecF( &hmth, 33.0, "High-mid threshold (in)");
  ps->NextSpecF( &mavg, 28.5, "Mid avg height (in)");
  ps->NextSpecF( &mlth, 22.0, "Mid-low threshold (in)");
  ps->NextSpecF( &lavg, 16.0, "Low avg height (in)");

  ps->NextSpecF( &flr,   4.0, "Floor threshold (in)");
  ps->NextSpecF( &dh,    1.0, "Match height error (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for quantizing position and azimuth.

int jhcSupport::location_params (const char *fname)
{
  jhcParam *ps = &lps;
  int ok;

  ps->SetTag("sup_loc", 0);
  ps->NextSpecF( &dfar,  96.0, "Far-mid threshold (in)");
  ps->NextSpecF( &dmid,  48.0, "Mid-close threshold (in)");
  ps->NextSpecF( &band,  24.0, "Distance band width (in)");
  ps->NextSpecF( &dxy,    6.0, "Match position error (in)");
  ps->NextSpecF( &hfov,  50.0, "Horizontal view span (deg)");
  ps->NextSpecF( &vfov,  40.0, "Vertical view span (deg)");

  ps->NextSpecF( &atol,   3.0, "Gaze done limit (deg)");
  ps->NextSpecF( &drop, 144.0, "Abandon patch distance (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for signaling new and close tables.

int jhcSupport::event_params (const char *fname)
{
  jhcParam *ps = &eps;
  int ok;

  ps->SetTag("sup_evt", 0);
  ps->NextSpecF( &d1,    50.0, "Detection drop out range (in)");
  ps->NextSpecF( &d0,    48.0, "Detection notice range (in)");
  ps->NextSpecF( &dhys,  26.0, "Table no longer near (in)");
  ps->NextSpecF( &dnear, 24.0, "Table near range (in)");
  ps->Skip();
  ps->NextSpec4( &tnew,   5,   "Detections before event");

  ps->NextSpecF( &h1,    36.0, "Max height for table (in)");
  ps->NextSpecF( &h0,    12.0, "Min height for table (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for tracking and approaching surface patches.

int jhcSupport::track_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("sup_trk", 0);
  ps->NextSpecF( &ztol,   3.0, "Height offset for match (in)");
  ps->NextSpecF( &xytol,  8.0, "Center offset for match (in)");
  ps->NextSpecF( &mix,    0.1, "Blending of new detection");
  ps->NextSpecF( &inset,  6.0, "Inset of gaze from edge (in)");
  ps->NextSpecF( &gtol,   5.0, "Current gaze tolerance (deg)");
  ps->NextSpecF( &gacc,  10.0, "Adequate gaze offset (deg)");

  ps->NextSpecF( &app,   22.0, "Desired approach dist (in)");
  ps->NextSpecF( &acc,   28.0, "Adequate approach dist (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcSupport::Defaults (const char *fname)
{
  int ok = 1;

  ok &= height_params(fname);
  ok &= location_params(fname);
  ok &= event_params(fname);
  ok &= track_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSupport::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= hps.SaveVals(fname);
  ok &= lps.SaveVals(fname);
  ok &= eps.SaveVals(fname);
  ok &= tps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Set up for new run of system.

void jhcSupport::local_reset (jhcAliaNote *top)
{
  int i;

  // noisy messages
  rpt = top;
  dbg = 1;

  // no surfaces nearby yet
  tok = 0;
  any = 0;
  prox = 0;

  // no saved surface markers
  for (i = 0; i < smax; i++)
    sid[i] = 0;
  last_id = 0;
//  current = -1;
}


//= Post any spontaneous observations to attention queue.

void jhcSupport::local_volunteer ()
{
  update_patches();
  table_seen();
  table_close();
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcSupport::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(surf_enum);
  JCMD_SET(surf_on_ok);
  JCMD_SET(surf_look);
  JCMD_SET(surf_goto);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcSupport::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(surf_enum);
  JCMD_CHK(surf_on_ok);
  JCMD_CHK(surf_look);
  JCMD_CHK(surf_goto);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                            Event Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Alter saved surface locations using base motion and possibly discard.

void jhcSupport::update_patches ()
{
  jhcMatrix mid(4);
  jhcAliaDesc *surf;
  int i;

  // erase any surface that no longer has a node (GC can happen at any time)
  if (rpt == NULL)
    return;
  for (i = 0; i < smax; i++)
    if (sid[i] > 0)
      if (rpt->NodeFor(sid[i], 2) == NULL)       // no node
        sid[i] = 0;                              

  // shift saved positions based on robot motion
  if ((rwi == NULL) || !rwi->Accepting())
    return;
  for (i = 0; i < smax; i++)
    if (sid[i] > 0)
    {
      (rwi->base)->AdjustTarget(saved[i]);
      if (saved[i].PlaneVec3() > drop)           // too far
      {
        if ((surf = rpt->NodeFor(sid[i], 2)) != NULL)
        {
          // revoke visible assertion if nodified
          rpt->StartNote();
          rpt->NewProp(surf, "hq", "visible", 1);
          rpt->FinishNote();
          rpt->VisAssoc(sid[i], NULL, 2);        // erase entry
        }
        sid[i] = 0;                              
      }
    }

  // possibly update closest saved surface with current detection
  if (tab->SurfOK() && ((i = saved_gaze()) >= 0))
  {
    tab->SurfMid(mid);    
    if (fabs(mid.Z() - saved[i].Z()) < ztol)
    {
      saved[i].MixVec3(mid, mix);
      soff[i] = (1.0 - mix) * soff[i] + mix * tab->SurfOff();
    }
  }
}


//= Find saved surface edge closest to current gaze point.
// rejects if angular deviation is too high

int jhcSupport::saved_gaze () const
{
  jhcMatrix edge(4);
  double dev, best, ht = lift->Height();
  int i, win = -1;

  for (i = 0; i < smax; i++)
    if (sid[i] > 0)
    {
      tab->SurfEdge(edge, saved[i], soff[i] - inset);
      dev = neck->GazeErr(edge, ht);
      if ((dev <= gtol) && ((win < 0) || (dev < best)))
      {
        win = i;
        best = dev;
      }
    }
  return win;
}


//= Tell whether non-floor surface suddenly appears.

void jhcSupport::table_seen ()
{
  jhcAliaDesc *obj;
  double h = tab->SurfHt(), d = tab->SurfDist();
  int born, prev = any;

  // wait for next sensor cycle then lock robot data
  if ((rwi == NULL) || (rpt == NULL) || !rwi->Accepting())
    return;  
  if (neck->Saccade())
    return;

  // see if any non-floor surface is nearby (and let estimate stabilize)
  if (!tab->SurfOK() || (h < h0) || (h > h1) || (d > d1))
    any = 0;
  else if (d <= d0)
    any++;

  // generate event if suddenly appears or gets near enough
  if ((any >= tnew) && (prev < tnew))
  {
    obj = current_vis(born);
    rpt->StartNote();
    std_props(obj, born);
    if (born <= 0)
      rpt->NewProp(obj, "hq", "visible");         // do not repeat
    rpt->FinishNote();
  }
}


//= Tell whether surface from table_seen is now close (separate event).

void jhcSupport::table_close ()
{
  jhcAliaDesc *obj;
  double h = tab->SurfHt(), d = tab->SurfDist();
  int born, prev = prox;

  // wait for next sensor cycle then lock robot data
  if ((rwi == NULL) || (rpt == NULL) || !rwi->Accepting())
    return;
  if (neck->Saccade())
    return;

  // see if selected service is currently nearby (and let estimate stabilize)
  if (!tab->SurfOK() || (h < h0) || (h > h1) || (d > dhys))
    prox = 0;
  else if (d <= dnear)
    prox++;

  // if surface just became near then generate event
  if ((prox >= tnew) && (prev < tnew))
  {
    obj = current_vis(born);
    rpt->StartNote();
    std_props(obj, born);
    rpt->NewProp(obj, "hq", "close");
    rpt->FinishNote();
  }
}


//= Find or make semantic node associated with current active surface.
// sets "born" to 0 if already existing, 1 if new semantic node (needs HQ and AKO)
// NOTE: this is generally called before StartNote (to omit object itself)

jhcAliaDesc *jhcSupport::current_vis (int& born)
{
  jhcAliaDesc *obj = NULL;
  int current = saved_detect();

  // find old node (if any)
  born = 0;
  if (current >= 0)           
    if ((obj = rpt->NodeFor(sid[current], 2)) != NULL)
      return obj;

  // create new node for current surface (and make eligible for FIND)
  born = 1;
  obj = rpt->NewNode("surf");
  rpt->VisAssoc(save_patch(), obj, 2);
  rpt->NewFound(obj);                      
  return obj;
}


///////////////////////////////////////////////////////////////////////////
//                            Surface Finding                            //
///////////////////////////////////////////////////////////////////////////

//= First call to surface detector but not allowed to fail.
// can take restrictions on azimuth, height, and distance (ignores others)
// saves quantized restrictions in cpos[i] = (azm, dist, ht)
// answers "Find a far high surface on the left" repeatedly
// returns 1 if okay, -1 for interpretation error

int jhcSupport::surf_enum0 (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *obj;

  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc->Val("arg")) == NULL)
    return -1;
  obj = cobj[i];
  cpos[i].SetP(surf_azm(obj));
  cpos[i].SetY(surf_dist(obj));
  cpos[i].SetZ(surf_ht(obj));
  camt[i] = 0.0;                       // reset scan
  return 1;
}


//= Basic call to surface detector returns one new object matching description each step.
// assumes cpos[i] = (azm, dist, ht) as quantized ranges
// cst[i] = 0 on first call only, camt[i] = distance of last candidate
// returns 1 if done, 0 if still working, -1 for failure

int jhcSupport::surf_enum (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *obj = desc->Val("arg");
  double dmax = dfar + band;
  double dhi[4]  = {dmax, dmid, dfar, dmax};
  double zavg[5] = {mavg,  0.0, lavg, mavg, havg};
  int aqnt = (int) cpos[i].P(), dqnt = (int) cpos[i].X(), hqnt = (int) cpos[i].Z();
  int current, id, born = 0;

  // lock to sensor cycle then figure out where to look for surfaces
  if (!rwi->Accepting())                         // should be READABLE if no motion???
    return 0;

/*
  double pan, head, tilt, err, vfov2 = 0.5 * vfov;

  pan = ((aqnt <= 0) ? neck->Pan() : hfov * (aqnt - 2));
  head = neck->HeadZ(lift->Height());
  tilt = -R2D * atan2(head - zavg[hqnt], dhi[dqnt]) - vfov2;

  // gaze toward likely patch center location
  err = neck->GazeErr(pan, tilt);
  if (err > atol)
  {
    if (chk_neck(i, err) > 0)                    // no progress
      return -1;
    tab->dpref = ((dqnt <= 0) ? 0.0 : dhi[dqnt] - 0.5 * band);
    tab->hpref = zavg[hqnt];
    neck->GazeTarget(pan, tilt, 1.0, 1.0, cbid[i]);  
    return 0;
  }
*/

  // find next farthest candidate surface and save information
  if ((camt[i] = scan_suitable(dqnt, hqnt, camt[i])) < 0.0)
    return -1;
  if ((current = saved_detect()) >= 0)
    id = sid[current];
  else 
    id = save_patch();
  if (id <= 0)
    return -1;
  cst[i] += 1;

  // make semantic node for patch and copy description
  if ((obj = rpt->NodeFor(id, 2)) == NULL)                 
  {
    obj = rpt->NewNode("surf");
    rpt->NewFound(obj);                          // make eligible for FIND
    born = 1;
  }
  rpt->StartNote();     
  std_props(obj, born);
  add_azm(obj, aqnt);
  add_dist(obj, dqnt);
  add_ht(obj, hqnt);     
  jprintf(1, dbg, "surf_enum %d ==> %s\n", cst[i], obj->Nick());
  rpt->FinishNote();

  // associate new node with surface position
  rpt->VisAssoc(id, obj, 2);                 
  return 1;
}


//= Find next farthest candidate that meets constraints.
// sets tsel in jhcTable to winning surface (also tmid and offset)
// returns distance of candidate, negative if nothing good

double jhcSupport::scan_suitable (int dqnt, int hqnt, double d0)
{
  double rng, dmax = dfar + band, ht = tab->PlaneZ();
  double zhi[5] = {hmax,  flr, mlth, hmth, hmax};
  double zlo[5] = {-flr, -flr,  flr, mlth, hmth}; 
  double dhi[4] = {dmax, dmid, dfar, dmax};
  double dlo[4] = { 0.0,  0.0, dmid, dfar};

  if ((hqnt > 0) && ((ht < zlo[hqnt]) || (ht > zhi[hqnt])))
    return -1.0;
  tab->InitSurf();
  while ((rng = tab->NextSurf()) >= 0.0)
  {
    if ((dqnt > 0) && (rng > dhi[dqnt]))
      break;                                      // too far
    if ((rng > d0) && (rng >= dlo[dqnt]))
      return rng;
  }
  return -1.0;
}


//= First call to surface object testing but not allowed to fail.
// tests if some object is on some surface
// returns 1 if okay, -1 for interpretation error

int jhcSupport::surf_on_ok0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc->Val("arg")) == NULL)
    return -1;
  if ((cspot[i] = desc->Val("arg2")) == NULL)
    return -1;
  return 1;
}


//= Basic call to surface object testing determines if object is on surface.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSupport::surf_on_ok (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *loc;
  int current, id, t; 

  // only able to answer if surface is the currently detected one
  if ((current = saved_detect()) < 0)
    return 1;
  if ((id = rpt->VisID(cspot[i], 2)) < 0)
    return err_vis(cspot[i]);
  if (id != sid[current])
    return 1;
  if ((t = (rwi->sobj).ObjTrack(rpt->VisID(cobj[i]))) < 0)
    return err_vis(cobj[i]);

  // object is on current surface if it is currently tracked
  rpt->StartNote();
  loc = rpt->NewProp(cobj[i], "loc", "on", (((rwi->sobj).ObjOK(t)) ? 0 : 1));
  rpt->AddArg(loc, "ref", cspot[i]);
  rpt->FinishNote();
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Surface Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= First call to surface gaze director but not allowed to fail.
// assumes object of verb is already linked to a saved position
// returns 1 if okay, -1 for interpretation error

int jhcSupport::surf_look0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc->Val("arg")) == NULL)
    return -1;
  ct0[0] = 0;                                    // reset timeout
  return 1;
}


//= Basic call to surface gaze director attempts to move head appropriately.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSupport::surf_look (const jhcAliaDesc *desc, int i)
{
  jhcMatrix edge(4);
  double ht, da;
  int s;

  // check for known surface and working body
  if (!rwi->Accepting())
    return 0;
  if ((s = saved_index(cobj[i])) < 0)
    return err_vis(cobj[i]);
  if (rwi->Ghost() || ((rwi->neck)->CommOK() <= 0)) 
    return err_hw("neck");

  // look slightly beyond edge of table
  jprintf(2, dbg, "|- Support %d: gaze at target %s\n", cbid[i], cobj[i]->Nick()); 
  ht = lift->Height();
  tab->SurfEdge(edge, saved[s], soff[s] - inset);
  neck->GazeAt(edge, ht, 1.0, cbid[i]);

  // see if close enough yet
  da = neck->GazeErr(edge, ht);
  jprintf(3, dbg, "  da = %3.1f\n", da);
  if (da > atol)
  {
    // if not making progress see if tolerably close
    if (chk_neck(i, da) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: da = %3.1f\n", da);
    if (da > gacc)
      return -1;
  }
  return 1;                                      // success
}

 
//= First call to surface approach routine but not allowed to fail.
// assumes object of verb is already linked to a saved position
// returns 1 if okay, -1 for interpretation error

int jhcSupport::surf_goto0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc->Val("arg")) == NULL)
    return -1;
  ct0[i] = 0;                                    // reset timeout
  return 1;
}


//= Basic call to surface approach routine attempts to move base appropriately.
// returns 1 if done, 0 if still working, -1 for failure

int jhcSupport::surf_goto (const jhcAliaDesc *desc, int i)
{
  jhcMatrix edge(4);
  double dist;
  int s;

  // check for known surface and working body
  if (!rwi->Accepting())
    return 0;
  if ((s = saved_index(cobj[i])) < 0)
    return err_vis(cobj[i]);
  if (rwi->Ghost() || ((rwi->body)->CommOK() <= 0)) 
    return err_hw("body");

  // determine location of closest edge of surface (and attempt to find again)
  tab->SurfEdge(edge, saved[s], soff[s] - inset);   
  tab->BiasSurf(saved[s]);                       

  // look at surface edge and approach while allowing navigation saccades
  jprintf(2, dbg, "|- Support %d: approach target %s\n", cbid[i], cobj[i]->Nick()); 
  neck->GazeAt(edge, lift->Height(), 1.0, cbid[i]);
  rwi->MapPath(cbid[i]);                           
  rwi->ServoLoc(edge, app + inset - 2.0, 1.0, cbid[i]);

  // see if close enough yet
  dist = edge.PlaneVec3() - inset;
  jprintf(3, dbg, "  dist = %3.1f\n", dist); 
  if (dist > app)
  {
    // if not making progress see if tolerably close
    if (chk_base(i, dist) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: dist = %3.1f\n", dist);
    if (dist > acc)
      return -1;
  }
  return 1;                                      // success
}


///////////////////////////////////////////////////////////////////////////
//                             Utilities                                 //
///////////////////////////////////////////////////////////////////////////

//= Save local surface patch for later reference.
// waits until very end in case some earlier patch is erased
// returns unique id number, zero or negative if problem

int jhcSupport::save_patch ()
{
  int i, current;

  // make sure local patch is valid
  if (!tab->SurfOK())
    return -1;
  if ((current = saved_detect()) >= 0)
    return sid[current];

  // find first open entry (barf if none)
  for (i = 0; i < smax; i++)
    if (sid[i] <= 0)
      break;
  if (i >= smax)
    return jprintf(">>> More than %d patches in jhcSupport::save_patch !\n", smax);

  // take a snapshot of current local patch's nearest edge
  tab->SurfMid(saved[i]);
  soff[i] = tab->SurfOff();
  sid[i] = ++last_id;
  return last_id;
}


//= Find index in array of saved patches that matches id associated with surface node.
// patches invalidated based on distance or garbage collection (cf. update_patches)
// return negative if nothing suitable

int jhcSupport::saved_index (const jhcAliaDesc *obj) const
{
  int i, id;

  if ((id = rpt->VisID(obj, 2)) > 0)
    for (i = 0; i < smax; i++)
      if (sid[i] == id)
        return i;
  return -1;
}


//= Find best match of a saved surface edge to the currently detected one.
// if match found then shifts saved surface to be average of two detections
// returns the saved centroid's index, negative if nothing close

int jhcSupport::saved_detect () 
{
  jhcMatrix mid(4), diff(4);
  double xy, best;
  int i, win = -1;

  tab->SurfMid(mid);
  for (i = 0; i < smax; i++)
    if (sid[i] > 0)
    {
      diff.DiffVec3(mid, saved[i]);
      xy = diff.PlaneVec3();
      if ((xy <= xytol) && (fabs(diff.Z()) <= ztol))
        if ((win < 0) || (xy < best))
        {
          win = i;
          best = xy;
        }
    }

  // average winner with current detection
  if (win >= 0) 
  {
    saved[win].MixVec3(mid, 0.5);
    soff[win] = 0.5 * (soff[win] + tab->SurfOff());
  }
  return win;
}


//= Check for lack of substantial neck angle change over given time.
// ct0[i] holds timestamp of last cycle when progress was made
// returns 1 if at asymptote, 0 if still moving toward goal

int jhcSupport::chk_neck (int i, double err)
{
  double prog = 1.0, tim = 0.5;        // 1 deg over about 15 cycles

  if ((ct0[i] == 0) || ((cerr[i] - err) >= prog))
  {
    ct0[i] = jms_now();
    cerr[i] = err;
  }
  else if (jms_elapsed(ct0[i]) > tim)
    return 1;
  return 0;
}


//= Check for lack of substantial base motion over given time.
// ct0[i] holds timestamp of previous cycle whether in saccade or not
// ccnt[i] holds total milliseconds when in control but no progress
// returns 1 if at asymptote, 0 if still moving toward goal

int jhcSupport::chk_base (int i, double err)
{
  double prog = 0.5;                   // 0.5" over about 30 cycles
  int tim = 1000;                      
  UL32 prev = ct0[i];
  
  // record cycle timestamp but ignore err if in saccade
  ct0[i] = jms_now();
  if (rwi->Survey())
    return 0;

  // possibly reset last error if enough progress made
  if ((prev == 0) || ((cerr[i] - err) >= prog))
  {
    cerr[i] = err;
    ccnt[i] = 0;
    return 0;
  }

  // increment amount of time since noticeable progress
  if (prev != 0) 
    ccnt[i] += (int)(ct0[i] - prev);
  if (ccnt[i] > tim) 
    return 1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                        Quantized Constraints                          //
///////////////////////////////////////////////////////////////////////////

//= Find canonical range for surface azimuth based on adjectives.
// keys into a desired neck pan angle
// returns: 0 = any, 1 = right, 2 = straight, 3 = left 

int jhcSupport::surf_azm (const jhcAliaDesc *obj) const
{
  int cat = 0;

  return cat;
}


//= Find canonical range for surface distance based on adjectives.
// keys into a preferred distance and acceptable distance range
// returns: 0 = any, 1 = close, 2 = medium, 3 = far

int jhcSupport::surf_dist (const jhcAliaDesc *obj) const
{
  int cat = 0;

  return cat;
}


//= Find canonical range for surface height based on adjectives.
// keys into a preferred height and acceptable height range
// returns: 0 = any, 1 = floor, 2 = low, 3 = middle, 4 = high

int jhcSupport::surf_ht (const jhcAliaDesc *obj) const
{
  int cat = 0;

  return cat;
}


//= Quantize current surface direction into some azimuth bin.
// returns: 1 = right, 2 = straight, 3 = left 

int jhcSupport::surf_azm (const jhcMatrix& patch) const
{
  double hfov2 = 0.5 * hfov, ang = patch.PanVec3();

  if (ang < -hfov2)
    return 1;
  if (ang > hfov2)
    return 3;
  return 2;
}


//= Quantize current surface distance into some bin.
// returns: 1 = close, 2 = medium, 3 = far

int jhcSupport::surf_dist (const jhcMatrix& patch) const
{
  double dist = patch.PlaneVec3();

  if (dist < dmid)
    return 1;
  if (dist < dfar)
    return 2;
  return 3;
}


//= Quantize current surface height into some bin.
// returns: 1 = floor, 2 = low, 3 = middle, 4 = high

int jhcSupport::surf_ht (const jhcMatrix& patch) const
{
  double ht = patch.Z();

  if (ht < flr)
    return 1;
  if (ht < mlth)
    return 2;
  if (ht < hmth)
    return 3;
  return 4;
}


///////////////////////////////////////////////////////////////////////////
//                           Net Assertions                              //
///////////////////////////////////////////////////////////////////////////

//= Add standard properties to item if newly created.
// NOTE: this should be called after StartNote

void jhcSupport::std_props (jhcAliaDesc *obj, int born)
{ 
  if (born > 0)
  {
    rpt->NewProp(obj, "ako", "surface");
    rpt->NewProp(obj, "hq", "visible");
  }
}


//= Possibly add to description which side the found surface is on.
// aqnt: 0 = any, 1 = right, 2 = straight, 3 = left 

void jhcSupport::add_azm (jhcAliaDesc *obj, int aqnt) const
{
  if (aqnt == 1)
    rpt->NewProp(obj, "loc", "right", 0, 1.0, 1);
  else if (aqnt == 2)
    rpt->NewProp(obj, "loc", "ahead", 0, 1.0, 1);
  else if (aqnt == 3)
    rpt->NewProp(obj, "loc", "left", 0, 1.0, 1);
}


//= Possibly add to description how far away the found surface is.
// dqnt: 0 = any, 1 = close, 2 = medium, 3 = far

void jhcSupport::add_dist (jhcAliaDesc *obj, int dqnt) const
{
  if (dqnt == 1)
    rpt->NewProp(obj, "hq", "close", 0, 1.0, 1);
  else if (dqnt == 2)
    rpt->NewProp(obj, "hq", "medium close", 0, 1.0, 1);
  else if (dqnt == 3)
    rpt->NewProp(obj, "hq", "far", 0, 1.0, 1);
}


//= Possibly add to description how high the found surface is.
// hqnt: 0 = any, 1 = floor, 2 = low, 3 = middle, 4 = high

void jhcSupport::add_ht (jhcAliaDesc *obj, int hqnt) const
{
  if (hqnt == 1)
    rpt->NewProp(obj, "ako", "floor", 0, 1.0, 1);
  else if (hqnt == 2)
    rpt->NewProp(obj, "hq", "low", 0, 1.0, 1);
  else if (hqnt == 3)
    rpt->NewProp(obj, "hq", "medium high", 0, 1.0, 1);
  else if (hqnt == 4)
    rpt->NewProp(obj, "hq", "high", 0, 1.0, 1);
}


///////////////////////////////////////////////////////////////////////////
//                          Semantic Messages                            //
///////////////////////////////////////////////////////////////////////////

//= Complain about some part of the body not working.
// <pre>
//   NOTE[ act-1 -lex-  work
//               -neg-  1
//               -agt-> obj-1
//         ako-1 -lex-  sys
//               -ako-> obj-1
//               -wrt-> self-1 ]
// </pre>
// always returns -1 for convenience

int jhcSupport::err_hw (const char *sys)
{
  jhcAliaDesc *part, *own, *arm, *fail;

  rpt->StartNote();
  part = rpt->NewNode("obj");
  own = rpt->NewProp(part, "ako", sys);
  rpt->AddArg(own, "wrt", rpt->Self());
  arm = rpt->Resolve(part);                      // find or make part
  fail = rpt->NewNode("act", "work", 1);
  rpt->AddArg(fail, "agt", arm);                 // mark as not working
  rpt->FinishNote(fail);
  return -1;
}


//= Complain that surface or object is not visible.
// <pre>
//   NOTE[ act-1 -lex-  see
//               -neg-  1
//               -agt-> self-1
//               -obj-> surf-1 ]
// </pre>
// always returns -1 for convenience

int jhcSupport::err_vis (jhcAliaDesc *item)
{
  jhcAliaDesc *fail;

  if (item == NULL)
    return -1;
  rpt->StartNote();
  fail = rpt->NewNode("act", "see", 1);
  rpt->AddArg(fail, "agt", rpt->Self());
  rpt->AddArg(fail, "obj", item);
  rpt->FinishNote(fail);
  return -1;
}
