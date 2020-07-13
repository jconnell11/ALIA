// jhcSlotVal.h : functions for manipulating association lists from parser
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2020 IBM Corporation
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

#ifndef _JHCSLOTVAL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSLOTVAL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Functions for manipulating association lists from parser.
// typically used as a base class for some command interpreter
// <pre> 
//
// SLOT VALUE PAIRS:
//
//   Capitalized non-terminals are slots which receive the first non-terminal of their 
//   expansion as their value. If the first character is "^" or there are no non-terminals, 
//   then the value is the set of words spanned by the non-terminal.
//
//   Non-terminals that start with "!" (actions) or "$" (arguments) are emitted as fragment
//   markers only, and still allow retrieval of slot value pairs beneath them in the tree.
//
//   Example:
//
//     =[top]
//       <!ingest>
//       <!yack>
//
//     =[!ingest]
//       eat <^FOOD>
//       drink <BEV>
//
//     =[BEV]
//       (some) <soda>
//       a glass of milk
//
//     =[soda]
//       soda
//       pop
//       Coke
//
//     =[^FOOD]
//       (<quant>) +
//
//     =[quant]
//       a lot of
//       a piece of 
//
//   "drink some Coke"          --> !ingest BEV=soda
//   "drink a glass of milk"    --> !ingest BEV=a glass of milk
//   "eat a piece of apple pie" --> !ingest FOOD=a piece of apple pie    
//
//   All entries in the association list are separated by tabs.
//
// </pre>

class jhcSlotVal
{
// PUBLIC MEMBER VARIABLES
public:
  int dbg;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcSlotVal () {dbg = 0;}

  // main functions
  void CallList (int lvl, const char *fcn, const char *alist, int skip =0, const char *entry = NULL) const;
  void PrintList (const char *alist, const char *tag =NULL, int skip =0, int cr =1) const;
  const char *SetList (char *alist, const char *src, int ssz) const;
  int ChkAttn (const char *alist) const;
  char *CleanVal (char *dest) const;
  char *CleanVal (char *dest, const char *src) const;
  const char *StripEntry (const char *alist) const {return NextEntry(alist, NULL, 0);}
  const char *NextEntry (const char *alist, char *entry, int ssz) const;
  const char *NextMatches (const char *alist, const char *tag, int n =0) const;

  // main functions - convenience
  template <size_t ssz>
    const char *SetList (char (&alist)[ssz], const char *src) const
      {return SetList(alist, src, ssz);}
  template <size_t ssz>
    const char *NextEntry (const char *alist, char (&entry)[ssz]) const
      {return NextEntry(alist, entry, ssz);}

  // slot functions
  bool HasSlot (const char *alist, const char *slot, int local =0) const;
  bool AnySlot (const char *alist, const char *marks, int local =0) const;
  const char *FindSlot (const char *alist, const char *slot, char *val, int local, int ssz) const;
  const char *NextSlot (const char *alist, char *slot, char *val, int local, int ssz, int vsz) const;
  bool SlotMatch (const char *pair, const char *slot) const;
  int SlotStart (const char *pair, const char *prefix =NULL) const;
  char *SlotRef (char *pair) const;
  const char *SlotVal (const char *pair) const;
  const char *SlotGet (char *pair, const char *prefix =NULL, int lower =1) const;

  // slot functions - convenience
  template <size_t ssz>
    const char *FindSlot (const char *alist, const char *slot, char (&val)[ssz], int local =0) const
      {return FindSlot(alist, slot, val, local, ssz);}
  template <size_t ssz, size_t vsz>
    const char *NextSlot (const char *alist, char (&slot)[ssz], char (&val)[vsz], int local =0) const
      {return NextSlot(alist, slot, val, local, ssz, vsz);}

  // fragment functions
  bool HasFrag (const char *alist, const char *frag) const;
  bool AnyFrag (const char *alist, const char *kinds) const;
  const char *FindFrag (const char *alist, const char *frag) const;
  const char *NextFrag (const char *alist, char *frag, int ssz) const;
  const char *PeekFrag (char *head, const char *alist, int ssz) const {return NextFrag(alist, head, ssz);}
  const char *FragNextFrag (const char *alist, char *frag, int ssz) const;
  const char *FragNextPair (const char *alist, char *pair, int ssz) const;
  bool FragHasSlot (const char *alist, const char *slot) const;
  const char *FragClose (const char *alist, int skip =1) const;

  // fragment functions - convenience
  template <size_t ssz>
    const char *NextFrag (const char *alist, char (&frag)[ssz]) const
      {return NextFrag(alist, frag, ssz);}
  template <size_t ssz>
    const char *PeekFrag (char (&head)[ssz], const char *alist) const
      {return PeekFrag(head, alist, ssz);}
  template <size_t ssz>
    const char *FragNextFrag (const char *alist, char (&frag)[ssz]) const
      {return FragNextFrag(alist, frag, ssz);}
  template <size_t ssz>
    const char *FragNextPair (const char *alist, char (&pair)[ssz]) const
      {return FragNextPair(alist, pair, ssz);}

  // fragment parsing
  const char *ExtractFrag (char *frag, const char *alist, int fsz) const
    {return ExtractFrag(NULL, frag, alist, 0, fsz);}
  const char *ExtractFrag (char *head, char *frag, const char *alist, int hsz, int fsz) const;
  const char *SplitFrag (char *head, char *body, const char *alist, int hsz, int bsz) const;
  const char *ExtractBody (const char *match, char *body, const char *alist, int bsz, int prefix) const;

  // fragment parsing - convenience
  template <size_t fsz>
    const char *ExtractFrag (char (&frag)[fsz], const char *alist) const
      {return ExtractFrag(frag, alist, fsz);}
  template <size_t hsz, size_t fsz>
    const char *ExtractFrag (char (&head)[hsz], char (&frag)[fsz], const char *alist) const
      {return ExtractFrag(head, frag, alist, hsz, fsz);}
  template <size_t hsz, size_t bsz>
    const char *SplitFrag (char (&head)[hsz], char (&body)[bsz], const char *alist) const
      {return SplitFrag(head, body, alist, hsz, bsz);}
  template <size_t bsz>
    const char *ExtractBody (const char *match, char (&body)[bsz], const char *alist, int prefix =0) const
      {return ExtractBody(match, body, alist, bsz, prefix);}


// PRIVATE MEMBER FUNCTIONS
private:
  // slot functions
  int all_lower (char *txt) const;


};


#endif  // once




