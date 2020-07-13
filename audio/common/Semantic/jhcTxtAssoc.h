// jhcTxtAssoc.h : key and values in a singly linked association list
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2020 IBM Corporation
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

#ifndef _JHCTXTASSOC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTXTASSOC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Semantic/jhcTxtList.h"


//= Key and values in a singly linked association list.
// keys are always unique but ALWAYS include a default "" category
// values are also unique but CANNOT include "" blank entries
// each key can have an optional prior probability (saved) 
// Note: automatically deallocates values and rest of list when deleted

class jhcTxtAssoc
{
// PRIVATE MEMBER VARIABLES
private:
  char tmp[200];               /** Temporary output string. */
  char key[200];               /** Indexing term string.    */
  jhcTxtAssoc *next;           /** Rest of indexing terms.  */
  jhcTxtList *vals;            /** Expansion for index.     */
  int klen;                    /** Length of key string.    */
  int last;                    /** Set if last request.     */


// PUBLIC MEMBER VARIABLES
public:
  double prob;                 /** Likelihood of term. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTxtAssoc ();
  ~jhcTxtAssoc ();
  void Seed ();

  // properties and retrieval
  int NumKeys () const;
  int MaxDepth (int blank =0) const;
  int TotalVals () const;
  int MaxLength () const;
  const char *KeyTxtN (int n =0) const;
  const char *KeyTxtFor (const char *vtxt) const;
  bool Member (const char *ktag, const char *vtxt, int def =0, int caps =0) const;
  int Invert (const jhcTxtAssoc &src, int clean =0);

  // random selection
  void Reset ();
  const char *PickRnd (const char *ktag =NULL, int def =0);

  // keys and values
  const char *KeyTxt () const {return key;}
  int KeyLen () const {return klen;}
  const jhcTxtAssoc *NextKey () const {return next;}
  const jhcTxtAssoc *ReadKey (const char *ktag) const;
  jhcTxtAssoc *GetKey (const char *ktag);
  int NumVals () const;
  const jhcTxtList *Values () const {return vals;}

  // building and editing
  jhcTxtAssoc *AddKey (const char *ktag, double p =1.0, int force =0);
  int RemKey (const char *ktag);
  void ClrKeys ();
  const jhcTxtList *AddVal (const char *vtxt, double w =1.0);
  int IncVal (const char *vtxt, double amt =1.0);
  int RemVal (const char *vtxt);
  int ClrVals ();

  // file operations
  int LoadList (const char *fname, int clean =0, int merge =0);
  int SaveList (const char *fname) const;

  // variable substitution
  char *MsgRnd (const char *ktag =NULL, const char *arg0 =NULL, 
                const char *arg1 =NULL, const char *arg2 =NULL, const char *arg3 =NULL, 
                const char *arg4 =NULL, const char *arg5 =NULL, const char *arg6 =NULL,
                const char *arg7 =NULL, const char *arg8 =NULL, const char *arg9 =NULL);
  char *FillRnd (char *full, const char *ktag =NULL, const char *arg0 =NULL, 
                 const char *arg1 =NULL, const char *arg2 =NULL, const char *arg3 =NULL, 
                 const char *arg4 =NULL, const char *arg5 =NULL, const char *arg6 =NULL,
                 const char *arg7 =NULL, const char *arg8 =NULL, const char *arg9 =NULL);
  char *Compose (char *full, const char *pattern, const char *arg0 =NULL,
                 const char *arg1 =NULL, const char *arg2 =NULL, const char *arg3 =NULL, 
                 const char *arg4 =NULL, const char *arg5 =NULL, const char *arg6 =NULL,
                 const char *arg7 =NULL, const char *arg8 =NULL, const char *arg9 =NULL) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // low-level value functions
  jhcTxtList *nth_val (int n) const;
  const jhcTxtList *read_val (const char *vtxt, int caps) const;
  jhcTxtList *get_val (jhcTxtList **p, const char *vtxt);
  jhcTxtList *insert_val (const char *vtxt, double w);
  void drop_val (jhcTxtList *p, jhcTxtList *v);

  // random selection
  int enabled (int force);
  jhcTxtList *first_choice ();
  jhcTxtList *rand_choice ();

  // file operations
  char *trim_wh (char *src) const;


};


#endif  // once




