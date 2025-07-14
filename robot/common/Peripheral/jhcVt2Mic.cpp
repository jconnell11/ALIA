// jhcVt2Mic.cpp : sound direction from Acoustic Magic VT-2 array microphone
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2020 IBM Corporation
// Copyright 2021-2024 Etaoin Systems
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

#include "Interface/jprintf.h"         // common video

#include "Peripheral/jhcVt2Mic.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcVt2Mic::jhcVt2Mic ()
{
  // variously smoothed sound responses
  raw.SetSize(256);
  ssm.SetSize(256);
  snd.SetSize(256);

  // get standard processing values
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcVt2Mic::~jhcVt2Mic ()
{
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for Gaussian mixture model of source directions.
// nothing geometric that differs between bodies

int jhcVt2Mic::gmix_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("mic_gmix", 0);
  ps->NextSpec4( &box,    9,    "Value smoothing");  
  ps->NextSpecF( &msc,    0.48, "Value to degrees");         // was 0.42
  ps->NextSpecF( &mix,    0.8,  "Temporal smoothing");
  ps->NextSpecF( &zone,   3.0,  "Sample claim wrt std");  
  ps->NextSpecF( &blend,  0.02, "Sample update fraction");
  ps->NextSpec4( &gcnt,   5,    "New Gaussian wait (cyc)");

  ps->NextSpecF( &istd,   3.0,  "Min Gaussian std (deg)");
  ps->NextSpecF( &dlim,  10.0,  "Max Gaussian std (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Utilities                         //
///////////////////////////////////////////////////////////////////////////

//= Set processing values to be the same as some other instance.
// useful for when menu used to set value of one microphone of a set

void jhcVt2Mic::CopyVals (const jhcGenMic& ref)
{
  const jhcVt2Mic *vt2 = dynamic_cast<const jhcVt2Mic *>(&ref);

  // direction interpretation
  msc  = vt2->msc;
  mix  = vt2->mix;
  box  = vt2->box;

  // Gaussian mixture
  blend = vt2->blend;
  istd  = vt2->istd;
  dlim  = vt2->dlim;
  gcnt  = vt2->gcnt;

  // beam matching
  jhcGenMic::CopyVals(ref);
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcVt2Mic::Defaults (const char *fname, int geom)
{
  int ok = 1;

  ok &= jhcGenMic::Defaults(fname, geom);
  ok &= gmix_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcVt2Mic::SaveVals (const char *fname, int geom) const
{
  int ok = 1;

  ok &= jhcGenMic::SaveVals(fname, geom);
  ok &= mps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

int jhcVt2Mic::Reset (int rpt)
{
  int p = Port();

  // announce entry 
  if (rpt > 0)
    jprintf("\nMic reset ...\n");
  jhcGenMic::Reset();

  // connect to proper serial port (if needed)
  if ((mok < 0) && (p > 0))
    if (mcom.SetSource(p, 2400) <= 0)            // std baudrate for VT-2
    {
      if (rpt >= 2)
        Complain("Could not open serial port %d in jhcVt2Mic::Reset", p);
      else if (rpt > 0)
        jprintf(">>> Could not open serial port %d in jhcVt2Mic::Reset !\n", p);
      return mok;
    }
  mok = 0;

  // make sure Voice Tracker II is broadcasting
  if (rpt > 0)
    jprintf("  direction ...\n");
  if (mcom.Rcv() < 0)
    return mok;
  mcom.Flush();
  mok = 1;

  // clear beam angle smoothing
  b0 = 0.0;
  b1 = 0.0;
  b2 = 0.0;
  beam = 0.0;
  slow = 0.0;

  // clear speech direction
  init_mix();
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

int jhcVt2Mic::Update (int voice)
{
  int dir, cnt = 0, up = 100;

  // clear histogram and final smooth version (for display)
  snd.Fill(0);
  raw.Fill(0);
  pk = 125;

  // fill histogram with responses since last call (if mic connected)
  if (mok > 0)
    while (mcom.Check() > 0)
      if ((dir = mcom.Rcv()) <= 250)   // 255 = invalid
      {
        raw.AInc(dir, up);
        cnt++;
      }

  // smooth and find mode else assume straight forward
  if (cnt <= 0)
  {
    pk = 125;
    audio = __min(audio, 0) - 1;
  }
  else
  {
    // get short time estimate of reported directions
    ssm.Boxcar(raw, box);
    snd.Boxcar(ssm, box);
    pk = snd.MaxBin();

    // convert to angle and clean up with median filter
    b0 = b1;
    b1 = b2;
    b2 = -msc * (pk - 125);
    if ((b2 >= __min(b0, b1)) && (b2 <= __max(b0, b1)))
      beam = b2;
    else if ((b1 >= __min(b0, b2)) && (b1 <= __max(b0, b2)))
      beam = b1;
    else
      beam = b0;
  
    // simple IIR filter
    slow = mix * slow + (1.0 - mix) * beam;
    pk2 = 125 - ROUND(slow / msc);               // for display
    audio = __max(0, audio) + 1;
  }

  // when speech starts ascribe it to non-background Gaussian
  update_mix(beam);
  jhcGenMic::Update(voice);                      // for spcnt
  if (NewVoice())
    talk = favg;
  return 1;
}


//= Initialize Gaussian mixture components.

void jhcVt2Mic::init_mix ()
{
  // ambient Gaussian
  bavg = 0.0;
  bvar = 1.0;
  bwt  = 0.0;

  // speaker Gaussian
  favg = 0.0;
  fvar = 1.0;
  fwt  = 0.0;

  // foreground status
  skip = 0;
  fgnd = 0;
}


//= Maintain Gaussian mixture model for background and event directions.
// adapted from Zivkovic ICPR-2004 with variance limits and rejection wait

void jhcVt2Mic::update_mix (double val)
{
  double bdev = val - bavg, bdsq = bdev * bdev, fdev = val - favg, fdsq = fdev * fdev;
  double temp, norm, vf = zone * zone, ivar = istd * istd, vlim = dlim * dlim; 

  // try to assign data to one of the Gaussians 
  if ((bwt > 0.0) && (bdsq < (vf * bvar)))
  {
    bavg += (blend / bwt) * bdev;
    bvar += (blend / bwt) * (bdsq - bvar);
    bwt  += blend * (1.0 - bwt);
    bvar = __min(ivar, __max(bvar, vlim));
    fgnd = 0;
    skip = 0;
  }
  else if ((fwt > 0.0) && (fdsq < (vf * fvar)))
  {
    favg += (blend / fwt) * fdev;
    fvar += (blend / fwt) * (fdsq - fvar);
    fwt  += blend * (1.0 - fwt);
    fvar = __min(ivar, __max(fvar, vlim));
    fgnd = 1;
    skip = 0;
  }
  else if (++skip > gcnt)
  {
    // make new Gaussian with some weight
    favg = val;
    fvar = istd * istd;
    fwt  = blend;
    fgnd = 1;
    skip = 0;
  }

  // make sure weights always sum to one
  temp = bwt + fwt;
  if ((temp > 0.0) && (temp != 1.0))
  {
    norm = 1.0 / temp;
    bwt *= norm;
    fwt *= norm;
  }

  // possibly swap Gaussians so wt[0] >= wt[1] always.
  if (bwt < fwt)
  {
    temp = bavg;             // averages
    bavg = favg;
    favg = temp;
    temp = bvar;             // variances
    bvar = fvar;
    fvar = temp;
    temp = bwt;              // weights
    bwt  = fwt;
    fwt  = temp;
  }
}

