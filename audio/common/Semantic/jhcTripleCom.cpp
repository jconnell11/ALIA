// jhcTripleCom.cpp : sends and receives semantic triples over a socket
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

#include <stdio.h>

#include "Interface/jhcMessage.h"
#include "Interface/jms_x.h"

#include "Semantic/jhcTripleCom.h"


// parts of transmission protocol

#define J3_SEP   '\t'         /** Symbol between fields of a triple.    **/

#define J3_END   '\n'         /** Symbol at the end of a triple.        **/

#define J3_DONE  "over"       /** String marks complete set of triples. **/ 


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTripleCom::jhcTripleCom ()
{
  *host = '\0';
  Defaults();
  Reset();
}


//= Reset state for the beginning of a sequence.

void jhcTripleCom::Reset ()
{
  *b = '\0';
  fill = 0;
}


//= Establish connection with a specific host.
// saves name if no other valid default

int jhcTripleCom::ForgeOut (const char *name_url)
{
  int ans;

  if (*host == '\0')
    strcpy_s(host, name_url);
  jprintf("Attempting socket connection to %s port %d ... ", name_url, port);
  ans = Connect(name_url, port, 1);
  if (ans > 0)
    jprintf("succeeded\n");
  else
    jprintf("FAILED !\n");
  return ans;
}


//= Establish connection with default host.
// uses either stored host name (from config file) or URL digits

int jhcTripleCom::ForgeOut (int use_name)
{
  char url[80];

  if (use_name > 0)
    return ForgeOut(host);
  sprintf_s(url, "%d.%d.%d.%d", add1, add2, add3, add4);
  return ForgeOut(url);
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcTripleCom::Defaults (const char *fname)
{
  int ok = 1;

  ok &= link_params(fname);
  ok &= lps.LoadText(host, fname, "triple_host", NULL, 80);    // named host
  return ok;
}


//= Write current processing variable values to a file.

int jhcTripleCom::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= lps.SaveVals(fname);
  ok &= lps.SaveText(fname, "triple_host", host);
  return ok;
}


//= Parameters used for specifying socket connection.

int jhcTripleCom::link_params (const char *fname)
{
  jhcParam *ps = &lps;
  int ok;

  ps->SetTag("triple_link", 0);
  ps->NextSpec4( &add1,     9, "URL field 1");  
  ps->NextSpec4( &add2,   116, "URL field 1");  
  ps->NextSpec4( &add3,    57, "URL field 1");  
  ps->NextSpec4( &add4,   219, "URL field 1");  
  ps->Skip();
  ps->NextSpec4( &port, 52779, "Port number");

  ps->NextSpec4( &echo,     1, "Echo to console");
  ps->NextSpec4( &cmode,    0, "Connect (none, url, name)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Message Passing                            //
///////////////////////////////////////////////////////////////////////////

//= Send standard three part message. 
// can optionally print transmitted message to console if "echo" is positive
// returns positive for okay, 0 or negative for error (connection broken)

int jhcTripleCom::SendTriple (const char *obj, const char *slot, const char *val)
{
  char msg[200];

  if ((obj == NULL)  || (slot == NULL)  || (val == NULL) || 
      (*obj == '\0') || (*slot == '\0') || (*val == '\0'))
    return -1;

  // possible announce what is being sent
  if (echo > 0)
    jprintf("  tx: %s --%s--> %s\n", obj, slot, val);

  // form and send message
  sprintf_s(msg, "%s%c%s%c%s%c", obj, J3_SEP, slot, J3_SEP, val, J3_END);
  return Transmit((UC8 *) msg, (int) strlen(msg));
}


//= Transmit a special message saying that information is complete for now.
// can also transmit next gensym value (e.g obj-7 --> 8) for convenience
// can optionally print transmitted message to console if "echo" is positive
// returns positive for okay, 0 or negative for error 

int jhcTripleCom::SendOver (int gnum)
{
  char msg[40];
  int i = __max(2, gnum);

  if (echo > 0)
    jprintf("  tx: *over* @ %d\n", i);
  sprintf_s(msg, "%s%c@%c%d%c", J3_DONE, J3_SEP, J3_SEP, i, J3_END);
  return Transmit((UC8 *) msg, (int) strlen(msg));
}


//= Checks whether there are any complete incoming messages ready.
// returns 1 for some message, 0 for none, negative for error

int jhcTripleCom::AnyTriples ()
{
  UC8 ch;
  int ans;

  // see if a complete packet is already in the buffer
  if ((fill > 0) && (b[fill - 1] == J3_END))
    return 1;

  while (1)
  {
    // check for a new character
    if ((ans = Any()) <= 0)
      return ans;
    if (Rx8(&ch, 0.0) <= 0)
      return -1;

    // save characeter and check if packet done
    b[fill++] = ch;
    b[fill] = '\0';
    if (ch == J3_END)
      return 1;
  }
  return 0;
}


//= Parse next message received (if any) into component parts.
// can optionally print received message to console if "echo" is positive
// returns next gnum if "over" message, 1 for valid message, 0 if nothing, negative for error

int jhcTripleCom::GetTriple (char *obj, char *slot, char *val, int osz, int ssz, int vsz)
{
  char marks[3] = {J3_SEP, J3_END, '\0'};
  char *token, *rest = b;
  char **src = &rest;
  int ok, ans = 1;

  // check for completed packet 
  if ((ok = AnyTriples()) <= 0)
    return ok;
  ok = 0;

  while (1)
  {
    // get object
    if ((token = next_token(src, marks)) == NULL)
      break; 
    if (obj != NULL)
      strcpy_s(obj, osz, token);
    if (strcmp(token, J3_DONE) == 0)         // see if special "over" symbole
      ans = 2;

    // get slot 
    if ((token = next_token(src, marks)) == NULL)
      break;
    if (slot != NULL)
      strcpy_s(slot, ssz, token);

    // get value (possibly next gnum)
    if ((token = next_token(src, marks)) == NULL)
      break;
    if (val != NULL)
      strcpy_s(val, vsz, token);
    if (ans >= 2)                           // extract next gensym number
      if (sscanf_s(token, "%d", &ans) != 1)
        break;
   
    // successful parse
    ok = 1;
    break;
  }

  // invalid current packet (consumed now)
  fill = 0;
  b[0] = '\0';
  if (ok <= 0)
    return 0;                               // discard bad packets

  // announce what was received if so desired
  if (echo > 0)
  {
    if (ans >= 2)
      jprintf("    rx: *over* @ %d\n", ans);
    else
      jprintf("    rx: %s --%s--> %s\n", obj, slot, val);
  }
  return ans;
}


//= Safer form of strtok function that allows call nesting.
// alters input to point to remainder of original string (NULL when done)
// returns pointer to found token (original string is altered with '\0')

char *jhcTripleCom::next_token (char **src, const char *delim)
{
  char *start, *end;

  if ((src == NULL) || (*src == NULL))
    return NULL;
    
  // skip over delimiters to start of token
  start = *src;
  while ((*start != '\0') && (strchr(delim, *start) != NULL))
    start++;
  if (*start == '\0')
  {
    *src = NULL;       // no more tokens
    return NULL;
  }

  // pass through valid characters to first delimiter
  end = start + 1;
  while ((*end != '\0') && (strchr(delim, *end) == NULL))
    end++;
  if (*end == '\0')
  {
    *src = NULL;       // no more tokens
    return start;
  }

  // terminate token and point at where to look next
  *end = '\0';
  *src = end + 1;
  return start;
}


///////////////////////////////////////////////////////////////////////////
//                         Communication Functions                       //
///////////////////////////////////////////////////////////////////////////

//= Send any new triples over socket and get any responses.
// attempts to connect to default server if socket not already open
// returns number of triples received, negative for error

int jhcTripleCom::Sync ()
{
  char id[40], fcn[40], val[200];
  const jhcTripleNode *n, *n2;
  int rc, cnt = 0;

  // send all new facts then mark end of pod 
  while (update != NULL)
  {
    n = update->Head();
    n2 = update->Fill();
    if (n2 != NULL)
      SendTriple(n->Name(), update->Fcn(), n2->Name());
    update = update->next;
  }
  SendOver(gnum);

  // get all response up until end of pod
  reply = NULL;
  while (1)
  {
    // get next valid triple from host (or wait a while)
    rc = GetTriple(id, fcn, val, 40, 40, 200);
    if (rc < 0)
      return -1;
    if (rc == 0)
    {
      jms_sleep(1);
      continue;
    }
  
    // build new triple unless pod is finished 
    if (rc > 1)
      break;
    if (BuildTriple(id, fcn, val) <= 0)
      return -1;
    cnt++;

    // possibly initialize reply pointer
    if (reply == NULL)
      reply = facts;
  }

  // update gensym generator and reset focus
  gnum = __max(gnum, rc);
  focus = reply;
  update = NULL;                 // host already knows all its own facts
  return cnt;
}


//= Gets the next triple in the order received from the host.
// use RewindReply to get to beginning of most recent pod
// returns 1 if successful, 0 if no more triples

int jhcTripleCom::NextReply (char *id, char *fcn, char *val, int isz, int fsz, int vsz)
{
  const jhcTripleNode *n, *n2;

  // check if at end of list
  if ((focus == NULL) || (focus == update))
    return 0;

  // extract parts of current fact
  n = focus->Head();
  n2 = focus->Fill();
  strcpy_s(id, isz, n->Name());
  strcpy_s(fcn, fsz, focus->Fcn());
  if (n2 == NULL)
    return 0;
  strcpy_s(val, vsz, n2->Name());

  // advance focus pointer
  focus = focus->next;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Print out all triples just received from remote host.

void jhcTripleCom::PrintReply ()
{
  char id[40], fcn[40], val[200];

  // write header
  jprintf("======================\n");
  jprintf("Pod received from host:\n");

  // write out all triples
  RewindReply();
  while (NextReply(id, fcn, val, 40, 40, 200) > 0)
    jprintf("  %s --%s--> %s\n", id, fcn, val);
  jprintf("\n");
}
