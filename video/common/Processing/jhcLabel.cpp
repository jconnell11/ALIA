// jhcLabel.cpp : uses operating system to add captions to an image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2018 IBM Corporation
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
#include <math.h>
#include <string.h>

#include "Interface/jhcMessage.h"
#include "Interface/jhcString.h"

#include "Processing/jhcLabel.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcLabel::jhcLabel (int xmax, int ymax)
{
  // create a local device context 
  dc = CreateCompatibleDC(NULL);

  // create and bind bitmap (monochrome)
  bmap = CreateCompatibleBitmap(dc, xmax, ymax); 
  bmap0 = SelectObject(dc, bmap);

  // create and bind font
  sz = 0;
  set_font(16);

  // set up for GetDIBits in xfer_text
  make_header();
}


//= Make generic monochrome bitmap header for transferring text images.

void jhcLabel::make_header ()
{
  RGBQUAD *cols;
  BYTE v = 0;
  int i, hsz = sizeof(BITMAPINFOHEADER), csz = 256 * sizeof(RGBQUAD);

  // allocate standard monochrome header (no size yet)
  hdr = (LPBITMAPINFOHEADER) new char[hsz + csz];
  hdr->biBitCount = 8;
  hdr->biClrUsed  = 256;
  hdr->biSize          = hsz;
  hdr->biPlanes        = 1;
  hdr->biCompression   = BI_RGB;
  hdr->biSizeImage     = 0;
  hdr->biXPelsPerMeter = 1000;
  hdr->biYPelsPerMeter = 1000;
  hdr->biClrImportant  = 0;

  // set up of 8 bit pixels to gray shades (irrelevant)
  cols = (RGBQUAD *)((BYTE *) hdr + hsz);
  for (i = 0; i < 256; i++, cols++, v++)
  {
    cols->rgbRed   = v;
    cols->rgbGreen = v;
    cols->rgbBlue  = v;
  }
}


//= Default destructor does necessary cleanup.

jhcLabel::~jhcLabel ()
{
  // delete font
  set_font(0);

  // delete bitmap
  SelectObject(dc, bmap0); 
  DeleteObject(bmap);

  // delete device context
  DeleteDC(dc); 

  // delete bitmap header
  delete [] ((char *) hdr);
}


///////////////////////////////////////////////////////////////////////////
//                              Core Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Change characteristics of font for drawing text.

void jhcLabel::set_font (int ht)
{
  jhcString fname("ANSI_VAR_FONT");
  if (ht == sz)
    return;
  
  // get rid of old font (if any)
  if (sz != 0)
  {
    SelectObject(dc, font0); 
    DeleteObject(font);
  }

  // record new size (0 = purge)
  sz = ht;
  if (sz == 0)
    return;

  // set up new font 
  font = CreateFont(abs(sz), 0, 0, 0, ((sz > 0) ? FW_REGULAR : FW_BOLD), 
                    FALSE, FALSE, FALSE, ANSI_CHARSET, 
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, 
                    FF_DONTCARE, fname.Txt());
  font0 = SelectObject(dc, font);
}


//= Render message to temporary image of sufficient size.
// "just" only matters for L/C/R justified multi-line labels
// saves string in member variable "src" with dims "tw" and "th"
// text is black (0) on white (255) stored in DC's bitmap

void jhcLabel::make_label (const char *txt, int ht, int just)
{
  HBRUSH hbr;
  RECT area = {0, 0, 1, 1};
  UINT fmt = DT_TOP | DT_NOPREFIX;
  jhcString msg(txt);

  // set font size and figure out how to justify text
  set_font(ht);
  if (just < 0)
    fmt |= DT_LEFT;
  else if (just > 0)
    fmt |= DT_RIGHT;
  else
    fmt |= DT_CENTER;

  // determine size of text block
  DrawText(dc, msg.Txt(), -1, &area, DT_CALCRECT | fmt);
  th = area.bottom - area.top;
  tw = area.right - area.left;

  // actually generate text pixels (default = black on white)
  hbr = CreateSolidBrush(GetBkColor(dc)); 
  FillRect(dc, &area, hbr); 
  DrawText(dc, msg.Txt(), -1, &area, fmt);
  DeleteObject(hbr);

  // copy all pixels to a DIB for reading (GetPixel is miserably slow)
  src.SetSize(tw, th, 1);
  hdr->biWidth  = tw;
  hdr->biHeight = th;
  GetDIBits(dc, bmap, 0, th, src.PxlDest(), (LPBITMAPINFO) hdr, DIB_RGB_COLORS);
}


//= Copy non-background pixels from member "src" to given image.
// takes left bottom corner of destination location 
// if chk > 0 will only alter pixels if no clipping happens
// if negative red value given, then will use standard colors
// returns 1 if okay, 0 if some clipping

int jhcLabel::xfer_text (jhcImg& dest, int x, int y, int r, int g, int b, int chk)
{
  int dw = dest.XDim(), x0 = __max(0, x), x1 = x + tw, rw = __min(x1, dw) - x0;
  int dh = dest.YDim(), y0 = __max(0, y), y1 = y + th, rh = __min(y1, dh) - y0;
  int red, grn, blu, dx, dy, dsk = dest.RoiSkip(rw), ssk = src.RoiSkip(rw), clean = 1;
  UC8 *d = dest.RoiDest(x0, y0);
  const UC8 *s = src.RoiSrc(x0 - x, y0 - y);

  // see if text will be clipped (or nothing to show)
  if ((rw <= 0) || (rh <= 0))
    return 0;
  if ((rw < tw) || (rh < th))
    clean = 0;
  if ((chk > 0) && (clean <= 0))
    return 0;

  // figure out real drawing color
  cvt_col(red, grn, blu, dest, r, g, b);
  
  // mark monochrome foreground pixels 
  if (dest.Valid(1))
  {
    for (dy = rh; dy > 0; dy--, d += dsk, s += ssk)
      for (dx = rw; dx > 0; dx--, d++, s++)
        if (*s == 0)
          *d = (UC8) red;
    return clean;
  }

  // mark color foreground pixels 
  for (dy = rh; dy > 0; dy--, d += dsk, s += ssk)
    for (dx = rw; dx > 0; dx--, d += 3, s++)
      if (*s == 0)
      {
        d[0] = (UC8) blu;
        d[1] = (UC8) grn;
        d[2] = (UC8) red;
      }
  return clean;
}


//= Copy rotated non-background pixels from member "src" to given image.
// takes center of destination location, assumes "src" is at double resolution
// if negative red value given, then will use standard colors
// returns 1 if okay, 0 if some clipping

int jhcLabel::xfer_text_rot (jhcImg& dest, double x, double y, double degs, int r, int g, int b)
{
  double rads = -D2R * degs, c = cos(rads), s = sin(rads);
  double dtx, dty, cdy, sdy, sxf, syf, c2 = 2.0 * c, s2 = 2.0 * s;
  int red, grn, blu, w = dest.XDim(), h = dest.YDim(), f = dest.Fields();
  int ix0, ix1, iw, iy0, iy1, ih, rw, rh, dx, dy, sx, sy, skip, clean = 1;
  UC8 *d;

  // get maximum extent of source image in destination
  dtx = 0.25 * (fabs(tw * c) + fabs(th * s));
  dty = 0.25 * (fabs(tw * s) + fabs(th * c));

  // compute destination X insertion bounds 
  ix0 = ROUND(x - dtx);
  if (ix0 >= w)
    return 0;
  iw = ix0;
  ix0 = __max(0, ix0);
  ix1 = ROUND(x + dtx);
  if (ix1 < 0)
    return 0;
  iw = ix1 - iw;
  ix1 = __min(ix1, w);

  // compute destination Y insertion bounds 
  iy0 = ROUND(y - dty);
  if (iy0 >= h)
    return 0;
  ih = iy0;
  iy0 = __max(0, iy0);
  iy1 = ROUND(y + dty);
  if (iy1 < 0)
    return 0;
  ih = iy1 - ih;
  iy1 = __min(iy1, h);

  // see if anything left or if clipping might occur
  rw = ix1 - ix0;
  rh = iy1 - iy0;
  if ((rw <= 0) || (rh <= 0))
    return 0;
  if ((rw < iw) || (rh < ih))
    clean = 0;

  // figure out real drawing color
  cvt_col(red, grn, blu, dest, r, g, b);

  // determine source pixel to start scan from
  sdy = 0.5 * tw + c2 * (ix0 - x) - s2 * (iy0 - y);
  cdy = 0.5 * th + s2 * (ix0 - x) + c2 * (iy0 - y);

  // scan region of destination image and get closest pixel from source
  d = dest.RoiDest(ix0, iy0);
  skip = dest.RoiSkip(rw);
  for (dy = rh; dy > 0; dy--, d += skip, sdy -= s2, cdy += c2)
  {
    // starting location in source image
    sxf = sdy;
    syf = cdy;

    // scan pixels in this line to see where they should come from
    for (dx = rw; dx > 0; dx--, d += f, sxf += c2, syf += s2)
    {
      // make sure sample position falls inside source image
      sx = ROUND(sxf);
      if ((sx < 0) || (sx >= tw))
        continue;
      sy = ROUND(syf);
      if ((sy < 0) || (sy >= th))
        continue;
  
      // set destination color if pixel in temporary is marked
      if (src.ARef(sx, sy) != 0)
        continue;
      if (f == 1)
        *d = (UC8) r;
      else
      {
        d[0] = (UC8) blu;
        d[1] = (UC8) grn;
        d[2] = (UC8) red;
      } 
    }
  }
  return clean;
}


//= Pick proper pixel value(s) based on requested color and image type.
// if negative red value given, then will use standard colors

void jhcLabel::cvt_col (int& red, int& grn, int& blu, const jhcImg& dest, int r, int g, int b) const
{
  int cols[8] = {0, 200, 128, 230, 50, 215, 70, 255};

  // constrain between 0 and 255
  red = BOUND(r); 
  grn = BOUND(g); 
  blu = BOUND(b);
  if (r >= 0)
    return;

  // adjust color if negative red supplied (jhcDraw::Color8)
  if (dest.Fields() == 1)
    red = cols[-r & 0x07];
  else
  {
    red = (((-r & 0x01) == 0) ? 0 : 255); 
    grn = (((-r & 0x02) == 0) ? 0 : 255); 
    blu = (((-r & 0x04) == 0) ? 0 : 255); 
  }
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Add text to image with end at X and centered on Y.
// height is font size in pixels, negative generates bold face
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping

int jhcLabel::LabelLeft (jhcImg& dest, double x, double y, const char *msg, 
                         int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelLeft");

  make_label(msg, ht, 1);
  return xfer_text(dest, ROUND(x - tw), ROUND(y - 0.5 * th), r, g, b, chk);
}


//= Add text to image with start at X and centered on Y.
// height is font size in pixels, negative generates bold face
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping

int jhcLabel::LabelRight (jhcImg& dest, double x, double y, const char *msg, 
                          int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelRight");

  make_label(msg, ht, -1);
  return xfer_text(dest, ROUND(x), ROUND(y - 0.5 * th), r, g, b, chk);
}


//= Add text to image with center at X and bottom at Y.
// height is font size in pixels, negative generates bold face
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping

int jhcLabel::LabelOver (jhcImg& dest, double x, double y, const char *msg,
                         int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelOver");

  make_label(msg, ht, 0);
  return xfer_text(dest, ROUND(x - 0.5 * tw), ROUND(y), r, g, b, chk);
}


//= Add text to image with center at X and top at Y.
// height is font size in pixels, negative generates bold face
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping

int jhcLabel::LabelUnder (jhcImg& dest, double x, double y, const char *msg,
                          int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelUnder");

  make_label(msg, ht, 0);
  return xfer_text(dest, ROUND(x - 0.5 * tw), ROUND(y - th), r, g, b, chk);
}


//= Add text to image centered on X and Y.
// height is font size in pixels, negative generates bold face
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping

int jhcLabel::LabelCenter (jhcImg& dest, double x, double y, const char *msg,
                           int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelCenter");

  make_label(msg, ht, 0);
  return xfer_text(dest, ROUND(x - 0.5 * tw), ROUND(y - 0.5 * th), r, g, b, chk);
}


//= Write out text string centered on X and Y but rotated by some number of degrees.
// height is font size in pixels, negative generates bold face
// generates double-sized original so that rotated sampling is dense enough
// returns 1 if okay, 0 if some clipping

int jhcLabel::LabelRotate (jhcImg& dest, double x, double y, double degs, const char *msg,
                           int ht, int r, int g, int b)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelRotate");

  make_label(msg, 2 * ht, 0);
  return xfer_text_rot(dest, x, y, degs, r, g, b);
}


//= Put label near box somewhere, preferably over top or to the left.
// height is font size in pixels, negative generates bold face
// returns 1 if okay, 0 if some clipping

int jhcLabel::LabelBox (jhcImg& dest, const jhcRoi& box, const char *msg, 
                        int ht, int r, int g, int b, int gap)
{
  int xmid, ymid;

  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelBox");

  // figure out size of message and offset from center to corner
  make_label(msg, ht, 0);
  xmid = ROUND(box.RoiMidX() - 0.5 * tw);
  ymid = ROUND(box.RoiMidY() - 0.5 * th);

  // try various positions
  if (xfer_text(dest, xmid, ROUND(box.RoiY2() + gap), r, g, b, 1) > 0)      // over
    return 1;
  if (xfer_text(dest, ROUND(box.RoiX2() + gap), ymid, r, g, b, 1) > 0)      // right
    return 1;
  if (xfer_text(dest, ROUND(box.RoiX() - gap - tw), ymid, r, g, b, 1) > 0)  // left
    return 1;
  if (xfer_text(dest, xmid, ROUND(box.RoiY() - gap - th), r, g, b, 1) > 0)  // under
    return 1;
  return xfer_text(dest, xmid, ymid, r, g, b, 0);                           // center
}
