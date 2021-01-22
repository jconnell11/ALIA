// jhcAliaDesc.h : simple external interface to ALIA semantic networks
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCALIADESC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIADESC_
/* CPPDOC_END_EXCLUDE */


//= Simple external interface to ALIA semantic networks.
// basically watered down version of jhcNetNode class

class jhcAliaDesc
{
// PUBLIC MEMBER FUNCTIONS
public:

  //= Get the n'th filler for the given role.
  // returns NULL if invalid index
  virtual jhcAliaDesc *Val (const char *slot, int i =0) const =0;

  //= Get the n'th node that has this node as a filler for the given role.
  // useful for asking about this node relative to "ako" or "hq" 
  // most recently added properties returned first
  // returns NULL if invalid index
  virtual jhcAliaDesc *Fact (const char *role, int i =0) const =0;

  //= Get a specific tag out of all the words associated with this item.
  // if "bth" > 0.0 then only returns non-negated words with belief over threshold
  // most recently added terms returned first
  virtual const char *Word (int i =0, double bth =0.0) const =0;

  //= Checks if particular word is one of the tags associated with this item.
  // can alternatively check if the node is definitely NOT associated with some word
  virtual bool HasWord (const char *word, int tru_only =0) const =0;

  //= Checks if lexical tag is one of several items.
  // largely for convenience in grounding commands
  virtual bool WordIn (const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
                       const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const =0;

  //= Get any literal text string associated with the item.
  virtual const char *Literal () const =0;

  //= Check if the node is negated.
  virtual int Neg () const =0;

  //= Check if the node has no arguments.
  virtual bool ObjNode () const =0;

  //= Give pretty name for node (mostly for debugging).
  virtual const char *Nick () const =0;


};


#endif  // once




