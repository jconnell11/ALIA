// jhcSpeechWeb.cpp : uses DLL to access Microsoft Azure Speech Services
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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
#include <ctype.h>

#include "Data/jhcParam.h"             // common video

#include "Acoustic/sp_reco_web.h"      // common audio

#include "Acoustic/jhcSpeechWeb.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSpeechWeb::~jhcSpeechWeb ()
{
}


//= Default constructor initializes certain values.

jhcSpeechWeb::jhcSpeechWeb ()
{
  clr_state();
  Defaults();
}


//= Clear all state variables.

void jhcSpeechWeb::clr_state ()
{
  *utt0 = '\0';
  *utt = '\0';
  rcv = NULL;
  hear = 0;
  mute = 0;
  txtin = 0;
  quit = 0;
  dbg = 0;
}


//= Tell version of DLL being used.

const char *jhcSpeechWeb::Version () const
{
  return reco_version();
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Load online credentials from file.

int jhcSpeechWeb::Defaults (const char *fname)
{
  jhcParam ps;
  int ok = 1;
  
  ok &= ps.LoadText(key, fname, "sp_key", "");
  ok &= ps.LoadText(reg, fname, "sp_reg", "");
  return ok;
}


//= Save online credentials to file.

int jhcSpeechWeb::SaveVals (const char *fname) const
{
  jhcParam ps;
  int ok = 1;

  ok &= ps.SaveText(fname, "sp_key", key);
  ok &= ps.SaveText(fname, "sp_reg", reg);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Connect to online speech service at start of session.
// uses key and region loaded from configuration file
// can optionally print out partial recognition strings
// returns 1 if properly configured, 0 if cannot connect, negative for problem

int jhcSpeechWeb::Init (int partial)
{
  int ans;

  clr_state();
  dbg = partial;
  LoadFix("misheard.map");                 // always try fixup file
  Listen(1);
  if ((ans = reco_setup(key, reg)) <= 0)
    return ans;
  return reco_start();
}


//= Possibly stop processing via some other signal.

void jhcSpeechWeb::Inject (const char *txt, int stop)
{
  if (stop > 0)
    quit = 1;
  if ((txt != NULL) && (*txt != '\0'))
  {
    // prefer this string to reco result
    strcpy_s(utt, txt);
    rcv = utt;
    hear = 2;
    txtin = 1;               
  }
}


//= Turn default microphone on and off (e.g. during TTS).

void jhcSpeechWeb::Listen (int doit)
{
  if (((doit > 0) && (mute > 0)) || ((doit <= 0) && (mute <= 0)))
  {
    reco_listen(doit);
    mute = ((doit <= 0) ? 1 : 0);
  }   
}


//= See if any new recognition result is available.
// also mutes input microphone if TTS output is occurring
// return: 2 new result, 1 speaking, 0 silence, -1 unintelligible, -2 lost connection 

int jhcSpeechWeb::Update (int tts)
{
  const char *recent;

  // ignore audio if robot is speaking
  Listen((tts > 0) ? 0 : 1);
  if (txtin <= 0)
  {
    // check actual speech recognition results
    rcv = NULL;
    hear = reco_status();
    if (hear == 2)
    {
      strcpy_s(raw, reco_heard());               
      fix_up(utt, 200, raw);
      rcv = utt;                                 // for single shot
    }
    else if ((hear == 1) && (dbg > 0))
    {
      recent = reco_partial();
      if (strcmp(recent, utt0) != 0)             // print if changed
      {
        if (*recent != '\0')
          jprintf("  %s ...\n", recent);           
        strcpy_s(utt0, recent);
      }
    }
    else if (hear == -2)
      jprintf("\n>>> Lost internet connection to web speech service!\n\n");
  }
  txtin = 0;
  return hear;
}


//= Substitute canonical terms for commonly misheard phrases in given string.

void jhcSpeechWeb::fix_up (char *fix, int ssz, const char *orig) const
{
  char tail[200];
  const jhcTxtAssoc *k = canon.NextKey();        // first always blank
  const jhcTxtList *v;
  const char *subst, *term;
  char *txt;
  int slen, tlen, rem;

  // walk down list of canonical forms
  strcpy_s(fix, ssz, orig);
  while (k != NULL)
  {
    // get correct sustitution and list of misheard variants
    subst = k->KeyTxt();
    slen  = k->KeyLen();
    v = k->Values();
    while (v != NULL)
    {
      // look for matches to a particular variant
      term = v->ValTxt(); 
      tlen = v->ValLen();
      txt = fix;
      rem = ssz;

      // scan sentence for term bounded by whitespace (or string limits)
      while (*txt != '\0')
      {
        if (((txt == fix) || (isalpha(txt[-1]) == 0)) &&
            (_strnicmp(txt, term, tlen) == 0) && 
            (isalpha(txt[tlen]) == 0))
        {
          // save rest of sentence, change phrase, tack rest back on
          strcpy_s(tail, txt + tlen);
          strcpy_s(txt, rem, subst);
          txt += slen;
          rem -= slen;
          strcpy_s(txt, rem, tail);        // always terminates
        }

        // move on to next character in string
        txt++;
        rem--;
      }
  
      // try next variant
      v = v->NextVal();
    }

    // try next canonical form
    k = k->NextKey();
  }
}


//= Disconnect from online speech service at end of session.

void jhcSpeechWeb::Close ()
{
  clr_state();
  Listen(1);
  reco_cleanup();
}

