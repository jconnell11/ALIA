// jhcSlotVal.cpp : functions for manipulating association lists from parser
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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
#include <ctype.h>

#include "Parse/jhcSlotVal.h"


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Print a shortened "pretty" version of association list (no tabs).
// can automatically terminate at the end of the current fragment (skip >= 0)

void jhcSlotVal::PrintList (const char *alist, const char *tag, int skip, int cr) const
{
  char out[1000];
  const char *end;
  int i, n, n0 = 0, trim = ((skip > 0) ? 0 : 2);

  // copy list with substitutions for some characters
  if (alist != NULL)
    n0 = (int) strlen(alist);
  n = __min(n0, 499);
  for (i = 0; i < n; i++)  
    if (alist[i] == '\t')
      out[i] = ' ';
    else if (alist[i] == ' ')
      out[i] = '_';
    else
      out[i] = alist[i];
  out[i] = '\0';

  // figure out where to stop (out and alist same length)
  if (skip >= 0)
    if ((end = FragClose(alist, skip)) != NULL)
      out[end - alist - trim] = '\0';

  // print new string, possibly with a prefix
  if (tag != NULL)
    jprintf("%s ", tag);
  if (n > 0)
    jprintf("%s", ((*out == ' ') ? out + 1 : out)); 
  if (cr > 0)
    jprintf("\n");
}


//= Take a "pretty" version of an association list and convert to tab form.
// returns converted list

const char *jhcSlotVal::SetList (char *alist, const char *src, int ssz) const
{
  int i, n0 = (int) strlen(src), n = __min(n0, ssz - 2);

  if (n > 0)
    alist[0] = '\t';
  for (i = 0; i < n; i++)  
    if (src[i] == '_')
      alist[i + 1] = ' ';
    else if (src[i] == ' ')
      alist[i + 1] = '\t';
    else
      alist[i + 1] = src[i];
  alist[i + 1] = '\0';
  return alist;
}


//= Goes down list looking for any attentional marker.
// returns 1 if found, 0 if missing

int jhcSlotVal::ChkAttn (const char *alist) const
{
  if (FindSlot(alist, "ATTN", NULL, 0, 0) != NULL)
    return 1;
  return 0;
}


//= Alters given string by stripping prefixes like "r-" and removing internal dashes.
// returns pointer into original string (following chars might be changed)
// Example: "!r-foo-bar" --> "foo bar"

char *jhcSlotVal::CleanVal (char *dest) const
{
  char *d, *s = dest;

  // sanity check
  if (dest == NULL)
    return NULL;

  // strip fragment symbol and "r-" prefix
  if (strchr("!$%", *s) != NULL)
    s++;
  if ((s[0] != '\0') && (s[1] == '-'))
    s += 2;

  // remove any hyphens by changing characters
  d = s;
  while (*d != '\0')
  {
    if (*d == '-')
      *d = ' ';
    d++;
  }
  return s;
}


//= Forms new string by stripping prefixes like "r-" and removing internal dashes.
// Example: "!r-foo-bar" --> "foo bar"

char *jhcSlotVal::CleanVal (char *dest, const char *src) const
{
  char *d = dest;
  const char *s = src;           

  // sanity check
  if (dest == NULL)
    return NULL;

  if (src != NULL)
  {
    // strip fragment symbol and "r-" prefix
    if (strchr("!$%", *s) != NULL)
      s++;
    if ((s[0] != '\0') && (s[1] == '-'))
      s += 2;

    // copy rest but remove any hyphens
    while (*s != '\0')
    {
      *d++ = ((*s == '-') ? ' ' : *s);
      s++;
    }
  }

  // terminate copied string
  *d = '\0';
  return dest;
}


//= Advance to next entry of any type (slot-value pair or fragment) in alist.
// returns pointer to part of list after returned entry (NULL if none)

const char *jhcSlotVal::NextEntry (const char *alist, char *entry, int ssz) const
{
  const char *tail, *head = alist;

  // searches for tab separated entry (even first has a tab)
  if (head == NULL)
    return NULL;
  while (1)
  {
    if ((head = strchr(head, '\t')) == NULL)
      return NULL;
    head++;
    break;
  }

  // find end of entry, walking backwards over trailing whitespace
  if ((tail = strchr(head, '\t')) == NULL)
    tail = head + strlen(head);
  while (--tail > head)
    if (*tail != ' ')
      break; 
  tail++;

  // copy entry as a single string
  if (entry != NULL)
    strncpy_s(entry, ssz, head, tail - head);      // always adds terminator
  return tail;
}


//= Advance to next entry of any type in alist and compare with given tag.
// can restrict match to first n characters if n > 0
// returns pointer to part of list after returned entry (NULL if bad match)

const char *jhcSlotVal::NextMatches (const char *alist, const char *tag, int n) const
{
  char entry[200];
  const char *tail;
  int order;

  if ((tail = NextEntry(alist, entry)) == NULL)
    return NULL;
  if (n > 0)
    order = strncmp(entry, tag, n);
  else 
    order = strcmp(entry, tag);
  return((order == 0) ? tail : NULL);
}


//= Strip any leading slot-value pairs and return alist headed by some fragment (or marker).

const char *jhcSlotVal::StripPairs (const char *alist) const
{
  char entry[200];
  const char *t2, *tail = alist;

  while ((t2 = NextEntry(tail, entry, 200)) != NULL)
  {
    if (strchr("!$%", *entry) != NULL)
      return tail;
    tail = t2;
  }
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                              Slot Functions                           //
///////////////////////////////////////////////////////////////////////////

//= See if the current fragment has a tag of the single given type.

bool jhcSlotVal::HasSlot (const char *alist, const char *slot, int local) const
{
  if (FindSlot(alist, slot, NULL, local, 0) != NULL)
    return true;
  return false;
}


//= See if the current fragment has a tag of any of the given types.
// types separated by single spaces in probe list

bool jhcSlotVal::AnySlot (const char *alist, const char *marks, int local) const
{
  char slot[80];
  const char *tail, *head = marks;

  while (1)
  {
    // get next term in list
    tail = strchr(head, ' ');
    if (tail != NULL)
      strncpy_s(slot, head, tail - head);                // always adds terminator
    else
      strcpy_s(slot, head);

    // see if found in list of tags
    if (FindSlot(alist, slot, NULL, local, 0) != NULL)
      return true;
    if (tail == NULL)
      break;
    head = tail + 1;
  }
  return false;
}


//= Look for tag "slot" within the association list and bind its value.
// if local > 0 then only searches up until the next fragment marker
// does not change input "val" if slot is not found (for defaults)
// returns portion of list after entry for convenience, NULL if not found

const char *jhcSlotVal::FindSlot (const char *alist, const char *slot, char *val, int local, int ssz) const
{
  char s[80], v[200];
  const char *tail = alist;

  if ((slot == NULL) || (*slot == '\0'))
    return NULL;

  while ((tail = NextSlot(tail, s, v, local, 80, 200)) != NULL)
    if (_stricmp(s, slot) == 0)
    {
      if (val != NULL)
        strcpy_s(val, ssz, v);
      return tail;
    }
  return NULL;
}


//= Find the next slot value pair within the current fragment.
// if local > 0 then only searches up until the next fragment marker
// binds both the slot name and the associated value (unchanged if none)
// returns pointer to part of list after this pair (NULL if none)

const char *jhcSlotVal::NextSlot (const char *alist, char *slot, char *val, int local, int ssz, int vsz) const
{
  char entry[200];
  const char *sep, *tail = alist;

  // check input
  if ((alist == NULL) || (*alist == '\0'))
    return NULL;

  while (1)
  {
    // possibly stop if new fragment or run out of list
    if ((tail = NextEntry(tail, entry, 200)) == NULL)
      return NULL;
    if ((local > 0) && (strchr("!$%", *entry) != NULL))
      return NULL;

    // check format and find end of slot string 
    if ((sep = strchr(entry, '=')) != NULL)
      break;
  }

  // copy out slot name
  if (slot != NULL)
    strncpy_s(slot, ssz, entry, sep - entry);      // always adds terminator

  // copy value string
  if (val != NULL)
    strcpy_s(val, vsz, sep + 1);
  return tail;
}


//= See if slot-value pair has exactly the given slot.

bool jhcSlotVal::SlotMatch (const char *pair, const char *slot) const
{
  int n = SlotStart(pair, slot);

  return((n > 0) && (pair[n] == '='));
}


//= See if slot-value pair begins with the given prefix (if any).
// returns length of prefix if matched, negative otherwise

int jhcSlotVal::SlotStart (const char *pair, const char *prefix) const
{
  int n;

  if (prefix == NULL)
    return 0;
  n = (int) strlen(prefix);
  if (strncmp(pair, prefix, n) != 0)
    return -1;
  return n;
}


//= Simple parsing of slot-value pair to return MODIFIABLE value part.
// return pointer allows "pair" itself to be altered (!)

char *jhcSlotVal::SlotRef (char *pair) const
{
  char *val;

  if ((val = strchr(pair, '=')) == NULL)
    return NULL;
  return(val + 1);
}


//= Simple parsing of slot-value pair to return value part.

const char *jhcSlotVal::SlotVal (const char *pair) const
{
  const char *val;

  if ((val = strchr(pair, '=')) == NULL)
    return NULL;
  return(val + 1);
}


//= Extract the value from pair if its slot name begins with the given prefix (if any).
// returns following value pointer into pair string (NULL if prefix does not match)

const char *jhcSlotVal::SlotGet (char *pair, const char *prefix, int lower) const
{
  char *sep;
  int n;

  if ((n = SlotStart(pair, prefix)) < 0)
    return NULL;
  if ((sep = strchr(pair + n, '=')) == NULL)
    return NULL;
  if (lower > 0)
    all_lower(sep + 1);
  return(sep + 1);
}


//= Extract the slot name (possibly lowercased) from a pair and return its value.

const char *jhcSlotVal::SplitPair (char *slot, const char *pair, int lower, int ssz) const
{
  const char *sep;

  // sanity check
  if ((slot == NULL) || (ssz <= 0))
    return NULL;
  *slot = '\0';
  if (pair == NULL)
    return NULL;

  // copy just slot part and return pointer to tail of pair
  if ((sep = strchr(pair, '=')) == NULL)
    return NULL;
  strncpy_s(slot, ssz, pair, sep - pair);        // always terminates
  if (lower > 0)
    all_lower(slot);
  return(sep + 1);
} 


//= Convert a word (in place) to all lowercase characters.
// returns length of word for convenience

int jhcSlotVal::all_lower (char *txt) const
{
  int i, n = (int) strlen(txt);

  for (i = 0; i < n; i++)
    txt[i] = (char) tolower(txt[i]);
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                           Fragment Functions                          //
///////////////////////////////////////////////////////////////////////////

//= See if the association list has a fragment of the single given kind.
// returns 1 if present, 0 if missing

bool jhcSlotVal::HasFrag (const char *alist, const char *frag) const
{
  if (FindFrag(alist, frag) != NULL)
    return true;
  return false;
}


//= See if the association list has a fragment of any of the given kinds.
// kinds separated by single spaces in probe list 
// returns 1 if present, 0 if missing

bool jhcSlotVal::AnyFrag (const char *alist, const char *kinds) const
{
  char frag[200];
  const char *tail, *head = kinds;

  while (1)
  {
    // get next term in list
    tail = strchr(head, ' ');
    if (tail != NULL)
      strncpy_s(frag, head, tail - head);        // always adds terminator
    else
      strcpy_s(frag, head);

    // see if found in list of tags
    if (FindFrag(alist, frag) != NULL)
      return true;
    if (tail == NULL)
      break;
    head = tail + 1;
  }
  return false;
}


//= Look through association list to find fragment of given type.
// returns portion of list after entry for convenience, NULL if not found

const char *jhcSlotVal::FindFrag (const char *alist, const char *frag) const
{
  char kind[200];
  const char *tail = alist;

  while ((tail = NextFrag(tail, kind, 200)) != NULL)
    if (_stricmp(kind, frag) == 0)
      return tail;
  return NULL;
}


//= Advance to next fragment and bind type.
// returns pointer to part of list after fragment marker

const char *jhcSlotVal::NextFrag (const char *alist, char *frag, int ssz) const
{
  char entry[200];
  const char *tail = alist;

  while ((tail = NextEntry(tail, entry, 200)) != NULL)
    if (strchr("!$%", *entry) != NULL)
    {
      if (frag != NULL)
        strcpy_s(frag, ssz, entry);
      return tail;
    }
  return NULL;
}


//= Advance to next fragment within current fragment and bind type.
// returns pointer to part of list after COMPLETE fragment found

const char *jhcSlotVal::FragNextFrag (const char *alist, char *frag, int ssz) const
{
  char entry[200];
  const char *tail = alist;

  while ((tail = NextEntry(tail, entry, 200)) != NULL)
    if (strchr("!$%", *entry) != NULL)
    {
      if (entry[1] == '\0')            // end of main fragment encountered
        return NULL;
      if (frag != NULL)
        strcpy_s(frag, ssz, entry);
      return FragClose(tail, 0);       // just past end of embedded fragment
    }
  return NULL;
}


//= Find and copy out next slot-value pair within this same fragment.
// stays within current fragment, skipping over intervening embedded fragments
// returns pointer to remaining alist (NULL if not found)

const char *jhcSlotVal::FragNextPair (const char *alist, char *pair, int ssz) const
{
  const char *tail = alist;
  int depth = 0;

  // keep track of fragment nesting depth
  while ((tail = NextEntry(tail, pair, ssz)) != NULL)
    if (strchr("!$%", *pair) != NULL)          
    {
      depth += ((pair[1] == '\0') ? -1 : 1);   // embedded fragment  
      if (depth < 0)
        return NULL;
    }
    else if ((depth == 0) && (strchr(pair, '=') != NULL))
      return tail;
  return NULL;
}


//= See if fragment has given slot as part of top level structure.
// copies filler for slot into "val" string (if given)
// stays within current fragment, skipping over intervening embedded fragments
// returns remainder of alist if successful (NULL if not found)

const char *jhcSlotVal::FragFindSlot (const char *alist, const char *slot, char *val, int ssz) const
{
  char pair[200];
  char *sep;
  const char *tail = alist;

  while ((tail = FragNextPair(tail, pair)) != NULL)
  {
    sep = strchr(pair, '=');
    *sep = '\0';
    if (strcmp(pair, slot) == 0)
    {
      if (val != NULL)
        strcpy_s(val, ssz, sep + 1);
      return tail;
    }
  }
  return NULL;
}



//= Advance to start of next fragment but do not consume any of it.

const char *jhcSlotVal::FragStart (const char *alist) const
{
  char entry[200];
  const char *t2, *tail = alist;

  while ((t2 = NextEntry(tail, entry)) != NULL)
  {
    if (strchr("!$%", *entry) != NULL)
      return tail;
    tail = t2;
  }
  return NULL;
}


//= Look for end of current fragment after possibly skipping fragment head.
// returns pointer to remaining alist (NULL if not found)

const char *jhcSlotVal::FragClose (const char *alist, int skip) const
{
  char entry[200];
  const char *tail = alist;
  int depth = ((skip > 0) ? -1 : 0);

  while ((tail = NextEntry(tail, entry)) != NULL)
    if (strchr("!$%", *entry) == NULL)
      continue;
    else if (entry[1] != '\0')         // embedded fragment
      depth++;
    else if (depth == 0)               // matched ending
      return tail;
    else                               // end of embedded
      depth--;
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                            Fragment Parsing                           //
///////////////////////////////////////////////////////////////////////////

//= Copy just next fragment into a new string (more copying but clearer).
// will copy fragment marker to "head" if non-NULL (always part of fragment)
// returns remainder of alist, else NULL if failed to get a fragment

const char *jhcSlotVal::ExtractFrag (char *head, char *frag, const char *alist, int hsz, int fsz) const
{
  char entry[200] = "";
  const char *tail, *start = alist;
  int n;

  // set up defaults
  if (frag != NULL)
    *frag = '\0';
  if (head != NULL)
    *head = '\0';

  // find start of fragment
  while ((tail = NextEntry(start, entry, 200)) != NULL)
  {
    if (strchr("!$%", *entry) != NULL)
      break;
    start = tail;
  }
  if ((start == NULL) || (*entry == '\0'))
    return NULL;

  // copy everything from start up to and including matching delimiter
  if ((tail = FragClose(tail, 0)) != NULL)
    n = (int)(tail - start);
  else
    n = (int) strlen(start);
  strncpy_s(frag, fsz, start, n);

  // possibly copy out fragment marker separately
  if (head != NULL)
    strcpy_s(head, hsz, entry);
  return tail;
}


//= Divide next fragment into two strings (more copying but clearer).
// "head" is fragment marker and "body" is rest of fragment (minus final delimiter)
// returns remainder of alist, else NULL if failed to get fragment
// <pre>
// Examples (spaces are really TABs - use SetList/PrintList):
//
//   alist  = "%Rule $cond !note %obj-i0 % $add HQ=orange $ ! $ $res !note %obj REF=it % $add %obj-i AKO=tiger % $ ! $ %"
//   head   = "%Rule"
//   body   = "$cond !note %obj-i0 % $add HQ=orange $ ! $ $res !note %obj REF=it % $add %obj-i AKO=tiger % $ ! $"
//   return = ""
//
//   alist  = "$cond !note %obj-i0 % $add HQ=orange $ ! $ $res !note %obj REF=it % $add %obj-i AKO=tiger % $ ! $"
//   head   = "$cond"
//   body   = "!note %obj-i0 % $add HQ=orange $ !"
//   return = "$res !note %obj REF=it % $add %obj-i AKO=tiger % $ ! $"
//
//   alist  = "!note %obj-i0 % $add HQ=orange $ !"
//   head   = "!note"
//   body   = "%obj-i0 % $add HQ=orange $"
//   return = ""
//
// </pre> 

const char *jhcSlotVal::SplitFrag (char *head, char *body, const char *alist, int hsz, int bsz) const
{
  const char *rest, *end;
  int n;
  
  // set defaults and find fragment type (if any)
  if (head != NULL) 
    *head = '\0';
  if (body != NULL)
    *body = '\0';
  if ((rest = NextFrag(alist, head, hsz)) == NULL)
    return NULL;

  // copy rest into body up to matching delimiter
  if ((end = FragClose(alist, 1)) != NULL)
    n = (int)(end - rest);
  else
    n = (int) strlen(rest);
  if (body != NULL)
    strncpy_s(body, bsz, rest, n - 2);       // 2 = tab + delimiter
  return FragClose(rest, 0);
}


//= Check if next fragment starts with given head then copy out rest of fragment.
// if prefix > 0 then head must start with match string 
// returns remainder of alist, else NULL if failed to get suitable fragment

const char *jhcSlotVal::ExtractBody (const char *match, char *body, const char *alist, int bsz, int prefix) const
{
  char hd[80];
  const char *rest, *end;
  int comp, n;
  
  // set defaults and find fragment type (if any)
  if (body != NULL)
    *body = '\0';
  if ((rest = NextFrag(alist, hd)) == NULL)
    return NULL;

  // make sure fragment is of correct type
  if (prefix > 0)
    comp = strncmp(hd, match, strlen(match));
  else
    comp = strcmp(hd, match);
  if (comp != 0)
    return NULL;

  // copy rest into body up to matching delimiter
  if ((end = FragClose(alist, 1)) != NULL)
    n = (int)(end - rest);
  else
    n = (int) strlen(rest);
  strncpy_s(body, bsz, rest, n - 2);       // 2 = tab + delimiter
  return FragClose(rest, 0);
}
