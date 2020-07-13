// jhcTripleLink.h : properties of entities and relations between them
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

#ifndef _JHCTRIPLELINK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTRIPLELINK_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>                         // needed for FILE

#include "Semantic/jhcTripleNode.h"


//= Properties of entities and relations between them.
// conceptually slots are required arguments of the topic predicate
// the value can either be a string (for tags) or a node (for relations)
// both arguments and tags are not expected to change much (or ever)
// NOTE: destructor cleans up ALL associated lists (incl. history) 

class jhcTripleLink
{
// PRIVATE MEMBER VARIABLES
private:
  // primary content (read-only)
  jhcTripleNode *topic;            /** Head node of relation.      */
  char slot[40];                   /** Function label of relation. */
  jhcTripleNode *filler;           /** Value node for relation.    */
  char txt[200];                   /** Value text for tag.         */

  // related links (read-only)
  jhcTripleLink *alist;            /** Next link with same topic.  */
  jhcTripleLink *plist;            /** Next link with same filler. */


// PUBLIC MEMBER VARIABLES
public:
  // history list (external)
  jhcTripleLink *prev;             /** Previous fact in history.   */
  jhcTripleLink *next;             /** Next fact in history.       */
  int multi;                       /** Set if not first value.     */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTripleLink (jhcTripleNode *n, const char *fcn);
  ~jhcTripleLink ();

  // read-only access
  jhcTripleNode *Head () const {return topic;}
  const char *Fcn () const     {return slot;}
  jhcTripleNode *Fill () const {return filler;}
  const char *Tag () const     {return((filler != NULL) ? NULL : txt);}
  jhcTripleLink *NextArg () const  {return alist;}
  jhcTripleLink *NextProp () const {return plist;}

  // main functions
  int SetFill (jhcTripleNode *n2);
  int SetTag (const char *tag);

  // debugging functions 
  void Print (const char *tag =NULL) const;
  int Tabbed (FILE *out) const;


// PRIVATE MEMBER FUNCTIONS
private:
  void rem_arg ();
  void rem_prop ();


};


#endif  // once




