// jhcDirMic.cpp : sound direction from Acoustic Magic VT-2 array microphone
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2020 IBM Corporation
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

#include "Interface/jhcMessage.h"

#include "jhcDirMic.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcDirMic::jhcDirMic ()
{
  // no serial port yet
  mok = -1;
  unit = -1;

  // 3D homogeneous vectors
  loc.SetSize(4);
  axis.SetSize(4);

  // variously smoothed sound responses
  raw.SetSize(256);
  ssm.SetSize(256);
  snd.SetSize(256);

  // get standard processing values
  LoadCfg();
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcDirMic::~jhcDirMic ()
{
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for interpreting sound direction.

int jhcDirMic::geom_params (const char *fname)
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
  ps->NextSpecF( &x0,     -1.0, "X location (in)");
  ps->NextSpecF( &y0,     16.0, "Y location (in)");
  ps->NextSpecF( &z0,    104.0, "Z location (in)");
  ps->NextSpecF( &pan,     0.0, "Pan of connector end (deg)");
  ps->NextSpecF( &tilt,    0.0, "Tilt of connector end (deg)");
  ps->Skip();

  ps->NextSpec4( &mport,   0,   "Serial port (0 if invalid)"); 
  ps->NextSpec4( &light,   0,   "Controls LED");   
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for getting and reading sound direction.
// nothing geometric that differs between bodies

int jhcDirMic::mic_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("mic_dir", 0);
  ps->NextSpec4( &box,    9,    "Value smoothing");  
  ps->NextSpecF( &mix,    0.9,  "Temporal smoothing");
  ps->NextSpec4( &spej,   2,    "Voice start (frames)");     // was 5 then 3
  ps->NextSpecF( &oth,   12.0,  "Max match offset (in)");    // was 18
  ps->NextSpecF( &ath,   15.0,  "Max match angle (in)");
  ps->NextSpecF( &dth,  120.0,  "Max match distance (in)");

  ps->NextSpecF( &msc,    0.48, "Value to degrees");         // was 0.42
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set processing values to be the same as some other instance.

void jhcDirMic::CopyVals (const jhcDirMic& ref)
{
  box  = ref.box;
  mix  = ref.mix;
  msc  = ref.msc;
  spej = ref.spej;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Utilities                         //
///////////////////////////////////////////////////////////////////////////

//= Set all geometry parameters in order that they appear in configuration file.

void jhcDirMic::SetGeom (double x, double y, double z, double p, double t, int n, int i)
{
  x0 = x;
  y0 = y;
  z0 = z;
  pan = p;
  tilt = t;
  mport = n;
  light = i;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcDirMic::Defaults (const char *fname, int geom)
{
  int ok = 1;

  ok &= mic_params(fname);
  if (geom > 0)
    ok &= geom_params(fname);
  return ok;
}


//= Read just body specific values from a file.

int jhcDirMic::LoadCfg (const char *fname)
{
  return geom_params(fname);
}


//= Write current processing variable values to a file.

int jhcDirMic::SaveVals (const char *fname, int geom) const
{
  int ok = 1;

  ok &= mps.SaveVals(fname);
  if (geom > 0)
    ok &= gps.SaveVals(fname);
  return ok;
}


//= Write current body specific values to a file.

int jhcDirMic::SaveCfg (const char *fname) const
{
  return gps.SaveVals(fname);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

int jhcDirMic::Reset (int rpt)
{
  // announce entry 
  if (rpt > 0)
    jprintf("\nMic reset ...\n");

  // set up location adn axis vectors
  loc.SetVec3(x0, y0, z0);
  axis.SetPanTilt3(pan, tilt);
mok = -1;

  // connect to proper serial port (if needed)
  if ((mok < 0) && (mport > 0))
    if (mcom.SetSource(mport, 2400) <= 0)        // std baudrate for VT-2
    {
      if (rpt >= 2)
        Complain("Could not open serial port %d in jhcDirMic::Reset", mport);
      else if (rpt > 0)
        jprintf(">>> Could not open serial port %d in jhcDirMic::Reset !\n", mport);
      return mok;
    }
  mok = 0;

  // make sure Voice Tracker II is broadcasting
  if (rpt > 0)
    jprintf("  direction ...\n");
  if (mcom.Rcv() < 0)
    return mok;
  mok = 1;

  // clear average beam angle
  mcom.Flush();
  spcnt = 0;
  beam = 0.0;
  slow = 0.0;
  talk = 0.0;

  // make sure green light is off
  mcom.SetRTS(0); 
  if (rpt > 0)
    jprintf("    ** good **\n");
  return mok;
}


//= Get current sound direction and smooth in various ways.
// angle is measured orthogonal to mic array axis as +/- 90 degs
// 0 is forward, negative means closer to the connector end
// stores directions in "beam", "slow", and "talk" variables

int jhcDirMic::Update (int voice)
{
  int dir, up = 100;

  if (mok <= 0)
    return 0;

  // clear histogram and final smooth version (for display)
  snd.Fill(0);
  raw.Fill(0);
  pk = 125;
  cnt = 0;

  // fill histogram with responses since last call
  while (mcom.Check() > 0)
    if ((dir = mcom.Rcv()) <= 250)   // 255 = invalid
    {
      raw.AInc(dir, up);
      cnt++;
    }

  // smooth and find mode else assume straight forward
  if (cnt > 0)
  {
    ssm.Boxcar(raw, box);
    snd.Boxcar(ssm, box);
    pk = snd.MaxBin();
  }
  else
    pk = 125;

  // convert to an angle and filter
  // decays toward forward if no responses
  beam = -msc * (pk - 125);
  slow = mix * slow + (1.0 - mix) * beam;
  pk2 = 125 - ROUND(slow / msc);       // for display

  // remember direction at start of voice (flag is delayed)
  if (voice <= 0)
    spcnt = __min(spcnt, 0) - 1;       // always negative
  else if (cnt > 0)
  {
    if (spcnt <= spej)                 // tracks raw for a short time 
      talk = beam;
    spcnt = __max(0, spcnt) + 1;       // always positive
  }
  return 1;
}


//= Find closest point on sensed directional cone to given reference.
// pt can be NULL if not needed for graphics, src: 0 = raw, 1 = smoothed, 2 = latched
// returns distance in inches, negative if unsuitable match

double jhcDirMic::ClosestPt (jhcMatrix *pt, const jhcMatrix& ref, int src, int chk) const
{
  jhcMatrix rel(4), norm(4), ortho(4);
  double off, a = OffsetAng(ref, src), rads = D2R * a, ca = cos(rads), sa = sin(rads);

  // find mic-ref vector, then opposite edge of triangle with angle a
  rel.DiffVec3(ref, loc);
  off = rel.LenVec3() * sa;

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

  // see if within matching tolerances
  off = fabs(off);
  if (chk > 0)
    if ((off > oth) || (a > ath) || (rel.LenVec3() > dth))
      return -1.0;
  return off;
}


//= Determine angular offset of reference from sensed directional cone.
//   rel dot axis = |rel| cos(ang) since |axis| = 1
// returns signed degrees, src: 0 = raw, 1 = smoothed, 2 = latched 

double jhcDirMic::OffsetAng (const jhcMatrix& ref, int src) const
{
  jhcMatrix rel(4);

  rel.DiffVec3(ref, loc);
  return((rel.DirDiff3(axis) - 90.0) - Dir(src));
}

