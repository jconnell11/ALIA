// jhcSwapMic.cpp : direction-of-arrival from ReSpeaker Mic Array V2.0
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 Etaoin Systems
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

#include "Peripheral/jhcSwapMic.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSwapMic::~jhcSwapMic ()
{

}


//= Default constructor initializes certain values.

jhcSwapMic::jhcSwapMic ()
{
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

int jhcSwapMic::Reset ()
{
  jhcGenMic::Reset();
  v0  = 0;
  mok = 1;
  return 1;
}


//= Get current sound direction and smooth in various ways.
// input "voice" is 0 if no speech currently being heard
// records how many consecutive calls had speaking

int jhcSwapMic::Update (int voice)
{
  jhcGenMic::Update(voice);
  if ((voice >= 2) && (v0 < 2))        // utterance recognized
    talk = slow;
  v0 = voice;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Sensor Input                             //
///////////////////////////////////////////////////////////////////////////

//= Combine new sensor reading into smoothed average.
// assumes 0 = no sound, 1-360 = CCW angle from front
// maintains smoothed angle in "slow" and silence count in "audio"

void jhcSwapMic::Smooth (int dir)
{
  double diff, mix = 0.7;              // not much averaging
  int gap = -audio, drop = 10;         // about 330ms

  // see if any sound heard (1-360 is valid)
  if (dir <= 0)
  {
    audio = __min(audio, 0) - 1;       // "slow" unchange
    return;
  }
 
  // start new average if beginning of sound pod
  audio = __max(0, audio) + 1;
  beam = ((dir > 180) ? dir - 360 : dir);
  if (gap >= drop)
  {
    slow = beam;
    return;
  }

  // find change relative to average        
  diff = beam - slow;
  if (diff > 180.0)
    diff -= 360.0;
  else if (diff <= -180.0)
    diff += 360.0;

  // smooth with IIR filter
  slow += mix * diff;
  if (slow > 180.0)
    slow -= 360.0;
  else if (slow <= -180.0)
    slow += 360.0;
}

