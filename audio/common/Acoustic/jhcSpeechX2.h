// jhcSpeechX2.h : does acoustic recognition followed by CFG reparsing
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

#ifndef _JHCSPEECHX2_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSPEECHX2_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Acoustic/jhcSpeechX.h"
#include "Parse/jhcGramExec.h"


//= Does acoustic recognition followed by CFG reparsing.
// overrides some virtuals in jhcGramExec to make grammars consistent
// jhcSpeechX grammar makes sure output string is acceptable
// reparsing with jhcGramExec gives better control over wildcards

class jhcSpeechX2 : public jhcSpeechX, public jhcGramExec
{
// PUBLIC MEMBER FUNCTIONS
public:
  // input status and processing (override jhcSpeechX)
  int Update (int reco =1, int prolong =0);

  // creation and configuration (override jhcGramExec)
  int Init (int dbg =0, int noisy =0);
  int PrintCfg ();

  // parsing (override jhcGramExec)
  void SetGrammar (const char *fname, ...);
  void ClearGrammar (int keep =1);
  int LoadGrammar (const char *fname, ...);
  int MarkRule (const char *name =NULL, int val =1);
  int ExtendRule (const char *name, const char *phrase);


};


#endif  // once




