// jhcVisObj.cpp : data about a visual object and its parts
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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

#include <math.h>
#include <string.h>

#include "Interface/jhcMessage.h"

#include "Objects/jhcVisObj.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcVisObj::jhcVisObj ()
{
  // no overall size or orientation yet
  valid = -1;
  cnum = 0;
  dir = 0.0;
  asp = 1.0;

  // nothing else in list
  next = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                               Properties                              //
///////////////////////////////////////////////////////////////////////////

//= Look for a subpart with a given name, or possibly add it.

jhcVisPart *jhcVisObj::GetPart (const char *id, int add)
{
  jhcVisPart *p = &part;

  // if no name given then just return first part (bulk)
  if ((id == NULL) || (*id == '\0'))
    return p;

  // walk down list looking for matching name
  // stop at first unused entry or last entry in list or 
  while (p->status >= 0)
  {
    if (strcmp(p->name, id) == 0)
      return p;
    if (p->NextPart() == NULL)
      break;
    p = p->NextPart();
  }

  // name not found in list
  if (add <= 0)
    return NULL;

  // possibly create a new entry at end of list
  if (p->status >= 0) 
    p = p->AddPart();
  strcpy_s(p->name, id);
  p->status = 1;
  return p; 
}


//= Amount of overlap between two objects' bounding boxes.
// expressed as fraction of total area of larger item

double jhcVisObj::OverlapBB (jhcVisObj& ref)
{
  int w = part.rw, h = part.rh, rw = (ref.part).rw, rh = (ref.part).rh;
  int x0 = part.rx, x1 = x0 + w, rx0 = (ref.part).rx, rx1 = rx0 + rw;
  int y0 = part.ry, y1 = y0 + h, ry0 = (ref.part).ry, ry1 = ry0 + rh;
  int xlo = __max(x0, rx0), ylo = __max(y0, ry0);
  int xhi = __min(x1, rx1), yhi = __min(y1, ry1);
  int a = w * h, a2 = rw * rh, lap = (xhi - xlo) * (yhi - ylo);

  if ((xlo >= xhi) || (ylo >= yhi))
    return 0.0;
  return(lap / (double) __max(a, a2));
}


///////////////////////////////////////////////////////////////////////////
//                                Analysis                               //
///////////////////////////////////////////////////////////////////////////

//= Invalidate object and all its subparts.

void jhcVisObj::Clear ()
{
  jhcVisPart *p = &part;

  valid = -1;
  while (p != NULL)
  {
    p->status = -1;
    *(p->name) = '\0';
    p = p->NextPart();
  }
}


//= Copy bulk properties and all subparts.

void jhcVisObj::Copy (jhcVisObj& src)
{
  jhcVisPart *p = &part, *s = &(src.part);

  // bulk properties
  valid = src.valid;
  if (valid < 0)
    return;
  dir = src.dir;
  asp = src.asp;
  
  // subparts
  while (s != NULL)
  {
    p->Copy(*s);
    s = s->NextPart();
    if ((s == NULL) || (s->status < 0))
    {
      if ((p = p->NextPart()) != NULL)
        p->status = -1;
      break;
    }
    p = p->AddPart();
  }
}
    
  
//= Set properties of overall object.

void jhcVisObj::BulkProps (double x, double y, double ang, double ecc)
{
  dir = ang;
  asp = ecc;
  part.cx = x;
  part.cy = y;
}


//= Digests basic information about a connected component in an image.

void jhcVisObj::Ingest (const jhcImg& src, const jhcImg& comp, const jhcBlob& blob, int i, const int *clim)
{
  if ((i < 0) || (i >= blob.Active()) || !src.Valid(3) || !src.SameSize(comp, 2))
    Fatal("Bad input to jhcVisObj::Ingest");

  // save some overall properties
  dir = blob.BlobAngle(i);
  asp = blob.BlobAspect(i);

  // save position in first part (bulk)
  part.status = 1;
  blob.BlobCentroid(&(part.cx), &(part.cy), i);

  // build icon and mask images then extract color histograms
  part.area = get_patches(src, comp, blob, i);
  part.Analyze(clim);

  // mark object as in use and still part of selection
  valid = 1;
}


//= Get binary mask and carved out portion of main image corresponding to object.
// fills in the images for the first "bulk" part of the object
// adds 1 pixel black buffer around object's bounding box (for morphology)
// returns number of valid pixels in mask image

int jhcVisObj::get_patches (const jhcImg& src, const jhcImg& comp, const jhcBlob& blob, int i)
{
  jhcRoi box;
  const US16 *c;
  const UC8 *s;
  UC8 *m, *d;
  int x, y, rx, ry, rw, rh, msk, dsk, csk, ssk, sz0 = 10, n = 0; 

  // get parameters from object bounding box
  blob.GetRoi(box, i);

  // make sure big enough to shrink then add 1 pixel boundary
  rw = __max(box.RoiW(), 9) + 2;
  rh = __max(box.RoiH(), 9) + 2;
  rx = __max(0, box.RoiMidX() - (rw >> 1));
  ry = __max(0, box.RoiMidY() - (rh >> 1));
  box.SetRoi(rx, ry, rw, rh);

  // mark where image patch came from and set image patch sizes
  part.rx = rx;
  part.ry = ry;
  part.IconSize(rw, rh);
  (part.mask).FillArr(0);
  (part.crop).FillRGB(0, 0, 255);

  // set up image pointers and end of line advances for patches
  m = (part.mask).PxlDest();
  d = (part.crop).PxlDest();
  msk = (part.mask).Skip();
  dsk = (part.crop).Skip();
  c = (const US16 *) comp.RoiSrc(box);
  s = src.RoiSrc(box);
  csk = comp.RoiSkip(box) >> 1;
  ssk = src.RoiSkip(box);

  // get cropped version of main image and create binary mask
  for (y = 0; y < rh; y++, m += msk, d += dsk, c += csk, s += ssk)
    for (x = 0; x < rw; x++, m++, d += 3, c++, s += 3)
      if (*c == i)
      {
        *m = 255;
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        n++;
      }
  return n;
}



