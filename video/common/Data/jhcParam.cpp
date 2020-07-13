// jhcParam.cpp : simple manipulation of parameters
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
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
#include <stdarg.h>
#include <string.h>

#include "Data/jhcParam.h"
#include "Interface/jhcMessage.h"


///////////////////////////////////////////////////////////////////////////

/* CPPDOC_BEGIN_EXCLUDE */

// Length of descriptive string for each parameter entry.

const int NAMLEN = 80;  

/* CPPDOC_END_EXCLUDE */


///////////////////////////////////////////////////////////////////////////
//                  Construction and Destruction                         //
///////////////////////////////////////////////////////////////////////////

//= Free whatever space was allocated.

jhcParam::~jhcParam ()
{
  dealloc_param();
}


//= Create a list of 8 entries (standard length for jhcPickVals).

jhcParam::jhcParam ()
{
  init_param();
  SetSize(8);
}


//= Create a list of blank entries.

jhcParam::jhcParam (int n)
{
  init_param();
  SetSize(n);
}


//= Allocate various arrays.

void jhcParam::SetSize (int n)
{
  // sanity check
#ifdef _DEBUG
  if ((n <= 0) || (n > 100))
    Pause("Trying to allocate %d parameters!", n);
#endif 

  // reallocate if different size requested
  if (n != total)
  {
    if (total > 0)
      dealloc_param();

    locks = new int [n];
    ldefs = new int [n];
    fdefs = new double [n];
    lvals = (int **) new char [n * sizeof(int *)];
    fvals = (double **) new char [n * sizeof(double *)];
    names = (char *) new char[n * NAMLEN];
    total = n;
  }
  ClearAll();
}


//= Free all arrays.

void jhcParam::dealloc_param ()
{
  if (locks != NULL)
    delete [] locks;
  if (ldefs != NULL)
    delete [] ldefs;
  if (fdefs != NULL)
    delete [] fdefs;
  if (lvals != NULL)
    delete [] lvals;
  if (fvals != NULL)
    delete [] fvals;
  if (names != NULL)
    delete [] names;
  init_param();
}


//= Default values for memeber variables.

void jhcParam::init_param ()
{
  total = 0;
  next  = 0;
  lbad = 0;
  fbad = 0.0;
  locks = NULL;
  ldefs = NULL;
  fdefs = NULL;
  lvals = NULL;
  fvals = NULL;
  names = NULL;
  *tag = '\0';
  *title = '\0';
}


///////////////////////////////////////////////////////////////////////////
//                            List Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Duplicate all fields from another list.
// helps adjust multiple instances of a class using a single dialog box
// this duplicates default values and current parameter values
// it does not copy the actual pointers to the variables
// also does not mess with "tag" or "title" strings

void jhcParam::CopyAll (const jhcParam& src)
{
  int i, n = __min(total, src.total);

  for (i = 0; i < n; i++)
  {
    locks[i] = (src.locks)[i];
    if ((lvals[i] != NULL) && ((src.lvals)[i] != NULL))
      *(lvals[i]) = *((src.lvals)[i]);
    ldefs[i] = (src.ldefs)[i];
    if ((fvals[i] != NULL) && ((src.fvals)[i] != NULL))
      *(fvals[i]) = *((src.fvals)[i]);
    fdefs[i] = (src.fdefs)[i];
    strcpy0(names + NAMLEN * i, src.names + NAMLEN * i, NAMLEN);
  }
  for (i = n; i < total; i++)
  {
    lvals[i] = NULL;
    fvals[i] = NULL;
  }
  next = n;
}


//= Fill self with non-information, reset fill pointer to begining.

void jhcParam::ClearAll ()
{
  int i;

  for (i = 0; i < total; i++)
  {
    locks[i] = 0;
    lvals[i] = NULL;
    fvals[i] = NULL;
    ldefs[i] = 0;
    fdefs[i] = 0.0;
    *(names + NAMLEN * i) = '\0';
  }
  next = 0;
}


//= Set original variable (wherever it is) to default value.
// NOTE: have to be careful to avoid dangling pointers 
// (e.g. pointing to part of a structure which has been deleted)

void jhcParam::RevertAll ()
{
  int i;

  for (i = 0; i < total; i++)
    if (locks[i] == 0)
    {
      if (lvals[i] != NULL)
        *(lvals[i]) = ldefs[i];
      else if (fvals[i] != NULL)
        *(fvals[i]) = fdefs[i];
    }
}


///////////////////////////////////////////////////////////////////////////
//                            File Operations                            //
///////////////////////////////////////////////////////////////////////////

//= Record file line identifier string.
// Can also clear all entries at same time (common sequence).

void jhcParam::SetTag (const char *token, int no_clr)
{
  strcpy0(tag, token, 40);
  if (no_clr <= 0)
    ClearAll();
}


//= Record title to use on dialog box.
// can send in format string and arguments like printf

void jhcParam::SetTitle (const char *label, ...)
{
  va_list args;

  va_start(args, label); 
  vsprintf_s(title, label, args);
  title[79] = '\0';
}


//= Store to a file where "alt_tag" is first part of text line.
// If no string given, value of internal "tag" string is used instead.
// If prefix is positive, alt_tag is concatenated with the original tag.
// Returns 1 if okay, zero if failed somehow.

int jhcParam::SaveVals (const char *file_name, const char *alt_tag, int prefix) const
{
  char frag[80], tail[200] = "", mix[40] = "";
  const char *key = mix;
  int i;

  // determine line identifier to use
  if ((alt_tag == NULL) || (*alt_tag == '\0'))
    key = tag;
  else if ((prefix <= 0) || (tag == NULL) || (*tag == '\0'))
    key = alt_tag;
  else 
    sprintf_s(mix, "%s%s", alt_tag, tag);
  if ((key == NULL) || (*key == '\0'))
    return 0;

  // remove any current line that matches enhanced tag
  if (excise_line(file_name, key) <= 0)
    return 0;
    
  // compose line with values of parameters
  for (i = 0; i < total; i++)
    if (lvals[i] != NULL)
    {
      sprintf_s(frag, "%ld ", *(lvals[i]));
      strcat_s(tail, frag);
    }
    else if (fvals[i] != NULL)
    {
//      sprintf_s(frag, "%.17g ", *(fvals[i]));  // was %f
      sprintf_s(frag, "%.4g ", *(fvals[i])); 
      strcat_s(tail, frag);
    }

  // save values after enhanced tag
  if (add_line(file_name, key, tail) <= 0)
    return 0;
  return 1;
}


//= Removes line associated with these parameters from file.

int jhcParam::RemVals (const char *file_name, const char *alt_tag, int prefix) const
{
  char mix[40] = "";
  const char *key = mix;

  // determine line identifier to use
  if ((alt_tag == NULL) || (*alt_tag == '\0'))
    key = tag;
  else if ((prefix <= 0) || (tag == NULL) || (*tag == '\0'))
    key = alt_tag;
  else 
    sprintf_s(mix, "%s%s", alt_tag, tag);
  if ((key == NULL) || (*key == '\0'))
    return 0;

  // remove any current line that matches enhanced tag
  if (excise_line(file_name, key) <= 0)
    return 0;
  return 1;
}


//= Retrieve from file using "tag" to parse correct line.
// If no string given, value of internal "tag" string is used instead.
// If prefix is positive, alt_tag is concatenated with the original tag.
// Returns 1 if okay or NULL file_name, 0 if file failure.

int jhcParam::LoadDefs (const char *file_name, const char *alt_tag, int prefix)
{
  char line[200], mix[40] = "";
  const char *key = mix;
  char *ntk = line;
  char **src = &ntk;
  int ans;

  // if no file name then no load was intended
  if (file_name == NULL)
    return 1;

  // determine line identifier to use
  if ((alt_tag == NULL) || (*alt_tag == '\0'))
    key = tag;
  else if ((prefix <= 0) || (tag == NULL) || (*tag == '\0'))
    key = alt_tag;
  else 
    sprintf_s(mix, "%s%s", alt_tag, tag);
  if ((key == NULL) || (*key == '\0'))
    return 0;

  // open file and look for first line with the specified tag then skip over tag
  // some cases used to return 0 since specified tag not found or no data present
  if ((ans = find_line(line, file_name, key)) < 0)
    return 0;
  if ((ans == 0) || (next_token(src, " \t\n") == NULL))
    return 1;

  // extract default values for this array of parameters
  return SetDefsTxt(*src);
}


//= Fill default parameter values from a line of text.
// convenient for setting defaults to match line from configuration file
// returns 1 if all values found, 0 if some missed

int jhcParam::SetDefsTxt (char *line)
{
  char *rest, *ntk = line;
  char **src = &ntk;
  int i, needed = 0, filled = 0;

  // figure out how many values are required
  for (i = 0; i < total; i++)
    if ((lvals[i] != NULL) || (fvals[i] != NULL))
      needed++;

  // fills in until first incompatible match or end of line
  for (i = 0; i < total; i++)
  {
    // find next value in line
    if ((rest = next_token(src, " \t\n")) == NULL)
      break;

    // find next entry to fill in
    while ((lvals[i] == NULL) && (fvals[i] == NULL))
      if (++i >= total)
        break;
    if (i >= total)
      break;

    // attempt to put value into entry (be careful with doubles)
    if (lvals[i] != NULL)
    {
      if (sscanf_s(rest, "%ld", ldefs + i) != 1)
        break;
    }
    else if (fvals[i] != NULL)
    {
      if (sscanf_s(rest, "%lf", fdefs + i) != 1)  // was %f into a float
        break;
    }
    filled++;
  }

  // see if all parameters found
  return((filled < needed) ? 0 : 1);
}


//= Safer form of strtok function that allows call nesting.
// alters input to point to remainder of original string (NULL when done)
// returns pointer to found token (original string is altered with '\0')

char *jhcParam::next_token (char **src, const char *delim) const
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
//                           Entry Loading                               //
///////////////////////////////////////////////////////////////////////////

//= Create a int entry and possibly change name.
//   (equivalent to an int under Microsoft)
// Advances "next" pointer to directly after this entry.

void jhcParam::SetSpec4 (int i, int *loc, int init, const char *descr)
{
  if ((i < 0) || (i >= total))
    return;

  locks[i] = 0;
  lvals[i] = ((loc != NULL) ? loc : &lbad);
  fvals[i] = NULL;

  ldefs[i] = init;
  if (descr != NULL)
    strcpy0(names + NAMLEN * i, descr, NAMLEN);
  next = i + 1;
} 


//= Create a floating point entry and possibly change name.
// Advances "next" pointer to directly after this entry.

void jhcParam::SetSpecF (int i, double *loc, double init, const char *descr)
{
  if ((i < 0) || (i >= total))
    return;

  locks[i] = 0;
  lvals[i] = NULL;
  fvals[i] = ((loc != NULL) ? loc : &fbad);

  fdefs[i] = init;
  if (descr != NULL)
    strcpy0(names + NAMLEN * i, descr, NAMLEN);
  next = i + 1;
} 


//= Like SetSpec4 but index generated automatically.

void jhcParam::NextSpec4 (int *loc, int init, const char *descr)
{
  if (next < total)
    SetSpec4(next++, loc, init, descr);
}


//= Like SetSpecF but index generated automatically.

void jhcParam::NextSpecF (double *loc, double init, const char *descr)
{
  if (next < total)
    SetSpecF(next++, loc, init, descr);
}


//= Protected version of strcpy that truncates if necessary.
// will not overflow buffer and always null terminates

char *jhcParam::strcpy0 (char *dest, const char *src, int dest_size) const
{
  strncpy_s(dest, dest_size, src, dest_size - 2);    // always adds terminator
  return dest;
}


///////////////////////////////////////////////////////////////////////////
//                      Alter Selected Entries                           //
///////////////////////////////////////////////////////////////////////////

//= Look for first entry with lval pointer equal to given loc.
// Returns 0 if none found, else sets entry and default to given value.

int jhcParam::SetMatch (int *loc, int v)
{
  int i;

  for (i = 0; i < total; i++)
    if (lvals[i] == loc)
    {
      if (locks[i] != 0)
        return -1;
      *(lvals[i]) = v;
      ldefs[i]    = v;
      return 1;
    }
  return 0;
}


//= Like other SetMatch but for floating point value.

int jhcParam::SetMatch (double *loc, double v)
{
  int i;

  for (i = 0; i < total; i++)
    if (fvals[i] == loc)
    {
      if (locks[i] != 0)
        return -1;
      *(fvals[i]) = v;
      fdefs[i]    = v;
      return 1;
    }
  return 0;
}


//= Look for first entry with lval pointer equal to given loc.
// Returns 0 if none found, else scales entry by given factor.
// Can force a number to be even or odd if requested,
//   otherwise rounds to closest integer of either kind.

int jhcParam::ScaleMatch (int *loc, double f, int odd)
{
  double hf = 0.5 * f;
  int i, vn, vf, dv = ((f < 1.0) ? 1 : -1);

  for (i = 0; i < total; i++)
    if (lvals[i] == loc)
    {
      if (locks[i] != 0)
        return -1;
      if (odd > 0)
      {
//        vn = ((int)(*(lvals[i]) * hf) << 1) + dv;
//        vf = ((int)(  ldefs[i]  * hf) << 1) + dv;
        vn = ((int)((*(lvals[i]) - 1) * hf + 0.5) << 1) + 1;
        vf = ((int)((  ldefs[i]  - 1) * hf + 0.5) << 1) + 1;

        *(lvals[i]) = __max(1, vn);
        ldefs[i]    = __max(1, vf);
      }
      else if (odd < 0)
      {
        vn = (int)((*(lvals[i]) + dv) * hf) << 1;
        vf = (int)((  ldefs[i]  + dv) * hf) << 1;
        *(lvals[i]) = __max(2, vn);
        ldefs[i]    = __max(2, vf);
      }
      else
      {
        vn = (int)(*(lvals[i]) * f + 0.5);
        vf = (int)(  ldefs[i]  * f + 0.5);
        *(lvals[i]) = __max(1, vn);
        ldefs[i]    = __max(1, vf);
      }
      return 1;
    }
  return 0;
}


//= Same as other ScaleMatch but for a float pointer.

int jhcParam::ScaleMatch (double *loc, double f)
{
  int i;

  for (i = 0; i < total; i++)
    if (fvals[i] == loc)
    {
      if (locks[i] != 0)
        return -1;
      *(fvals[i]) *= f;
      fdefs[i]    *= f;
      return 1;
    } 
  return 0;
}


//= Look for first entry with lval pointer equal to given loc.
// Useful for displaying value but preventing it from being changed

int jhcParam::LockMatch (int *loc, int lockit)
{
  int i;

  for (i = 0; i < total; i++)
    if (lvals[i] == loc)
    {
      locks[i] = lockit;
      return 1;
    }
  return 0;
}


//= Like other LockMatch but for floating point value.

int jhcParam::LockMatch (double *loc, int lockit)
{
  int i;

  for (i = 0; i < total; i++)
    if (fvals[i] == loc)
    {
      locks[i] = lockit;
      return 1;
    }
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                          Basic Array Access                           //
///////////////////////////////////////////////////////////////////////////

//= Return value explanation string for item (NULL if invalid).

const char *jhcParam::Text (int i) const
{
  if ((i < 0) || (i >= total))
    return NULL;
  return((const char *)(names + NAMLEN * i));
}


//= Return 1 if int type pointer is bound.

int jhcParam::Ltype (int i) const
{
  if ((i < 0) || (i >= total) || (lvals[i] == NULL))
    return 0;
  return 1;
}


//= Return 1 if double type pointer is bound.

int jhcParam::Ftype (int i) const
{
  if ((i < 0) || (i >= total) || (fvals[i] == NULL))
    return 0;
  return 1;
}


//= Get current value of int variable (0 if invalid).

int jhcParam::Lval (int i) const
{
  if ((i < 0) || (i >= total))
    return 0;
  return *(lvals[i]);
}


//= Get current value of double variable (0.0 if invalid).

double jhcParam::Fval (int i) const
{
  if ((i < 0) || (i >= total))
    return 0.0;
  return *(fvals[i]);
}


//= Get current int default for entry (0 if invalid).

int jhcParam::Ldef (int i) const
{
  if ((i < 0) || (i >= total))
    return 0;
  return ldefs[i];
}


//= Get current double default for entry (0.0 if invalid).

double jhcParam::Fdef (int i) const
{
  if ((i < 0) || (i >= total))
    return 0.0;
  return fdefs[i];
}


//= Use pointer to set current int value (returns 0 if invalid index).

int jhcParam::Lset (int i, int val)
{
  if ((i < 0) || (i >= total))
    return 0;
  if (locks[i] != 0)
    return -1;
  *(lvals[i]) = val;
  return 1;
}


//= Use pointer to set current double value (returns 0 if invalid index).

int jhcParam::Fset (int i, double val)
{
  if ((i < 0) || (i >= total))
    return 0;
  if (locks[i] != 0)
    return -1;
  *(fvals[i]) = val;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                      Configuration File Editing                       //
///////////////////////////////////////////////////////////////////////////

//= Read a string from configuration file following a given tag.
// auxilliary function unrelated to stored variables and defaults
// example line: ihc_cam A024 RGB  -->  "A024 RGB"
// text can be edited separately by jhcPickString::EditString
// if no tag found then does NOT alter original contents of "text"
// unless "def" is non-NULL, in which case "def" is copied to "text"

int jhcParam::LoadText (char *text, const char *fname, const char *tag, const char *def, int ssz) const
{
  char line[200];
  char *end, *start;

  // find line beginning with tag then strip off tag 
  if (find_line(line, fname, tag) <= 0)
  {
    if (def != NULL)
      strcpy_s(text, ssz, def);               // possibly use default value
    return 0;
  }
  start = line + (int) strlen(tag);

  // trim whitepace from front and back (ok if whitespace in middle)
  while ((*start != '\0') && (strchr(" \t\n", *start) != NULL))
    start++;
  end = start + strlen(start) - 1;
  while ((end > start) && (strchr(" \t\n", *end) != NULL))
    end--;

  // save trimmed string to output
  if (end >= start)
    end[1] = '\0';
  strcpy_s(text, ssz, start);
  return 1;
}


//= Write a string to a configuration file prefixed by the given tag.
// auxilliary function unrelated to stored variables and defaults
// example line: ihc_cam A024 RGB
// if text is NULL or "" then does NOT write any line

int jhcParam::SaveText (const char *fname, const char *tag, const char *text) const
{
  excise_line(fname, tag);
  if ((text == NULL) || (*text == '\0'))
    return 1;
  return add_line(fname, tag, text);
}


//= Find line in file marked with given tag then copy whole line.
// returns 0 if no line found (tail left unaltered)
// returns 1 if successful, -1 for bad file (cannot open)

int jhcParam::find_line (char *line, const char *fname, const char *tag) const
{
  FILE *in;
  int n = (int) strlen(tag);

  // check inputs and try opening source file
  if ((fname == NULL) || (tag == NULL))
    return 0;
  if (fopen_s(&in, fname, "r") != 0)
    return -1;

  // search for line with given leading tag followed by whitespace
  while (fgets(line, 200, in) != NULL)
    if ((strncmp(line, tag, n) == 0) && (strchr(" \t\n", line[n]) != NULL))
    {
      // success, line already contains correct data
      fclose(in);
      return 1;
    }

  // clean up if failure
  fclose(in);
  return 0;
}


//= Remove from a file all lines that start with the given tag.
// alters original file; uses a temporary file to copy "good" parts to
// returns 0 if fails, 1 if successful

int jhcParam::excise_line (const char *fname, const char *tag) const
{
  char line[200], tname[80] = "jhc_temp.txt";
  FILE *in, *out, *tmp = NULL;
  int n, any = 0;

  // check for valid inputs 
  if ((fname == NULL) || (tag == NULL))
    return 0;
  n = (int) strlen(tag);

  // open source file and temporary location
  if (fopen_s(&in, fname, "r") != 0)
    return 1;                                        // no line if no file
  if (fopen_s(&tmp, tname, "w+") != 0)
  {
    fclose(in);
    return 0;
  }

  // copy most lines to this new file but omit tagged line (if any)
  while (fgets(line, 200, in) != NULL)
    if ((strncmp(line, tag, n) == 0) && (strchr(" \t\n", line[n]) != NULL))
      any++;
    else
      fputs(line, tmp);
     
  // clean up and see if any lines erased
  fclose(in);
  if (any == 0)
  {
    fclose(tmp);
    return 1;
  }

  // open output file (over-writes original)
  if (fopen_s(&out, fname, "w") != 0)
  {
    fclose(tmp);
    return 0;
  }

  // copy back bulk of old file
  fseek(tmp, 0, SEEK_SET);
  while (fgets(line, 200, tmp) != NULL)
    fputs(line, out);

  // cleanup 
  fclose(tmp);
  fclose(out);
  return 1;
}


//= Add a new line with the given starting tag to the current file.
// returns 0 if fails, 1 if successful

int jhcParam::add_line (const char *fname, const char *tag, const char *text) const
{
  FILE *out;

  // check inputs then open file for appending
  if ((fname == NULL) || (tag == NULL))
    return 0;
  if (fopen_s(&out, fname, "a") != 0)
    return 0;

  // write out line identifier then string 
  if (text == NULL)
    fprintf(out, "%s\n", tag);
  else
  {
    fprintf(out, "%s\t", tag);
    if (strlen(tag) < 8)
      fprintf(out, "\t");
    fprintf(out, " %s\n", text);
  }

  // clean up
  fclose(out);
  return 1;
}
