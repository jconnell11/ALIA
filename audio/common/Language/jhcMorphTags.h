// jhcMorphTags.h : collection of grammatical bit defintions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#ifndef _JHCMORPHTAGS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMORPHTAGS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= A variety of largely grammatical properties associated with a node.
// seldom used outside this header file

enum JTAG_VAL 
{
  JTV_DEF,    // definite object ("the")
  JTV_ALT,    // alternative object ("other")

  JTV_NZERO,  // absent noun ("none") 
  JTV_NSING,  // singular noun
  JTV_NPL,    // plural noun
  JTV_NMASS,  // mass noun

  JTV_APROP,  // normal adjective
  JTV_ACOMP,  // comparative adjective (-er)
  JTV_ASUP,   // superlative adjective (-est)

  JTV_VIMP,   // imperative verb
  JTV_VPRES,  // present tense verb (-s)
  JTV_VPAST,  // past tense verb (-ed)
  JTV_VPROG,  // progressive verb (-ing)
  JTV_VFUT,   // future tense verb ("will X")
  JTV_VINF,   // infinitive verb ("to X")

  JTV_MAX

};


//= String names for various properties.
// useful for file I/O

const char JTAG_STR[JTV_MAX][10] = 
{
  "def",
  "alt",

  "nzero", 
  "nsing",
  "npl", 
  "nmass", 

  "aprop", 
  "acomp",
  "asup",

  "vimp", 
  "vpres", 
  "vpast",
  "vprog",
  "vfut", 
  "vinf"

};


//= Bit masks for properities based on their enumerated value.
// most commonly used declarations

enum JTAG_MASK
{
  JTAG_DEF   = (0x01 << JTV_DEF),
  JTAG_ALT   = (0x01 << JTV_ALT),

  JTAG_NZERO = (0x01 << JTV_NZERO),
  JTAG_NSING = (0x01 << JTV_NSING),
  JTAG_NPL   = (0x01 << JTV_NPL),
  JTAG_NMASS = (0x01 << JTV_NMASS),

  JTAG_APROP = (0x01 << JTV_APROP),
  JTAG_ACOMP = (0x01 << JTV_ACOMP),
  JTAG_ASUP  = (0x01 << JTV_ASUP),

  JTAG_VIMP  = (0x01 << JTV_VIMP),
  JTAG_VPRES = (0x01 << JTV_VPRES),
  JTAG_VPAST = (0x01 << JTV_VPAST),
  JTAG_VPROG = (0x01 << JTV_VPROG),
  JTAG_VFUT  = (0x01 << JTV_VFUT),
  JTAG_VINF  = (0x01 << JTV_VINF)

};


//= All mask bits associated with nouns.

const UL32 JTAG_NOUN = (JTAG_DEF | JTAG_ALT | JTAG_NZERO | JTAG_NSING | JTAG_NPL | JTAG_NMASS);


//= All mask bits associated with verbs.

const UL32 JTAG_VERB = (JTAG_VIMP | JTAG_VPRES | JTAG_VPAST | JTAG_VPROG | JTAG_VFUT | JTAG_VINF);


//= All mask bits associated with adjectives.

const UL32 JTAG_ADJ = (JTAG_APROP | JTAG_ACOMP | JTAG_ASUP);


#endif  // once