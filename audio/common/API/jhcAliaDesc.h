// jhcAliaDesc.h : simple external interface to ALIA semantic networks
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#pragma once


//= Simple external read interface to ALIA semantic networks.
// basically watered down version of jhcNetNode class

class jhcAliaDesc
{
// PUBLIC MEMBER FUNCTIONS
public:

  //= Give pretty name for node (mostly for debugging).
  virtual const char *Nick () const =0;

  //= Tell whether node has been initialized for matching or is hidden.
  virtual bool Visible () const =0;

  //= Check if the node is negated.
  virtual int Neg () const =0;

  //= Check if the node represents an action which has been completed.
  virtual int Done () const =0;

  //= Get the (unique) lexical term associated with this predicate.
  // some nodes have blank string (but not NULL)
  virtual const char *Lex () const =0;

  //= Checks if particular word is the lexical terms associated with this predicate.
  virtual bool LexMatch (const char *txt) const =0;

  //= Checks if predicate lexical term is one of several items.
  // largely for convenience in grounding commands
  virtual bool LexIn (const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
                      const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const =0;

  //= Get the n'th filler for the given role.
  // returns NULL if invalid index
  virtual jhcAliaDesc *Val (const char *slot, int i =0) const =0;

  //= Get the n'th node that has this node as a filler for the given role.
  // useful for asking about this node relative to "ako" or "hq" 
  // most recently added properties returned first
  // returns NULL if invalid index
  virtual jhcAliaDesc *Fact (const char *role, int i =0) const =0;

  //= Checks if particular name is one of the references associated with this item.
  // can alternatively check if the node is definitely NOT associated with some word
  virtual bool HasName (const char *word, int tru_only =0) const =0;

  //= Get any literal text string associated with the item.
  virtual const char *Literal () const =0;

  //= Check if the node has no arguments.
  virtual bool ObjNode () const =0;

  //= Check if the node has no properties.
  virtual bool Naked () const =0;

  //= Give unique numeric index associated with node.
  virtual int Inst () const =0;

  //= Tell what cycle the node was last mentioned in conversation.
  virtual int LastConvo () const =0;


};

