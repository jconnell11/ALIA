// jhcAliaNote.h : interface for asserting facts in ALIA architecture
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

#ifndef _JHCALIANOTE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIANOTE_
/* CPPDOC_END_EXCLUDE */

#include "Semantic/jhcAliaDesc.h"       // common audio


//= Interface for asserting facts in ALIA architecture.
// basically watered down version of jhcAliaAttn class

class jhcAliaNote
{
// PUBLIC MEMBER FUNCTIONS
public:

  //= Open up a potential top level focus NOTE directive for construction.
  // can call NewNode and NewProp to fill in, call FinishNote at end
  virtual void StartNote () =0;

  //= Add a new node of some type to current note.
  // returns a pointer to the new node or NULL if error
  virtual jhcAliaDesc *NewNode (const char *kind, const char *word =NULL, int neg =0, double blf =1.0) =0;

  //= Create a new node to represent a property of this node.
  // returns a pointer to the new node or NULL if error
  virtual jhcAliaDesc *NewProp (jhcAliaDesc *head, const char *role, const char *word,
                                int neg =0, double blf =1.0, const char *kind =NULL) =0;

  //= Make some other node be an named argument of the head.
  virtual void AddArg (jhcAliaDesc *head, const char *slot, jhcAliaDesc *val) =0;

  //= Create a new node to represent linguistic term associated with this node.
  virtual void NewLex (jhcAliaDesc *head, const char *word, int neg =0, double blf =1.0) =0;

  //= Locate most recent existing node with compatible person name.
  virtual jhcAliaDesc *Person (const char *name) const =0;

  //= Reference to the robot itself.
  virtual jhcAliaDesc *Self () const =0;

  //= Reference to the current user.
  virtual jhcAliaDesc *User () const =0;

  //= Add current note as a focus or delete it.
  // return number of focus if added, -2 if deleted
  virtual int FinishNote (int keep =1) =0;

};


#endif  // once




