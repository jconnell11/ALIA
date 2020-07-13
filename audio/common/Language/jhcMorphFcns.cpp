// jhcMorphFcns.cpp : does something
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#include <ctype.h>
#include <string.h>

#include "Acoustic/jhcSpeechX.h"       // since only spec'd as class in header
#include "Language/jhcMorphTags.h"
#include "Parse/jhcGenParse.h"         // since only spec'd as class in header

#include "Language/jhcMorphFcns.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcMorphFcns::~jhcMorphFcns ()
{

}


//= Default constructor initializes certain values.

jhcMorphFcns::jhcMorphFcns ()
{
  ClrExcept();
}


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Clear all exceptions to morphology rules.

void jhcMorphFcns::ClrExcept ()
{
  int i;

  // clear all nouns
  for (i = 0; i < nmax; i++)
  {
    nsing[i][0] = '\0';
    npl[i][0]   = '\0';
  }

  // clear all adjectives
  for (i = 0; i < amax; i++)
  {
    adj[i][0]  = '\0';
    sup[i][0]  = '\0';
    comp[i][0] = '\0';
  }

  // clear all verbs
  for (i = 0; i < vmax; i++)
  {
    vimp[i][0]  = '\0';
    vpres[i][0] = '\0';
    vprog[i][0] = '\0';
    vpast[i][0] = '\0';
  }

  // clear counts
  nn = 0;
  nv = 0;
  na = 0;
}


//= Load some exceptions to morphology rules (generally appends).
// format intended for embedding in a normal *.sgm file:
// <pre>
//   =[XXX]
//     base1   * tag1  = surface1
//     base2   * tag2  = surface2
//     man     * npl   = men
//     elegant * acomp = more elegant
// </pre>
// where tags come from JTAG_STR[] = npl, acomp, asup, vpres, vprog, vpast
// returns positive if successful, 0 or negative for problem

int jhcMorphFcns::LoadExcept (const char *fname, int append)
{
  char line[200], base[80], surf[80];
  FILE *in;
  UL32 tag;
  int cnt = 0;

  // possibly clear all previous morphology then try opening file
  if (append <= 0)
    ClrExcept();
  if (fopen_s(&in, fname, "r") != 0)
    return -1;

  // look for one or move sections starting with XXX (never two in sequence!)
  while (clean_line(line, 200, in) != NULL)
    if (line[0] == '=')
      if (strchr("[<", line[1]) != NULL)
        if (_strnicmp(line + 2, "xxx", 3) == 0)
          if (strpbrk(line + 4, "]>") != NULL)
            while (clean_line(line, 200, in) != NULL)
            {
              // interpret each line as: base * tag = surface
              if (*line == '\0')
                continue;
              if (*line == '=')
                break;
              if ((tag = parse_line(base, surf, line)) == 0)
              {
                jprintf(">> Bad format in: %s\n", line);
                continue;
              }

              // add pair to correct arrays
              if ((tag & JTAG_NPL) != 0)
                cnt += add_morph(nn, nsing, npl, nmax, base, surf);
              else if ((tag & JTAG_ACOMP) != 0)
                cnt += add_morph(na, adj, comp, amax, base, surf);
              else if ((tag & JTAG_ASUP) != 0)
                cnt += add_morph(na, adj, sup, amax, base, surf);
              else if ((tag & JTAG_VPRES) != 0)
                cnt += add_morph(nv, vimp, vpres, vmax, base, surf);
              else if ((tag & JTAG_VPROG) != 0)
                cnt += add_morph(nv, vimp, vprog, vmax, base, surf);
              else if ((tag & JTAG_VPAST) != 0)
                cnt += add_morph(nv, vimp, vpast, vmax, base, surf);
              else
                jprintf(">> Unknown tag in: %s\n", line);
            }

  // cleanup
  fclose(in);
  return cnt;
}


//= Add a new morphology transform to some pair of arrays.
// returns 1 if okay, 0 if out of space

int jhcMorphFcns::add_morph (int &n, char key[][40], char val[][40], int wmax, const char *base, const char *surf)
{
  int i;

  // get index associated with base form
  for (i = 0; i < n; i++)
    if (strcmp(key[i], base) == 0)
      break;
 
  // try to add key if missing
  if (i >= n)
  {
    if (n >= wmax)
      return jprintf(">> Already %d morphology entries: %s !\n", wmax, base);
    strcpy_s(key[n], base);
    n++;
  } 

  // overwrite any previous entry
  strcpy_s(val[i], surf);
  return 1;
}


//= Get next line while stripping off comment portion and any newline character.
// also removes leading and trailing white space and eliminates tabs

char *jhcMorphFcns::clean_line (char *ans, int ssz, FILE *in) const
{
  char raw[200];
  char *start, *last, *end;

  // get line from file (unless at end)
  if (fgets(raw, 200, in) == NULL)
    return NULL;

  // copy minus leading whitespace
  start = raw;
  while (strchr(" \t", *start) != NULL)
    start++;
  strcpy_s(ans, ssz, start);

  // remove final newline and anything after semicolon
  if ((end = strchr(ans, '\n')) != NULL)
    *end = '\0';
  if ((end = strchr(ans, ';')) != NULL)
    *end = '\0';

  // remove double slashes and following comment
  end = ans;
  while ((end = strchr(end, '/')) != NULL)
    if (*(end + 1) == '/')
    {
      *end = '\0';
      break;
    }
    else
      end++;

  // trim trailing whitespace
  end = ans;
  last = ans;
  while (*(++end) != '\0')
    if (*end != ' ')
      last = end;
  *(last + 1) = '\0';
  return ans;
}


//= Read a file line composed of base word, kind tag, and surface form.
// binds "base" to word and "surf" to expansion (both assumed 80 chars long)
// returns tag if okay, 0 for error

UL32 jhcMorphFcns::parse_line (char *base, char *surf, const char *line) const
{
  char kind[20];
  const char *sep, *start;
  UL32 tag;
  int i;

  // get base form (could be several words?)
  if ((sep = strchr(line, '*')) == NULL)
    return 0;
  if (trim_tail(base, line, sep, 80) <= 0)
    return 0;

  // get kind tag and test it
  start = sep;
  while (*(++start) != '\0')
    if (isalnum(*start))
      break;
  if ((sep = strchr(start, '=')) == NULL)
    return 0;
  if (trim_tail(kind, start, sep, 20) <= 0)
    return 0;

  // convert kind string to numeric tag
  for (i = 0; i < JTV_MAX; i++)
    if (strcmp(kind, JTAG_STR[i]) == 0)
      break;
  if (i >= JTV_MAX)
    return 0;
  tag = 0x01 << i;

  // get the associated surface form
  start = sep;
  while (*(++start) != '\0')
    if (isalnum(*start))
      break;
  if (trim_tail(surf, start, NULL, 80) <= 0)
    return 0;

  // add to appropriate list
  return tag;
}


//= Copy portion of one string into another, trimming any spaces at the end.
// returns 1 if successful, 0 if final string is empty

int jhcMorphFcns::trim_tail (char *dest, const char *start, const char *end, int ssz) const
{
  int i, n = (int) strlen(start);

  // copy fraction of string
  if (end != NULL)
    n = (int)(end - start - 1);
  strncpy_s(dest, ssz, start, n);

  // remove trailing spaces
  for (i = n - 1; i >= 0; i--)
  {
    if (dest[i] != ' ')
      return 1;
    dest[i] = '\0';
  }
  return 0;
}


//= Save all known irregular morphologies to a file.
// return positive if successful, 0 or negative for problem

int jhcMorphFcns::SaveExcept (const char *fname) const
{
  FILE *out;
  int i;
  
  // try opening file then write header
  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  fprintf(out, "// irregular morphologies (np, acomp, asup, vpres, vprog, vpast)\n\n");
  fprintf(out, "=[XXX-morph]\n");

  // nouns
  for (i = 0; i < nn; i++)
    if (npl[i][0] != '\0')
      fprintf(out, "  %s * npl = %s\n", nsing[i], npl[i]);

  // adjectives
  for (i = 0; i < na; i++)
  {
    if (comp[i][0] != '\0')
      fprintf(out, "  %s * acomp = %s\n", adj[i], comp[i]);
    if (sup[i][0] != '\0')
      fprintf(out, "  %s * asup  = %s\n", adj[i], sup[i]);
  }

  // verbs
  for (i = 0; i < nv; i++)
  {
    if (vpres[i][0] != '\0')
      fprintf(out, "  %s * vpres = %s\n", vimp[i], vpres[i]);
    if (vprog[i][0] != '\0')
      fprintf(out, "  %s * vprog = %s\n", vimp[i], vprog[i]);
    if (vpast[i][0] != '\0')
      fprintf(out, "  %s * vpast = %s\n", vimp[i], vpast[i]);
  }

  // clean up
  fclose(out);
  return(nn + na + nv);
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Loads a grammar file to parser as well as all morphological variants.
// assumes original grammar file has some section labelled "=[XXX-morph]"
// returns positive if successful, 0 or negative for problem

int jhcMorphFcns::AddVocab (jhcGenParse *p, const char *fname)
{
  const char deriv[80] = "jhc_temp.txt";

  // load basic grammar file like usual
  if ((p == NULL) || (fname == NULL) || (*fname == '\0'))
    return -3;
  if (p->LoadGrammar(fname) <= 0)
    return -2;

  // read and apply morphology to add derived forms as well
  if (LoadExcept(fname) <= 0)
    return 1;
  if (LexDeriv(fname, 0, deriv) < 0)
    return -1;
  if (p->LoadGrammar(deriv) <= 0)
    return 0;
  return 2;
}


//= Loads a grammar file to speech recognition as well as all morphological variants.
// assumes original grammar file has some section labelled "=[XXX-morph]"
// returns positive if successful, 0 or negative for problem

int jhcMorphFcns::AddVocab (jhcSpeechX *p, const char *fname)
{
  const char deriv[80] = "jhc_temp.txt";

  // load basic grammar file like usual
  if ((p == NULL) || (fname == NULL) || (*fname == '\0'))
    return -3;
  if (p->LoadGrammar(fname) <= 0)
    return -2;

  // read and apply morphology to add derived forms as well
  if (LoadExcept(fname) <= 0)
    return 1;
  if (LexDeriv(fname, 0, deriv) <= 0)
    return -1;
  if (p->LoadGrammar(deriv) <= 0)
    return 0;
  return 2;
}


///////////////////////////////////////////////////////////////////////////
//                             Derived Forms                             //
///////////////////////////////////////////////////////////////////////////

//= Get appropriate surface form of some base word given desired categories.
// NOTE: string might be member variable "stemp" which can be rewritten (use soon)

const char *jhcMorphFcns::SurfWord (const char *base, UL32 tags)
{
  const char *irr;

  if ((irr = lookup_surf(base, tags)) != NULL)
    return irr;
  strcpy_s(stemp, base);
  if ((tags & JTAG_NOUN) != 0)
    return noun_morph(stemp, tags);
  if ((tags & JTAG_VERB) != 0)
    return verb_morph(stemp, tags);
  if ((tags & JTAG_ADJ) != 0)
    return adj_morph(stemp, tags);
  return NULL;
}


//= Retrieve some possibly irregular surface form of a word given the base form.
// needs specification of what POS mask should be applied to base form
// returns special form if known, NULL if assumed regular 

const char *jhcMorphFcns::lookup_surf (const char *base, UL32 tags) 
{
  // nouns
  if ((tags & JTAG_NPL) != 0)
    return scan_for(base, nsing, npl, nn);

  // verbs
  if ((tags & JTAG_VPRES) != 0)
    return scan_for(base, vimp, vpres, nv);
  if ((tags & JTAG_VPROG) != 0)
    return scan_for(base, vimp, vprog, nv);
  if ((tags & JTAG_VPAST) != 0)
    return scan_for(base, vimp, vpast, nv);

  // adjectives 
  if ((tags & JTAG_ACOMP) != 0)
    return scan_for(base, adj, comp, na);
  if ((tags & JTAG_ASUP) != 0)
    return scan_for(base, adj, sup, na);

  // invalid conversion
  return NULL;
}


//= Add correct suffix to some noun based on tags.

char *jhcMorphFcns::noun_morph (char *val, UL32 tags) const
{
  if ((tags & JTAG_NSING) != 0)
    return val;
  if ((tags & JTAG_NPL) != 0)
    return add_s(val, 80);
  return NULL;
}


//= Add correct suffix to some verb based on tags.

char *jhcMorphFcns::verb_morph (char *val, UL32 tags) const
{
  if ((tags & JTAG_VIMP) != 0)
    return val;
  if ((tags & JTAG_VPRES) != 0)
    return add_s(val, 80);
  if ((tags & JTAG_VPROG) != 0)
    return add_vowel(val, "ing", 80);          
  if ((tags & JTAG_VPAST) != 0)
    return add_vowel(val, "ed", 80); 
  return NULL;
}


//= Add correct suffix to some adjective based on tags.

char *jhcMorphFcns::adj_morph (char *val, UL32 tags) const
{
  if ((tags & JTAG_APROP) != 0)
    return val;
  if ((tags & JTAG_ACOMP) != 0)
    return add_vowel(val, "er", 80);
  if ((tags & JTAG_ASUP) != 0)
    return add_vowel(val, "est", 80);
  return NULL;
}


//= Add -s to end of word, either for plural nouns or present tense verbs.
// checks for i-es, ch/sh/s-es, and simple -s cases

char *jhcMorphFcns::add_s (char *val, int ssz) const
{
  int n = (int) strlen(val);
  char *end = val + n;

  if ((n >= 2) && !vowel(end[-2]) && 
      (end[-1] == 'y'))
  {
    end[-1] = 'i';                                         // transmute y to i
    strcat_s(val, ssz, "es");
  }
  else if ((n >= 2) && 
           ((strcmp(end - 2, "ch") == 0) || 
            (strcmp(end - 2, "sh") == 0)))
    strcat_s(val, ssz, "es");                              // ends in ch or sh
  else if ((n >= 1) && (strchr("sz", end[-1]) != NULL))
    strcat_s(val, ssz, "es");                              // ends in s or z
  else
    strcat_s(val, ssz, "s");
  return val;
}


//= Prepare base form then append a suffix beginning with a vowel.
// used for adjectives with -er and -est, verbs with -ing and -ed
// handles doubled consonant, e elision, and transmuted y
// assumes string is long enough to add a single letter

char *jhcMorphFcns::add_vowel (char *val, const char *suffix, int ssz) const
{
  int n = (int) strlen(val);
  char *end = val + n;

  if ((n >= 2) && 
      ((n < 3) || !vowel(end[-3])) &&
      vowel(end[-2]) && 
      !vowel(end[-1]) && (strchr("rwy", end[-1]) == NULL))
  {
    end[0] = end[-1];                                      // double consonant
    end[1] = '\0';
  }
  else if ((n >= 2) && 
           ((suffix[0] == 'e') || !vowel(end[-2])) &&
           (end[-1] == 'e'))
    end[-1] = '\0';                                        // remove final e
  else if ((n >= 2) && !vowel(end[-2]) && 
           (end[-1] == 'y') &&
           (suffix[0] != 'i'))
    end[-1] = 'i';                                         // transmute y to i
  else if (strchr(val, '-') != NULL)
  {
    end[0] = '-';                                          // add final hyphen
    end[1] = '\0';
  }
  strcat_s(val, ssz, suffix);
  return val;
}


///////////////////////////////////////////////////////////////////////////
//                             Normalization                             //
///////////////////////////////////////////////////////////////////////////

//= Get appropriate base form of some surface word given known categories.
// NOTE: string might be member variable "btemp" which can be rewritten (use soon)

const char *jhcMorphFcns::BaseWord (const char *surf, UL32 tags)
{
  const char *irr;

  if ((irr = lookup_base(surf, tags)) != NULL)
    return irr;
  strcpy_s(btemp, surf);
  if ((tags & JTAG_NOUN) != 0)
    return noun_stem(btemp, tags);
  if ((tags & JTAG_VERB) != 0)
    return verb_stem(btemp, tags);
  if ((tags & JTAG_ADJ) != 0)
    return adj_stem(btemp, tags);
  return NULL;
}

 
//= Retrieve the base form of a word given some possibly irregular surface form.
// needs specification of what POS mask applies to surface form
// returns special form if known, NULL if assumed regular 

const char *jhcMorphFcns::lookup_base (const char *surf, UL32 tags) const
{
  // nouns
  if ((tags & JTAG_NPL) != 0)
    return scan_for(surf, npl, nsing, nn);

  // verbs
  if ((tags & JTAG_VPRES) != 0)
    return scan_for(surf, vpres, vimp, nv);
  if ((tags & JTAG_VPROG) != 0)
    return scan_for(surf, vprog, vimp, nv);
  if ((tags & JTAG_VPAST) != 0)
    return scan_for(surf, vpast, vimp, nv);

  // adjectives 
  if ((tags & JTAG_ACOMP) != 0)
    return scan_for(surf, comp, adj, na);
  if ((tags & JTAG_ASUP) != 0)
    return scan_for(surf, sup, adj, na);

  // invalid conversion
  return NULL;
}


//= Strip plural suffix from surface word to get base noun.
// returns pointer to modified "val" string

char *jhcMorphFcns::noun_stem (char *val, UL32 tags) const
{
  if ((tags & JTAG_NSING) != 0)
    return val;
  if ((tags & JTAG_NPL) != 0)
    return rem_s(val);
  return NULL;
}


//= Strip tense suffixes from surface word to get base verb.
// returns pointer to modified "val" string

char *jhcMorphFcns::verb_stem (char *val, UL32 tags) const
{
  int n = (int) strlen(val);
  char *end = val + n;

  // check for known forms
  if ((tags & JTAG_VIMP) != 0)
    return val;
  if ((tags & JTAG_VPRES) != 0)
    return rem_s(val);
  if ((tags & JTAG_VPROG) != 0)
  {
    // progressive verb ending in -ing
    if ((n > 3) && (strcmp(end - 3, "ing") == 0))
      return rem_vowel(val, 3);
  }
  else if ((tags & JTAG_VPAST) != 0)
  {
    // past tense verb ending in -ed
    if ((n > 2) && (strcmp(end - 2, "ed") == 0))
      return rem_vowel(val, 2);
  }
  return NULL;
}


//= Strip tense suffixes from surface word to get base adjective.
// handles "bigger", "fuller", and "noisier" (but "nicer" not reversible)
// returns pointer to modified "val" string

char *jhcMorphFcns::adj_stem (char *val, UL32 tags) const
{
  int n = (int) strlen(val);
  char *end = val + n;

  // check for known forms
  if ((tags & JTAG_APROP) != 0)
    return val;
  if ((tags & JTAG_ACOMP) != 0)
  {
    // comparative adjective ending in -er
    if ((n > 2) && (strcmp(end - 2 , "er") == 0))
      return rem_vowel(val, 2);
  }
  else if ((tags & JTAG_ASUP) != 0)
  {
    // superlative adjective ending in -est
    if ((n > 3) && (strcmp(end - 3, "est") == 0))
      return rem_vowel(val, 3);
  }
  return NULL;
}


//= Removes final -s from word and attempts to restore base.
// handles z/s-es, ch/sh-es, i-es, and simple -s cases

char *jhcMorphFcns::rem_s (char *val) const
{
  int n = (int) strlen(val);
  char *end = val + n;

  if ((n >= 4) && 
      ((strcmp(end - 4, "ches") == 0) ||
       (strcmp(end - 4, "shes") == 0)))
    end[-2] = '\0';                                        // remove -es
  else if ((n >= 4) && vowel(end[-4]) &&
           ((strcmp(end - 3, "zes") == 0) ||
            (strcmp(end - 3, "ses") == 0)))
    end[-1] = '\0';                                        // remove -s
  else if ((n >= 3) && 
           ((strcmp(end - 3, "zes") == 0) ||
            (strcmp(end - 3, "ses") == 0)))
    end[-2] = '\0';                                        // remove -es
  else if ((n >= 3) && (strcmp(end - 3, "ies") == 0))
  {
    end[-3] = 'y';                                         // transmute i-es to y
    end[-2] = '\0';
  } 
  else if ((n >= 1) && (end[-1] == 's'))
    end[-1] = '\0';                                        // remove -s
  return val;
}


//= Attempt to restore base word after removing suffix beginning with a vowel.
// "strip" is the length of the suffix to remove before restoring base
// handles doubled consonant, e elision, and transmuted y

char *jhcMorphFcns::rem_vowel (char *val, int strip) const
{
  int n = (int) strlen(val) - strip;
  char *end = val + n;

  if ((n >= 1) && (end[-1] == '-'))
    end[-1] = '\0';                                        // drop hyphen (must be first)
  else if ((n >= 3) && ((n < 4) || !vowel(end[-4])) &&
           vowel(end[-3]) &&
           (end[-2] == end[-1]) &&            
           !vowel(end[-1]) && (strchr("flsz", end[-1]) == NULL))
    end[-1] = '\0';                                        // double consonant
  else if ((n >= 2) && (vowel(end[-2]) || (end[-2] == 'n')) && 
           (strchr("csz", end[-1]) != NULL))
  {
    end[0] = 'e';                                          // e elision (must be first)
    end[1] = '\0';
  }
  else if ((n >= 2) && ((n < 3) || !vowel(end[-3])) && 
           vowel(end[-2]) && 
           !vowel(end[-1]) && (strchr("rwy", end[-1]) == NULL))
  {
    end[0] = 'e';                                          // e elision
    end[1] = '\0';
  }
  else if ((n >= 1) && (end[-1] == 'i'))
  {
    end[-1] = 'y';                                         // transmute i to y
    end[0] = '\0';
  }
  else
    end[0] = '\0';                                         // just strip
  return val;
}


///////////////////////////////////////////////////////////////////////////
//                           Shared Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Look in list of keys for probe and get corresponding value.
// return NULL if probe not found or no special case listed

const char *jhcMorphFcns::scan_for (const char *probe, const char key[][40], const char val[][40], int n) const
{
  int i;

  for (i = 0; i < n; i++)
    if (strcmp(key[i], probe) == 0)
    {
      if (val[i][0] == '\0')
        return NULL;
      return val[i];
    }
  return NULL;
}


//= Whether a particular character is a vowel.

bool jhcMorphFcns::vowel (char c) const
{
  return(strchr("aeiou", c) != NULL);
}


///////////////////////////////////////////////////////////////////////////
//                          Graphizer Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Get normalized noun based on surface category extracted from pair.
// pair input is part of an association list with a part of speech like "AKO-S=birds"
// also sets "tags" to proper morphology bits as a mask (not value)
// returns canonical version of value (singular) or NULL if not a noun
// NOTE: tail end of pair might get re-written!

const char *jhcMorphFcns::NounLex (UL32& tags, char *pair) const
{
  char dummy[2][40] = {"thing", "something"};
  char *val = SlotRef(pair);
  const char *irr;
  int i;

  // check for special non-informative words
  if (val == NULL)
    return NULL;
  for (i = 0; i < 2; i++)
    if (strcmp(val, dummy[i]) == 0)
      return NULL;

  // determine tags then generate base form
  tags = gram_tag(pair);
  if ((tags & JTAG_NOUN) == 0)
    return NULL;
  if ((irr = lookup_base(val, tags)) != NULL)
    return irr;
  return noun_stem(val, tags);
}


//= Get normalized verb based on surface category extracted from pair.
// pair input is part of an association list with a part of speech like "V-ED=washed"
// also sets "tags" to proper morphology bits as a mask (not value)
// returns canonical version of value (present) or NULL if not a verb
// NOTE: tail end of pair might get re-written!

const char *jhcMorphFcns::VerbLex (UL32& tags, char *pair) const
{
  char *val = SlotRef(pair);
  const char *irr;

  // check for verbatim echo
  if (val == NULL)
    return NULL;
  if (SlotMatch(pair, "SAY"))
  {
    tags = JTAG_VIMP;
    return val;
  }

  // determine tags then generate base form
  tags = gram_tag(pair);
  if ((tags & JTAG_VERB) == 0)
    return NULL;
  if ((irr = lookup_base(val, tags)) != NULL)
    return irr;
  return verb_stem(val, tags);
}


//= Get normalized adjective based on surface category extracted from pair.
// pair input is part of an association list with a part of speech like "HQ-ER=bigger"
// also sets "tags" to proper morphology bits as a mask (not value)
// returns canonical version of value (present) or NULL if not an adjective
// NOTE: tail end of pair might get re-written!

const char *jhcMorphFcns::AdjLex (UL32& tags, char *pair) const
{
  char *val = SlotRef(pair);
  const char *irr;

  // determine tags then generate base form
  tags = gram_tag(pair);
  if ((tags & JTAG_ADJ) == 0)
    return NULL;
  if ((irr = lookup_base(val, tags)) != NULL)
    return irr;
  return adj_stem(val, tags);
}


//= Convert parser class into a part of speech tag (mask not value).

UL32 jhcMorphFcns::gram_tag (const char *pair) const
{
  // noun counts
  if (SlotMatch(pair, "AKO"))          
    return JTAG_NSING;
  if (SlotMatch(pair, "AKO-S"))
    return JTAG_NPL;
  
  // verb tenses
  if (SlotMatch(pair, "ACT"))     
    return JTAG_VIMP;
  if (SlotMatch(pair, "ACT-S"))
    return JTAG_VPRES;
  if (SlotMatch(pair, "ACT-D"))
    return JTAG_VPAST;
  if (SlotMatch(pair, "ACT-G"))
    return JTAG_VPROG;

  // adjective forms
  if (SlotMatch(pair, "HQ"))          
    return JTAG_APROP;
  if (SlotMatch(pair, "HQ-ER"))          
    return JTAG_ACOMP;
  if (SlotMatch(pair, "HQ-EST"))
    return JTAG_ASUP;

  // unknown
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                           Degugging Tools                             //
///////////////////////////////////////////////////////////////////////////

//= Generate a derived lexicon grammar file from a base open-class grammar file.
// should check this by eye to find cases where incorrect forms are produced
// looks for =[AKO], =[HQ], and =[ACT] sections of input file
// generates =[AKO-S], =[HQ-ER], =[HQ-EST], =[ACT-S], =[ACT-G], and =[ACT-D]
// assumes morphology patterns have already been loaded
// returns number of inversion errors (chk > 0) or negative for problem

int jhcMorphFcns::LexDeriv (const char *gram, int chk, const char *deriv)
{
  char fname[80] = "derived.sgm";
  const char *n2, *name = gram;
  FILE *in, *out;
  int pat, cnt = 0;

  // get short name of input grammar file
  if ((n2 = strrchr(name, '/')) != NULL)
    name = n2 + 1;
  if ((n2 = strrchr(name, '\\')) != NULL)
    name = n2 + 1;   

  // possibly load morphology from grammar file (only source)
  if (chk > 0)
  {
    pat = LoadExcept(gram, 0);
    jprintf("\nLoaded %d morphology patterns from: %s\n", pat, name);
    if (pat < 0)
      return -3;
  }

  // open files (possibly override default output name)
  if (fopen_s(&in, gram, "r") != 0)
    return -2;
  if ((deriv != NULL) && (*deriv != '\0'))
    strcpy_s(fname, deriv);
  if (fopen_s(&out, fname, "w") != 0)
    return -1;

  // produce derived forms
  fprintf(out, "// forms derived from grammar: %s\n", name);
  fprintf(out, "// ================================================\n\n");
  cnt += base2surf(out, in, JTAG_NPL, chk);
  cnt += base2surf(out, in, JTAG_ACOMP, chk);
  cnt += base2surf(out, in, JTAG_ASUP, chk);
  fprintf(out, "// -----------------------------------------\n\n");
  cnt += base2surf(out, in, JTAG_VPRES, chk);
  cnt += base2surf(out, in, JTAG_VPROG, chk);
  cnt += base2surf(out, in, JTAG_VPAST, chk);

  // cleanup
  fclose(out);
  fclose(in);
  if (chk > 0)
    jprintf("- Found %d inconsistent derived forms\n", cnt);
  return cnt;
}


//= For each base word of some category, write out a specific derived form.

int jhcMorphFcns::base2surf (FILE *out, FILE *in, UL32 tags, int chk) 
{
  char line[200];
  const char *val, *inv;
  int n = 0, err = 0;

  // go through file looking for noun category
  rewind(in);
  while (clean_line(line, 200, in) != NULL)
    if (base_sec(line, tags))
      while (clean_line(line, 200, in) != NULL)
      {
        // convert base word to desired surface form
        if (*line == '=')
          break;
        if (*line == '\0') 
          continue;
        if ((val = SurfWord(line, tags)) == NULL)
        {
          jprintf("  %s: %s -> (null) !\n", cat_txt(tags), line);
          err++;
          continue;
        }

        // write file line for entry in derived grammar category
        if (n++ <= 0)
          cat_hdr(out, tags);
        fprintf(out, "  %s\n", val);

        // possibly check if inverse is consistent
        if (chk > 0)
        {
          inv = BaseWord(val, tags);
          if ((inv != NULL) && (strcmp(inv, line) == 0))
            continue;
          jprintf("  %s: %s -> %s -> %s !\n", cat_txt(tags), line, val, inv);
          err++;
        }
      }
  if (n > 0)
    fprintf(out, "\n\n");
  return err;
}


//= Check if this line includes the grammar header for the appropriate base category.

bool jhcMorphFcns::base_sec (const char *line, UL32 tags) const
{
  if ((tags & JTAG_NOUN) != 0)
    return(strncmp(line, "=[AKO]", 6) == 0);
  if ((tags & JTAG_VERB) != 0)
    return(strncmp(line, "=[ACT]", 6) == 0);
  if ((tags & JTAG_ADJ) != 0)
    return(strncmp(line, "=[HQ]", 5) == 0);
  return false;
}


//= Write appropriate grammar category header for a particular derived kind.

void jhcMorphFcns::cat_hdr (FILE *out, UL32 tags) const
{
  const char *cat = cat_txt(tags);

  if ((tags & JTAG_NPL) != 0)
    fprintf(out, "// plural noun (%s)\n\n=[AKO-S]\n", cat);
  else if ((tags & JTAG_ACOMP) != 0)
    fprintf(out, "// comparative adjectives (%s)\n\n=[HQ-ER]\n", cat);
  else if ((tags & JTAG_ASUP) != 0)
    fprintf(out, "// superlative adjectives (%s)\n\n=[HQ-EST]\n", cat);
  else if ((tags & JTAG_VPRES) != 0)
    fprintf(out, "// present verb (%s)\n\n=[ACT-S]\n", cat);
  else if ((tags & JTAG_VPROG) != 0)
    fprintf(out, "// progressive verb (%s)\n\n=[ACT-G]\n", cat);
  else if ((tags & JTAG_VPAST) != 0)
    fprintf(out, "// past or passive verb (%s)\n\n=[ACT-D]\n", cat);
}


//= Generate a list of base words from a derived lexicon file.
// should check to make sure no weird stemming problems occur
// looks for =[AKO-S], =[HQ-ER], =[HQ-EST], =[ACT-S], =[ACT-G], and =[ACT-D]
// assumes morphology patterns have already been loaded
// returns number of inversion errors (chk > 0) or negative for problem

int jhcMorphFcns::LexBase (const char *deriv, int chk, const char *morph) 
{
  char fname[80] = "base_words.txt";
  const char *n2, *name = morph;
  FILE *in, *out;
  int pat, cnt = 0;

  // possibly load morphology from grammar file (only source)
  if ((morph != NULL) && (*morph != '\0'))
  {
    pat = LoadExcept(morph, 0);
    if ((n2 = strrchr(name, '/')) != NULL)
      name = n2 + 1;
    if ((n2 = strrchr(name, '\\')) != NULL)
      name = n2 + 1;    
    jprintf("\nLoaded %d morphology patterns from: %s\n", pat, name);
    if (pat < 0)
      return -2;
  }

  // open files
  if (fopen_s(&in, deriv, "r") != 0)
    return -1;
  if (fopen_s(&out, fname, "w") != 0)
    return 0;

  // produce derived forms
  cnt += surf2base(out, in, JTAG_NPL, chk);
  cnt += surf2base(out, in, JTAG_ACOMP, chk);
  cnt += surf2base(out, in, JTAG_ASUP, chk);
  cnt += surf2base(out, in, JTAG_VPRES, chk);
  cnt += surf2base(out, in, JTAG_VPROG, chk);
  cnt += surf2base(out, in, JTAG_VPAST, chk);

  // cleanup
  fclose(out);
  fclose(in);
  if (chk > 0)
    jprintf("- Found %d inconsistent base forms\n", cnt);
  return cnt;
}


//= For each surface word of a particular category, write out its base form.

int jhcMorphFcns::surf2base (FILE *out, FILE *in, UL32 tags, int chk) 
{
  char line[200];
  const char *val, *inv;
  int n = 0, err = 0;

  // go through file looking for noun category
  rewind(in);
  while (clean_line(line, 200, in) != NULL)
    if (surf_sec(line, tags))
      while (clean_line(line, 200, in) != NULL)
      {
        // convert base noune to plural form
        if (*line == '=')
          break;
        if (*line == '\0')
          continue;
        if ((val = BaseWord(line, tags)) == NULL)
        {
          jprintf("  %s: %s -> (null) !\n", cat_txt(tags), line);
          err++;
          continue;
        }

        // write file line
        if (n++ <= 0)
          deriv_hdr(out, tags);
        fprintf(out, "  %s%*s<- %s\n", val, 20 - strlen(val), "", line);

        // possibly check inverse
        if (chk > 0)
        {
          inv = SurfWord(val, tags);
          if ((inv != NULL) && (strcmp(inv, line) == 0))
            continue;
          jprintf("  %s: %s -> %s -> %s !\n", cat_txt(tags), line, val, inv);
          err++;
        }
      }
  if (n > 0)
    fprintf(out, "\n\n");
  return err;
}


//= Check if this line includes the grammar header for the appropriate surface category.

bool jhcMorphFcns::surf_sec (const char *line, UL32 tags) const
{
  if ((tags & JTAG_NPL) != 0)
    return(strncmp(line, "=[AKO-S]", 8) == 0);
  if ((tags & JTAG_ACOMP) != 0)
    return(strncmp(line, "=[HQ-ER]", 8) == 0);
  if ((tags & JTAG_ASUP) != 0)
    return(strncmp(line, "=[HQ-EST]", 9) == 0);
  if ((tags & JTAG_VPRES) != 0)
    return(strncmp(line, "=[ACT-S]", 8) == 0);
  if ((tags & JTAG_VPROG) != 0)
    return(strncmp(line, "=[ACT-G]", 8) == 0);
  if ((tags & JTAG_VPAST) != 0)
    return(strncmp(line, "=[ACT-D]", 8) == 0);
  return false;
}


void jhcMorphFcns::deriv_hdr (FILE* out, UL32 tags) const
{
  const char *cat = cat_txt(tags);

  if ((tags & JTAG_NPL) != 0)
    fprintf(out, "// nouns from plurals (%s)\n", cat);
  else if ((tags & JTAG_ACOMP) != 0)
    fprintf(out, "// adjectives from comparatives (%s)\n", cat);
  else if ((tags & JTAG_ASUP) != 0)
    fprintf(out, "// adjectives from superlatives (%s)\n", cat);
  else if ((tags & JTAG_VPRES) != 0)
    fprintf(out, "// verbs from present tense (%s)\n", cat);
  else if ((tags & JTAG_VPROG) != 0)
    fprintf(out, "// verbs from progressive tense (%s)\n", cat);
  else if ((tags & JTAG_VPAST) != 0)
    fprintf(out, "// verbs from past tense (%s)\n", cat);
}


//= Get the standard text name (first one only) associated with a particular kind.

const char *jhcMorphFcns::cat_txt (UL32 tags) const
{
  int i;

  for (i = 0; i < JTV_MAX; i++)
    if ((tags & (0x01 << i)) != 0)
      return JTAG_STR[i];
  return NULL;
}

