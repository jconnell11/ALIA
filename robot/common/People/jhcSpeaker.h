// jhcSpeaker.h : determines which person is speaking using mic arrays
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#ifndef _JHCSPEAKER_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSPEAKER_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "People/jhcStare3D.h"          // common robot
#include "Peripheral/jhcDirMic.h"

#include "Interface/jhcSerial.h"        // common video
#include "Processing/jhcDraw.h"         


//= Determines which person is speaking using mic array(s).
// generally all mics live in this class which does update, etc. (for Dataspace/WMX)
// alternatively a single remotely-managed mic can be bound (for ELI)

class jhcSpeaker : private jhcDraw
{
// PRIVATE MEMBER VARIABLES
private:
  static const int amax = 6;   /** Maximum number of microphones. */

  jhcStare3D *s3;              /** Person finder and tracker. */
  const jhcDirMic *m0;         /** Mic part of some robot.    */
  int vth, det, vcnt, spk;               


// PUBLIC MEMBER VARIABLES
public:
  jhcDirMic mic[amax];         /** Collection of microphones. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSpeaker ();
  jhcSpeaker ();
  void Bind (jhcStare3D *stare) {s3 = stare;}
  void RemoteMic (const jhcDirMic *m) {m0 = m;}
  int NumMic () const {return amax;}
  jhcSerial *AttnLED ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;
  void MicDup ();

  // main functions
  void Reset ();
  void Update (int voice =1);
  int Analyze (int voice =1);
  bool Decision () const {return(det > 0);}
  int Speaker () const   {return spk;}
  int Speaking () const  {return((vcnt < vth) ? -1 : spk);}
  jhcBodyData *SpInfo () const
    {return((s3 == NULL) ? NULL : s3->RefID(Speaking()));}

  // debugging graphics
  int MicsMap (jhcImg& dest, int invert =0) const;
  int SoundMap (jhcImg& dest, int invert =0, int src =0) const;
  int SoundCam (jhcImg& dest, int cam =0, int rev =0, int src =0) const;
  int OffsetsMap (jhcImg& dest, int trk =1, int invert =0, int src =0, int style =0) const;
  int OffsetsCam (jhcImg& dest, int cam =0, int trk =1, int rev =0, int src =0, int style =0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  int pick_dude (const jhcDirMic& m, double& best) const;

  // debugging graphics
  void draw_mic (jhcImg& dest, const jhcDirMic& m, int invert) const;
  void map_beam (jhcImg& dest, const jhcDirMic& m, int invert, int src) const;
  void front_beam (jhcImg& dest, const jhcDirMic& m, int rev, int src) const;
  void map_off (jhcImg& dest, const jhcDirMic& m, int trk, int invert, int src, int style) const;
  void front_off (jhcImg& dest, const jhcDirMic& m, int trk, int rev, int src, int style) const;


};


#endif  // once




