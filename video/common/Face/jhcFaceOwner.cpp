// jhcFaceOwner.cpp : stores all face information about a particular person
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 IBM Corporation
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

#include "Face/jhcFaceOwner.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcFaceOwner::~jhcFaceOwner ()
{
  clr_vect();
}


//= Get rid of all instance vectors.

void jhcFaceOwner::clr_vect ()
{
  jhcFaceVect *rest, *v = vect;

  while (v != NULL)
  {
    rest = v->next;
    delete v;
    v = rest;
  }
  vect = NULL;
  nv = 0;
}


//= Default constructor initializes certain values.

jhcFaceOwner::jhcFaceOwner (const char *who, int sz)
{
  // basic info
  strcpy_s(name, who);
  vsz = sz;

  // data and people list
  vect = NULL;
  next = NULL;
  nv = 0;
  ibig = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Add a new recognition vector to end of list.
// if "vcnt" > 0 then removes least useful vectors if more room needed
// returns total number of vectors for person

int jhcFaceOwner::AddVect (jhcFaceVect *v, int vcnt)
{
  jhcFaceVect *v0 = NULL;

  // make room for this vector (if needed)
  if (vcnt > 0)
    while (nv >= vcnt)
      rem_weakest();

  // find end of current list (after any editing)
  while ((v0 = NextVect(v0)) != NULL)
    if (v0->next == NULL)
      break;

  // tack on new vector 
  if (v0 == NULL)
    vect = v;
  else
    v0->next = v;
  nv++;

  // possibly assign image number
  if ((v->thumb).Valid())
    v->inum = ++ibig;
  return nv;
}


//= Splice out the single least useful vector in the list.
// does NOT scavenge associated thumbnail image from directory

void jhcFaceOwner::rem_weakest ()
{
  jhcFaceVect *prev, *weak, *last = NULL, *v = NULL;
  int low = -1;

  // scan through vectors to find least useful
  while ((v = NextVect(v)) != NULL)
  {
    if ((low < 0) || (v->util <= low))
    {
      // remember previous item in list also
      prev = last;
      weak = v;
      low = v->util;
    }
    last = v;
  }

  // splice particular instance out of list and delete it
  if (low < 0)
    return;
  if (prev == NULL)
    vect = weak->next;
  else
    prev->next = weak->next;
  delete weak;
  nv--;
}


///////////////////////////////////////////////////////////////////////////
//                              File Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Load vectors and other info associated with this person.
// uses "dir" in conjunction with member variable "name" to pick file
// returns number of vectors loaded

int jhcFaceOwner::Load (const char *dir)
{
  FILE *in;
  jhcFaceVect *v, *last = NULL;
  char fname[200];
  int i, cnt;

  // try opening appropriate file
  clr_vect();
  sprintf_s(fname, "%s/%s.dat", dir, name);
  if (fopen_s(&in, fname, "r") != 0)
    return 0;

  // read header then all vectors
  if (fscanf_s(in, "%d %d", &ibig, &cnt) != 2)
    return 0;
  for (i = 0; i < cnt; i++)
  {
    // quit at first bad vector
    v = new jhcFaceVect(vsz);
    if (v->Load(in) <= 0)
    {
      delete v;
      break;
    }

    // add good vector to end of list
    if (last == NULL)
      vect = v;
    else
      last->next = v;
    last = v;
    nv++;
  }

  // clean up
  fclose(in);
  return nv;
}


//= Save vectors and other info associated with this person.
// uses "dir" in conjunction with member variable "name" to pick file
// format:
//   ibig nv
//   inum0 
//   v00 v01 v02 ... (8 floats across)
//   inum1 
//   v10 v11 v12 ... (8 floats across)
// returns number of vectors saved

int jhcFaceOwner::Save (const char *dir) const
{
  FILE *out;
  jhcFaceVect *v = vect;
  char fname[200];
  int cnt = 0;

  // try opening appropriate file
  sprintf_s(fname, "%s/%s.dat", dir, name);
  if (fopen_s(&out, fname, "w") != 0)
   return 0;

  // write header followed by all vectors
  fprintf(out, "%d %d\n", ibig, nv);
  while (v != NULL)
  {
    v->Save(out);
    v = v->next;
  }

  // clean up
  fclose(out);
  return cnt;
}
