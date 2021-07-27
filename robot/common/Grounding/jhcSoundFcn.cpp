// jhcSoundFcn.cpp : sound effect output for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include <Windows.h>
#include <Mmsystem.h>
#include <process.h>

#include "Interface/jms_x.h"           // common video

#include "Grounding/jhcSoundFcn.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSoundFcn::~jhcSoundFcn ()
{

}


//= Default constructor initializes certain values.

jhcSoundFcn::jhcSoundFcn ()
{
  ver = 1.00;
  strcpy_s(tag, "SoundFcn");
  strcpy_s(sdir, "sfx/");
  bg = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcSoundFcn::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(play_snd);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcSoundFcn::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(play_snd);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                          Sound File Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Start playing sound indicated by verb.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSoundFcn::play_snd0 (const jhcAliaDesc *desc, int i)
{
  return find_file(fname.ch, desc->Val("arg"), 200);
}


//= Check whether sound defined by verb is launched yet.
// plays sound synchronously in a background thread
// returns 1 if done, 0 if still working, -1 for failure

int jhcSoundFcn::play_snd (const jhcAliaDesc *desc, int i)
{
  // request sound or wait for completion
  if (cst[i] <= 0)
  {
    if (bg != NULL)
    {
      // see if waiting too long for start
      if (jms_diff(jms_now(), ct0[i]) > 1000)   
        return -1;                                   
    }
    else
    {
      // get file name for sound and start playing in background thread
      find_file(fname.ch, desc->Val("arg"), 200);
      fname.C2W();
      bg = (void *) _beginthreadex(NULL, 0, snd_backg, this, 0, NULL);
      cst[i] = 1;
    }
  }
  else if (WaitForSingleObject((HANDLE) bg, 0) == WAIT_OBJECT_0)
  {
    // background playing has finished
    bg = NULL;
    return 1;
  }
  return 0;
}


//= Tries to find sound file associated with lexical term of node.
// full file name saved in member variable "fname"
// returns 1 if successful, -1 if cannot be found

int jhcSoundFcn::find_file (char *fn, const jhcAliaDesc *n, int ssz) 
{
  FILE *in;
  const char *spec;

  if ((spec = n->Lex()) == NULL)
    return 0;
  sprintf_s(fn, ssz, "%s%s.wav", sdir, spec);
  if (fopen_s(&in, fn, "rb") != 0)
    return 0;
  fclose(in);
  return 1;
}


//= Backkground thread plays sound and waits for it to be done

unsigned int __stdcall jhcSoundFcn::snd_backg (void *inst)
{
  jhcSoundFcn *me = (jhcSoundFcn *) inst; 

  PlaySound((me->fname).Txt(), NULL, SND_FILENAME | SND_NOSTOP | SND_SYNC); 
  return 1;
}
