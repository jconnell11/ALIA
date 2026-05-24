// jhcGenMic.cpp : read speech direction from some audio sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2026 Etaoin Systems
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

#include <string.h>
#include <math.h>

#include "Peripheral/jhcGenMic.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcGenMic::jhcGenMic ()
{
  // no connection yet
  unit = -1;
  mok = -1;

  // 3D homogeneous vectors
  loc.SetSize(4);
  axis.SetSize(4);

  // get standard processing values
  Defaults();
//  Reset();
}


//= Default destructor does necessary cleanup.

jhcGenMic::~jhcGenMic ()
{
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for checking source locations against beam.
// nothing geometric that differs between bodies

int jhcGenMic::off_params (const char *fname)
{
  jhcParam *ps = &ops;
  int ok;
  ps->SetTag("mic_ctrl", 0);
  ps->NextSpec4( &deaf,    0,   "Use head pan (no mic)");
  ps->NextSpec4( &sang,   90,   "Max sound deviation (deg)");
  ps->NextSpec4( &fresh,  30,   "Reorient after sound (cyc)"); 
  ps->NextSpecF( &oth,    12.0, "Max lateral offset (in)");    // was 18
  ps->NextSpecF( &ath,    15.0, "Max angle wrt beam (in)");     
  ps->NextSpecF( &dth,    96.0, "Max radial distance (in)");   // was 120
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for specifying sensor installation pose.

int jhcGenMic::geom_params (const char *fname)
{
  char num[10], tag[40] = "mic_geom";
  jhcParam *ps = &gps;
  int ok;

  // customize for some microphone
  if (unit >= 0)
  {
    ps->SetTitle("Microphone %d Geometry", unit);
    _itoa_s(unit, num, 10);
    strcat_s(tag, num);
  }

  // set up parameters
  ps->SetTag(tag, 0);
  ps->NextSpecF( &x0,     0.0, "X location (in)");
  ps->NextSpecF( &y0,     0.9, "Y location (in)");
  ps->NextSpecF( &z0,    44.5, "Z location (in)");
  ps->NextSpecF( &pan,    0.0, "Pan of connector end (deg)");
  ps->NextSpecF( &tilt,   0.0, "Tilt of connector end (deg)");
  ps->Skip();

  ps->NextSpec4( &mport,  8,   "Serial port (0 if invalid)"); 
  ps->NextSpec4( &light,  0,   "Controls LED");   
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Utilities                         //
///////////////////////////////////////////////////////////////////////////

//= Set all geometry parameters in order that they appear in configuration file.

void jhcGenMic::SetGeom (double x, double y, double z, double p, double t, int n, int i)
{
  x0 = x;
  y0 = y;
  z0 = z;
  pan = p;
  tilt = t;
  mport = n;
  light = i;
}


//= Set processing values to be the same as some other instance.
// useful for when menu used to set value of one microphone of a set

void jhcGenMic::CopyVals (const jhcGenMic& ref)
{
  oth = ref.oth;
  ath = ref.ath;
  dth = ref.dth;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read just body specific values from a file.
//= Read all relevant defaults variable values from a file.

int jhcGenMic::Defaults (const char *fname, int geom)
{
  int ok = 1;

  ok &= off_params(fname);
  if (geom > 0)
    ok &= LoadCfg(fname);
  return ok;
}


//= Read just body specific values from a file.

int jhcGenMic::LoadCfg (const char *fname)
{
  return geom_params(fname);
}


//= Write current processing variable values to a file.

int jhcGenMic::SaveVals (const char *fname, int geom) const
{
  int ok = 1;

  ok &= ops.SaveVals(fname);
  if (geom > 0)
    ok &= SaveCfg(fname); 
  return ok;
}


//= Write current body specific values to a file.

int jhcGenMic::SaveCfg (const char *fname) const
{
  return gps.SaveVals(fname);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

int jhcGenMic::Reset ()
{
  loc.SetVec3(x0, y0, z0);
  axis.SetPanTilt3(pan, tilt);
  beam  = 0.0;
  slow  = 0.0;
  talk  = 0.0;
  audio = -60;                         // no sound heard yet
  spcnt = -60;                         // no speaking heard yet
  mok = -1;
  return mok;
}


//= Get current sound direction and smooth in various ways.
// input "voice" is 0 if no speech currently being heard
// records how many consecutive calls had speaking

int jhcGenMic::Update (int voice)
{
  if (voice <= 0)
    spcnt = __min(spcnt, 0) - 1;       // always negative
  else
    spcnt = __max(0, spcnt) + 1;       // always positive
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Head Matching                            //
///////////////////////////////////////////////////////////////////////////

//= Find closest point on sensed directional cone to given reference.
// pt can be NULL if not needed for graphics, src: 0 = raw, 1 = smoothed, 2 = latched
// returns distance in inches, negative if unsuitable match

double jhcGenMic::ClosestPt (jhcMatrix *pt, const jhcMatrix& ref, int src, int chk) const
{
  jhcMatrix rel(4), norm(4), ortho(4);
  double off, dist, a = OffsetAng(ref, Dir(src));
  double rads = D2R * a, ca = cos(rads), sa = sin(rads);

  // find mic-ref vector, then opposite edge of triangle with angle a
  rel.DiffVec3(ref, loc);
  dist = rel.LenVec3();
  off = dist * sa;

  if (pt != NULL)
  {
    // get vector orthogonal to mic-ref vector in mic-ref-axis plane
    norm.CrossVec3(rel, axis);  
    ortho.CrossVec3(norm, rel);
    ortho.UnitVec3();

    // set both lengths to adjacent edge of triangle with angle a
    // then mix according to rotation angle to get result 
    rel.ScaleVec3(fabs(ca) * ca);
    ortho.ScaleVec3(fabs(ca) * off);   // off = len * sa
    pt->AddVec3(rel, ortho);

    // convert relative vector back to full world coordinates
    pt->IncVec3(loc);
    pt->SetH(1.0);
  }

  // see if within matching tolerances then return lateral offset (in inches)
  off = fabs(off);
  if (chk > 0)
    if ((off > oth) || (fabs(a) > ath) || (rel.LenVec3() > dth))
      return -1.0;
  return off;
}


//= Determine angular offset of reference from some sound angle (0 = forward).
//   rel dot axis = |rel| cos(ang) since |axis| = 1
// returns SIGNED degrees (use fabs() for most cases)

double jhcGenMic::OffsetAng (const jhcMatrix& ref, double aim) const
{
  jhcMatrix rel(4);

  rel.DiffVec3(ref, loc);
  return((rel.DirDiff3(axis) - 90.0) - aim);
}
