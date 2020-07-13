// jhcTxtTree.cpp : nodes of a tree containing short strings
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013 IBM Corporation
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

#include <stdarg.h>
#include <ctype.h>

#include "Parse/jhcTxtTree.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTxtTree::jhcTxtTree (const char *tag)
{
  *txt = '\0';
  next = NULL;
  prev = NULL;
  child = NULL;
  parent = NULL;
  SetText(tag);
}


//= Default destructor does necessary cleanup.
// deletes all following elements and children

jhcTxtTree::~jhcTxtTree ()
{
  Truncate();
  ClrSub();
}


///////////////////////////////////////////////////////////////////////////
//                           Content Management                          //
///////////////////////////////////////////////////////////////////////////

//= Use formatting print to load name for this node.

void jhcTxtTree::BuildText (const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt); 
  vsprintf(txt, fmt, args);
}


//= How many elements are in current list.

int jhcTxtTree::Length () const
{
  const jhcTxtTree *n = this;
  int len = 1;

  while (n->next != NULL)
  {
    n = n->next;
    len++;
  }
  return len;
}


//= Number of nodes including this one up to and including last.

int jhcTxtTree::Span (const jhcTxtTree *last) const
{
  const jhcTxtTree *n = this;
  int len = 1;

  while (n->next != NULL)
  {
    if (n == last) 
      break;
    n = n->next;
    len++;
  }
  return len;
}


//= Compares probe string to text of current node.
// can optionally require capitalization to match
// returns 1 if same, 0 if different

bool jhcTxtTree::Match (const char *probe, int caps) const
{
  if (((caps <= 0) && (_stricmp(probe, txt) == 0)) ||
      ((caps > 0)  && (strcmp(probe, txt) == 0)))
    return true;
  return false;
}


//= Creates sentence-like rendition of expansion at current tree level.
// if compact > 0 then removes space by punctuation
// to retrieve full text of all sentences use NextWord instead

char *jhcTxtTree::Linear (char *dest, int compact) const
{
  const jhcTxtTree *t0 = NULL, *t = child;

  // check and initialize output
  if (dest == NULL)
    return NULL;
  *dest = '\0';

  // walk down list accumulating words
  while (t != NULL)
  {
    if (!sp_veto(t0, t, compact))
      strcat(dest, " ");
    strcat(dest, t->txt);
    t0 = t;
    t = t->next;
  }
  strcat(dest, " ");
  return dest;
}


//= Write sentence-like rendition of current level to an already open file.
// if compact > 0 then removes space by punctuation

void jhcTxtTree::Linear (FILE *out, int compact) const
{
  const jhcTxtTree *t0 = NULL, *t = child;

  // make sure file exists
  if (out == NULL)
    return;

  // walk down list accumulating words
  while (t != NULL)
  {
    if (!sp_veto(t0, t, compact))
      fwrite(" ", 1, 1, out);
    fwrite(t->txt, 1, (int) strlen(t->txt), out);
    t0 = t;
    t = t->next;
  }
  fwrite(" ", 1, 1, out);
}


//= Creates phrase-like rendition from current node through end node.
// if compact > 0 then removes space by punctuation

char *jhcTxtTree::Fragment (char *dest, const jhcTxtTree *last, int compact) const
{
  const jhcTxtTree *t0 = NULL, *t = this;

  // check and initialize output
  if (dest == NULL)
    return NULL;
  *dest = '\0';

  // walk down list accumulating words
  while (t != NULL)
  {
    if (!sp_veto(t0, t, compact))
      strcat(dest, " ");
    strcat(dest, t->txt);
    if (t == last)
      break;
    t0 = t;
    t = t->next;
  }
  return dest;
}


//= Veto adding space before and after certain punctuation marks.

bool jhcTxtTree::sp_veto (const jhcTxtTree *t0, const jhcTxtTree *t, int compact) const
{
  // no space if first item in list
  if (t0 == NULL)
    return true;
  if (t == NULL)
    return true;
  if (compact <= 0)
    return false;

  // no space before these elements
  if (strchr(",;:.!?}])%", *(t->txt)) != NULL)
    return true;

  // no space after these elements
  if (strchr("{[(", *(t0->txt)) != NULL)
    return true;
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                            Basic Navigation                           //
///////////////////////////////////////////////////////////////////////////

//= Follow previous links to start node of current list.
// returns pointer to target node (could be this one)

jhcTxtTree *jhcTxtTree::First () 
{
  jhcTxtTree *n = this;

  while (n->prev != NULL)
    n = n->prev;
  return n;
}


//= Follow next links to end node of current list.
// returns pointer to target node (could be this one)

jhcTxtTree *jhcTxtTree::Last () 
{
  jhcTxtTree *n = this;

  while (n->next != NULL)
    n = n->next;
  return n;
}


//= Go all the way to the bottom-most leaf from this node.
// returns pointer to target node (could be this one)

jhcTxtTree *jhcTxtTree::Bottom ()
{
  jhcTxtTree *n = this;

  while (n->child != NULL)
    n = n->child;
  return n;
}


//= Follow previous links to start node of current list.
// returns const pointer to target node (could be this one)

const jhcTxtTree *jhcTxtTree::First () const
{
  const jhcTxtTree *n = this;

  while (n->prev != NULL)
    n = n->prev;
  return n;
}


//= Follow next links to end node of current list.
// returns const pointer to target node (could be this one)

const jhcTxtTree *jhcTxtTree::Last () const
{
  const jhcTxtTree *n = this;

  while (n->next != NULL)
    n = n->next;
  return n;
}


//= Go all the way to the bottom-most leaf from this node.
// returns const pointer to target node (could be this one)

const jhcTxtTree *jhcTxtTree::Bottom () const
{
  const jhcTxtTree *n = this;

  while (n->child != NULL)
    n = n->child;
  return n;
}


//= Go to start of current list then up one level.
// returns pointer to target node (possibly NULL)

jhcTxtTree *jhcTxtTree::Pop () const
{
  const jhcTxtTree *n = this;

  while (n->prev != NULL)    
    n = n->prev;
  return n->parent;
}


///////////////////////////////////////////////////////////////////////////
//                      Enumeration and Searching                        //
///////////////////////////////////////////////////////////////////////////

//= Emit words in linear order from the leaf nodes of all structures.
// input is the current leaf, goes through docs, paras, sents, etc.
// returns next leaf, NULL if no more leaves

const jhcTxtTree *jhcTxtTree::NextWord (int mv) const
{
  const jhcTxtTree *n = Bottom();

  // try going all the way down or across
  if ((mv <= 0) || (n != this))
    return n;
  if (next != NULL)
    return next->NextWord(0);

  // otherwise back up to front then go up one level
  while ((n = n->Pop()) != NULL)
    if (n->next != NULL)
    {
      // see if more to enumerate at this level
      n = n->next;
      return n->NextWord(0);
    }
  return NULL;
}


//= Emit sentence in linear order from the leaf nodes of all structures.
// input is the current sentence, goes through docs, paras, etc.
// returns next sublist, NULL if no more sublists

const jhcTxtTree *jhcTxtTree::NextSentence (int mv) const
{
  const jhcTxtTree *n = Bottom();

  // try going all the way down or across
  n = n->parent;
  if ((mv <= 0) || (n != this))
    return n;
  if (next != NULL)
    return next->NextSentence(0);

  // otherwise back up to front then go up one level
  while ((n = n->Pop()) != NULL)
    if (n->next != NULL)
    {
      // see if more to enumerate at this level
      n = n->next;
      return n->NextSentence(0);
    }
  return NULL;
}


//= Find part of expansion of current node which matches probe.
// can optionally be sensitive to string capitalization
// essentially searches within a given sentence for a literal string
// returns NULL if not found, else pointer to first occurrence

jhcTxtTree *jhcTxtTree::FindWord (const char *probe, int caps)
{
  jhcTxtTree *t = child;

  while (t != NULL)
  {
    if (((caps <= 0) && (_stricmp(probe, t->txt) == 0)) ||
        ((caps > 0)  && (strcmp(probe, t->txt) == 0)))
      return t;
    t = t->next;    
  }
  return NULL;
}


//= Find first leaf node that matches the given string pattern.
// can take a whole document node instead of just a sentence
// the pattern can contain several words and have wildcards at ends
// a fixed portion that is all capitals requires an all capitals match
// returns start of match (can continue using this), NULL if none
// NOTE: will not match multi-words across sentence boundaries

const jhcTxtTree *jhcTxtTree::FindPattern (const char *pattern) const
{
  jhcTxtSrc src;
  jhcTxtTree pat;
  const jhcTxtTree *p, *w, *p0, *w0 = this;

  // break pattern into words
  if ((pattern == NULL) || (*pattern == '\0'))
    return NULL;
  pat.SetText("pattern");
  src.Bind(pattern);
  pat.FillSent(src);
  p0 = pat.child;

  // advance through all words in document
  while ((w0 = w0->NextWord()) != NULL)
    if (w0->Satisfies(p0->txt))
    {
      // initial word matches so examine following ones
      w = w0->next;
      p = p0->next;
      while (1)
      {
        // success if end of pattern else check if next word okay
        if (p == NULL)   
          return w0;
        if ((w == NULL) || !w->Satisfies(p->txt))
          break;
        w = w->next;
        p = p->next;
      }
    }
  return NULL;
}


//= See if text for current word node matches given pattern.

bool jhcTxtTree::Satisfies (const char *pat) const
{
  const char *p = pat, *w = txt, *end = w;
  int i, sz, m, n = (int) strlen(p), caps = 1;
 
  // simplest cases
  if (n == 0)
    return false;
  if ((n == 1) && (p[0] == '*'))
    return true;

  // at most 2 wildcards (i.e. *US* needs at least 2 char word)
  m = (int) strlen(w);
  if (m < (n - 2))
    return false;
  sz = __max(m, n);

  // see if pattern is all capitals
  for (i = 0; i < n; i++)
    if ((pat[i] >= 'a') && (pat[i] <= 'z'))    // islower bad for weird chars
    {
      caps = 0;
      break;
    }

  // check if variable front (e.g. *US or *US*)
  if (pat[0] == '*')
  {

    // trim off first pattern character and set core match length
    p++;
    sz = n - 1;

    // check if variable back also 
    if (pat[n - 1] == '*')   
      sz--;                   // decrease match length (e.g. *US*)
    else
      w += m - sz;            // align ends of word and pattern (e.g. *US) 

    // find last place in word that core pattern might match
    end += m - sz;
  }
  else if (pat[n - 1] == '*')   // if variable back (e.g. US*) set match length 
    sz = n - 1;

  // see if pattern matches at various offsets in word
  if (m >= sz)
    while (1)
    {
      if (((caps <= 0) && (_strnicmp(w, p, sz) == 0)) ||
          ((caps >  0) && (  strncmp(w, p, sz) == 0)))
        return true;
      if (w >= end)
        break;
      w++;
    }
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                               Construction                            //
///////////////////////////////////////////////////////////////////////////

//= Follow next links to end and insert new node there.
// returns pointer to new element if successful, NULL for failure

jhcTxtTree *jhcTxtTree::Append (const char *fmt, ...)
{
  va_list args;
  jhcTxtTree *a, *n = this; 

  // make sure new node can be created
  if ((a = new jhcTxtTree) == NULL)
    return NULL;

  // set string for new node
  va_start(args, fmt); 
  if (fmt != NULL)
    vsprintf(a->txt, fmt, args);        // vsprintf fails on "%"

  // attach to end of current list
  while (n->next != NULL)
    n = n->next;
  n->next = a;
  a->prev = n;
  return a;
}


//= Follow next links to end and insert new node there with literal text.
// returns pointer to new element if successful, NULL for failure

jhcTxtTree *jhcTxtTree::TackOn (const char *txt)
{
  jhcTxtTree *a, *n = this; 

  // make sure new node can be created
  if ((a = new jhcTxtTree) == NULL)
    return NULL;

  // copy as much of label as will fit 
  if ((int) strlen(txt) < JTXT_MAX)
    strcpy(a->txt, txt);               
  else
  {
    strncpy(a->txt, txt, JTXT_MAX - 1);
    (a->txt)[JTXT_MAX - 1] = '\0';
  }

  // attach to end of current list
  while (n->next != NULL)
    n = n->next;
  n->next = a;
  a->prev = n;
  return a;
}


//= Add a new expansion sublist to this node.
// returns pointer to new element, NULL if already exists or failure
// Note: avoid vsprintf here since handles single "%" poorly

jhcTxtTree *jhcTxtTree::AddSub (const char *txt)
{
  jhcTxtTree *a;

  // make sure no child already and that new node can be created
  if (child != NULL)
    return NULL;
  if ((a = new jhcTxtTree) == NULL)
    return NULL;
  strcpy(a->txt, txt);

  // connect into tree
  child = a;
  a->parent = this;
  return a;
}


//= Remove all following elements in list and their children.
// returns 1 if successful, 0 if already at end

int jhcTxtTree::Truncate ()
{
  if (next == NULL)
    return 0;
  delete next;
  next = NULL;
  return 1;
}


//= Remove all elements in sublist and their children.
// returns 1 if successful, 0 if no child

int jhcTxtTree::ClrSub ()
{
  if (child == NULL)
    return 0;
  delete child;
  child = NULL;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Hierarchy Generation                       //
///////////////////////////////////////////////////////////////////////////

//= Load in multiple paragraphs from string as a list of lists of lists.
// clears former contents unless "ndoc" > 1
// returns total number of paragraphs read

int jhcTxtTree::FillDoc (jhcTxtSrc& src, int ndoc)
{
  jhcTxtTree *p, *d = this;
  int i = 1;

  // mark self as a new blank document
  if (ndoc > 1)
    d = Append("doc-%d", ndoc);
  else
  {
    Truncate();
    ClrSub();
    SetText("doc-1");
  }

  // start a new sublist and add paragraphs from source
  p = d->AddSub("para-1");
  while (p->FillPara(src) > 0)
    p = p->Append("para-%d", ++i);

  // clean up by removing any empty paragraph at end
  if (p->child == NULL)
    if (p->prev != NULL)
    {
      (p->prev)->Truncate();
      i--;
    }
  return i;
}


//= Expands node as a sequence of sentences from string or file.
// paragraph ends with a blank line or end of input
// either string pointer or file pointer has valid next read state at end
// returns 1 if end of paragraph, 0 for end of input

int jhcTxtTree::FillPara (jhcTxtSrc& src)
{
  jhcTxtTree *t;
  int rc, i = 1;

  // start a new sublist and add sentences from source
  ClrSub();
  t = AddSub("sent-1");
  while ((rc = t->FillSent(src)) > 0)
    t = t->Append("sent-%d", ++i);

  // clean up
  if (t->child == NULL)
    if (t->prev != NULL)
      (t->prev)->Truncate();
  return(rc + 1);
}


//= Expands node as a sequence of words from string or file.
// sentence ends with punctation or end of input
// either string pointer or file pointer has valid next read state at end
// returns 1 if end of sentence, 0 if blank lines, -1 for end of input

int jhcTxtTree::FillSent (jhcTxtSrc& src)
{
  jhcTxtTree *w = NULL;
  char token[JTXT_MAX];
  int rc;

  ClrSub();
  while ((rc = src.ReadWord(token)) > 0)
  {
    // add child if first word in sentence
    if (w == NULL)
      w = AddSub(token);              
    else
      w = w->TackOn(token);

    // stop if end of sentence punctuation
    if (rc < 2)
      break;
  }
  return rc;
}


///////////////////////////////////////////////////////////////////////////
//                               Debugging                               //
///////////////////////////////////////////////////////////////////////////

//= Print structure to console, indenting for each level.

void jhcTxtTree::Print (int indent) const
{
  int i;

  // print this node (printf handles "%" poorly)
  for (i = indent; i > 0; i--)
    jprintf("  ");
  jprintf("%s\n", txt);

  // print all elements of sublist (if any) then rest of nodes
  if (child != NULL)
    child->Print(indent + 1);
  if (next != NULL)
    next->Print(indent);
}


//= Print structure to file, indenting for each level.
// returns 1 if okay, 0 for bad file

int jhcTxtTree::Save (const char *fname) const
{
  FILE *out;

  if ((out = fopen(fname, "w")) == NULL)
    return 0;
  save_n(out, 0);
  fclose(out);
  return 1;
}


//= Helper function saves out current node to already opened file.

void jhcTxtTree::save_n (FILE *out, int indent) const
{
  int i;

  // print this node (fprintf handles "%" poorly)
  for (i = indent; i > 0; i--)
    fprintf(out, "  ");
  fprintf(out, "%s\n", txt);

  // print all elements of sublist (if any) then rest of nodes
  if (child != NULL)
    child->save_n(out, indent + 1);
  if (next != NULL)
    next->save_n(out, indent);
}


