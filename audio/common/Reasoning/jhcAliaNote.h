// jhcAliaNote.h : interface for asserting facts in ALIA architecture
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2021 Etaoin Systems
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


//= Write interface for asserting facts in ALIA architecture.
// basically watered down version of jhcActionTree class

class jhcAliaNote
{
// PUBLIC MEMBER FUNCTIONS
public:

  //= Open up a potential top level focus NOTE directive for construction.
  // can call NewNode and NewProp to fill in, call FinishNote at end
  virtual void StartNote () =0;

  //= Add a new node of some type to current note.
  // returns a pointer to the new node or NULL if error
  virtual jhcAliaDesc *NewNode (const char *kind, const char *word =NULL, int neg =0, double blf =1.0, int done =0) =0;

  //= Create a new node to represent a property of this node.
  // can optionally check if such a node already exists and return it instead 
  // returns a pointer to the appropriate node or NULL if error
  virtual jhcAliaDesc *NewProp (jhcAliaDesc *head, const char *role, const char *word,
                                int neg =0, double blf =1.0, int chk =0, int args =1) =0;

  //= Create a new node to represent a property of this node having some degree.
  // can optionally check if such a node already exists and return it instead 
  // returns a pointer to the appropriate degree node or NULL if error
  virtual jhcAliaDesc *NewDeg (jhcAliaDesc *head, const char *role, const char *word, const char *amt, 
                               int neg =0, double blf =1.0, int chk =0, int args =1) =0;

  //= Make some other node be an named argument of the head.
  virtual void AddArg (jhcAliaDesc *head, const char *slot, jhcAliaDesc *val) const =0;

  //= Keep the node from being erased by garbage collector.
  virtual void Keep (jhcAliaDesc *obj) const =0;

  //= Pretend the node was just added on this cycle (needed for FIND).
  virtual void NewFound (jhcAliaDesc *obj) const =0;

  //= Add morphological tag to aid in verbal response generation.
  virtual void GramTag (jhcAliaDesc *prop, int t) const =0;

  //= Locate most recent existing node with compatible person name.
  virtual jhcAliaDesc *Person (const char *name) const =0;

  //= Reference to the robot itself.
  virtual jhcAliaDesc *Self () const =0;

  //= Reference to the current user.
  virtual jhcAliaDesc *User () const =0;

  //= Associate a visual entity ID (not track) with some semantic network node.
  virtual int VisAssoc (int tid, jhcAliaDesc *obj, int kind =0) =0;

  //= Conversion from semantic network node to associated visual entity ID (not track).
  virtual int VisID (const jhcAliaDesc *obj, int kind =0) const =0;

  //= Conversion from visual entity ID (not track) to associated semantic network node.
  virtual jhcAliaDesc *NodeFor (int tid, int kind =0) const =0;

  //= Enumerate ID's for all items of a certain kind having an external link.
  virtual int VisEnum (int last, int kind =0) const =0; 

  //= Add current note as a focus, possibly marking some part as the main error.
  // return number of focus if added, -2 if empty
  virtual int FinishNote (jhcAliaDesc *fail =NULL) =0;

};


#endif  // once




