// jhcRoom.cpp : contour bounding local environment
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 IBM Corporation
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

#include "Data/jhcName.h"            // common video
#include "Data/jhcParam.h"          
#include "Interface/jhcMessage.h"   

#include "Geometry/jhcRoom.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcRoom::jhcRoom (int n)
{
  // make up a default array of segments
  segs = NULL;
  alloc(n);

  // get default map
  strcpy_s(wfile, "office");
  Clear();
  Defaults();
}


//= Default destructor does necessary cleanup.

jhcRoom::~jhcRoom ()
{
  delete [] segs;
}


//= Make array of line segments coordinate pairs.
// copies old values to new array for valid entries

void jhcRoom::alloc (int n)
{
  double *prev = segs;
  int i, ncopy = 4 * nseg;

  // make new array
  if (n < nlim)
    return;
  segs = new double [4 * n];
  nlim = n;

  // copy old values then delete
  if (prev == NULL)
    return;
  for (i = 0; i < ncopy; i++)
    segs[i] = prev[i];
  delete [] prev;
}


//= Invalidate all existing segments.

void jhcRoom::Clear ()
{
  nseg = 0;
  x0 = 0.0;
  x1 = 0.0;
  y0 = 0.0;
  y1 = 0.0;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcRoom::Defaults (const char *fname)
{
  int ans = 1;

  ans &= dps.LoadText(wfile, fname, "rm_walls");
  ans &= Load(wfile);
  ans &= draw_params(fname);
  return ans;
}


//= Write current processing variable values to a file.

int jhcRoom::SaveVals (const char *fname) const
{
  int ans = 1;

  ans &= dps.SaveText(fname, "rm_walls", wfile);
  ans &= dps.SaveVals(fname);
  return ans;
}


//= Parameters used for overhead map display.

int jhcRoom::draw_params (const char *fname)
{
  jhcParam *ps = &dps;
  int ok;

  ps->SetTag("rm_draw", 0);
  ps->NextSpecF( &ipp,  0.7, "Map inches per pixel");  // was 0.3
  ps->NextSpec4( &bdx, 10,   "Map X border (pel)");     
  ps->NextSpec4( &bdy, 10,   "Map Y border (pel)");     
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Load a set of contours from a file.
// format is x0 y0 x1 y1 on each line with coordinates nominally in inches
// blank lines are ignored, tail of lines after coordinates are also ignored
// generally entry door is near (0 0) with all (x y) coordinates positive
// returns number of segments loaded (possibly in addition to previous set)

int jhcRoom::Load (const char *fname, int clr)
{
  FILE *in;
  jhcName name;
  char line[200];
  const char *ext;
  double *s;
  double fx0, fy0, fx1, fy1;
  int n = 0;

  // fill in extension if missing
  name.ParseName(fname);
  ext = name.Extension();
  if (*ext == '\0')
    sprintf_s(line, "%s.ejs", fname);
  else
    strcpy_s(line, fname);

  // try opening file
  if (fopen_s(&in, line, "r") != 0)
    return -1;
  if (clr > 0)
  {
    strcpy_s(wfile, name.Base());
    Clear();
  }
  s = segs + 4 * nseg;

  while (fgets(line, 200, in) != NULL)
  {
    // attempt to read full set of coordinates fro segment
    if (sscanf_s(line, "%lf %lf %lf %lf", &fx0, &fy0, &fx1, &fy1) == 4)
    {
      // possibly increase array size (and copy old entries)
      if (nseg >= nlim)
        alloc(nlim + 50);

      // transfer coordinates
      s[0] = fx0;
      s[1] = fy0;
      s[2] = fx1;
      s[3] = fy1;

      if (nseg == 0)
      {
        // possibly initialize environmental limits
        x0 = __min(fx0, fx1);
        y0 = __min(fy0, fy1);
        x1 = __max(fx0, fx1);
        y1 = __max(fy0, fy1);
      }
      else
      {
        // update environmental limits
        x0 = __min(x0, __min(fx0, fx1));
        y0 = __min(y0, __min(fy0, fy1));
        x1 = __max(x1, __max(fx0, fx1));
        y1 = __max(y1, __max(fy0, fy1));
      }

      // advance to next entry
      nseg++;
      s += 4;
      n++;
    }
  }
  
  // cleanup
  fclose(in);
  return n;
}


//= Draw all the segments for a room in given image.
// can use multiple instance for different colors and thicknesses
// e.g. walls are drawn white x3 while tables are magenta x1 thick

int jhcRoom::DrawRoom (jhcImg& dest, int t, int r, int g, int b) const
{
  const double *s = segs;
  double ppi = 1.0 / ipp, x0 = XOff(), y0 = YOff();
  int i;

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcRoom::DrawRoom");
  for (i = 0; i < nseg; i++, s += 4)
    DrawLine(dest, x0 + ppi * s[0], y0 + ppi * s[1], 
                   x0 + ppi * s[2], y0 + ppi * s[3], 
                   t, r, g, b);
  return 1;
}



