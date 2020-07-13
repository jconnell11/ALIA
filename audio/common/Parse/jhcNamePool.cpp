// jhcNamePool.cpp : collection of name data for specific people
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017 IBM Corporation
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

#include "Parse/jhcNamePool.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcNamePool::~jhcNamePool ()
{
}


//= Default constructor initializes certain values.

jhcNamePool::jhcNamePool ()
{
  ClrAll();
}


///////////////////////////////////////////////////////////////////////////
//                            Ingest Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get rid of all people in the database.

void jhcNamePool::ClrAll ()
{
  int id;

  for (id = 0; id < pmax; id++)
    ClrPerson(id);
  np = 0;
  qcnt = 0;
}


//= Erase entry for a particular person.
// this ID number is then available for potential re-use (see AddPerson)

void jhcNamePool::ClrPerson (int id)
{
  if ((id < 0) || (id >= pmax))
    return;

  title[id][0] = '\0';
  first[id][0] = '\0';
  nick[id][0]  = '\0';
  last[id][0]  = '\0';
  recent[id] = 0;
}


//= Add a new person using fully combined name string.
// if recycle <= then assigns a new ID number, else might re-use old one
// words:   0     1       2      3 
//         Dr. Jonathan (Jon) Connell
// returns id if successful, negative for some problem

int jhcNamePool::AddPerson (const char *tag, int recycle)
{
  int n, id = np, next = 0;

  // possibly find earliest empty entry
  if (recycle > 0)
    for (id = 0; id < np; id++)
      if (empty(id))
        break;

  // otherwise add to end of list (if room)
  if (id >= np)
  {
    if (np >= pmax)
      return -3;
    np++;
  }

  // digest input tag into word array
  if ((n = get_words(tag)) < 1)
    return -2;

  // check for optional title 
  if (is_title(word[next]))
  {
    // must be followed by something
    if (--n < 1)
    {
      // possibly deallocate ID
      if (id == (np - 1))
        np--;               
      return -1;
    }

    // save title and check for full form: Dr. Connell
    strcpy_s(title[id], word[next++]);
    if (n == 1)
    {
      strcpy_s(last[id], word[next]);
      return id;
    }
  }

  // get required first name
  strcpy_s(first[id], word[next++]);
  if (--n < 1)
    return id;

  // remove nickname if in parems
  if (grab_nick(id, word[next]))
  {
    next++;
    if (--n < 1)
      return id;
  }

  // final word is last name
  strcpy_s(last[id], word[next]);
  return id;
}


//= No strings associated with this ID.

bool jhcNamePool::empty (int id) const
{
  return((id < 0) || (id >= np) || 
         ((title[id][0] == '\0') && (first[id][0] == '\0') && 
          (nick[id][0] == '\0') && (last[id][0] == '\0')));
}


//= Break reference into tokens stored in internal "word" array.
// returns number of words found (also in wcnt variable)

int jhcNamePool::get_words (const char *ref)
{
  const char *end, *start = ref;
  int i, n;

  // clear word array and count
  for (i = 0; i < 4; i++)
    word[i][0] = '\0';
  wcnt = 0;

  // break string into words (ignore beyond 4th word)
  for (i = 0; i < 4; i++)
  {
    // strip leading whitespace (if any)
    while (strchr(" \t", *start) != NULL)
      start++;
    if (*start == '\0')
      break;

    // find ending space (if any) or newline marker
    if ((end = strchr(start, ' ')) == NULL)
      if ((end = strchr(start, '\t')) == NULL)
        end = strchr(start, '\n');
    if (end != NULL)
      n = (int)(end - start);
    else
      n = (int) strlen(start);

    // copy out string portion (capitalize first letter)
    if (n <= 0)
      break;
    strncpy_s(word[i], start, n);              // always adds terminator
    word[i][0] = toupper(word[i][0]);
    wcnt++;

    // see if done (check newline for fgets)
    if ((end == NULL) || (*end == '\n'))
      break;
    start = end;
  }
  return wcnt;
}


//= If some string is a known honorific, copy it to the title field.

bool jhcNamePool::is_title (const char *word) const
{
  char addr[6][20] = {"Mr.", "Mrs.", "Ms.", "Miss", "Dr.", "Prof."};
  int i;

  for (i = 0; i < 6; i++)
    if (_stricmp(word, addr[i]) == 0)
      return true;
  return false;
}


//= If some string is enclosed in parentheses, copy it to the nickname field.

bool jhcNamePool::grab_nick (int id, const char *word) 
{
  int n;

  if (*word != '(')
    return false;
  n = (int) strlen(word + 1);
  if (word[n] == ')')
    n--;
  strncpy_s(nick[id], word + 1, n);          // always adds terminator
  return true;
}


//= Total number of VALID people in list (skips blank entries).

int jhcNamePool::CountPeople () const
{
  int id, n = 0; 

  for (id = 0; id < np; id++)
    if (!empty(id))
      n++;
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Determine how many people are consistent with the partial reference string.

int jhcNamePool::NumMatch (const char *ref)
{
  int id, n = 0; 

  get_words(ref);
  for (id = 0; id < np; id++)
    if (consistent(id))
      n++;
  return n;
}


//= See if referring phrase sufficiently matches the fields of a person.
// reference pre-digested in "word" array must be one of 7 standard styles:
// <pre>
//    Jon          Jon Connell         Dr. Jon Connell        Dr. Connell
//    Jonathan     Jonathan Connell    Dr. Jonathan Connell
// </pre>

bool jhcNamePool::consistent (int id) const
{
  // forms: Jon, Jonathan
  if ((wcnt == 1) && 
      ((_stricmp(word[0], nick[id]) == 0) || (_stricmp(word[0], first[id]) == 0)))
    return true;

  // forms: Jonathan Connell, Jon Connell
  if ((wcnt == 2) && 
      (((_stricmp(word[0], first[id]) == 0) || (_stricmp(word[0], nick[id]) == 0)) && 
       (_stricmp(word[1], last[id]) == 0)))
    return true;

  // form: Dr. Connell (or "Dr. Connell" matching to record <no title> "Connell")
  if ((wcnt == 2) && 
      ((_stricmp(word[0], title[id]) == 0) || (is_title(word[0]) && (title[id][0] == '\0'))) &&
      (_stricmp(word[1], last[id]) == 0))
    return true;

  // form: Dr. Jonathan Connell or Dr. Jon Connell (also matches if no title in record)
  if ((wcnt == 3) &&
      ((_stricmp(word[0], title[id]) == 0) || (is_title(word[0]) && (title[id][0] == '\0'))) &&
      ((_stricmp(word[1], first[id]) == 0) || (_stricmp(word[1], nick[id]) == 0)) &&
      (_stricmp(word[2], last[id]) == 0))
    return true;
  return false;
}


//= Get one of several person IDs consistent with the reference string.
// choices follow order in internal list, returns negative for problem

int jhcNamePool::PossibleID (const char *ref, int choice)
{
  int id, n = 0;

  if (choice < 0)
    return -2;
  get_words(ref);
  for (id = 0; id < np; id++)
    if (consistent(id))
      if (n++ == choice)
        return id;
  return -1;
}


//= Get best person ID consistent with the reference string.
// chooses entry most recently referenced, returns negative for problem

int jhcNamePool::PersonID (const char *ref)
{
  int id, win = -1, best = -1;

  // find most recently mentioned possibility
  get_words(ref);
  for (id = 0; id < np; id++)
    if (consistent(id))
      if (recent[id] > best) 
      {
        win = id;
        best = recent[id];
      }

  // update recency marker
  if (win < 0)
    return -1;
  recent[win] = ++qcnt;
  return win;
}


//= Get a short name to use for a particular person.
// this is typically used when addressing the person via TTS
// returns empty string if could not create something reasonable
// Note: uses local string which might get re-written so consider copying

const char *jhcNamePool::ShortName (int id)
{
  // set default then check for valid person
  *tmp = '\0';
  if ((id < 0) || (id >= np))
    return tmp;

  // return nickname or firstname if known
  if (nick[id][0] != '\0')    
    return nick[id];             // form: Jon
  if (first[id][0] != '\0')   
    return first[id];            // form: Jonathan

  // possibly return form: Dr. Connell
  if ((title[id][0] != '\0') && (last[id][0] != '\0'))
    sprintf_s(tmp, "%s %s", title[id], last[id]);
  return tmp;
}


//= Get full name for a particular person to help disambiguation.
// this is typically used for switching to a custom acoustic model
// returns empty string if could not create something reasonable 
// Note: uses local string which might get re-written so consider copying

const char *jhcNamePool::LongName (int id)
{
  char *spec = NULL;

  // set default then make sure lastname is known
  *tmp = '\0';
  if ((id < 0) || (id >= np) || (last[id][0] == '\0'))
    return tmp;

  // choose first word  
  if ((nick[id][0] != '\0') && (_stricmp(nick[id], last[id]) != 0))  // form: Jon Connell
    spec = nick[id];
  else if (first[id][0] != '\0')                                     // form: Jonathan Connell
    spec = first[id];
  else if (title[id][0] != '\0')                                     // form: Dr. Connell
    spec = title[id];

  // make up answer string (if any possible)
  if (spec != NULL)
    sprintf_s(tmp, "%s %s", spec, last[id]);
  return tmp;
}


//= Get an official name for a person (ignoring nickname).
// this is typically used for looking up people in directories
// returns empty string if could not create something reasonable 
// Note: uses local string which might get re-written so consider copying

const char *jhcNamePool::FormalName (int id)
{
  *tmp = '\0';
  if ((id < 0) || (id >= np) || (last[id][0] == '\0'))
    return tmp;
  if (first[id][0] == '\0')
    return last[id];                              // form: Connell
  sprintf_s(tmp, "%s %s", first[id], last[id]);   // form: Jonathan Connell
  return tmp;
}


//= Combine name pieces into a single string (possibly for saving).
// returns NULL if invalid ID, else form: Dr. Jonathan (Jon) Connell
// Note: uses local string which might get re-written so consider copying

const char *jhcNamePool::Condense (int id)
{
  // check for valid person entry
  if (empty(id))
    return NULL;
  *tmp = '\0';

  // title and first name
  if (title[id][0] != '\0')
  {
    strcat_s(tmp, title[id]);
    strcat_s(tmp, " ");
  }
  if (first[id][0] != '\0')
  {
    strcat_s(tmp, first[id]);
    strcat_s(tmp, " ");
  }

  // optional nick name in parentheses then last name
  if (nick[id][0] != '\0')
  {
    strcat_s(tmp, "(");
    strcat_s(tmp, nick[id]);
    strcat_s(tmp, ") ");
  }
  if (last[id][0] != '\0')
  {
    strcat_s(tmp, last[id]);
    strcat_s(tmp, " ");
  }

  // strip last space (always superfluous)
  if (*tmp != '\0')
    tmp[strlen(tmp) - 1] = '\0';
  return tmp;
}


///////////////////////////////////////////////////////////////////////////
//                          Enumeration Functions                        //
///////////////////////////////////////////////////////////////////////////

//= For a particular person ID, get the nth variant of their name.
// useful for dynamically writing grammar rules (i.e. no file)
// returns NULL if variant is invalid (e.g. title is unknown)
// Note: uses local string which might get re-written so consider copying

const char *jhcNamePool::GetVariant (int id, int n)
{
  if ((id < 0) || (id >= np))
    return NULL;

  // 0 = Jon
  if (n == 0)
    return((nick[id][0] == '\0') ? NULL : nick[id]);

  // 1 = Jon Connell
  if (n == 1)
  {
    if ((nick[id][0] == '\0') || (last[id][0] == '\0') || (_stricmp(nick[id], last[id]) == 0))
      return NULL;
    sprintf_s(tmp, "%s %s", nick[id], last[id]);
    return tmp;
  }

  // 2 = Jonathan
  if (n == 2)
    return((first[id][0] == '\0') ? NULL : first[id]);

  // 3 = Jonathan Connell
  if (n == 3)
  {
    if ((first[id][0] == '\0') || (last[id][0] == '\0'))
      return NULL;
    sprintf_s(tmp, "%s %s", first[id], last[id]);
    return tmp;
  }

  // 4 = Dr. Connell
  if (n == 4)
  {
    if ((title[id][0] == '\0') || (last[id][0] == '\0'))
      return NULL;
    sprintf_s(tmp, "%s %s", title[id], last[id]);
    return tmp;
  }

  // 5 = Dr. Jonathan Connell
  if (n == 5)
  {
    if ((title[id][0] == '\0') || (first[id][0] == '\0') || (last[id][0] == '\0'))
      return NULL;
    sprintf_s(tmp, "%s %s %s", title[id], first[id], last[id]);
    return tmp;
  }
  return NULL;
}


//= Enumerate all name variants (without repeats) to "list" array.
// returns the number of entries with valid information

int jhcNamePool::AllVars ()
{
  const char *form;
  int id, v, i, n = 0;

  // find all valid entries
  for (id = 0; id < np; id++)
    if (!empty(id))
      for (v = 0; v < 6; v++)
        if ((form = GetVariant(id, v)) != NULL)
        {
          // copy variant to list if not already there
          for (i = 0; i < n; i++)
            if (_stricmp(form, list[i]) == 0)
              break;
          if (i >= n)
            strcpy_s(list[n++], form);
        }
  return n;
}


//= Enumerate all nicknames and firnames (without repeats) to "list" array.
// can optionally make all string possessive (adds 's at end)
// returns the number of entries with valid information

int jhcNamePool::AllFirst (int poss)
{
  char form[40];
  int id, i, n = 0;

  // find all valid entries
  for (id = 0; id < np; id++)
    if (!empty(id))
    {
      // copy nickname to list if not already there
      if (nick[id][0] != '\0')
      {
        sprintf_s(form, "%s%s", nick[id], ((poss > 0) ? "'s" : ""));
        for (i = 0; i < n; i++)
          if (_stricmp(form, list[i]) == 0)
            break;
        if (i >= n)
          strcpy_s(list[n++], form);
      }

      // copy firstname to list if not already there
      if (first[id][0] != '\0')
      {
        sprintf_s(form, "%s%s", first[id], ((poss > 0) ? "'s" : ""));
        for (i = 0; i < n; i++)
          if (_stricmp(form, list[i]) == 0)
            break;
        if (i >= n)
          strcpy_s(list[n++], form);
      }
    }
  return n;
}


//= Enumerate all lastnames (without repeats) to "list" array.
// can optionally make all string possessive (adds 's at end)
// returns the number of entries with valid information

int jhcNamePool::AllLast (int poss)
{
  char form[40];
  int id, i, n = 0;

  // find all valid entries
  for (id = 0; id < np; id++)
    if (!empty(id))
      if (last[id][0] != '\0')
      {
        // copy lastname to list if not already there
        sprintf_s(form, "%s%s", last[id], ((poss > 0) ? "'s" : ""));
        for (i = 0; i < n; i++)
          if (_stricmp(form, list[i]) == 0)
            break;
        if (i >= n)
          strcpy_s(list[n++], form);
      }
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                             File Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Make person entries for a bunch of condensed names in a file.
// can optionally append to list of people already in database
// returns number of people added, negative for error

int jhcNamePool::Load (const char *fname, int clr)
{
  char line[200];
  FILE *in;
  int n = 0;

  if (clr > 0)
    ClrAll();
  if (fopen_s(&in, fname, "r") != 0)
    return -1;
  while (fgets(line, 200, in) != NULL)
    if (strncmp(line, "//", 2) != 0)      // must be in first column
      if (AddPerson(line) >= 0)
        n++;
  fclose(in);
  return n;
}


//= Write a file with the condensed entries for all know people.
// returns number of people saved, negative for error
// Note: reloading this might change the person IDs (if some blank entries)

int jhcNamePool::Save (const char *fname)
{
  FILE *out;
  int id, n = 0;

  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  for (id = 0; id < np; id++)
    if (!empty(id))
    {
      fprintf(out, "%s\n", Condense(id));
      n++;
    }
  fclose(out);
  return n;
}


//= Write a grammar category file with all variants of all peoples names.
// name of category defaults to NAME unless different "cat" is given
// returns number of variants saved, negative for error

int jhcNamePool::SaveVars (const char *fname, const char *cat)
{
  FILE *out;
  int i, n;

  // try opening file
  if (fopen_s(&out, fname, "w") != 0)
    return -1;

  // dump all forms of people's names
  n = AllVars();
  fprintf(out, "=[%s]\n", ((cat != NULL) ? cat : "NAME"));
  for (i = 0; i < n; i++)
    fprintf(out, "  %s\n", list[i]);

  // clean up
  fclose(out);
  return n;     
}


//= Save a partial grammar file with firstnames and last names.
// also saves possessive forms of each as separate categories
// suggest using with (overly permissive) grammar:
// <pre>
//   =[NAME]
//     <gname>
//     (<title>) <gname> <fname>
//     <title> <fname>
//
//   =[NAME_P]
//     <gname_p>
//     (<title>) <gname> <fname_p>
//     <title> <fname_p>
//
//   =[title]
//     Mr.
//     Mrs.
//     Ms.
//     Miss
//     Dr.
//     Prof.
// </pre>
// returns 1 if successful, 0 for problem

int jhcNamePool::SaveParts (const char *fname)
{
  FILE *out;
  int i, n;

  // try opening file
  if (fopen_s(&out, fname, "w") != 0)
    return 0;

  // normal firstnames (no repeats)
  n = AllFirst(0);
  fprintf(out, "// given (first) names of standard users\n\n");
  fprintf(out, "=[name-given]\n");
  for (i = 0; i < n; i++)
    fprintf(out, "  %s\n", list[i]);
  fprintf(out, "\n\n");

  // possessive forms of first names
  n = AllFirst(1);
  fprintf(out, "// possessive given (first) names\n\n");
  fprintf(out, "=[name-given-p]\n");
  for (i = 0; i < n; i++)
    fprintf(out, "  %s\n", list[i]);
  fprintf(out, "\n\n");

  // lastnames (no repeats)
  n = AllLast(0);
  fprintf(out, "// family (last) names of standard users\n\n");
  fprintf(out, "=[name-family]\n");
  for (i = 0; i < n; i++)
    fprintf(out, "  %s\n", list[i]);
  fprintf(out, "\n\n");

  // possessive forms of first names
  n = AllLast(1);
  fprintf(out, "// possessive family (last) names\n\n");
  fprintf(out, "=[name-family-p]\n");
  for (i = 0; i < n; i++)
    fprintf(out, "  %s\n", list[i]);
  fprintf(out, "\n\n");

  // cleanup
  fclose(out);
  return 1;
}

