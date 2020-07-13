// jhcTripleNode.h : entities which can have properties and relations
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

#ifndef _JHCTRIPLENODE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTRIPLENODE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Entities which can have properties and arguments (V only).
// NOTE: destructor does NOT clean up any of the lists (e.g. object array)

class jhcTripleNode
{
friend class jhcTripleLink;        // needed to maintain args and props

// PRIVATE MEMBER VARIABLES
private:
  // primary content
  char id[80];                     /** Node name (e.g. "box-3").      */

  // related links
  class jhcTripleLink *args;       /** Triples with this as topic.    */
  class jhcTripleLink *props;      /** Triples with this as filler.   */


// PUBLIC MEMBER VARIABLES
public:
  // object array (external)
  jhcTripleNode *prev;             /** Previous item in object array. */
  jhcTripleNode *next;             /** Next item in object array.     */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTripleNode (const char *name)
  {
    // basic information
    strcpy_s(id, name);

    // triples for which this is topic or filler
    args  = NULL;
    props = NULL;

    // object list
    prev  = NULL;
    next  = NULL;
  }

  // read-only access
  const char *Name () const {return id;}
  class jhcTripleLink *ArgList () const  {return args;}
  class jhcTripleLink *PropList () const {return props;}


};


#endif  // once




