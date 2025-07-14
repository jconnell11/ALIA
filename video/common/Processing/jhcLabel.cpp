// jhcLabel.cpp : add text captions to an image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2018 IBM Corporation
// Copyright 2024-205 Etaoin Systems
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

#ifndef __linux__
  #include <windows.h>                 // for WinExec
  #include <direct.h>                  // for _getcwd
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "Interface/jhcMessage.h"
#include "Processing/jhc_font.h"

#include "Processing/jhcLabel.h"


///////////////////////////////////////////////////////////////////////////
//                         Shared Class Variables                        //
///////////////////////////////////////////////////////////////////////////

//= How many instances are using font instance (loaded only once).

int jhcLabel::font_users = 0;


//= Variables must be declared even if not initialized.

int jhcLabel::cw[96], jhcLabel::ch[96];
UC8 *jhcLabel::pels[96];
int jhcLabel::full, jhcLabel::line;


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcLabel::jhcLabel ()
{
  FILE *out;
  int n = sizeof(jhc_font);

  // see if font already loaded from elsewhere
  if (font_users > 0)
  {
    font_users++;
    return;
  }

  // convert global array to file
  if (fopen_s(&out, "font_table.dat", "wb") == 0)
  {
    fwrite(jhc_font, 1, n, out);
    fclose(out);
  }
   
  // read data then cleanup
  load_font("font_table.dat");         // sets font_users
  remove("font_table.dat");    
}


//= Load character bitmaps from specially formatted file.
// set character widths, heights, and bitmaps
// also sets max char height "full" and spacing "line"

int jhcLabel::load_font (const char *fname)
{
  char hdr[80]; 
  FILE *in;
  int i, j, n;

  // initialize bitmaps then try opening file
  for (i = 0; i < 96; i++)
    pels[i] = NULL;
  if (fopen_s(&in, fname, "rb") != 0)
    return -2;

  // read clear-text header
  if (fgets(hdr, 80, in) == NULL)
    return fclose(in);
  if (strncmp(hdr, "JHC_FONT 96", 11) != 0)
    return fclose(in);
   
  // read in all character bitmaps
  full = 0;
  for (i = 0; i < 96; i++)
  {
    // get character dimensions  
    cw[i] = fgetc(in);
    ch[i] = fgetc(in);
    if (ferror(in) || feof(in) || (cw[i] <= 0) || (ch[i] <= 0))
      break;
    full = __max(full, ch[i]);

    // make bitmap and fill in values
    n = cw[i] * ch[i];
    pels[i] = new UC8 [n];
    for (j = 0; j < n; j++)
      (pels[i])[j] = (UC8) fgetc(in);
    if (ferror(in) || feof(in))
      break;
  }    

  // check for early quit then clean up
  line = full + 2;
  if (i >= 96)
    font_users = 1;
  fclose(in);
  return font_users;
}


//= Default destructor does necessary cleanup.

jhcLabel::~jhcLabel ()
{
  int i;

  if (font_users == 1)                 // last active user
    for (i = 95; i >= 0; i--)
      delete [] pels[i];
  font_users--;
}


//= If font not okay, try loading info based on this directory instead.
// assumes "path" name ends with a slash (or is NULL or empty)

void jhcLabel::SetDir (const char *path)
{
  char fname[80];

  if (font_users > 0) 
    return;
  sprintf_s(fname, "%sconfig/font_table.dat", ((path == NULL) ? "" : path));
  load_font(fname);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Add text to image with end at X and centered on Y.
// height is ignored (was font size in pixels, negative for bold)
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping, negative for font problem

int jhcLabel::LabelLeft (jhcImg& dest, double x, double y, const char *msg, 
                         int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelLeft");
  if (make_label(msg, 1, 1) <= 0)
    return -1;
  return xfer_text(dest, ROUND(x - tw), ROUND(y - 0.5 * th), r, g, b, chk);
}


//= Add text to image with start at X and centered on Y.
// height is ignored (was font size in pixels, negative for bold)
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping, negative for font problem

int jhcLabel::LabelRight (jhcImg& dest, double x, double y, const char *msg, 
                          int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelRight");

  if (make_label(msg, 1, -1) <= 0)
    return -1;
  return xfer_text(dest, ROUND(x), ROUND(y - 0.5 * th), r, g, b, chk);
}


//= Add text to image with center at X and bottom at Y.
// height is ignored (was font size in pixels, negative for bold)
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping, negative for font problem

int jhcLabel::LabelOver (jhcImg& dest, double x, double y, const char *msg,
                         int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelOver");

  if (make_label(msg, 1, 0) <= 0)
    return -1;
  return xfer_text(dest, ROUND(x - 0.5 * tw), ROUND(y), r, g, b, chk);
}


//= Add text to image with center at X and top at Y.
// height is ignored (was font size in pixels, negative for bold)
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping, negative for font problem

int jhcLabel::LabelUnder (jhcImg& dest, double x, double y, const char *msg,
                          int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelUnder");

  if (make_label(msg, 1, 0) <= 0)
    return -1;
  return xfer_text(dest, ROUND(x - 0.5 * tw), ROUND(y - th), r, g, b, chk);
}


//= Add text to image centered on X and Y.
// height is ignored (was font size in pixels, negative for bold)
// if chk > 0 will only alter pixels if no clipping happens
// returns 1 if okay, 0 if some clipping, negative for font problem

int jhcLabel::LabelCenter (jhcImg& dest, double x, double y, const char *msg,
                           int ht, int r, int g, int b, int chk)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelCenter");

  if (make_label(msg, 1, 0) <= 0)
    return -1;
  return xfer_text(dest, ROUND(x - 0.5 * tw), ROUND(y - 0.5 * th), r, g, b, chk);
}


//= Write out text string centered on X and Y but rotated by some number of degrees.
// height is ignored (was font size in pixels, negative for bold)
// generates double-sized original so that rotated sampling is dense enough
// returns 1 if okay, 0 if some clipping, negative for font problem

int jhcLabel::LabelRotate (jhcImg& dest, double x, double y, double degs, const char *msg,
                           int ht, int r, int g, int b)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelRotate");

  if (make_label(msg, 2, 0) <= 0)
    return -1;
  return xfer_text_rot(dest, x, y, degs, r, g, b);
}


//= Put label near box somewhere, preferably over top or to the left.
// height is ignored (was font size in pixels, negative for bold)
// returns 1 if okay, 0 if some clipping, negative for font problem

int jhcLabel::LabelBox (jhcImg& dest, const jhcRoi& box, const char *msg, 
                        int ht, int r, int g, int b, int gap)
{
  int xmid, ymid;

  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelBox");

  // figure out size of message and offset from center to corner
  if (make_label(msg, 1, 0) <= 0) 
    return -1;
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


//= Overlay left-to-right colored label with solid black (or white) background.
// text starts at X and centered on Y, if wht > 0 background is white instead
// returns 1 if okay, 0 if some clipping, negative for font problem

int jhcLabel::LabelSolid (jhcImg& dest, double x, double y, const char *msg,
                          int wht, int r, int g, int b)
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcLabel::LabelRight");

  if (make_label(msg, 1, -1) <= 0)
    return -1;
  blank_panel(dest, x, y, wht, 5);
  return xfer_text(dest, ROUND(x), ROUND(y - 0.5 * th), r, g, b, 0);
}


//= Erase a portion image as a background for copied text.
// needs tw and th from make_label() to calculate size

void jhcLabel::blank_panel (jhcImg& dest, double x, double y, int wht, int pad) const
{
  int x0 = ROUND(x) - pad, y0 = ROUND(y - 0.5 * th) - pad;
  int x1 = x0 + tw + 2 * pad, y1 = y0 + th + 2 * pad;
  int xlim = dest.XLim(), ylim = dest.YLim(), nf = dest.Fields();
  int rx, ry, f, sk;
  UC8 fill = ((wht > 0) ? 255 : 0);
  UC8 *d;

  // define rectangle slightly bigger than text
  x0 = __max(0, x0);
  x1 = __min(x1, xlim);
  y0 = __max(0, y0);
  y1 = __min(y1, ylim);
  if ((x0 > x1) || (y0 > y1))
    return;

  // set all pixels in rectange to some value
  d = dest.RoiDest(x0, y0);
  sk = dest.RoiSkip(x1 - x0 + 1);
  for (ry = y0; ry <= y1; ry++, d += sk)
    for (rx = x0; rx <= x1; rx++)
      for (f = nf; f > 0; f--, d++)
        *d = fill;
}


///////////////////////////////////////////////////////////////////////////
//                          Label Generation                             //
///////////////////////////////////////////////////////////////////////////

//= Render message to temporary image of sufficient size.
// just: -1 = left, 0 = center, 1 = right (ht ignored)
// saves string in member variable "src" (text is black)
// returns 1 if ok, 0 for font problem

int jhcLabel::make_label (const char *txt, int mag, int just)
{
  const char *scan = txt;
  int cnt, w, y0, x0 = 0;

  // check for font then setup output image
  if (font_users <= 0)
    return 0;
  cnt = set_size(txt, mag);
  y0 = lab.YDim() - mag * full;

  while (*scan != '\0')
  {
    // check for return character
    if (strchr("\n\r\x0A", *scan) != NULL)
    {
      x0 = 0;
      y0 -= mag * line;
      scan++;
      continue;
    }

    // possibly adjust x start for text justification
    if ((x0 == 0) && (cnt > 1) && (just >= 0))
    {
      line_len(w, scan);
      w *= mag;
      if (just > 0)
        x0 = tw - w;         // right aligned 
      else
        x0 = (tw - w) / 2;   // centered
    }

    // copy glyph for valid characters
    if ((*scan >= 32) && (*scan < 127))
      x0 = insert_pels(x0, y0, *scan - 32, mag);
    scan++;
  }
  return 1;
}


//= Set label image to appropriate size based on text.
// records overall size in tw and th for convenience
// returns number of lines in message 

int jhcLabel::set_size (const char *txt, int mag) 
{
  const char *scan = txt;
  int cnt = 0, sum = 0;

  // determine maximum width of any line
  tw = 0;
  while ((scan = line_len(sum, scan)) != NULL)
  {
    tw = __max(tw, sum);
    cnt++;
  }
  tw = __max(tw, sum);
  tw *= mag;

  // adjust height for number of lines and set canvass
  th = full + cnt * line;
  th *= mag;
  lab.SetSize(tw, th, 1);
  lab.FillArr(255);
  return(cnt + 1);
}


//= Determine number of characters up until next return character.
// returns remainder of string (NULL if none)

const char *jhcLabel::line_len (int& w, const char *txt) const
{
  const char *scan = txt;
 
  w = 0;
  while (*scan != '\0')
  {
    if (strchr("\n\r\x0A", *scan) != NULL)       // return found
      return(scan + 1);
    if ((*scan >= 32) && (*scan < 127))
      w += cw[*scan - 32];                       // width of char
    scan++;
  }  
  return NULL;                                   // end of text
}


//= Threshold and copy character bitmap into text image.
// can optionally expand each pixel by factor "mag"
// returns updated x0 for next character (for convenience)

int jhcLabel::insert_pels (int x0, int y0, int i, int mag)
{
  int x, y, j, k, dln2, w = cw[i], h = ch[i], dln = lab.Line();
  UC8 *d, *d0 = lab.RoiDest(x0, y0); 
  const UC8 *s = pels[i];

  // transfer character bitmap bottom-up to caption bottom-up
  if (mag <= 1)
  {
    // simple version
    for (y = h; y > 0; y--, d0 += dln)
      for (x = w, d = d0; x > 0; x--, s++, d++)
        *d = *s;
  }
  else if (mag == 2)
  {
    // specialized version
    dln2 = 2 * dln;
    for (y = h; y > 0; y--, d0 += dln2)
      for (x = w, d = d0; x > 0; x--, s++, d += 2)
      {
        d[0] = *s;
        d[1] = *s;
        d[dln] = *s;
        d[dln + 1] = *s;
      }
  }
  else
  {
    // generic version
    for (y = h; y > 0; y--)
      for (j = 0; j < mag; j++, d0 += dln)
        for (x = w, d = d0; x > 0; x--, s++, d += mag)
          for (k = 0; k < mag; k++)
            d[k] = *s;
  }
  return(x0 + mag * w);
}


///////////////////////////////////////////////////////////////////////////
//                              Pixel Setting                            //
///////////////////////////////////////////////////////////////////////////

//= Copy non-background pixels from member "src" to given image.
// takes left bottom corner of destination location 
// if chk > 0 will only alter pixels if no clipping happens
// if negative red value given, then will use standard colors
// returns 1 if okay, 0 if some clipping

int jhcLabel::xfer_text (jhcImg& dest, int x, int y, int r, int g, int b, int chk)
{
  int dw = dest.XDim(), x0 = __max(0, x), x1 = x + tw, rw = __min(x1, dw) - x0;
  int dh = dest.YDim(), y0 = __max(0, y), y1 = y + th, rh = __min(y1, dh) - y0;
  int red, grn, blu, dx, dy, dsk = dest.RoiSkip(rw), ssk = lab.RoiSkip(rw), clean = 1;
  UC8 *d = dest.RoiDest(x0, y0);
  const UC8 *s = lab.RoiSrc(x0 - x, y0 - y);

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
      if (lab.ARef(sx, sy) != 0)
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

