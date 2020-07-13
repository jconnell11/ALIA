// jhcExpVSrc.cpp : iterator for shoveling images to analysis programs
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2014 IBM Corporation
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

#include "Interface/jhcString.h"
#include "Interface/jhcPickStep.h"
#include "Interface/jhcPickVals.h"
#include "Interface/jhcPickString.h"
#include "Video/jhcVidReg.h"

#include "Video/jhcExpVSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                     Basic creation and deletion                          //
//////////////////////////////////////////////////////////////////////////////

//= Constructor makes up a stream of the requested class.

jhcExpVSrc::jhcExpVSrc () 
{
  noisy = 1;
  index = 1;
  Defaults();
}


//= Constructor that takes file name at creation time.

jhcExpVSrc::jhcExpVSrc (const char *name) : jhcGenVSrc(name)
{
  noisy = 1;
  index = 1;
  Defaults();
  SetSize(0, 0, 0);
}


//= Need to size internal array properly when source is changed.

int jhcExpVSrc::SetSource (const char *name)
{
  if (jhcGenVSrc::SetSource(name) != 1)
    return 0;
  SetSize(xlim, ylim, Mono);
  return 1;
}


//////////////////////////////////////////////////////////////////////////////
//                         Configuration Parameters                         //
//////////////////////////////////////////////////////////////////////////////

//= Pop dialog box asking for playback parameters.
// force stream to new values if not already there
// returns 1 if some alteration might have been made

int jhcExpVSrc::AskStep ()
{
  jhcPickStep dlg;
  double new_fps;

  // run dialog box 
  if (dlg.EditStep(play, freq) < 1)
    return 0;

  // ask for new framerate directly from source
  new_fps = freq / DispRate;
  SetRate(new_fps);
  DispRate = freq / new_fps;

  // adjust frame stepping parameters
  SetStep(Increment, ByKey);
  if ((nextread < 1) ||
      ((nframes > 0)    && (nextread > nframes))    ||
      ((FirstFrame > 0) && (nextread < FirstFrame)) ||
      ((LastFrame > 0)  && (nextread > LastFrame)))
    Rewind();
  return 1;
}


//= Pop dialog box asking for image resizing parameters.
// returns 1 if some alteration might have been made

int jhcExpVSrc::AskSize ()
{
  jhcPickVals dlg;

  if (dlg.EditParams(squash) < 1)
    return 0;
  SetSize(xlim, ylim, Mono);
  return 1;
}


//= Ask user to given a textual specification of the stream he wants.
// returns 1 if some alteration might have been made

int jhcExpVSrc::AskSource ()
{
  jhcPickString dlg;
  char fname[250];

  strcpy0(fname, FileName, 250);
  if (dlg.EditString(fname, 0, "Video source file:", 250) < 1)
    return 0;
  SetSource(fname);
  return 1;
}


//= Ask user to choose new file for stream.
// will copy choice into string if one is provided
// returns 1 if a was file selected, 0 if user cancelled

int jhcExpVSrc::SelectFile (char *choice, int ssz)
{ 
  CFileDialog dlg(TRUE);
  OPENFILENAME *vals = &(dlg.m_ofn);
  const char *filter;
  jhcString kinds, sel, idir(JustDir);
  int len;

  if (_stricmp(Flavor, "vfw") != 0)
    vals->lpstrInitialDir = idir.Txt();
  filter = jvreg.FilterTxt(0, &len);
  kinds.Set(filter, len);
  vals->lpstrFilter = kinds.Txt();

  if (dlg.DoModal() == IDOK)
  {
    sel.Set(vals->lpstrFile);
    SetSource(sel.ch);
    if (choice != NULL)
      strcpy_s(choice, ssz, sel.ch);
    return 1;
  }
  return 0;
}


//= Configure pointers and default values pairings.
// load new defaults from a file (if any)

void jhcExpVSrc::Defaults (char *fname)
{
  squash.SetTag("vid_size");
  squash.ClearAll();
  squash.NextSpec4( &xlim,   0, "Max width");
  squash.NextSpec4( &ylim,   0, "Max height");
  squash.NextSpec4( &Avg,    0, "Averaging style");
  squash.NextSpec4( &Mono,   0, "Monochrome style");
  squash.NextSpec4( &Quad,   0, "Extracted quadrant");
  squash.NextSpec4( &Shift,  0, "Downshift pixels");    // only for Kinect
  squash.NextSpec4( &w,      0, "Current width");
  squash.NextSpec4( &h,      0, "Current height");

  // some things are for display purposes only
  play.LockMatch( &w, 1);
  play.LockMatch( &h, 1);

  // get values from file (if any)
  squash.LoadDefs(fname);
  squash.RevertAll();
  play.LoadDefs(fname);
  play.RevertAll();
}


//= Save current values out as a defaults in specified file.

void jhcExpVSrc::SaveVals (char *fname)
{
  squash.SaveVals(fname);
  play.SaveVals(fname);
}


//////////////////////////////////////////////////////////////////////////////
//                             Core Functions                               //
//////////////////////////////////////////////////////////////////////////////

//= Force width, height, and base image to be consistent with these values.
// sometimes underlying stream or desired size is changed asynchronously
// can optionally request that frames be converted to monochrome (bw nonzero)
// should call this when quadrant processing is selected or deselected
// a positive quadrant picks that number, negative picks left or right side
// @see Processing.jhcGray#ForceMono

void jhcExpVSrc::SetSize (int xmax, int ymax, int bw)
{
  int xreq = xmax, yreq = ymax;
  double f = 1.0, f2 = 1.0;

  // adjust total image size if request is for one panel
  if (Quad > 0)
  {
    xreq *= 2;
    yreq *= 2;
  }
  else if (Quad < 0)
    xreq *= 2;

  // see if source can shrink images directly via partial decoding
  jhcGenVSrc::SetSize(xreq, yreq, bw);
  base.SetSize(w, h, d);
  base2.SetSize(w2, h2, d2);

  // figure out additional shrink factor needed using new w and h
  if (xreq > 0)
    f = w / (double) xreq;
  if (yreq > 0) 
    f2 = h / (double) yreq;
  if ((f > 1.0) || (f2 > 1.0))
    f = __max(f, f2);
  else
    f = __min(f, f2);

  // determine size of one quadrant (or possibly whole image)
  if (Quad > 0)
  {
    w /= 2;
    h /= 2;
    w2 /= 2;
    h2 /= 2;
  }
  else if (Quad < 0)
  {
    w /= 2;
    w2 /= 2;
  }
  qbase.SetSize(w, h, d); 
  qbase.SetSize(w2, h2, d2); 

  // mark whether monochrome conversion needed
  // special negative Mono mode makes RGB with all equal
  mc = 1;
  if (Mono < 0)
  {
    d = 3;
    if (d2 == 3)
      d2 = 3;
  }
  else if ((Mono > 0) && (d == 3))
  { 
    d = 1;
    if (d2 == 3)
      d2 = 1;
  }
  else 
    mc = 0;   
  mbase.SetSize(w, h, d);
  mbase2.SetSize(w2, h2, d2);

  // always promise to finally generate size that user asked for
  w = ROUND(w / f);
  h = ROUND(h / f);
  w2 = ROUND(w2 / f);
  h2 = ROUND(h2 / f);
}


//= Pass through to underlying video stream, reset pause counter.

int jhcExpVSrc::iSeek (int number)
{
  int ans;

  if (gvid == NULL)
    return 0;  
  ans = gvid->Seek(number);
  ok = gvid->Valid();
  return ans;
}


//= Try retrieving next image and possibly resizing it.
// tell actual number of frames advanced
// captures into base, gets quad into qbase, gets mono into mbase 

int jhcExpVSrc::iGet (jhcImg& dest, int *advance, int src)
{
  int ans;
  jhcImg *b = ((src > 0) ? &base2 : &base);
  jhcImg *q = ((src > 0) ? &qbase2 : &qbase);
  jhcImg *m = ((src > 0) ? &mbase2 : &mbase);
  jhcImg *s = b;

  // communicate possible auto-looping
  if (gvid == NULL)
    return 0;  
  gvid->LastFrame = LastFrame;               

  // get basic frame into base image or directly into output 
//  if (dest.SameFormat(base) && (mc <= 0))
  if ((xlim <= 0) && (ylim <= 0) && (Mono == 0))
  {
    s = &dest;
    ans = gvid->Get(*s, src);
    b->SetSize(*s);     // for jhcListVSrc where frames change size
    dest.SetSize(*b);
  }
  else
    ans = gvid->Get(*s, src);

  // determine amount actually advanced
  ok = gvid->Valid();
  if (ok > 0)
    *advance = gvid->Advance();  // extract "n"
  else
    *advance = 0;
  ParseName(gvid->File());

  // possibly abort if frame fetch failed or no alterations needed
  if ((ans <= 0) || (s == &dest))
    return ans;
  
  // see if should get only a quadrant or one side of the image
  if (Quad > 0)
  {
    if (dest.SameFormat(*q) && (mc <= 0))
      return jr.GetQuad(dest, *s, Quad);
    jr.GetQuad(*q, *s, Quad);
    s = q;
  }
  else if (Quad < 0)
  {
    if (dest.SameFormat(*q) && (mc <= 0))
      return jr.GetHalf(dest, *s, -Quad);
    jr.GetHalf(*q, *s, -Quad);
    s = q;
  }

  // see if color to monochrome conversion needed
  if (mc > 0) 
  {
    if (dest.SameFormat(*m))
      return jg.ForceMono(dest, *s, abs(Mono));
    jg.ForceMono(*m, *s, abs(Mono));
    s = m;
  }

  // do further down-sizing as required
  return jr.ForceSize(dest, *s, Avg);        
}


//= Get next pair of images from source.
// NOTE: ignores any resizing

int jhcExpVSrc::iDual (jhcImg& dest, jhcImg& dest2)
{
  int ans;

  // communicate possible auto-looping
  if (gvid == NULL)
    return 0;  
  gvid->LastFrame = LastFrame;               

  // get basic frames directly
  ans = gvid->DualGet(dest, dest2);
  ParseName(gvid->File());
  if (ans <= 0) 
    return ans;
  return 1;
}


