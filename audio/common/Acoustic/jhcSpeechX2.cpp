// jhcSpeechX2.cpp : does acoustic recognition followed by CFG reparsing
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2019 IBM Corporation
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

#include <stdarg.h>
#include <string.h>

#include "Acoustic/jhcSpeechX2.h"


///////////////////////////////////////////////////////////////////////////
//                          Convenience Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Update status of all speech-related status variables.
// clears parse tree so association list will be empty if nothing new
// returns status of recognition (0 = quiet, 1 = speech, 2 = recognition)

int jhcSpeechX2::Update (int reco, int prolong) 
{
  int ans = jhcSpeechX::Update(reco, prolong);

  if (ans >= 2)
    Parse(jhcSpeechX::Input());
  else
    ClrTree();
  return ans;
}


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

// Sets up components of speech system.
// needs to be called before using recognition or TTS
// returns positive if successful, 0 or negative for failure

int jhcSpeechX2::Init (int dbg, int noisy)
{
  int ans = jhcSpeechX::Init(dbg, noisy);  // does everything for jhcSpeechX base

  jhcGramExec::ClearGrammar();
  if (jhcGramExec::LoadGrammar(NULL) <= 0)
  {
    if (noisy > 0)
      jprintf("\n>>> grammar FAILED!\n");
    return 0;
  }
  return ans;
}


//= Print out full configuration data for current system.
// returns overall status of system (number of rules)

int jhcSpeechX2::PrintCfg ()
{
  jhcSpeechX::PrintCfg();
  return jhcGramExec::PrintCfg();
} 


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Remembers grammar to load but does NOT load it yet.
// generally "gfile" is only the first grammar loaded
// NOTE: most common pattern is SetGrammar then Init 

void jhcSpeechX2::SetGrammar (const char *fname, ...)
{
  char gf[200];
  va_list args;

  // assemble file name
  if ((fname == NULL) || (*fname == '\0'))
    return;
  va_start(args, fname); 
  vsprintf_s(gf, fname, args);
  if (strchr(gf, '.') == NULL)
    strcat_s(gf, ".sgm");

  // copy to both components
  jhcSpeechX::SetGrammar(gf);
  jhcGramExec::SetGrammar(gf);
}


//= Get rid of all old parsing rules.

void jhcSpeechX2::ClearGrammar (int keep)
{
  jhcSpeechX::ClearGrammar(keep);
  jhcGramExec::ClearGrammar(keep);
}


//= Load a recognition grammar from a generic file.
// appends new rules if some other grammar(s) already loaded
// to get rid of old rules first call ClearGrammar()
// all rules initially unmarked (i.e. not active top level)
// returns 0 if some error, else 1
// NOTE: most common pattern is SetGrammar then Init 

int jhcSpeechX2::LoadGrammar (const char *fname, ...)
{
  char gf[200];
  va_list args;
  int ans = 1;

  // assemble file name
  if ((fname == NULL) || (*fname == '\0'))
    return 0;
  va_start(args, fname); 
  vsprintf_s(gf, fname, args);
  if (strchr(gf, '.') == NULL)
    strcat_s(gf, ".sgm");

  // load into both components
  ans &= jhcSpeechX::LoadGrammar(gf);
  ans &= jhcGramExec::LoadGrammar(gf);
  return ans; 
}


//= Activate (val = 1) or deactivate (val = 0) a grammar rule.
// use NULL as name to mark all top level rules
// returns 0 if could not find rule, else 1

int jhcSpeechX2::MarkRule (const char *name, int val)
{
  int ans = 1;

  ans &= jhcSpeechX::MarkRule(name, val);
  ans &= jhcGramExec::MarkRule(name, val);
  return ans;
}


//= Add another valid expansion for some non-terminal.
// returns 2 if full update, 1 if not in file, 0 or negative for failure

int jhcSpeechX2::ExtendRule (const char *name, const char *phrase)
{
  int rc, ans = 2;

  rc = jhcSpeechX::ExtendRule(name, phrase);
  ans = __min(ans, rc);
  rc = jhcGramExec::ExtendRule(name, phrase);
  return __min(ans, rc);
}

