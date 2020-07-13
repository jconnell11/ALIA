// jhcJSON.h : builds common web JSON datastructures
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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

#ifndef _JHCJSON_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCJSON_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Builds common web JSON datastructures.
// kind: -1 = null, 0 = false, 1 = true, 2 = number, 3 = string, 4 = arr, 5 = map
// Note: very aggressive object deletion (garbage collection)
// beware dangling pointers and shared substructures
// designed primarily for web IO, not actual computation

class jhcJSON
{
// PRIVATE MEMBER VARIABLES
private:
  int kind;
  double num;
  char *tag;
  jhcJSON *head, *tail; 


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcJSON ();
  jhcJSON (bool v);
  jhcJSON (int v);
  jhcJSON (double v);
  jhcJSON (const char *v);
  ~jhcJSON ();
 
  // type functions
  bool Atom () const;
  bool Null () const;
  bool Boolean () const;
  bool False () const;
  bool True () const;
  bool Integer () const;
  bool Number () const;
  bool String () const;
  bool Array () const;
  bool Map () const;

  // atoms
  void Clr ();
  void Set (bool v);
  void Set (int v);
  void Set (double v);
  void Set (const char *v);
  bool BoolVal () const;
  int IntVal () const;
  double NumVal () const;
  const char *StrVal () const;

  // arrays
  void MakeArr (int n =0, int wipe =1);
  int SetVal (int index);
  int SetVal (int index, bool v);
  int SetVal (int index, int v);
  int SetVal (int index, double v);
  int SetVal (int index, const char *v);
  int SetVal (int index, jhcJSON *v);
  jhcJSON *NewVal ();
  void Add (bool v);
  void Add (int v);
  void Add (double v);
  void Add (const char *v);
  jhcJSON *Add (jhcJSON *v);
  int Truncate ();
  int Len () const;
  jhcJSON *GetVal (int index) const;
  bool HasVal (const char *txt) const;

  // maps
  void MakeMap (int wipe =1);
  jhcJSON *NewKey (const char *key);
  void SetKey (const char *key, bool v);
  void SetKey (const char *key, int v);
  void SetKey (const char *key, double v);
  void SetKey (const char *key, const char *v);
  jhcJSON *SetKey (const char *key, jhcJSON *v);
  int RemKey (const char *key);
  const char *Key () const;
  jhcJSON *GetKey (const char *key) const;
  jhcJSON *FindKey (const char *key);
  jhcJSON *First () const;
  jhcJSON *Rest () const;
  bool MatchStr (const char *key, const char *val) const;

  // serialization
  void Print () const;
  int Dump (char *dest, int lvl, int ssz) const;
  const char *Ingest (const char *src);

  // serialization - convenience
  template <size_t ssz>
    int Dump (char (&dest)[ssz], int lvl =0) const
      {return Dump(dest, lvl, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and configuration
  void init ();

  // printing
  int print_atom (char *dest, int ssz) const;
  int print_arr (char *dest, int lvl, int ssz) const;
  int print_map (char *dest, int lvl, int ssz) const;

  // ingest
  const char *build_arr (const char *src);
  const char *build_map (const char *src);
  const char *build_atom (const char *src);
  const char *get_token (char *token, int n, const char *src, const char *stop, int ssz) const;


};


#endif  // once




