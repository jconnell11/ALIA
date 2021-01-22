// jhcTxtAssoc.cpp : key and values in a singly linked association list
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2020 IBM Corporation
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
#include <string.h>
#include <ctype.h>

#include "Interface/jrand.h"         // common video

#include "Semantic/jhcTxtAssoc.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTxtAssoc::jhcTxtAssoc ()
{
  *key = '\0';
  klen = 0;
  next = NULL;
  vals = NULL;
  last = 0;
  prob = 1.0;
}


//= Default destructor does necessary cleanup.
// Note: gets rid of all values AND rest of list
// to delete a single key make sure "next" pointer is NULL

jhcTxtAssoc::~jhcTxtAssoc ()
{
  ClrKeys();
}


//= Seeds the random number generator (again).
// should call this once at program startup to prevent same pattern
// function LoadList calls this for convenience in normal programs
// NOTE: every new thread gets its own version that should be seeded

void jhcTxtAssoc::Seed ()
{
  jrand_seed();
}


///////////////////////////////////////////////////////////////////////////
//                     Properties and Retrieval                          //
///////////////////////////////////////////////////////////////////////////

//= Tell total number of keys in list (all are distinct).

int jhcTxtAssoc::NumKeys () const
{
  const jhcTxtAssoc *k = this;
  int n = 0;

  while (k != NULL)
  {
    n++;
    k = k->next;
  }
  return n;
}


//= Returns the maximum number of values under any key.
// can optionally include first "blank" key (normally skipped)
// useful to determine if mapping from key to value is unique

int jhcTxtAssoc::MaxDepth (int blank) const
{
  const jhcTxtAssoc *k = ((blank > 0) ? this : next);
  int cnt, n = 0;

  while (k != NULL)
  {
    cnt = k->NumVals();
    n = __max(n, cnt);
    k = k->next;
  }
  return n;
}


//= Get sum of number of values under all keys in list.

int jhcTxtAssoc::TotalVals () const
{
  const jhcTxtAssoc *k = this;
  int n = 0;

  while (k != NULL)
  {
    n += k->NumVals();
    k = k->next;
  }
  return n;
}


//= Returns the maximum string length for any value in list.

int jhcTxtAssoc::MaxLength () const
{
  const jhcTxtAssoc *k = this;
  const jhcTxtList *v;
  int len, n = 0;

  while (k != NULL)
  {
    v = k->vals;
    while (v != NULL)
    {
      len = (int) strlen(v->ValTxt());
      if (len > n)
        n = len;
      v = v->NextVal();
    }
    k = k->NextKey();
  }
  return n;
}


//= Get name of particular numbered key from the association list.
// useful for enumeration with NumKeys()
// return NULL if no such index (starts at 0)

const char *jhcTxtAssoc::KeyTxtN (int n) const
{
  const jhcTxtAssoc *k = this;
  int i;

  if (n < 0)
    return NULL;
  for (i = 0; i < n; i++, k = k->next)
    if (k == NULL)
      return NULL;
  return k->key;
}


//= Find the first key associated with some value.
// useful as a run-time partial inversion

const char *jhcTxtAssoc::KeyTxtFor (const char *vtxt) const
{
  const jhcTxtAssoc *k = this;
  const jhcTxtList *v;

  // scan across all keys sequentially
  while (k != NULL)
  {
    // scan down list of values for this key
    v = k->vals;
    while (v != NULL)
    {
      // check if matches search string
      if (strcmp(v->ValTxt(), vtxt) == 0)
        return k->KeyTxt();
      v = v->NextVal();
    }

    // continue down key list
    k = k->next;
  }
  return NULL;
}


//= See if given text string is a member of the tag's association list.
// can optionally compare to tag itself, or use case-sensitive matching
// returns 1 if txt is found among tag's elements, 0 otherwise

bool jhcTxtAssoc::Member (const char *ktag, const char *vtxt, int def, int caps) const
{
  const jhcTxtAssoc *k;

  // see if category matching allowed
  if (def > 0)
    if (((caps <= 0) && (_stricmp(ktag, vtxt) == 0)) || 
        ((caps > 0)  && (  strcmp(ktag, vtxt) == 0)))
      return true;

  // otherwise look up list of expansions for key
  if ((k = ReadKey(ktag)) == NULL)
    return false;
  if (k->read_val(vtxt, caps) != NULL)
    return true;
  return false;
}


//= Builds new list where values link to lists of keys.
// set "clean" if all values are known to be distinct
// weights are copied over for each key-value pair
// probability for each new key (old value) copied from old key
// returns the new number of unique "keys" (former values)
// NOTE: erases all previous structure attached to this node

int jhcTxtAssoc::Invert (const jhcTxtAssoc &src, int clean)
{
  jhcTxtAssoc *inv;
  const jhcTxtAssoc *k = &src;
  const jhcTxtList *v;

  // clear this node
  ClrKeys();

  // go through all old keys
  while (k != NULL)
  {
    // go through all values for old key
    v = k->vals;
    while (v != NULL)
    {
      // add corresponding new inverted pair
      if ((inv = AddKey(v->ValTxt(), k->prob, clean)) != NULL)
        inv->AddVal(k->key, v->ValWt());
      v = v->NextVal();
    }

    // continue down list of keys
    k = k->next;
  }
  return NumKeys();
}


///////////////////////////////////////////////////////////////////////////
//                           Random Selection                            //
///////////////////////////////////////////////////////////////////////////

//= Clear the "last" marker for all keys in the list.
// this and "mark" fields on values control non-return inhibition

void jhcTxtAssoc::Reset ()
{
  jhcTxtAssoc *k = this;

  while (k != NULL)
  {
    k->last = 0;
    k = k->next;
  }
}


//= Pick randomly among the values under a particular key.
// convenient for spicing up robot interaction by avoiding repetition
// if no values at all returns NULL, else if def > 0 returns key string
// prevents same value being returned if same key called twice in a row 
// NOTE: call Seed() or LoadList() somewhere else same sequence every time

const char *jhcTxtAssoc::PickRnd (const char *ktag, int def)
{
  jhcTxtAssoc *k;
  jhcTxtList *v;
  int n;

  // make sure key exists
  if ((k = GetKey(ktag)) == NULL)
    return NULL;

  // re-enable all expansions if not selected last time
  if (k->last <= 0)
  {
    Reset();
    k->last = 1;
//    k->enabled(1);             // force rotation thru values
  }

  // if no untried choices then re-enable them all
  n = k->enabled(0);
  if (n == 0)                    // not needed if done below
    n = k->enabled(1);
  if (n == 0)
    return((def > 0) ? k->key : NULL);   

  // if only one choice then select it, else pick any
  if (n == 1)
  {
    v = k->first_choice();
    k->enabled(1);               // prevent repeat on reset
  }
  else
    v = k->rand_choice();
  v->mark = 1;                   // comment out to allow repeats
  return v->ValTxt();
}


//= Count how many values have clear non-return "mark" fields.
// can optionally force all to be valid (unmarked)

int jhcTxtAssoc::enabled (int force)
{
  jhcTxtList *v = vals;
  int n = 0;

  while (v != NULL)
  {
    if (force > 0)
      v->mark = 0;
    if (v->mark <= 0)
      n++;
    v = v->GetNext();
  }
  return n;
}


//= Return the first (and only) value with clear non-return "mark".

jhcTxtList *jhcTxtAssoc::first_choice () 
{
  jhcTxtList *v = vals;

  while (v != NULL)
  {
    if (v->mark <= 0)
      return v;
    v = v->GetNext();
  }
  return NULL;               
}


//= Choose among enabled values with probability proportional to weight.
// only considers previously untried responses

jhcTxtList *jhcTxtAssoc::rand_choice ()
{
  jhcTxtList *v = vals;
  double th, sum = 0.0, acc = 0.0;

  // sanity check
  if (v == NULL)
    return NULL;

  // generate threshold as some fraction of total weight sum
  while (v != NULL)
  {
    if (v->mark <= 0)            
      sum += v->ValWt();
    v = v->GetNext();
  }
  th = sum * jrand();

  // skip entries until enough weight accumulated 
  v = vals;
  while (v->NextVal() != NULL)
  {
    if (v->mark <= 0)            
      acc += v->ValWt();
    if (acc >= th)
      break;
    v = v->GetNext();
  }
  return v;
}


///////////////////////////////////////////////////////////////////////////
//                            Keys and Values                            //
///////////////////////////////////////////////////////////////////////////

//= Locate an existing entry in the association list that has the given unique key.
// returns NULL if no such entry

const jhcTxtAssoc *jhcTxtAssoc::ReadKey (const char *ktag) const
{
  const jhcTxtAssoc *k = this;

  while (k != NULL)
    if (strcmp(k->key, ((ktag != NULL) ? ktag : "")) == 0)
      return k;
    else
      k = k->next;
  return NULL;
}


//= Locate a modifiable entry in the association list for the given unique key.
// returns NULL if no such entry

jhcTxtAssoc *jhcTxtAssoc::GetKey (const char *ktag)
{
  jhcTxtAssoc *k = this;

  while (k != NULL)
    if (strcmp(k->key, ((ktag != NULL) ? ktag : "")) == 0)
      return k;
    else
      k = k->next;
  return NULL;
}


//= Tells number of distinct values under current key.

int jhcTxtAssoc::NumVals () const
{
  const jhcTxtList *v = vals;
  int n = 0;

  while (v != NULL)
  {
    n++;
    v = v->NextVal();
  }
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                        Building and Editing                           //
///////////////////////////////////////////////////////////////////////////

//= Add a new key to tail of association list (can only be one copy).
// changes probability to given value if key already exists
// can force addition if tag is known to be unique (saves time)
// returns pointer to key, NULL if failure

jhcTxtAssoc *jhcTxtAssoc::AddKey (const char *ktag, double p, int force)
{
  jhcTxtAssoc *k, *tail = this;

  // possibly see if key is already present
  if (force <= 0)
    if ((k = GetKey(ktag)) != NULL)
    {
      k->prob = p;
      return k;
    }

  // generate a new key with proper tag (next default to NULL)
  if ((k = new jhcTxtAssoc) == NULL)
    return NULL;
  strcpy_s(k->key, ((ktag != NULL) ? ktag : ""));
  k->klen = (int) strlen(k->key);
  k->prob = p;

  // link it after the last key in current list
  while (tail->next != NULL)
    tail = tail->next;
  tail->next = k;
  return k;
}


//= Remove a key and its values from the association list.
// returns 1 if removed, 0 if not found

int jhcTxtAssoc::RemKey (const char *ktag)
{
  jhcTxtAssoc *p = NULL, *k = this;

  // find key and its predecessor in list
  while (k != NULL)
  {
    if (strcmp(k->key, ((ktag != NULL) ? ktag : "")) == 0)
      break;
    p = k;
    k = k->next;
  }
  if (k == NULL)
    return 0;

  // splice out of list if somewhere in middle
  if (p != NULL)
  {
    p->next = k->next;
    k->next = NULL;
    delete k;
    return 1;
  }

  // else get rid of own values and clear key string
  ClrVals();
  *key = '\0';
  klen = 0;
  if (next == NULL)
    return 1;

  // transfer second element properties to head  
  strcpy_s(key, next->key);
  klen = next->klen;
  vals = next->vals;
  next = next->next;

  // sacrifice second element instead
  next->vals = NULL;
  next->next = NULL;
  delete next;
  return 1;
}


//= Get rid of all keys in list.
// current key is reset to the blank value

void jhcTxtAssoc::ClrKeys ()
{
  *key = '\0';
  klen = 0;
  ClrVals();
  if (next != NULL)
    delete next;
  next = NULL;
}


//= Add a new value under current key and assign it some weight.
// if value already exists then forces weight to be value given
// returns pointer to value, NULL for error

const jhcTxtList *jhcTxtAssoc::AddVal (const char *vtxt, double w)
{
  jhcTxtList *v;

  if ((v = get_val(NULL, vtxt)) != NULL)
  {
    v->SetWt(w);
    return v;
  }
  return insert_val(vtxt, w);
}


//= Change the weight of some value, add if does not exist, remove if now zero.
// returns 2 if added, 1 if altered, 0 if removed, -1 for error

int jhcTxtAssoc::IncVal (const char *vtxt, double amt)
{
  jhcTxtList *v, *p;

  // look for specified entry
  if ((v = get_val(&p, vtxt)) == NULL)
  {
    // create with given weight
    if (insert_val(vtxt, amt) == NULL)
      return -1;
    return 2;
  }

  // alter existing weight by given amount
  v->IncWt(amt);
  if (v->ValWt() != 0.0)
    return 1;
  drop_val(p, v);   // remove if wt = 0
  return 0;
}


//= Remove a specific value from a current key's list.
// returns 1 if removed, 0 if value not found

int jhcTxtAssoc::RemVal (const char *vtxt)
{
  jhcTxtList *v, *p;

  if ((v = get_val(&p, vtxt)) == NULL)
    return 0;
  drop_val(p, v);
  return 1;
}


//= Remove ALL values associated with current key.
// always returns 1 for convenience

int jhcTxtAssoc::ClrVals ()
{
  jhcTxtList *p, *v = vals;

  while (v != NULL)
  {
    p = v;
    v = p->GetNext();
    delete p;
  }
  vals = NULL;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                      Low-Level Value Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Get a pointer to some specific value in list.
// returns NULL if bad index

jhcTxtList *jhcTxtAssoc::nth_val (int n) const
{
  jhcTxtList *v = vals;
  int i;

  if (n < 0)
    return NULL;
  for (i = 0; i < n; i++, v = v->GetNext())
    if (v == NULL)
      return NULL;
  return v;
}


//= Find read-only pointer to value node with matching text tag.
// returns NULL if nothing matches

const jhcTxtList *jhcTxtAssoc::read_val (const char *vtxt, int caps) const
{
  const jhcTxtList *v = vals;

  // check if valid search criterion
  if ((vtxt == NULL) || (*vtxt == '\0'))
    return NULL;
  
  // find matching node
  while (v != NULL)
  {
    if (((caps <= 0) && (_stricmp(v->ValTxt(), vtxt) == 0)) || 
        ((caps > 0)  && (strcmp(  v->ValTxt(), vtxt) == 0)))
      return v;
    v = v->NextVal();
  }
  return NULL;      // node not found
}


//= Find value node with matching text tag as well as previous node in list.
// returns NULL if nothing matches

jhcTxtList *jhcTxtAssoc::get_val (jhcTxtList **p, const char *vtxt)
{
  jhcTxtList *v = vals;

  // check if valid search criterion
  if (p != NULL)
    *p = NULL;
  if ((vtxt == NULL) || (*vtxt == '\0'))
    return NULL;
  
  // find matching node
  while (v != NULL)
  {
    if (strcmp(v->ValTxt(), vtxt) == 0)
      return v;
    if (p != NULL)
      *p = v;
    v = v->GetNext();
  }
  return NULL;      // node not found
}


//= Add a new value to front of list.
// returns pointer to element if successful, NULL if failed

jhcTxtList *jhcTxtAssoc::insert_val (const char *vtxt, double w)
{
  jhcTxtList *v;

  if ((vtxt == NULL) || (*vtxt == '\0'))
    return NULL;

  // make new value with correct string and weight
  if ((v = new jhcTxtList) == NULL)
    return NULL;
  v->SetTxt(vtxt);
  v->SetWt(w);

  // splice onto head of list
  v->SetNext(vals);
  vals = v;
  return v;
}


//= Splice out element from list given previous node (if any).

void jhcTxtAssoc::drop_val (jhcTxtList *p, jhcTxtList *v)
{
  if (p != NULL)
    p->SetNext(v->GetNext());
  else
    vals = v->GetNext();
  delete v;
}


///////////////////////////////////////////////////////////////////////////
//                            File Operations                            //
///////////////////////////////////////////////////////////////////////////

//= Read a list of keys and values from a text file.
// can optionally merge items from file with existing list
// set "clean" if known to have only one paragraph for each key
// ignores blank lines and lines starting with "//"
// "=" marks a category, if no initial marking then first category is ""
// patterns and expansions can contain variables like "?1" and "?2"
// also re-seeds random number generator (convenient place)
// returns number of categories if successful, 0 or negative for some error
// NOTE: if first element is a number then assumes it is a weight

int jhcTxtAssoc::LoadList (const char *fname, int clean, int merge)
{
  jhcTxtAssoc *k = this;
  char *start, *end, *tag, *txt;
  FILE *in;
  char line[100] = "=";     // for files with no categories
  double w, p;
  int cnt = 0, saved = 1;

  // try opening file
  Seed();
  if (fname == NULL)
    return -1;
  if (fopen_s(&in, fname, "r") != 0)
    return 0;

  // possibly purge old list
  if (merge <= 0)
    ClrKeys();

  // read lines looking for a category
  while ((saved > 0) || (fgets(line, 100, in) != NULL))
    if (strncmp(line, "#include", 8) == 0)
    {
     // load another file first
      if ((start = strchr(line, '\"')) != NULL)
        if ((end = strchr(start + 1, '\"')) != NULL)
        {
          *end = '\0';
          LoadList(start + 1, clean, 1);
          saved = 0;
        }
    }
    else if (*line == '=')
    {
      // if probability specified then skip to space afterward
      p = 1.0;
      tag = trim_wh(line + 1);
      if (isdigit(*tag) || (*tag == '-'))   // negative
        if (sscanf_s(line, "%lf", &p) == 1)
          tag = strpbrk(tag, " \t");

      // find or make key corresponding to line
      if ((k = AddKey(tag, p, ((*tag == '\0') ? 0 : clean))) == NULL)
        break;
      cnt++;

      // add values on following lines (except if commented out)
      while (fgets(line, 100, in) != NULL)
        if (strncmp(line, "//", 2) != 0)
        {
          // stop if next category found (and re-parse this line)
          saved = 1;
          if (strchr("=#", *line) != NULL)        // also looks for includes
            break;
          saved = 0;

          // if weight specified then skip to space afterward
          w = 1.0;
          txt = line;
          if (isdigit(*line) || (*line == '-'))   // negative
            if (sscanf_s(line, "%lf", &w) == 1)
              txt = strpbrk(line, " \t");

          // add under current key
          txt = trim_wh(txt);                
          k->insert_val(txt, w);
        }              
    }  

  // cleanup
  fclose(in);
  return cnt;
}


//= Get just core string without whitespace.
// returns pointer to valid section

char *jhcTxtAssoc::trim_wh (char *src) const
{
  char *end, *start = src;

  // trim leading whitespace (and maybe numbers)
  while (*start != '\0')
  {
    if (strchr(" \t", *start) == NULL)
      break;
    start++;
  }
 
  // trim whitespace from end
  end = start + (int) strlen(start) - 1;
  while (end >= start)
  {
    if (strchr(" \t\n", *end) == NULL)
      break;
    *end = '\0'; 
    end--;
  }
  return start;
}


//= Save all keys and values to a text file.
// general format is:
// <pre>
//     val1               <-- vals for default category
//     val2
//   = 0.003 key1         <-- key with prior probability
//     0.120 val3
//     0.573 val4         <-- value with non-integer weight
//     0.296 val5
//   = key4               <-- key with probability = 1
//     val6
//   5 val7               <-- value with weight = 5
//     val8
//     val9               <-- value with weight = 1
// <pre>
// returns 1 if successful, 0 or negative for some error

int jhcTxtAssoc::SaveList (const char *fname) const
{
  const jhcTxtAssoc *k = this;
  const jhcTxtList *v;
  FILE *out;

  // try opening file
  if (fname == NULL)
    return -1;
  if (fopen_s(&out, fname, "w") != 0)
    return 0;

  // go through list of all keys (including blank)
  while (k != NULL)
  {
    // write header (except for first category, always blank)
    if (k->prob != 1.0)
      fprintf(out, "\n= %g %s\n", k->prob, k->key);
    else if (*(k->key) != '\0')
      fprintf(out, "\n= %s\n", k->key);

    // list all distinct values 
    v = k->vals;
    while (v != NULL)
    {
      // possibly print weight first
      if (v->ValWt() == 1.0)
        fprintf(out, "  %s\n", v->ValTxt());
      else if (ROUND(v->ValWt()) == v->ValWt())
        fprintf(out, "%d %s\n", ROUND(v->ValWt()), v->ValTxt());
      else
        fprintf(out, "%g %s\n", v->ValWt(), v->ValTxt());

      // continue down value list
      v = v->NextVal();
    }

    // continue down key list
    k = k->next;
  }

  // cleanup
  fclose(out);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Variable Substitution                        //
///////////////////////////////////////////////////////////////////////////

//= Generate temporary string using randomly selected variant with given arguments.
// common calling sequence for speech synthesis
// returns pointer to filled in string for convenience (NULL for error)

char *jhcTxtAssoc::MsgRnd (const char *ktag, const char *arg0,
                            const char *arg1, const char *arg2, const char *arg3, 
                            const char *arg4, const char *arg5, const char *arg6,
                            const char *arg7, const char *arg8, const char *arg9) 
{
  return Compose(tmp, PickRnd(ktag, 1), arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}


//= Fill in given string using randomly selected variant with given arguments.
// common calling sequence for speech synthesis, must be passed a scratch string
// returns pointer to filled in string for convenience (NULL for error)

char *jhcTxtAssoc::FillRnd (char *full, const char *ktag, const char *arg0,
                            const char *arg1, const char *arg2, const char *arg3, 
                            const char *arg4, const char *arg5, const char *arg6,
                            const char *arg7, const char *arg8, const char *arg9) 
{
  return Compose(full, PickRnd(ktag, 1), arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}


//= Susbtitute string arguments for numbered variables in a pattern.
// assumes full string is long enough to accomodate result
// makes robot dialog more lively, also useful for changing languages
// Examples: 
//   Compose("I see ?0 ?1 things", "two", "red") --> "I see two red things"
//   Compose("I see ?0 of them",   "two", "red") --> "I see two of them"
//   Compose("I see some ?1 ones", "two", "red") --> "I see some red ones"
// if no text supplied for some argument, assumed to be blank string
// returns pointer to filled in string for convenience (NULL for error)
// NOTE: arguments start with "?0" NOT "?1"

char *jhcTxtAssoc::Compose (char *full, const char *pattern, const char *arg0,
                            const char *arg1, const char *arg2, const char *arg3, 
                            const char *arg4, const char *arg5, const char *arg6,
                            const char *arg7, const char *arg8, const char *arg9) const
{
  const char *args[10] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};
  const char *item, *in = pattern;
  char *out = full;
  int i, j, n = 0;

  // check for reasonable output and base string
  *full = '\0';
  if ((full == NULL) || (pattern == NULL))
    return full;

  // find possible substitution points
  while (*in != '\0')
  {
    // check for variable marker (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, or ?9)
    i = (int)(in[1] - '0');
    if ((*in != '?') || (i < 0) || (i > 9))
    {
      *out++ = *in++;
      continue;
    }

    // strip variable symbol and bind argument
    in += 2;
    item = args[i];
    if (item == NULL)
      continue;

    // copy argument to output 
    j = (int) strlen(item);
    if (j > 0)
      while (j-- > 0)
        *out++ = *item++;
    else if (out > full)
      out--;                // blank argument erases leading space
    n++;
  }

  // clean up
  *out = '\0';
  return full;
}



