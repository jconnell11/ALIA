// jhcNamePool.h : collection of name data for specific people
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017 IBM Corporation
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

#ifndef _JHCNAMEPOOL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNAMEPOOL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Collection of name data for specific people.
// input: Dr. Jonathan (Jon) Connell

class jhcNamePool
{
// PROTECTED MEMBER VARIABLES
protected:
  static const int pmax = 100;    /** Maximum number of people. */


// PRIVATE MEMBER VARIABLES
private:
  // core database
  char title[pmax][20];           /** Title (Dr.).              */
  char first[pmax][40];           /** First name (Jonathan).    */
  char nick[pmax][20];            /** Nick name (Jon).          */
  char last[pmax][40];            /** Last name (Connell).      */
  int np;                         /** Number of people known.   */

  // tie braking
  int recent[pmax];               /** Last addition or lookup.  */
  int qcnt;                       /** Number of queries so far. */

  // general operations
  char tmp[200];                  /** Temporary output string.  */
  char word[4][40];               /** Input tokenization.       */
  int wcnt;                       /** Number of tokens found.   */


// PUBLIC MEMBER VARIABLES
public:
  char list[2 * pmax][40];        /** Result of AllX functions. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcNamePool ();
  jhcNamePool ();
 
  // ingest functions
  void ClrAll ();
  void ClrPerson (int id);
  int AddPerson (const char *mixed, int recycle =1);
  int CountPeople () const;

  // main functions
  int NumMatch (const char *ref);
  int PossibleID (const char *ref, int choice =0);
  int PersonID (const char *ref);
  const char *Condense (int id);
  const char *ShortName (int id);
  const char *ShortName (const char *ref)
    {return ShortName(PersonID(ref));}
  const char *LongName (int id);
  const char *LongName (const char *ref)
    {return LongName(PersonID(ref));}
  const char *FormalName (int id);
  const char *FormalName (const char *ref)
    {return FormalName(PersonID(ref));}

  // enumeration functions
  int NumPeople () const {return np;}
  int Variants () const {return 6;}
  const char *GetVariant (int id, int n);
  int AllVars ();
  int AllFirst (int poss =0);
  int AllLast (int poss =0);

  // file functions
  int Load (const char *fname, int clr =1);
  int Save (const char *fname);
  int SaveVars (const char *fname, const char *cat =NULL);
  int SaveParts (const char *fname);


// PRIVATE MEMBER FUNCTIONS
private:
  // ingest functions
  bool empty (int id) const;
  int get_words (const char *ref);
  bool is_title (const char *word) const;
  bool grab_nick (int id, const char *word);
  bool consistent (int id) const;


};


#endif  // once




