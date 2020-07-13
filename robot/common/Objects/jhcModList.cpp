// jhcModList.cpp : list of visual object models
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011 IBM Corporation
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

#include "Interface/jhcMessage.h"

#include "Objects/jhcModList.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcModList::jhcModList ()
{
  db = NULL;
  Defaults();
}


//= Default destructor does necessary cleanup.

jhcModList::~jhcModList ()
{
  rem_all();
}


//= Clear out all database models.

void jhcModList::rem_all ()
{
  jhcVisModel *m2, *m = db;

  while (m != NULL)
  {
    m2 = m->next;
    delete m;
    m = m2;
  }
  db = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcModList::Defaults (const char *fname)
{
  int ok = 1;

  ok &= match_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcModList::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= mps.SaveVals(fname);
  return ok;
}


//= Parameters used for comparing models.

int jhcModList::match_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("mod_match", 0);
  ps->NextSpecF( &af,     1.0, "Wt for area");  
  ps->NextSpecF( &wf,     1.0, "Wt for width");  
  ps->NextSpecF( &ef,     0.1, "Wt for aspect");  
  ps->NextSpecF( &cf,     0.1, "Wt for colors");  
  ps->Skip(2);

  ps->NextSpecF( &match,  8.0, "Match threshold");  
  ps->NextSpecF( &close,  4.0, "Add threshold");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             File Operations                           //
///////////////////////////////////////////////////////////////////////////

//= Load self with all models from a file.
// header line tells size of vectors

int jhcModList::LoadModels (const char *fname, int append)
{
  jhcArr mod;
  FILE *in;
  char line[200], hdr[80] = "JHC vis mod";
  int i, n, val, len = (int) strlen(hdr), ok = 1, cnt = 0;

  // possibly flush current models then try opening file 
  if (append <= 0)
    rem_all();
  if (fopen_s(&in, fname, "r") != 0)
    return -2;

  // check header
  if ((fgets(line, 200, in) == NULL) || 
      (strncmp(line, hdr, len) != 0) ||
      (sscanf_s(line + len, "%d", &n) != 1))
  {
    fclose(in);
    return -1;
  }
  mod.SetSize(n);

  // read individual model lines  
  while (1)
  {
    // load numerical values into temporary array
    for (i = 0; i < n; i++)
    {
      if (fscanf_s(in, "%d", &val) != 1)
        break;
      mod.ASet(i, val); 
    }
    if (i < n)
      break;

    // get class name (no CR) then add to database
    if (fscanf_s(in, "%s", hdr) != 1)
      break;
    len = (int) strlen(hdr) - 1;
    if ((len >= 0) && (hdr[len] == '\n'))
      hdr[len] = '\0';
    AddModel(hdr, mod);
    cnt++;
  }

  // clean up
  fclose(in);
  printf("Loaded %d visual object models\n", cnt);
  return cnt;
}


//= Save all models to a file.
// header line tells size of vectors
  
int jhcModList::SaveModels (const char *fname)
{
  FILE *out;
  jhcVisModel *m = db;
  char hdr[80] = "JHC vis mod";
  int i, n, cnt = 0;

  // try opening file and write header
  if (db == NULL)
    return 0;
  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  fprintf(out, "%s %d\n", hdr, (db->data).Size());

  // write one visual model per line
  while (m != NULL)
  {
    // write numerical values with name at end
    n = (m->data).Size();
    for (i = 0; i < n; i++)
      fprintf(out, "%3d ", (m->data).ARef(i));
    fprintf(out, "  %s\n", m->name);

    // advance to next model
    m = m->next;
    cnt++;
  }

  // clean up
  fclose(out);
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= See if there are any visual models for the item specified.

int jhcModList::AnyModels (const char *kind)
{
  jhcVisModel *m = db;

  while (m != NULL)
    if (strcmp(m->name, kind) == 0)
      return 1;
    else
      m = m->next;
  return 0;
}


//= See if the vector is plausible an example of the given kind.
// returns 2 if very close, 1 if sufficient, 0 if far

int jhcModList::IsKind (const char *kind, jhcArr& vec)
{
  double d = DistKind(kind, vec);

  if (d <= close)
    return 2;
  if (d <= match)
    return 1;
  return 0;
}


//= Get the smallest distance for some model of the given kind.

double jhcModList::DistKind (const char *kind, jhcArr& vec)
{
  jhcVisModel *win = NULL, *m = db;
  double d, best = 1000.0;

  // check against all records of the given type
  while (m != NULL)
  {
    if (strcmp(m->name, kind) == 0)
    {
      // save smallest distance
      d = dist(m->data, vec);
      if ((win == NULL) || (d < best))
      {
        win = m;
        best = d;
      }
    }
    m = m->next;
  }
  return best;
}


//= Find the best match (if any) in the database for the given vector.
// returns 0 if no good match, 1 if kind is valid

int jhcModList::FindKind (char *kind, jhcArr& vec, int ssz)
{
  jhcVisModel *win = NULL, *m = db;
  double d, best;

  // find best matching model in whole database
  while (m != NULL)
  {
    d = dist(m->data, vec);
    if ((win == NULL) || (d < best))
    {
      win = m;
      best = d;
    }
    m = m->next;
  }

  // check for some hit then copy name and see if good enough
  *kind = '\0';
  if (win == NULL)
    return 0;
  strcpy_s(kind, ssz, win->name);
  if (best <= match)
    return 1;
  return 0;
}


//= Compute difference distance between two models.

double jhcModList::dist (jhcArr& vec1, jhcArr& vec2)
{
  jhcArr diff(12);
  double d = 0.0;
  int i;

  if ((vec1.Size() != 12) || (vec2.Size() != 12))
    return 1000.0;
  diff.AbsDiff(vec1, vec2);

  // add in contribution from area, width, and elongation
  d += af * diff.ARef(0);
  d += wf * diff.ARef(1);
  d += ef * diff.ARef(2);
    
  // add in contributions from color bins
  for (i = 0; i < 9; i++)
    d += cf * diff.ARef(i + 3);
  return d;
}


//= Add a new model vector for the specified kind to the database.
// can force an addition even if vector already matches
// returns 2 if added, 1 if added as a touchup, 0 if not needed

int jhcModList::AddModel (const char *kind, jhcArr& vec, int force)
{
  jhcVisModel *list = db;
  int known = IsKind(kind, vec);

  // skip addition if already a good match
  if ((force <= 0) && (known >= 2))
    return known;

  // add a new model to list 
  db = new jhcVisModel(vec.Size());
  db->next = list;

  // fill in details
  strcpy_s(db->name, kind);  
  (db->data).Copy(vec);
  return known;
}


//= Remove the most recently created model.

void jhcModList::RemModel ()
{
  jhcVisModel *last = db;

  if (last == NULL)
    return;
  db = last->next;
  delete [] last;
}
