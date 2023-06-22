
// jhcVocab.cpp : collection of known words and various input fixes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2022 Etaoin Systems
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

#include "Parse/jhcGramStep.h"

#include "Parse/jhcVocab.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcVocab::~jhcVocab ()
{
  char **list;
  int i, j, n;

  for (i = 0; i < nbins; i++)
  {
    list = wlen[i];
    n = wmax[i];
    for (j = 0; j < n; j++)
      delete [] list[j];
    delete [] list;
  }
}


//= Default constructor initializes certain values.
// sets up initial list of words sorted by length so each has n entries.

jhcVocab::jhcVocab (int nb)
{
  char **list;
  int i, j, n;

  for (i = 0; i < nbins; i++)
  {
    // choose smaller bins for very short and very long words
    if (i == 0)
      n = 10;
    else if ((i <= 2) || (i >= 9))
      n = 50;
    else
      n = nb;

    // allocate array of strings
    list = new char * [n];
    for (j = 0; j < n; j++)
      list[j] = new char [nchar];
    wlen[i] = list;
    wmax[i] = n;
    nwds[i] = 0;
  }

  // no temporary strings yet
  *clean = '\0';
  *mark = '\0';
  *oov = '\0';

  // no messages
  dbg = 0;
//dbg = 2;
}


///////////////////////////////////////////////////////////////////////////
//                                Word List                              //
///////////////////////////////////////////////////////////////////////////

//= Forget about all previously harvested words.

void jhcVocab::Clear ()
{
  int i;

  for (i = 0; i < nbins; i++)
    nwds[i] = 0;
}


//= Find all words that are known to the parser.
// takes list of grammer rules from jhcGramExec (most complete source)
// returns initial size of the vocabulary

int jhcVocab::GetWords (const jhcGramRule *gram)
{
  const jhcGramRule *r = gram;
  const jhcGramStep *t;
  int cnt = 0;

  // scan all grammer rules
  Clear();
  while (r != NULL)
  {
    // look for terminals in expansion
    t = r->tail;
    while (t != NULL)
    {
      if (t->non <= 0)
        cnt += Add(t->symbol);
      t = t->tail;                     // next part of expansion
    }
    r = r->next;                       // next grammar rule
  }
  return cnt;
}


//= Add a new word to the list of known things.
// does not do anything if word already known
// returns 1 if added, 0 if already known

int jhcVocab::Add (const char *word)
{
  char **list;
  int i0, i, n;

  // sanity check then get bin index
  if ((word == NULL) || (*word == '\0') || (strcmp(word, "#") == 0))
    return 0;
  if (known(word))
    return 0;
  i0 = (int) strlen(word);
  i = __min(i0, nbins) - 1;

  // possibly make space for more words then add at end
  n = nwds[i];
  if (n >= wmax[i])
    extend(i, 50);           // make 50 more slots
  list = wlen[i];
  strcpy_s(list[n], nchar, word);
  nwds[i] = n + 1;  
  return 1;
}


//= Just check to see if word is known.
// allows registered text string and valid numeric forms

bool jhcVocab::known (const char *word) const
{
  char extra[nchar];
  double val;

  if (sscanf_s(word, "%lf%s", &val, &extra, nchar) == 1) 
    return true;
  return(lookup(word) != NULL);
}


//= Lookup word in list to get standardized version.
// returns canonical form if known, NULL if unknown

const char *jhcVocab::lookup (const char *word) const
{
  char **list;
  int i, j, n;

  if ((i = bin(word)) < 0)
    return NULL;
  list = wlen[i];
  n = nwds[i];
  for (j = 0; j < n; j++)
    if (_stricmp(word, list[j]) == 0)
      return list[j];
  return NULL;
}


//= Tells what bin to use based on length of word.

int jhcVocab::bin (const char *word) const
{
  int n = (int) strlen(word);

  return(__min(n, nbins) - 1);
}


//= Make list for a certain length of word longer by "add" more entries.
// resets wmax value and copies old array values to newly allocated one

void jhcVocab::extend (int i, int add)
{
  char **list2, **list = wlen[i];
  int j, len = wmax[i], len2 = len + add, n = nwds[i];

  // create new longer array
  list2 = new char * [len2];
  for (j = 0; j < len2; j++)
    list2[j] = new char [nchar];
  wmax[i] = len2;

  // copy any valid old words then swap arrays
  for (j = 0; j < n; j++)
    strcpy_s(list2[j], nchar, list[j]);
  wlen[i] = list2;

  // get rid of old list
  for (j = 0; j < len; j++)
    delete [] list[j];
  delete [] list;
}


//= Remove a word from the list of known things.
// only removes the most recent copy, does not shrink array
// returns 1 if found and removed, 0 if unknown

int jhcVocab::Remove (const char *word)
{
  char **list;
  int j, i = bin(word), n = nwds[i] - 1;

  // look for position of word in its particular list
  list = wlen[i];
  for (j = 0; j <= n; j++)
    if (_stricmp(word, list[j]) == 0)
      break;
  if (j > n)
    return 0;                // never found

  // move all later words forward
  nwds[i] = n;
  while (j < n)
  {
    strcpy_s(list[j], nchar, list[j + 1]);
    j++;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Typing Correction                           //
///////////////////////////////////////////////////////////////////////////

//= Try to make all words known by fiddling with small edit distance variations.
// prefers swapping characters to inserting or altering them (never deletes)
// returns pointer to fixed up string, NULL if no fixed performed

const char *jhcVocab::FixTypos (const char *txt)
{
  char word[nchar], prev[nchar] = "", next[nchar] = "";
  const char *s = txt;
  char *d = clean;
  int fixed = 0;

  // go through text input looking for word candidates
  *clean = '\0';
  while ((s != NULL) && (*s != '\0'))
  {
    // copy whitespace to output then get word into local string
    s = copy_gap(clean, s);
    if ((s == NULL) || (*s == '\0'))
      break;
    s = next_word(word, s);
    d = clean + strlen(clean);

    // try a variety of fixes if word is unknown
    if (!known(word))
    {
      fixed++;
      if (try_fadd(d, prev, word) <= 0)
        if (try_frem(d, prev, word) <= 0)
          if (try_badd(word, next, &s) <= 0)
            if (try_brem(word, next, &s) <= 0)
              if (try_split(word) <= 0)
                if (try_swap(word) <= 0)
                  if (try_ins(word) <= 0)
                    fixed--;
    }

    // add revised word (and possibly next one) to output
    strcat_s(clean, word);
    if (next[0] != '\0')
    {
      strcat_s(clean, " ");
      strcat_s(clean, next);
    }

    // set up for finding more words in text
    d = clean + strlen(clean);
    strcpy_s(prev, word);
    next[0] = '\0';
  }
  return((fixed > 0) ? clean : NULL);            // also accessible via Fixed()
}


//= See if can swap preceeding space with last character of previous word.
// example: "ee" unknown in "... is ee" -> "... i see" with head borrow (cheap)
// sets word = "see" and alters before from "... is " to just "... i "
// returns 1 if successful and alters destination string, 0 if not possible 

int jhcVocab::try_fadd (char *d, const char *prev, char *word) const
{
  char w2[nchar];
  char *back = d - 2;        // valid if some previous word
  int n;

  // see if there is a character that can be borrowed
  if ((prev[0] == '\0') || (back[1] != ' ') || !word_part(back[0]))
    return 0;

  // see if previous word can loan last character
  strcpy_s(w2, prev);
  n = (int) strlen(prev);
  w2[n - 1] = '\0';
  if (!known(w2))
    return 0;

  // see if extra character helps unknown word
  w2[0] = back[0];
  strcpy_s(w2 + 1, nchar - 1, word);  
  if (!known(w2))
    return 0;

  // remove borrowed character from destination and keep augmented word 
  back[0] = ' ';
  back[1] = '\0';            // d gets recomputed in FixTypos
  strcpy_s(word, nchar, w2);
  return 1;    
}


//= See if first letter of unknown word can be re-homed with previous word.
// example: unknown "nthe" in "... i nthe" -> "... in the" with front trim (cheap)
// sets word = "the" and alters before from "... i " to "... in "
// returns 1 if successful and alters destination string, 0 if not possible 

int jhcVocab::try_frem (char *d, const char *prev, char *word) const
{
  char w2[nchar];
  char *back = d - 2;        // valid if some previous word
  int n;

  // see if there is a place character can be ejected to
  if ((prev[0] == '\0') || (back[1] != ' ') || !word_part(back[0]))
    return 0;

  // see if end of previous word can absorb first character
  strcpy_s(w2, prev);
  n = (int) strlen(prev);
  w2[n] = word[0];
  w2[n + 1] = '\0';
  if (!known(w2))
    return 0;

  // see if stripping character off front helps unknown word
  strcpy_s(w2, word + 1);
  if (!known(w2))
    return 0;

  // push extra character onto destination string and keep trimmed word 
  back[1] = word[0];
  back[2] = ' '; 
  back[3] = '\0';            // d gets recomputed in FixTypos
  strcpy_s(word, nchar, w2);
  return 1;  
}


//= See if can swap following space with first character of next word.
// example: "th" unknown in "th eobject ..." -> "the object ..." with end borrow (cheap)
// sets word = "the" and next = "object" and alters after from " eobject ..." to " ..."
// returns 1 if successful and alters destination string, 0 if not possible 

int jhcVocab::try_badd (char *word, char *next, const char **after) const
{
  char w2[nchar];
  const char *s, *s2;
  int n;

  // see if there is a character that can be borrowed
  if (after == NULL)
    return 0;
  s = *after;
  if ((s[0] != ' ') || !word_part(s[1]))
    return 0;

  // see if extra character helps unknown word
  strcpy_s(w2, word);
  n = (int) strlen(word);
  w2[n] = s[1];
  w2[n + 1] = '\0';
  if (!known(w2))
    return 0;

  // see if next word can loan first character
  s2 = next_word(w2, s + 2);
  if (!known(w2))
    return 0;

  // record re-assorted word pair and advance input read pointer
  word[n] = s[1];
  word[n + 1] = '\0';
  strcpy_s(next, nchar, w2);
  *after = s2;
  return 1;
}


//= See if last letter of unknown word can be re-homed with next word.
// example: unknown "shew" in "shew as ..." -> "she was ..." with back trim (cheap)
// sets word = "she" and next = "was" and alters after from " as ..." to " ..."
// returns 1 if successful and alters destination string, 0 if not possible 

int jhcVocab::try_brem (char *word, char *next, const char **after) const
{
  char w2[nchar];
  const char *s, *s2;
  int n;

  // see if there is a character that can be ejected
  if (after == NULL)
    return 0;
  s = *after;
  if ((s[0] != ' ') || !word_part(s[1]))
    return 0;

  // see if stripping last character helps unknown word
  strcpy_s(w2, word);
  n = (int) strlen(word);
  w2[n - 1] = '\0';
  if (!known(w2))
    return 0;

  // see if next word can accept last character
  w2[0] = word[n - 1];
  if ((s2 = next_word(w2 + 1, s + 1)) == NULL)
    return 0;
  if (!known(w2))
    return 0;

  // record re-assorted word pair and advance input read pointer
  word[n - 1] = '\0';
  strcpy_s(next, nchar, w2);
  *after = s2;
  return 1;
}


//= Tries inserting a space inside word to split it into two.
// example: "isthe" unknown -> "is the" with new split (moderately expensive)
// returns 1 if successful with split word, 0 retains original

int jhcVocab::try_split (char *word) const
{
  char orig[nchar];
  int i, n = (int) strlen(word);

  strcpy_s(orig, word);
  for (i = n - 1; i > 0; i--)
  {
    word[i] = '\0';
    if (known(word))
      if (lookup(orig + i) != NULL)
      {
        strcat_s(word, nchar, " ");
        strcat_s(word, nchar, orig + i);
        return 1;
      }
  }
  strcpy_s(word, nchar, orig);
  return 0;
}


//= Try transposing each pair of letters in a word to get a valid version.
// example: "hwat" unknown -> "what" with "w/h" transpose (moderately expensive)
// returns 1 if successful and leaves fixed version in "word", 0 retains original

int jhcVocab::try_swap (char *word) const
{
  char orig[nchar];
  char c;
  int i, n = (int) strlen(word);

  strcpy_s(orig, word);
  for (i = n - 1; i > 0; i--)
  {
    c = word[i];
    word[i] = word[i - 1];
    word[i - 1] = c;
    if (known(word))
      return 1;
    strcpy_s(word, nchar, orig);
  }
  strcpy_s(word, nchar, orig);
  return 0;
}


//= Try adding a letter somewhere in word to match a known word.
// example: "blac" unknown -> "black" with "k" insertion (expensive)
// returns 1 if successful and leaves fixed version in "word", 0 retains original

int jhcVocab::try_ins (char *word) const
{
  char **list;
  char *known;
  int ext = (int) strlen(word) + 1, idx = __min(ext, nbins) - 1;
  int i, j, n, add, nw = nwds[idx];

  // scan all known words that are one letter longer than probe
  list = wlen[idx];
  for (i = 0; i < nw; i++)
  {
    // examine word letter by letter allowing only one mismatch
    known = list[i];
    n = (int) strlen(known);           // for upper bins
    add = 0;
    for (j = 0; j < n; j++)
      if (word[j - add] != known[j])
        if (add++ > 0)
          break;

    // check if completely matched with one insertion
    if (add <= 1)
    {
      strcpy_s(word, nchar, known);
      return 1;
    }
  }
  return 0;
}


//= See if changing one letter somewhere in original word makes it known.
// example: "ans" unknown -> "and" with "d" substitution (expensive)
// returns 1 if successful and leaves fixed version in "word", 0 retains original
// NOTE: will convert unknown "block" into "black"!

int jhcVocab::try_sub (char *word) const
{
  char **list;
  char *known;
  int i, j, n, sub, idx = bin(word), nw = nwds[idx];

  // scan all known words that the same length as probe
  list = wlen[idx];
  for (i = 0; i < nw; i++)
  {
    // examine word letter by letter allowing only one mismatch
    known = list[i];
    n = (int) strlen(known);           // for upper bins
    sub = 0;
    for (j = 0; j < n; j++)
      if (word[j] != known[j])
        if (sub++ > 0)
          break;

    // check if completely matched with one insertion
    if (sub <= 1)
    {
      strcpy_s(word, nchar, known);
      return 1;
    }
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                          Category Inference                           //
///////////////////////////////////////////////////////////////////////////

//= Initialize surface analyzer for new sentence.

void jhcVocab::InitGuess ()
{
  int i;

  // output string and selected unknown word
  jprintf(1, dbg, "jhcVocab::InitGuess\n");
  *mark = '\0';
  *oov = '\0';
  worst = 0;

  // initialize 6 element window
  for (i = 0; i < 6; i++)
  {
    sep[i][0] = '\0';
    item[i][0] = '\0';
    fcn[i] = 1;
  }
}


//= Find next unknown word for which grammatical category can be inferred.
// selected word is given by Unknown() and grammar class by Category()
// word must be added to vocabulary, morphology, parser, and speech externally
// incrementally constructs "mark" string as a side effect and picks "oov"
// returns remainder of string after current guess, NULL if all done

const char *jhcVocab::NextGuess (const char *txt)
{
  const char *s = txt;
  char *d = mark;
  int i, n, emit = 0;

  // keep processing words in input string
  if (s == NULL)
    return NULL;
  while (1)
  {
    // if word is unknown then check for various buffer patterns
    if (fcn[2] < 0)
      if ((emit = guess_word()) > 0)
        fcn[2] = 0;

    // add leading separator and current word to marked string
    strcat_s(mark, sep[2]);              
    if (fcn[2] >= 0) 
      strcat_s(mark, item[2]);
    else
    {
      // indicate unknown words using parentheses
      strcat_s(mark, "(");
      strcat_s(mark, item[2]);
      strcat_s(mark, ")");

      // save longest unknown word
      n = (int) strlen(item[2]);
      if (n > worst)
      {
        strcpy_s(oov, item[2]);
        worst = n;
      }
    }

    // shift pattern window one element to left
    for (i = 0; i < 5; i++)
    {
      strcpy_s(sep[i], sep[i + 1]);
      strcpy_s(item[i], item[i + 1]);
      fcn[i] = fcn[i + 1];
    }

    // get next element (whitespace + word) from input
    sep[5][0] = '\0';
    s = copy_gap(sep[5], s);
    s = next_word(item[5], s);
    fcn[5] = gram_fcn(item[5]);

    // stop if unknown but guessable word or all words processed
    if (emit > 0)
      return s;
    if ((*sep[2] == '\0') && (*item[2] == '\0') && 
        (*item[3] == '\0') && (*item[4] == '\0') && (*item[5] == '\0'))
      break;
  }

  // capitalize first word of total marked string
  while (*d != '\0')
    if (word_part(*d))
      break;
    else
      d++;
  *d = (char) toupper(*d);
  return NULL;
}


//= Determines interpretation of word for shallow parser.
// if word is known then probe string converted to canonical form
// return: -1 = unknown, 0 = known, 1 = xp start, 2 = vp start, 3 = pp start, 4 = np start

int jhcVocab::gram_fcn (char *word) const
{
  char npi[5][10]  = {"a", "an", "the", "my", "your"};
  char ppi[24][15] = {"in", "on", "at", "to", "from", "into", "onto", "with", "of",
                      "left", "right", "front", "back", "behind", "near", "close", "between",
                      "inside", "outside", "under", "underneath", "over", "above", "toward"};
  char vpi[8][10]  = {"is", "am", "are", "was", "were", "do", "does", "did"};
  char xpi[20][15] = {"and", "but", "I", "me", "you", "he", "she", "him", "her", "it", "they", "them",
                      "here", "there", "that", "this", "something", "anything", "someone", "anyone"};
  const char *norm;
  double val;
  int i;

  // check for end of sentence or unknown
  if (*word == '\0')
    return 1;                          
  if (sscanf_s(word, "%lf", &val) == 1)
    return 4;
  if ((norm = lookup(word)) == NULL)
    return -1;                         
  strcpy_s(word, nchar, norm);

  // look for phrase starting markers
  for (i = 0; i < 5; i++)
    if (strcmp(word, npi[i]) == 0)
      return 4;
  for (i = 0; i < 24; i++)
    if (strcmp(word, ppi[i]) == 0)
      return 3;
  for (i = 0; i < 8; i++)
    if (strcmp(word, vpi[i]) == 0)
      return 2;
  for (i = 0; i < 20; i++)
    if (strcmp(word, xpi[i]) == 0)
      return 1;
  return 0;
}


//= Uses local word and function buffer to match a variety of patterns.
// assumes sliding window of 6 elements (4 words) around item2:
// <pre>
//        sep0      sep1      sep2      sep3      sep4      sep5
//            item0     item1     item2     item3     item4     item5
//             fcn0      fcn1      fcn2      fcn3      fcn4      fcn5
// 
// adjective example: "the_big_dog_at_the_house ..."
// 
//  sep:   ""        ""        "_"       "_"       "_"       "_"
// word:       ""       "the"     "big"     "dog"      "at"     "the"
//  fcn:        0         4        -1         0         3         4
//  
// </pre>
// on success variable "unk" holds word and "cat" holds non-terminal name
// returns 1 if guess has been made about "item" else 0

int jhcVocab::guess_word ()
{
  // possibly show buffer contents
  jprintf(2, dbg, "  buffer:   %s %d   %s %d   <%s>   %s %d   %s %d   %s %d\n", 
          item[0], fcn[0], item[1], fcn[1], item[2], item[3], fcn[3], item[4], fcn[4], item[5], fcn[5]);

  // examine prepositional phrases
  if ((fcn[1] == 3) && (fcn[3] > 0))             // [prep ? .] very weak!
    return name_ctx(item[2], 1);

  // examine noun phrases
  if ((fcn[1] == 4) && (fcn[3] > 0))             // [det ? .] 
    return noun_ctx(item[2]);
  if ((fcn[0] >= 3) && (fcn[3] > 0))             // [det x ? .] or [prep x ? .]
    return noun_ctx(item[2]);
  if ((fcn[1] == 4) && (fcn[4] > 0))             // [det ? x .] 
    return adj_ctx(item[2]);
  if (fcn[3] == 2)                               // [? aux]
    return name_ctx(item[2], 1);

  // examine verb phrases
  if ((fcn[1] > 0) && (fcn[3] >= 3))             // [. ? det] or [. ? prep]
    return verb_ctx(item[2]);

  // check for explicit definition 
  if ((strcmp(item[0], "name") == 0) && (strcmp(item[1], "is") == 0))    // ["name" "is" ? ]
    return name_ctx(item[2], 0);
  if (strcmp(item[3], "is") == 0) 
  {
    if (strcmp(item[5], "name") == 0)            // [. ? "is" x "name"]
      return name_ctx(item[2], 0);
    if (strcmp(item[5], "property") == 0)        // [. ? "is" x "property"]
      return adj_ctx(item[2]);
    if (strcmp(item[5], "action") == 0)          // [. ? "is" x "action"]
      return verb_ctx(item[2]);
    if (strcmp(item[5], "manner") == 0)          // [. ? "is" x "manner"]
    {
      strcpy_s(unk, item[2]);                    // no -ly suffix needed
      strcpy_s(cat, "MOD");
      return 1;
    }
  }

  // try guessing based on suffix (Note: -'s could be AKO or NAME)
  if (adv_end(item[2]) || verb_end(item[2]))   
  {
    jprintf(1, dbg, "    suffix: %s\n", item[2]);
    return 1;
  }

  // examine prepositional phrases
  if ((fcn[1] == 3) && (fcn[3] > 0))             // [prep ? .]  very weak!
    return name_ctx(item[2], 1);
  return 0;
}


//= Determine proper non-termninal of unknown name-like word.
// always returns 1 if for convenience

int jhcVocab::name_ctx (const char *word, int npl)
{
  int n = (int) strlen(word);
  const char *end = word + n;

  if ((npl > 0) && (*(end - 1) == 's') && !isupper(*word))
    return noun_ctx(word);
  jprintf(1, dbg, "    name_ctx: %s\n", word);
  if ((n >= 5) && (strcmp(end - 2, "'s") == 0))
    strcpy_s(cat, "NAME-P");
  else if ((n >= 5) && (strcmp(end - 2, "s'") == 0))
    strcpy_s(cat, "NAME-P");
  else
    strcpy_s(cat, "NAME");
  strcpy_s(unk, word);
  return 1;
}


//= Determine proper non-termninal of unknown noun-like word.
// always returns 1 for convenience

int jhcVocab::noun_ctx (const char *word)
{
  int n = (int) strlen(word);
  const char *end = word + n;

  jprintf(1, dbg, "    noun_ctx: %s\n", word);
  if (!poss_end(word))
  {
    if ((n >= 4) && (strcmp(end - 1, "s") == 0))
      strcpy_s(cat, "AKO-S");
    else
      strcpy_s(cat, "AKO");
    strcpy_s(unk, word);
  }
  return 1;
}


//= Determine proper non-termninal of unknown adjective-like word.
// always returns 1 for convenience

int jhcVocab::adj_ctx (const char *word)
{
  int n = (int) strlen(word);
  const char *end = word + n;

  jprintf(1, dbg, "    adj_ctx: %s\n", word);
  if (!poss_end(word) && !verb_end(word))
  {
    if ((n >= 6) && (strcmp(end - 3, "est") == 0))
      strcpy_s(cat, "HQ-EST");
    else if ((n >= 5) && (strcmp(end - 2, "er") == 0))
      strcpy_s(cat, "HQ-ER");
    else
      strcpy_s(cat, "HQ");
    strcpy_s(unk, word);
  }
  return 1;
}


//= Determine proper non-termninal of some verb-like word.
// always returns 1 for convenience

int jhcVocab::verb_ctx (const char *word)
{
  int n = (int) strlen(word);
  const char *end = word + n;

  jprintf(1, dbg, "    verb_ctx: %s\n", word);
  if (!verb_end(word))
  {
    if ((n >= 4) && (strcmp(end - 1, "s") == 0))
      strcpy_s(cat, "ACT-S");
    else
      strcpy_s(cat, "ACT");
    strcpy_s(unk, word);
  }
  return 1;
}


//= If word ends in -'s or -s' then assume it is a noun.
// NOTE: same suffixes apply also to members of NAME class

bool jhcVocab::poss_end (const char *word) 
{
  int n = (int) strlen(word);
  const char *end = word + n;

  if ((n >= 5) && (strcmp(end - 2, "'s") == 0))
    strcpy_s(cat, "AKO-P");
  else if ((n >= 5) && (strcmp(end - 2, "s'") == 0))
    strcpy_s(cat, "AKO-P");
  else
    return false;
  strcpy_s(unk, word);
  return true;
}


//= If word ends in -ing or -ed then assume it is a verb.

bool jhcVocab::verb_end (const char *word) 
{
  int n = (int) strlen(word);
  const char *end = word + n;

  if ((n >= 6) && (strcmp(end - 3, "ing") == 0))
    strcpy_s(cat, "ACT-G");
  else if ((n >= 5) && (strcmp(end - 2, "ed") == 0))
    strcpy_s(cat, "ACT-D");
  else
    return false;
  strcpy_s(unk, word);
  return true;
}


//= If word ends in -ly then assume it is an adverb.

bool jhcVocab::adv_end (const char *word) 
{
  int n = (int) strlen(word);
  const char *end = word + n;

  if ((n >= 5) && (strcmp(end - 2, "ly") == 0))
    strcpy_s(cat, "MOD");
  else
    return false;
  strcpy_s(unk, word);
  return true;
}


///////////////////////////////////////////////////////////////////////////
//                             String Elements                           //
///////////////////////////////////////////////////////////////////////////

//= Copy sequence of non-word characters to string at dest.
// returns equivalent remaining portion of txt

const char *jhcVocab::copy_gap (char *dest, const char *txt) const
{
  const char *s = txt;
  char *d = dest + strlen(dest);       // appends to old

  if (s == NULL)
    return NULL;
  while ((*s != '\0') && !word_part(*s))
    *d++ = *s++;
  *d = '\0';
  return s;
}


//= Extract next word from text string which has been trimmed to a valid start.
// return pointer to input string after word

const char *jhcVocab::next_word (char *word, const char *txt) const
{
  const char *s = txt;
  char *w = word;

  *w = '\0';                           // complete string
  if (s == NULL)
    return NULL;
  while (word_part(*s))
    *w++ = *s++;
  *w = '\0';
  return s;
}


//= Tell if character is a valid part of a word.

bool jhcVocab::word_part (char c) const
{
  return((c != '\0') && ((isalnum(c) != 0) || (strchr("-_'", c) != NULL)));
}


///////////////////////////////////////////////////////////////////////////
//                               Utilities                               //
///////////////////////////////////////////////////////////////////////////

//= Write out all know words to file "words.txt".
// returns total number of entries

int jhcVocab::ListAll () const
{
  FILE *out;
  char **list;
  int i, j, n, cnt = 0;

  // try opening file
  if (fopen_s(&out, "words.txt", "w") != 0)
    return -1;

  // go through all length buckets
  for (i = 0; i < nbins; i++)
  {
    list = wlen[i];
    n = nwds[i];
    for (j = 0; j < n; j++)
      fprintf(out, "%s\n", list[j]);
    cnt += n;
  }

  // cleanup 
  fclose(out);
  return cnt;
}


//= Find any non-terminals that are not used in other rules.
// takes list of grammer rules from jhcGramExec (most complete source)
// writes disconnected non-terminal names in file "orphans.txt"
// returns number of bad non-terminals found

int jhcVocab::WeedGram (const jhcGramRule *gram, int nt) const
{
  char **nterm;
  int *any;
  FILE *out;
  const jhcGramRule *r = gram;
  const jhcGramStep *t;
  int i, n = 0, cnt = 0;

  // trying opening file then allocate arrays
  if (fopen_s(&out, "orphans.txt", "w") != 0)
    return -1;
  any = new int [nt];
  nterm = new char * [nt];
  for (i = 0; i < nt; i++)
    nterm[i] = new char [nchar];

  // scan for all non-terminals and whether they are used
  while (r != NULL)
  {
    // add non-terminal to list if not already there
    for (i = 0; i < n; i++)
      if (strcmp(r->head, nterm[i]) == 0)
        break;
    if ((i >= n) && (n < nt))
    {
      strcpy_s(nterm[n++], nchar, r->head);
      if (strcmp(r->head, "toplevel") == 0)
        any[i] = 1;                    // used externally
      else
        any[i] = 0;                    // new but not used
    }

    // look for non-terminals in expansion
    t = r->tail;
    while (t != NULL)
    {
      // add non-terminal to list if not already there
      if (t->non > 0)
      {
        for (i = 0; i < n; i++)
          if (strcmp(t->symbol, nterm[i]) == 0)
          {
            any[i] = 1;                // found and used
            break;
          }
        if ((i >= n) && (n < nt))
        {
          strcpy_s(nterm[n++], nchar, t->symbol);
          any[i] = 1;                  // new and used
        }
      }
      t = t->tail;                     // next part of expansion
    }
    r = r->next;                       // next grammar rule
  }

  // list any unused non-terminals in file
  for (i = 0; i < n; i++)
    if (any[i] <= 0)
    {
      fprintf(out, "%s\n", nterm[i]);
      cnt++;
    }

  // clean up and deallocate
  fclose(out);
  for (i = 0; i < nt; i++)
    delete [] nterm[i];
  delete [] nterm;
  delete [] any;
  if (n >= nt)
    jprintf(">>> More than %d non-terminals in jhcVocab::WeedGram!\n", nt);
  return cnt;
}


//= Create a version of input text where unknown words are surrounded by brackets.
// preserves punctuation but substitutes normalized form of each word 
// generally the "mark" variable has already been set by jhcAliaCore::Interpret
// returns pointer to temporary string holding modified input (same as Marked())

const char *jhcVocab::MarkBad (const char *txt)
{
  const char *s = txt;

  InitGuess();
  while ((s = NextGuess(s)) != NULL);
  return mark;
}

