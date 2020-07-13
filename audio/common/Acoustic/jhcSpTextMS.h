// jhcSpTextMS.h : text-to-speech functions using Microsoft SAPI
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2018 IBM Corporation
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

#ifndef _JHCSPTEXTMS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSPTEXTMS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Text-to-speech functions using Microsoft SAPI.
// implements functions found in "sp_tts.h"

class jhcSpTextMS
{
// PRIVATE MEMBER VARIABLES
private:
  void *v;             /** Microsoft text-to-speech interface.  */
  int pitch;           /** Alteration from base pitch of voice. */
  int hlen[10];        /** Length of pitch header on message.   */
  char buf[10][200];   /** Queued utterances for system.        */


// PROTECTED MEMBER VARIABLES
protected:
  int t_ok;            /** Overall status of TTS system. */
  char tfile[200];     /** DLL name (not used).          */
  char vname[80];      /** Name of voice for TTS output. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcSpTextMS ();
  ~jhcSpTextMS ();
  int BindTTS (const char *fname, const char *cfg, int start =1);
 
  // low level sp_tts DLL functions
  const char *tts_version (char *spec, int ssz) const;
  int tts_setup (const char *cfg_file =NULL);
  const char *tts_voice (char *spec, int ssz) const;
  int tts_set_voice (const char *spec, int pct =0);
  const char *tts_output (char *spec, int ssz) const;
  int tts_start (int level =0, const char *log_file =NULL);
  void tts_cleanup ();
  int tts_say (const char *msg);
  int tts_status (char *rest, int ssz);
  int tts_status () {return tts_status(NULL, 0);}
  int tts_wait ();
  int tts_shutup ();

  // sp_tts - convenience
  template <size_t ssz>
    const char *tts_version (char (&spec)[ssz]) const
      {return tts_version(spec, ssz);}
  template <size_t ssz>
    const char *tts_voice (char (&spec)[ssz]) const
      {return tts_voice(spec, ssz);}
  template <size_t ssz>
    const char *tts_output (char (&spec)[ssz]) const
      {return tts_output(spec, ssz);}
  template <size_t ssz>
    int tts_status (char (&rest)[ssz])
      {return tts_status(rest, ssz);}


};


#endif  // once




