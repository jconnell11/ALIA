// jhcTripleMem.h : collection of semantic triples with search operators
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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

#ifndef _JHCTRIPLEMEM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTRIPLEMEM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Semantic/jhcTripleNode.h"
#include "Semantic/jhcTripleLink.h"


//= Collection of semantic triples with search operators.
// contains a singly linked list of numbered entities
// contains a doubly linked list of all triples in historical order
// links are moved within history if their values/fillers are changed
// update and reply pointers are used for turn taking with remote host
// the focus pointer is used to replay the triples received from the host 
// only exposes node names externally, never node or triple pointers

class jhcTripleMem 
{
// PRIVATE MEMBER VARIABLES
private:
  jhcTripleNode *items;     /** List of all nodes by recency. */
  jhcTripleLink *dawn;      /** List of all links in order.   */
  

// PROTECTED MEMBER VARIABLES
protected:
  jhcTripleLink *facts;     /** List of all links in reverse order. */     
  jhcTripleLink *update;    /** Start of new links to send.         */
  jhcTripleLink *reply;     /** Start of recently received links.   */
  jhcTripleLink *focus;     /** Next link to sent or interpret.     */
  int gnum;                 /** Number for next node generated.     */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTripleMem ();
  ~jhcTripleMem ();
  void BlankSlate ();
  void ClrUpdate () {update = NULL;}
  int AnyUpdate () const {return((update == NULL) ? 0 : 1);}

  // node and link modification
  int NewItem (char *id, const char *kind, int ssz);
  int SetValue (const char *id, const char *fcn, const char *val);
  int AddValue (const char *id, const char *fcn, const char *val);
  int BuildTriple (const char *id, const char *fcn, const char *val, int append =0);

  // modification - convenience
  template <size_t ssz>
    int NewItem (char (&id)[ssz], const char *kind =NULL)
      {return NewItem(id, kind, ssz);}

  // node and link interrogation
  int NodeKind (const char *id, const char *kind) const;
  int NumVals (const char *id, const char *fcn);
  int GetValue (const char *id, const char *fcn, char *val, int i, int ssz);
  int NumHead (const char *fcn, const char *val);
  int GetHead (char *id, const char *fcn, const char *val, int i, int ssz);
  int GetHeadKind (char *id, const char *kind, const char *fcn, const char *val, int i, int ssz);
  int MatchTriple (const char *id, const char *fcn, const char *val);

  // interrogation - convenience
  template <size_t ssz>
    int GetValue (const char *id, const char *fcn, char (&val)[ssz], int i =0)
      {return GetValue(id, fcn, val, i, ssz);}
  template <size_t ssz>
    int GetHead (char (&id)[ssz], const char *fcn, const char *val, int i =0)
      {return GetHead(id, fcn, val, i, ssz);}
  template <size_t ssz>
    int GetHeadKind (char (&id)[ssz], const char *kind, const char *fcn, const char *val, int i =0)
      {return GetHeadKind(id, kind, fcn, val, i, ssz);}

  // debugging functions
  int DumpItems (const char *fname, const char *hdr =NULL) const;
  int DumpHist (const char *fname, const char *hdr =NULL) const;
  void PrintItems () const;
  void PrintLinks (const char *id) const;
  void PrintArgs (const char *id) const;
  void PrintProps (const char *id) const;
  void PrintHist () const;
  void PrintUpdate () const;


// PRIVATE MEMBER FUNCTIONS
private:
  void init_ptrs ();

  // low level nodes 
  int node_num (const char *id) const;
  jhcTripleNode *find_node (const char *id);
  const jhcTripleNode *read_node (const char *id) const;
  jhcTripleNode *add_node (const char *kind);

  // low level links
  jhcTripleLink *add_link (jhcTripleNode *n, const char *fcn, jhcTripleNode *n2);
  jhcTripleLink *add_link (jhcTripleNode *n, const char *fcn, const char *txt);
  void pluck (jhcTripleLink *t);
  void push (jhcTripleLink *t);


};


#endif  // once




