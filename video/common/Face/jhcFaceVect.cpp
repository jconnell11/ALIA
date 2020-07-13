// jhcFaceVect.cpp : stores condensed representation for one face instance
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

#include "Face/jhcFaceVect.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcFaceVect::~jhcFaceVect ()
{
  delete [] data;
}


//= Default constructor initializes certain values.

jhcFaceVect::jhcFaceVect (int vsz)
{
  data = new double [vsz];
  sz = vsz;
  inum = 0;
  util = 0;
  next = NULL;
}


//= Fill self with recognition vector and thumbnail from reference.
// does not change member variables "inum", "next", or "util"
// returns 1 if successful, 0 if some problem

int jhcFaceVect::Copy (const jhcFaceVect *ref)
{
  int i;

  if ((ref == NULL) || (ref->sz != sz))
    return 0;
  for (i = 0; i < sz; i++)
    data[i] = (ref->data)[i];
  thumb.Clone(ref->thumb);
  return 1;
} 


///////////////////////////////////////////////////////////////////////////
//                              File Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Load basic date from already opened file.
// returns 1 if successful, 0 or negative for error

int jhcFaceVect::Load (FILE *in)
{
  char hdr[20], sep[20];
  int i;

  // read header giving image number and utility
  if (fscanf_s(in, "%s %d %d %s", hdr, 20, &inum, &util, sep, 20) != 4)
    return -1;
  if ((strcmp(hdr, "inst") != 0) || (strcmp(sep, "=") != 0))
    return -1;

  // read in data for recognition signature
  for (i = 0; i < sz; i++)
    if (fscanf_s(in, "%lf", data + i) != 1)
      return 0;
  return 1;
}


//= Save out basic data to already opened file.

void jhcFaceVect::Save (FILE *out) const
{
  int i;

  fprintf(out, "\ninst %d %d =\n", inum, util);
  for (i = 0; i < sz; i++)
  {
    fprintf(out, "%10.6lf ", data[i]);
    if ((i % 8) == 7)  
      fprintf(out, "\n");
  }
}
