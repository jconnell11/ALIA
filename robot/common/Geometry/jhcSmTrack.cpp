// jhcSmTrack.cpp : tracks objects in 3D with simple smoothing
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

#include <stdio.h>
#include <math.h>

#include "Geometry/jhcSmTrack.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSmTrack::jhcSmTrack (int n)
{
  // top level options
  strcpy_s(name, "item");
  stats = 0;

  // tracking arrays
  total = 0;
  SetSize(n);

  // processing values
  bin_sz = 0.5;
  SetTrack(1.0, 1.0, 1.0, 0.2, 0.0, 0.0, 2.0);
  SetFilter(3.0, 3.0, 3.0, 0.1, 0.1, 0.1, 5, 5);

  // read values from file and clear state
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcSmTrack::~jhcSmTrack ()
{
  dealloc();
}


//= Make bigger arrays and copy over all values.
// wipes out all previous information if bigger size requested
// returns 1 if successful, 0 for allocation failure

int jhcSmTrack::SetSize (int n)
{
  int i, ok = 1;

  // if change needed then erase previous arrays 
  if (n <= total)
    return 1;
  if (total > 0)
    dealloc();

  // create status vectors
  if ((ena = new int [n]) == NULL)
    return 0;
  if ((id  = new int [n]) == NULL)
    return 0;
  if ((cnt = new int [n]) == NULL)
    return 0;
  if ((fwd = new int [n]) == NULL)
    return 0;
  if ((back = new int [n]) == NULL)
    return 0;
  if ((state = new int [n]) == NULL)
    return 0;
  
  // create base arrays
  if ((pos = new double * [n]) == NULL)
    return 0;
  if ((var = new double * [n]) == NULL)
    return 0;
  if ((dist = new double * [n]) == NULL)
    return 0;
  if ((tag = new char * [n]) == NULL)
    return 0;

  // create positions, variances, and match distances
  for (i = 0; i < n; i++)
  {
    if ((pos[i] = new double [3]) == NULL)
      ok = 0;
    if ((var[i] = new double [3]) == NULL)
      ok = 0;
    if ((dist[i] = new double [n]) == NULL)
      ok = 0;
    if ((tag[i] = new char [80]) == NULL)
      ok = 0;
  }

  // check for success
  if (ok > 0)
    total = n;
  return ok;
}


//= Get rid of allocated arrays.

void jhcSmTrack::dealloc ()
{
  int i;

  // get rid of coordinate and matching arrays
  if (tag != NULL)
    for (i = total - 1; i >= 0; i--)
      delete [] tag[i];
  if (dist != NULL)
    for (i = total - 1; i >= 0; i--)
      delete [] dist[i];
  if (var != NULL)
    for (i = total - 1; i >= 0; i--)
      delete [] var[i];
  if (pos != NULL)
    for (i = total - 1; i >= 0; i--)
      delete [] pos[i];

  // get rid of base arrays
  delete [] tag;
  delete [] dist;
  delete [] var;
  delete [] pos;

  // get rid of simple arrays
  delete [] state;
  delete [] back;
  delete [] fwd;
  delete [] cnt;
  delete [] id;
  delete [] ena;
  
  // clear previous pointers 
  pos  = NULL;
  var  = NULL;
  dist = NULL;
  ena  = NULL;
  id   = NULL;
  cnt  = NULL;
  fwd  = NULL;
  back = NULL;
  total = 0;
  valid = 0;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcSmTrack::Defaults (const char *fname)
{
  int ok = 1;

  ok &= track_params(fname);
  ok &= filter_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSmTrack::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= tps.SaveVals(fname);
  ok &= fps.SaveVals(fname);
  return ok;
}


//= Parameters used for establishing and linking tracks.

int jhcSmTrack::track_params (const char *fname)
{
  jhcParam *ps = &tps;
  char tag[80];
  int ok;

  // customize for thing being tracked
  ps->SetTitle("Tracking of %s", name);
  sprintf_s(tag, "%s_track", name);

  // set up parameters
  ps->SetTag(tag, 0);
  ps->NextSpecF( close,     "Max X move (in)");  
  ps->NextSpecF( close + 1, "Max Y move (in)");  
  ps->NextSpecF( close + 2, "Max Z move (in)");  
  ps->NextSpecF( &frac,     "Max fractional move");
  ps->NextSpecF( &dsf,      "Shape diff wt (pct/in)");
  ps->NextSpecF( &daf,      "Angle diff wt (deg/in)");

  ps->NextSpecF( &rival,    "Elder preference ratio");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set tracking parameters in same order as in configuration file line.

void jhcSmTrack::SetTrack (double dx, double dy, double dz, double f, double sw, double aw, double rv)
{
  close[0] = dx;
  close[1] = dy;
  close[2] = dz;
  frac = f;
  dsf = sw;
  daf = aw;
  rival = rv;
}


//= Parameters used for smoothing position estimates.

int jhcSmTrack::filter_params (const char *fname)
{
  jhcParam *ps = &fps;
  char tag[80];
  int ok;

  // customize for thing being tracked
  ps->SetTitle("Filtering of %s", name);
  sprintf_s(tag, "%s_filter", name);

  // set up parameters
  ps->SetTag(tag, 0);
  ps->NextSpecF( noise,     "X jitter (in)");  
  ps->NextSpecF( noise + 1, "Y jitter (in)");  
  ps->NextSpecF( noise + 2, "Z jitter (in)");  
  ps->NextSpecF( mix,       "X blending");
  ps->NextSpecF( mix + 1,   "Y blending");
  ps->NextSpecF( mix + 2,   "Z blending");

  ps->NextSpec4( &born,     "Valid after detected for");  
  ps->NextSpec4( &gone,     "Delete after missing for");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set filtering parameters in same order as in configuration file line.

void jhcSmTrack::SetFilter (double nx, double ny, double nz, double mx, double my, double mz, int b, int g)
{
  noise[0] = nx;
  noise[1] = ny;
  noise[2] = nz;
  mix[0] = mx;
  mix[1] = my;
  mix[2] = mz;
  born = b;
  gone = g;
}


//= Duplicate parameters found in some other instance.

void jhcSmTrack::CopyParams (const jhcSmTrack& ref)
{
  tps.CopyAll(ref.tps);
  fps.CopyAll(ref.fps);
}


///////////////////////////////////////////////////////////////////////////
//                               Track Status                            //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcSmTrack::Reset (int dbg)
{
  int i;

  // nothing found yet
  for (i = 0; i < total; i++)
  {
    id[i] = -1;
    tag[i][0] = '\0';
    state[i] = 0;
  }
  valid = 0;
  last_id = 0;
  stats = dbg;

  // possibly set up debugging arrays for jitter
  if (dbg > 0)
    for (i = 0; i < 3; i++)
    {
      move[i].SetSize(2 * ROUND(close[i] / bin_sz) + 1);
      move[i].Fill(0);
    }
}


//= Set updating eligibility for all non-empty tracks.

void jhcSmTrack::EnableAll (int active)
{
  int i;

  for (i = 0; i < valid; i++)
    ena[i] = active;
}


//= Set updating eligibility for a particular track.

void jhcSmTrack::SetEnable (int i, int active)
{
  if ((i < 0) || (i >= valid))
    return;
  ena[i] = active;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get best match between eligible tracks and new detections.
// optional "shp" is width, length, and height associated with track
// updates match positions and validity, can optionally delete bad tracks
// only pays attention to first "total" detections, even if "n" is bigger
// always sets "state" to zero and "tag" to empty for newly added tracks

void jhcSmTrack::MatchAll (const double * const *detect, int n, int rem, const double * const *shp)
{
  int i, j, nt = __min(n, total);

  // link all reasonable pairs and penalize unlinked tracks
  score_all(detect, nt, shp);
  greedy_pair(detect, nt, 1);    // match established tracks first
  greedy_pair(detect, nt, 0);    // probationary tracks get leftovers
  if (rem > 0)
    Prune();

  // make new track entries for all remaining detections
  for (j = 0; j < nt; j++)
    if (back[j] < 0)
    {
      i = add_track(detect[j]);
      back[j] = i;
      fwd[i] = j;
    }
}


//= Clear detection assignments and compute compatibilities with all tracks.

void jhcSmTrack::score_all (const double * const *detect, int n, const double * const *shp)
{
  int i, j;

  // clear all match linkages
  for (i = 0; i < total; i++)
    fwd[i] = -1;
  for (j = 0; j < total; j++)
    back[j] = -1;

  // cache pair distance (squared) if position change is acceptable.
  if (shp == NULL) 
  {
    // just use position difference
    for (i = 0; i < valid; i++)
      if ((id[i] >= 0) && (ena[i] > 0))
        for (j = 0; j < n; j++)
          (dist[i])[j] = get_d2(i, detect[j]);
  }
  else
  {
    // use shape differences also
    for (i = 0; i < valid; i++)
      if ((id[i] >= 0) && (ena[i] > 0))
        for (j = 0; j < n; j++)
          (dist[i])[j] = get_d2s(i, detect[j], shp[j]);
  }
}


//= Get squared distance to some detection with absolute limit (only).
// barfs if too far in absolute distance (e.g. > 4 inches)

double jhcSmTrack::get_d2 (int i, const double *item) const
{
  const double *trk = pos[i];
  double dx, dy, dz;

  if ((dx = fabs(trk[0] - item[0])) <= close[0])
    if ((dy = fabs(trk[1] - item[1])) <= close[1])
      if ((dz = fabs(trk[2] - item[2])) <= close[2])
        return(dx * dx + dy * dy + dz * dz);
  return -1.0;
}


//= Get squared distance to some detection with absolute and fractional limits.
// barfs if too far in fixed or fractional distance (e.g. > 1.5 * width)
// index i references "pos" (trk[3]) is (x, y, z) after Kalman smoothing
// "shp[6]" is track shape info                  (w, l, h, maj, min, ang)
// "item[9]" is the detection candidate (x, y, z, w, l, h, maj, min, ang)

double jhcSmTrack::get_d2s (int i, const double *item, const double *shp) const
{
  const double *trk = pos[i];
  double xtol = close[0], ytol = close[1], ztol = close[2];
  double d2, dx, dy, dz, dw, dl, dh, da;

  // only allow position shift up to a fraction of bounding box dimension 
  if (frac > 0.0)
  {
    xtol = __max(xtol, frac * shp[0]);
    ytol = __max(ytol, frac * shp[1]);
    ztol = __max(ztol, frac * shp[2]);
  }

  // see if valid candidate at all based on hard bounds for matching
  if ((dx = fabs(trk[0] - item[0])) > xtol)
    return -1.0;
  if ((dy = fabs(trk[1] - item[1])) > ytol)
    return -1.0;
  if ((dz = fabs(trk[2] - item[2])) > ztol)
    return -1.0;

  // basic centroid offset (squared)
  d2 = dx * dx + dy * dy + dz * dz;   

  // shape difference (in percent)
  if (dsf > 0.0)
  {
    dh = (100.0 / dsf) * (shp[2] - item[5]) / shp[2];       // bounding box height
    dw = (100.0 / dsf) * (shp[3] - item[6]) / shp[3];       // ellipse length
    dl = (100.0 / dsf) * (shp[4] - item[7]) / shp[4];       // ellipse width
    d2 += dw * dw + dl * dl + dh * dh;
  }

  // orientation difference (in degrees)
  if (daf > 0.0)
  {
    da = shp[5] - item[8];               // ellipse angle
    if (da >= 90.0)
      da -= 180.0;
    else if (da < -90.0)
      da += 180.0;
    da /= daf;
    d2 += da * da;
  }
  return d2;
}


//= Let tracked people grab closest new detection.
// can optionally restrict matching to establishied (numbered) tracks
// "dist" searched to find pair which is recorded in "fwd" and "back"

void jhcSmTrack::greedy_pair (const double * const *detect, int n, int solid)
{
  double best, best2, r2 = rival * rival;
  int i, j, jwin, iwin, alt, th = ((solid > 0) ? 1 : 0);

  while (1)
  {
    // find smallest remaining distance between unpaired items
    best = -1.0;
    for (i = 0; i < valid; i++)
      if ((id[i] >= th) && (ena[i] > 0))
        if (fwd[i] < 0)
          for (j = 0; j < n; j++)
            if ((back[j] < 0) && ((dist[i])[j] >= 0.0))
              if ((best < 0.0) || ((dist[i])[j] < best))
              {
                best = (dist[i])[j];
                iwin = i;
                jwin = j;
              }

     // stop if one set or other exhausted
     if (best < 0.0)
       return;

     if (rival > 0.0)
     {
       // find second best solid track for selected detection
       best2 = -1.0;
       for (i = 0; i < valid; i++)
         if ((id[i] > 0) && (ena[i] > 0))
           if ((fwd[i] < 0) && (i != iwin))
             if ((dist[i])[jwin] >= 0.0)
               if ((best2 < 0.0) || ((dist[i])[jwin] < best2))
               {
                 best2 = (dist[i])[jwin];
                 alt = i;
               }

       // prefer older track if almost as good (or candidate not solid)
       if (best2 >= 0.0) 
         if (((id[iwin] == 0) || (id[alt] < id[iwin])) && (best2 <= (r2 * best)))
           iwin = alt;
     }

     // record pairing and update tracking info
     PairUp(iwin, detect, jwin);
  }
}


//= Force pairing of some detection to a particular track.
// remove items from further consideration

void jhcSmTrack::PairUp (int i, const double * const *detect, int j)
{
  shift_pos(i, detect[j]);
  back[j] = i;
  fwd[i] = j;
}


//= Penalize unmatched tracks and possibly remove them.
// can be called after MatchAll if some way to salvage missed tracks
// needs size of detection list as an input

void jhcSmTrack::Prune ()
{
  int i;

  for (i = 0; i < valid; i++)
    if ((id[i] >= 0) && (ena[i] > 0))
      if (fwd[i] < 0)
        mark_miss(i);
}


//= Returns the squared distance between some track and a new detection.
// return negative value if detection too far away or track invalid

double jhcSmTrack::Dist2 (int i, const double *item) const
{
  if ((i < 0) || (i >= valid))
    return -3.0;
  if ((id[i] < 0) || (ena[i] <= 0))
    return -2.0;
  return get_d2(i, item);
}


//= Update a particular track with position of new detection.
// can also create new entry or reject matching and penalize track
// returns 1 if updated or added, 0 or negative if ignored

int jhcSmTrack::Update (int i, const double *item)
{
  // see if no existing entry yet
  if ((i < 0) || (i >= total))
    return -1;
  if ((id[i] < 0) || (i >= valid))
    return add_track(item);

  // alter existing item if position change is acceptable
  if (ena[i] <= 0)
    return 0;
  if (get_d2(i, item) >= 0.0)
    return shift_pos(i, item);

  // penalize if poor match suggested
  return mark_miss(i);
}


//= Attempt to add a new track with a unique identifier based on detection.
// returns number of new entry, negative if list is full

int jhcSmTrack::add_track (const double *item)
{
  int i, j;

  // find first open entry in tracking list
  for (i = 0; i < total; i++)
    if (id[i] < 0)
      break;
  if (i >= total)
    return -1;
  valid = __max(valid, i + 1);

  // copy position and set default variances
  for (j = 0; j < 3; j++)
  {
    (pos[i])[j] = item[j];
    (var[i])[j] = noise[j] * noise[j];
  }

  // clear text description and reset state
  (tag[i])[0] = '\0';
  state[i] = 0;

  // assign brand new track to item (probationary at start)
  ena[i] = 1;
  cnt[i] = 0;
  mark_hit(i);
  return i;
}


//= Alter coordinates of tracked item using new detection location.
//
//   measurement:  M  = P + Vm          where Vm = variance in measurement
//       process:  P' = d * P + c       where c is expected jumpiness 
//                                        and d is a time decay constant 
// Arguments for each of XYZ axes:
//   pos   = smoothed prediction (item at start)
//   var   = variance of prediction (noise^2 at start)
//   item  = new observation of raw value 
//   noise = expected noise in raw value
//   mix   = temporal mixing for measurement variance
// always returns 1 for convenience

int jhcSmTrack::shift_pos (int i, const double *item)
{
  double dfrac, d, vm, k;
  int j, half;

  // possibly histogram target moves based on new detection
  if (stats > 0)
    for (j = 0; j < 3; j++)
    {
      half = move[j].Size() / 2;
      dfrac = (item[j] - (pos[i])[j]) / close[j];
      move[j].AInc(ROUND(half * dfrac) + half, 100);
    }

  // update smoothed coordinates
  for (j = 0; j < 3; j++)
  {
    // estimate observation variance
    d = item[j] - (pos[i])[j];
    vm = mix[j] * d * d;
    vm += (1.0 - mix[j]) * (var[i])[j];

    // figure out Kalman gain
    k = vm / (vm + noise[j] * noise[j]);
    (pos[i])[j] += k * d;
    (var[i])[j] = (1.0 - k) * vm;
  }

  // update status
  return mark_hit(i);
}


//= See if something becomes valid based on consecutive detection count.
// bind unique identifier if track goes from probationary to confirmed
// always returns 1 for convenience

int jhcSmTrack::mark_hit (int i)
{
  // update tracking count
  if (cnt[i] < 0)
    cnt[i] = 0;
  cnt[i] += 1;

  // see if reached validation threshold
  if (id[i] < 0)
    id[i] = 0;
  if ((id[i] == 0) && (cnt[i] >= born))
    id[i] = ++last_id;
  return 1;
}


//= Penalize some track for missing a detection on this cycle.

void jhcSmTrack::Penalize (int i)
{
  if ((i < 0) || (i >= valid))
    return;
  if ((id[i] < 0) || (ena[i] <= 0))
    return;
  mark_miss(i);
}


//= Increment miss count and possibly remove some track. 
// always returns 0 for oconvenience

int jhcSmTrack::mark_miss (int i)
{
  // decrement missing count
  if (cnt[i] > 0)
    cnt[i] = 0;
  cnt[i] -= 1;

  // possibly get rid of track if missing too long
  if (cnt[i] <= -gone)
    rem_item(i);
  return 0;
}


//= Force a particular track to be invalidated.
// returns 1 if cleared, 0 if already invalid

int jhcSmTrack::Clear (int i)
{
  if ((i < 0) || (i >= total))
    return 0;
  if (id[i] < 0)
    return 0;
  rem_item(i);
  return 1;
}


//= Remove a particular track and reset max active index.

void jhcSmTrack::rem_item (int i)
{
  id[i] = -1;
  tag[i][0] = '\0';
  state[i] = 0;
  if (i == (valid - 1))
    for (valid = i; valid >= 0; valid--)
      if (id[valid - 1] >= 0)
        break;
}


///////////////////////////////////////////////////////////////////////////
//                            Track Information                          //
///////////////////////////////////////////////////////////////////////////

//= Tell number of items with currently valid tracks.

int jhcSmTrack::Count () const
{
  int i, n = 0;

  for (i = 0; i < valid; i++)
    if (id[i] > 0)
      n++;
  return n;
}


//= Tell whether specified track has been validated.
// returns -1 if bad index, 0 if probationary, positive if tracked

int jhcSmTrack::Valid (int i) const
{
  if ((i < 0) || (i >= valid))
    return -1;
  return id[i];
}


//= Tell whether specified track is eligiby for updating.

int jhcSmTrack::Enabled (int i) const
{
  if ((i < 0) || (i >= valid))
    return 0;
  return ena[i];
}


//= Return which track matched a certain recent detection.
// returns -1 if invalid index

int jhcSmTrack::TrackFor (int j) const
{
  if ((j < 0) || (j >= valid))
    return -1;
  return back[j];
}


// Return recent detection that matched with the given track.
// returns -1 if invalid index or no match was found

int jhcSmTrack::DetectFor (int i) const
{
  if ((i < 0) || (i >= valid))
    return -1;
  return fwd[i];
}


//= Get individual smoothed coordinates for specified tracked object.
// return unique ID if valid, 0 or negative for bad track

int jhcSmTrack::Coords (double& x, double& y, double& z, int i) const
{
  if ((i < 0) || (i >= valid))
    return -1;
  x = pos[i][0];
  y = pos[i][1];
  z = pos[i][2];
  return id[i];
}


//= Get smoothed coordinate vector for specified tracked object.

const double *jhcSmTrack::Coords (int i) const
{
  if ((i < 0) || (i >= valid))
    return NULL;
  return pos[i];
}

