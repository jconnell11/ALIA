// jhcNetAsm.cpp : builds semantic network from output of syntactic parser
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

#include <string.h>
#include <stdio.h>

#include "Semantic/jhcNetAsm.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcNetAsm::jhcNetAsm ()
{
  mem = NULL;
  edit  = 0.75;     // post command grace (sec)
  turn  = 1.5;      // wait for query (sec)
  flush = 5.0;      // give up missing (sec)
  Reset();
}


//= Default destructor does necessary cleanup.

jhcNetAsm::~jhcNetAsm ()
{
}


//= Reset state for beginning of interaction.

void jhcNetAsm::Reset ()
{
  // dialog state
  rc   = 0;
  miss = 0;
  nag  = 0;
  attn = 0;

  // command components
  *evt  = '\0';
  *obj  = '\0';
  *dest = '\0';
  need_obj  = 0;
  need_dest = 0;

  // intermediate components
  *np = '\0';
  *pp = '\0';
  need_dref = 0;
  need_pref = 0;
  need_base = 0;
  mass = 1;
}


//= Top level program should call this after handling non-zero return.

void jhcNetAsm::Ack ()
{
  if (rc > 0) 
    Reset(); 
  else if (rc < 0)
    nag = miss;
}


///////////////////////////////////////////////////////////////////////////
//                            Status Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Text key for most important of current problems.

const char *jhcNetAsm::MajorIssue ()
{
  if ((*evt == '\0') && (*obj == '\0') && (*np == '\0'))
    strcpy_s(problem, "MISS_SENT");
  else if (*evt == '\0')
    strcpy_s(problem, "MISS_VERB");
  else if (need_obj > 0)
    strcpy_s(problem, "MISS_OBJ");
  else if (need_dest > 0)
    strcpy_s(problem, "MISS_DEST");
  else
    strcpy_s(problem, "SYN_OK");
  return problem;
}


//= Text gloss for action being requested.

const char *jhcNetAsm::Action ()
{
  strcpy_s(evt_txt, "do something to");
  if (*evt != '\0')
    mem->GetValue(evt, "tag", evt_txt, 0, 80);
  return evt_txt;
}


//= Text gloss for object being affected by action.

const char *jhcNetAsm::Object ()
{
  *obj_txt = '\0';
  if (*obj != '\0')
    np_base(obj_txt, obj, "object", 80);
  else if (*np != '\0')
    np_base(obj_txt, np, "object", 80);
  return obj_txt;
}


//= Text gloss for destination location of action.

const char *jhcNetAsm::Dest ()
{
  char val[80];
  int n;

  *dest_txt = '\0';
  if (*dest != '\0')
  {
    // get preposition
    mem->GetValue(dest, "tag", dest_txt, 0, 80);
    if (mem->GetValue(dest, "wrt", val, 0, 80) > 0)
    {
      // add head of noun phrase
      n = (int) strlen(dest_txt);
      dest_txt[n] = ' ';
      np_base(dest_txt + n + 1, val, "thing", 80 - (n + 1));
    }
  }
  return dest_txt;
}


//= Try to find NAME or AKO link for object.
// uses def to build a default name if nothing found

char *jhcNetAsm::np_base (char *base, const char *node, const char *def, int ssz) const
{
  char head[80];
  
  // set up default then check for named object
  sprintf_s(base, ssz, "the %s", def);  
  if (mem->GetValue(node, "tag", base, 0, ssz) > 0)
    return base;

  // objects with a base noun type (insert after "the ")
  if (mem->GetHeadKind(head, "ako", "is", node, 0, 80) > 0)
    mem->GetValue(head, "tag", base + 4, 0, ssz - 4);
  return base;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Interprets string of markers in association list from parser.
// use MajorErr to retrieve string associated with biggest syntax problem
// returns -1 for syntax problem, 0 for building, 1 for complete utterance
// NOTE: should call Reset on completion, Continue for syntax problem

int jhcNetAsm::Build (const char *tags, double sil)
{
  char frag[80] = "";
  const char *alist = tags;

  // check for triple store 
  if ((mem == NULL) || (tags == NULL))
    return -2;

/*
  // check for attention word anywhere in utterance
  if (ChkAttn(alist) > 0)
    attn = 1;
  if (attn <= 0)
  {
    if (*tags != '\0')
      jprintf("... ignoring input ...\n\n");
    return 0;
  }
*/

  // walk down list of fragments
  while ((alist = NextFrag(alist, frag, 80)) != NULL)
  {
    // handle pairs based on fragment type
    if (strcmp(frag, "!evt") == 0)
      build_evt(alist);
    else if (strcmp(frag, "!desc") == 0)
      build_np(alist);
    else if (strcmp(frag, "%end") == 0)
      build_dest(alist);
    else if (strcmp(frag, "%pos") == 0) 
      build_pos(alist);
    else if (strcmp(frag, "%part") == 0) 
      build_part(alist);
    connect_up();
  }

  // make sure the last element was linked (if possible)
  if (*tags != '\0')
    connect_up();
  return chk_complete(sil);
}


//= Possibly attach new component when complete.
// final specifies that end of utterance has been reached

void jhcNetAsm::connect_up ()
{
  finish_dest();
  finish_pos();
  finish_part();
  finish_evt();    // must come last
}


//= See if a well-formed actionable utterance has been completed.
// returns 1 if ready to execute, 0 if assembling, -1 if some new problem
// NOTE: resets return code to 0 after signaling status change once

int jhcNetAsm::chk_complete (double sil) 
{
  // see if even trying to assemble anything
  if (attn <= 0)
    return 0;

  // create internal status vector of missing components
  miss = 0;
  if ((*evt == '\0') && (*np == '\0'))
    miss |= 0x08;
  if (*evt == '\0')
    miss |= 0x04;
  if (need_obj > 0) 
    miss |= 0x02;
  if (need_dest > 0)
    miss |= 0x01;

  // signal success or new error (once only)
  rc = 0;
  if ((miss == 0) && (sil >= edit))
  {
    attn = 0;                          // stop listening
    rc = 1;
    jprintf(">>> utterance complete <<<\n");
  }
  else if ((miss > 0) && (miss != nag) && (sil >= turn))
  {
    rc = -1;
    jprintf("??? missing something ???\n");
  }

  // timeout waiting for clarification
  if ((miss > 0) && (sil >= flush))
  {
    // *** should also erase partial results from net? ***
    Reset();
    jprintf("\n___ giving up ___\n");
  }
  return rc;
}


///////////////////////////////////////////////////////////////////////////
//                              Tag Handling                             //
///////////////////////////////////////////////////////////////////////////

//= Build up noun phrase representation.

void jhcNetAsm::build_np (const char *tags)
{
  char val[80], prop[80], kind[80] = "";
  const char *alist;

  // see if should be linked to already mentioned item
  if ((*np == '\0') || AnySlot(tags, "REF PRON PRON&"))
  {
    // otherwise make new thing (default as a mass noun)
    mem->NewItem(np, "#", 80);
    mass = 1;
  }

  // see if really non-mass noun phrase (for measure phrases etc.)
  if (AnySlot(tags, "DEF INDEF ANY ALT SELF USER PRON PRON& POINT POINT&"))
    mass = 0;

  // save particular object names (generally unique)
  if (FindSlot(tags, "LABEL", val, 0, 80) != NULL)
    mem->AddValue(np, "tag", val);

  // save people's names (generally unique)
  if (FindSlot(tags, "NAME", val, 0, 80) != NULL)
  {
    // add name to thing
    mem->AddValue(np, "tag", val);

    // build kind definition
    mem->NewItem(kind, "ako", 80);
    mem->SetValue(kind, "tag", "person");
    mem->SetValue(kind, "base", "kind");
    
    // add kind to thing
    mem->SetValue(kind, "is", np);
  }

  // add base type predicate (generally unique)
  if (FindSlot(tags, "AKO", val, 0, 80) != NULL)
  {
    // build kind definition
    mem->NewItem(kind, "ako", 80);
    mem->SetValue(kind, "tag", val);
    mem->SetValue(kind, "base", "kind");

    // add kind to thing
    mem->SetValue(kind, "is", np);
  }

  // add adjective predicates (could be many)
  alist = tags;
  while ((alist = FindSlot(alist, "HQ", val, 1, 80)) != NULL)
  { 
    // build property 
    mem->NewItem(prop, "hq", 80);
    mem->SetValue(prop, "tag", val);

    // property may be relative (e.g. "red fox", "big mouse")
    if (*kind != '\0')
      mem->SetValue(prop, "wrt", kind);   

    // add property to base thing
    mem->SetValue(prop, "is", np);
  }
}


//= Build up destination phrase.

void jhcNetAsm::build_dest (const char *alist)
{
  char val[80];

  if (FindSlot(alist, "DEST", val, 0, 80) != NULL)
  {
    // make a location item and figure out predicate
    mem->NewItem(dest, "loc", 80);
    mem->SetValue(dest, "tag", val);

    // next noun phrase is reference for location prepositional phrase
    *np = '\0';
    need_dref = 1;
  }
}


//= Attach argument to destination phrase.

void jhcNetAsm::finish_dest ()
{
  if ((*dest == '\0') || (need_dref <= 0) || (*np == '\0'))
    return;
  mem->SetValue(dest, "wrt", np);
  need_dref = 0;
}


//= Build up location phrase.

void jhcNetAsm::build_pos (const char *alist)
{
  char val[80];

  if (FindSlot(alist, "LOC", val, 0, 80) != NULL)
  {
    // attach it to object (should already be mentioned)
    mem->NewItem(pp, "loc", 80);
    mem->SetValue(pp, "tag", val);
    if (*np != '\0')
      mem->AddValue(pp, "loc", np);
    else if (*evt != '\0')
      mem->AddValue(pp, "loc", evt);

    // next noun phrase is reference for location prepositional phrase
    *np = '\0';
    need_pref = 1;
  }
}


//= Attach argument to prepositional phrase and finish.

void jhcNetAsm::finish_pos ()
{
  if ((*pp == '\0') || (need_pref <= 0) || (*np == '\0'))
    return;

  // "part" relations have reversed link labels
  if (mem->NodeKind(pp, "sub") > 0)
    mem->SetValue(pp, "is", np);       // part
  else
    mem->SetValue(pp, "wrt", np);      // position

  // PP already attached so pointer can be cleared
  *pp = '\0';                      
  need_pref = 0;
}


//= Build up part-whole relations.

void jhcNetAsm::build_part (const char *alist)
{
  if (HasSlot(alist, "PIECE"))
  {
    // for "with" phrases the owner is previous NP
    mem->NewItem(pp, "sub", 80);
    mem->SetValue(pp, "base", "part");
    if (*np != '\0')
      mem->SetValue(pp, "wrt", np);

    // next NP will be the subpart 
    *np = '\0';
    need_pref = 1;
  }

  if (HasSlot(alist, "OWNER"))
  {
    // save subpart NP for "of" phrases, next NP will be the owner
    strcpy_s(np0, np);
    *np = '\0';
    need_base = 1;
  }
}


//= Handle attachments in two variants of "of" prepositional phrases.

void jhcNetAsm::finish_part ()
{
  char head[80], val[80];
  int i = 0;

  if ((need_base <= 0) || (*np == '\0') || (*np0 == '\0'))
    return;

  if (mass <= 0)
  {
    // non-mass NP further specifies base type ("top of the table")
    if (mem->GetHeadKind(head, "ako", "is", np0, 0, 80) > 0)
      mem->SetValue(head, "wrt", np);
  }      
  else
  {
    // mass NP swaps AKO and HQ links over to base ("bottle of fresh milk")
    while (mem->GetHead(head, "is", np, i++, 80) > 0)
      mem->SetValue(head, "is", np0);

    // mass NP copies tag to base ("bottle of red Tylenol")
    if (mem->GetValue(np, "tag", val, 0, 80) > 0)
      mem->AddValue(np0, "tag", val);
  }        

  // top level NP no longer needed
  *np0 = '\0';
  need_base = 0;
}


//= Build main event object.

void jhcNetAsm::build_evt (const char *alist)
{
  char val[80];

  // set defaults (transitive verb)
  need_obj = 1;
  need_dest = 0;

  // determine which arguments are required
  if (FindSlot(alist, "ACT-0", val, 0, 80) != NULL)
    need_obj = 0;
  else if (FindSlot(alist, "ACT-2", val, 0, 80) != NULL)
    need_dest = 1;
  else if (FindSlot(alist, "ACT-1", val, 0, 80) == NULL)
    return;

  // build node of correct base type and mark for execution
  mem->NewItem(evt, "@", 80);
  mem->SetValue(evt, "tag", val);
  mem->SetValue(evt, "status", "interest");
}


//= Attach arguments to verb phrase.

void jhcNetAsm::finish_evt ()
{
  if (*evt == '\0')
    return;

  // try to attach direct object (or replace default)
  if ((need_obj > 0) && (*np != '\0'))
  {
    mem->SetValue(evt, "obj", np);
    strcpy_s(obj, np);
    need_obj = 0;
  }

  // try to attach destination (or replace default)
  if ((need_dest > 0) && (*dest != '\0'))
  {
    mem->SetValue(evt, "dest", dest);
    need_dest = 0;
  }
}

