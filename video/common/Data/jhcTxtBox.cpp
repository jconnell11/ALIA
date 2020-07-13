// jhcTxtBox.cpp : associates text string with a region of an image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2014 IBM Corporation
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

#include <string.h>
#include <ctype.h>
#include <math.h>

#include "Interface/jhcMessage.h"

#include "jhcTxtBox.h"


//= Maximum length of text in boxes (be careful if whole document).

#define JHC_TBOX  200     


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTxtBox::jhcTxtBox ()
{
  total = 0;
  valid = 0;
  text = NULL;
  area = NULL;
  mark = NULL;
}


//= Default destructor does necessary cleanup.

jhcTxtBox::~jhcTxtBox ()
{
  dealloc();
}


//= Get rid of all heap allocated items.

void jhcTxtBox::dealloc ()
{
  int i;

  // clean up   
  delete [] mark;
  delete [] area;
  if (text != NULL)
    for (i = total - 1; i >= 0; i--)
      delete [] text[i];
  delete [] text;

  // zero pointers
  total = 0;
  valid = 0;
  text = NULL;
  area = NULL;
  mark = NULL;
}


//= Set up to handle up to n text fragments.

void jhcTxtBox::SetSize (int n)
{
  int i, ans = 1;

  // possibly cleanup previous version
  if (n != total)
    dealloc();
  if (n <= 0)
    return;

  // allocate structures
  if ((text = new char * [n]) == NULL)
    ans = 0;
  else 
    for (i = 0; i < n; i++)
      if ((text[i] = new char[JHC_TBOX]) == NULL)
        ans = 0;
  if ((area = new jhcRoi [n]) == NULL)
    ans = 0;
  if ((mark = new int [n]) == NULL)
    ans = 0;

  // record size, nothing in list yet
  if ((n > 0) && (ans <= 0))
    Fatal("jhcTxtBox::SetSize - Array allocation failed!");   
  total = n;
  ClearAll();
}


//= Get rid of all data for all entries in list.

void jhcTxtBox::ClearAll ()
{
  int i;

  for (i = 0; i < total; i++)
    ClearItem(i);
  valid = 0;
}


//= Get rid of a particular item in list.
// always returns 0 for convenience

int jhcTxtBox::ClearItem (int i)
{
  if ((i >= 0) && (i < total))
  {
    *(text[i]) = '\0';
    area[i].ClearRoi();
    mark[i] = 0;
  }
  return 0;
}


//= Completely copy all entries from some other list of text boxes.
// if new array is smaller than src then some entries at tail may be skipped

void jhcTxtBox::CopyAll (const jhcTxtBox& src)
{
  int i, n = __min(total, src.total);

  ClearAll();
  for (i = 0; i < n; i++)
    if (((src.text)[i])[0] != '\0')
    {
      strcpy(text[i], (src.text)[i]);
      area[i].CopyRoi((src.area)[i]);
      mark[i] = (src.mark)[i];
      valid = i;
    }
}


///////////////////////////////////////////////////////////////////////////
//                              Building List                            //
///////////////////////////////////////////////////////////////////////////

//= Create a new list entry with given text string and position.
// can optionally massage string to normalize it
// returns 1 if added, 0 if string essentially empty

int jhcTxtBox::AddItem (const char *txt, int x, int y, int w, int h, int chk)
{
  int i;

  for (i = 0; i < total; i++)
    if ((text[i])[0] == '\0')
      break;
  return SetItem(i, txt, x, y, w, h, chk);
}


//= Alter a specific list entry to have new text string and position.
// can optionally massage string to normalize it
// returns 1 if added, 0 if string essentially empty

int jhcTxtBox::SetItem (int i, const char *txt, int x, int y, int w, int h, int chk)
{
  // check for reasonable index
  if ((i < 0) || (i >= total))
    return 0;

  // possibly clean up string
  if (chk <= 0)
    strcpy(text[i], txt);
  else if (norm_label(text[i], txt) <= 0)
    return ClearItem(i);

  // set other box information
  area[i].SetRoi(x, y, w, h);   
  mark[i] = 1;

  // possibly extend length of active list
  valid = __max(valid, i + 1);
  return 1;
}


//= Rejects OCR results that are unlikely to be real labels.
// substitutes symbols for special HTML sequences
// returns 1 if good, 0 if bad (and clears input string)

int jhcTxtBox::norm_label (char *txt, const char *src)
{
  char tmp[JHC_TBOX];
  const char *t = txt;

  // get rid of strange characters and translate HTML
  *txt = '\0';
  pure_ascii(tmp, src);
  if (*tmp == '\0')
    return 0;
  cvt_html(txt, tmp);

  // make sure not all punctuation (lone hyphens allowed)
  while (*t != '\0')
    if (isalnum(*t) || (*t == '-'))
      return 1;
    else
      t++;
  *txt = '\0';
  return 0;
}


//= Copy string but omit any non-ASCI core characters.

void jhcTxtBox::pure_ascii (char *dest, const char *src) const
{
  const char *s = src;
  char *d = dest;
  UC8 c, c2, c3;

  while (*s != '\0')
  {
    // get short window into head of remaining string
    c = (UC8)(*s++);
    c2 = (UC8) s[0];
    c3 = 0;
    if (c2 != 0)
      c3 = (UC8) s[1];

    // check for normal ascii and special kerning patterns
    if (c == 124)
      *d++ = 'l';                            // vertical bar uncommon
    else if ((c >= 32) && (c <= 126))
      *d++ = c;
    else if ((c == 226) && (c2 == 128) && (c3 == 148))
    {
      *d++ = '-';
      s += 2;
    }
    else if ((c == 194) && (c2 == 187))
    {
      *d++ = '-';
      s += 2;
    }
    else if ((c == 239) && (c2 == 172) && (c3 == 130))
    {
      *d++ = 'f'; 
      *d++ = 'l'; 
      s += 2;
    }
    else if ((c == 239) && (c2 == 172) && (c3 == 129))
    {
      *d++ = 'f'; 
      *d++ = 't'; 
      s += 2;
    }
//else
//jprintf("reject <%c> [%d] in %s\n", c, c, src);
  }
  *d = '\0';
}


//= Substitute single characters for HTML sequences.

void jhcTxtBox::cvt_html (char *dest, const char *src) const
{
  char html[11][20] = {"\"'&<>\"'&<>", 
                       "quot", "apos", "amp",  "lt",  "gt", 
                        "#34",  "#39", "#38", "#60", "#62"};
  char tag[20];
  const char *end, *s = src;
  char *d = dest;
  int i, n;

  // convert HTML sequences to single characters
  while (*s != '\0')
  {
    // look for start and end delimiters
    if ((*s != '&') || ((end = (strchr(s + 1, ';'))) == NULL))
    {
      // just copy character
      *d++ = *s++;
      continue;
    }

    // get central tag if delimiters are reasonably spaced
    n = end - (s + 1);
    if (n > 4) 
    {
      // give up and just copy
      *d++ = *s++;
      continue;
    }
    strncpy(tag, s + 1, n);
    tag[n] = '\0';

    // see if tag in lookup table (ignore most special characters)
    for (i = 1; i < 11; i++)
      if (strcmp(tag, html[i]) == 0)
      {
        // replace with equivalent character
        *d++ = html[0][(i - 1)];
         s++;
         continue;
      }

    // skip over sequence if no translation
    s = end + 1;
  }

  // terminate new string
  *d = '\0';
}


///////////////////////////////////////////////////////////////////////////
//                                Restrictions                          //
///////////////////////////////////////////////////////////////////////////

//= Count non-blank labels that have been found.
// only counts those whose mark is also at least mth (0 = all)

int jhcTxtBox::CountOver (int mth) const
{
  int i, n = 0;

  for (i = 0; i < valid; i++)
    if (text[i][0] != '\0')
      if (mark[i] >= mth)
        n++;
  return n;
}


//= Get rid of any text boxes that are too big or too small.
// returns number of boxes that are left

int jhcTxtBox::BoxHt (int hi, int lo)
{
  int i, ht, n = 0;

  for (i = 0; i < valid; i++)
    if (Selected(i))
	  {
      ht = area[i].RoiH();
	    if ((ht > hi) || (ht < lo))
        mark[i] = 0; 
	    else
        n++;
	  }
  return n;
}


//= Restrict boxes to single characters in given list (e.g. "ABC").
// trims non-letter or digit portions, ignores capitalization
// ignores previous marks, can optionally alter box text if matched
// returns number meeting criterion

int jhcTxtBox::MatchOnly (const char *choices, int alt)
{
  const char *t, *hit;
  int i, n = 0;

  for (i = 0; i < valid; i++)
    if (Selected(i))
    {
      // skip leading cruft
      mark[i] = 0;
      t = text[i];
      while (!isalnum(*t))
        if (*t != '\0')
          t++;
        else
          break;

      // check if in list (ignore capitalization)
      if ((hit = strchr(choices, tolower(*t))) == NULL) 
        if ((hit = strchr(choices, toupper(*t))) == NULL)
          continue;

      // check for only cruft at end
      t++;
      while (!isalnum(*t))
        if (*t != '\0')
          t++;
        else
        {
          // all checked so good match found
          mark[i] = 1;
          n++;
          if (alt > 0)
          {
            text[i][0] = *hit;
            text[i][1] = '\0';
          }
          break;
        }
      }
  return n;
}


//= Restrict to only letters with a few additions as given.
// ignores previous marks, can optionally enforce capitalization
// returns number meeting criterion

int jhcTxtBox::AlphaOnly (const char *extra, int cap)
{
  const char *t;
  int i, n = 0;

  for (i = 0; i < valid; i++)
    if (Selected(i))
    {
      // assume entry will pass
      mark[i] = 1;
      n++;

      // check all characters in entry
      t = text[i];
      while (*t != '\0')
        if ((isalpha(*t) && ((cap <= 0) || isupper(*t))) || 
            (strchr(extra, *t) != NULL))
          t++;
        else
        {
          // entry failed
          mark[i] = 0;
          n--;
          break;
        }
    }
  return n;
}


//= Restrict to only numbers with a few additions as given.
// extra is often "+-.", ignores previous marks
// returns number meeting criterion

int jhcTxtBox::NumOnly (const char *extra)
{
  const char *t;
  int i, n = 0;

  for (i = 0; i < valid; i++)
    if (Selected(i))
    {
      // assume entry will pass
      mark[i] = 1;
      n++;

      // check all characters in entry
      t = text[i];
      while (*t != '\0')
        if (isdigit(*t) || (strchr(extra, *t) != NULL))
          t++;
        else
        {
          // entry failed
          mark[i] = 0;
          n--;
          break;
        }
    }
  return n;
}


//= Restrict to only letters and numbers with a few additions as given.
// ignores previous marks, can optionally enforce capitalization
// returns number meeting criterion

int jhcTxtBox::AlnumOnly (const char *extra, int cap)
{
  const char *t;
  int i, n = 0;

  for (i = 0; i < valid; i++)
    if (Selected(i))
    {
      // assume entry will pass
      mark[i] = 1;
      n++;

      // check all characters in entry
      t = text[i];
      while (*t != '\0')
        if ((isalpha(*t) && ((cap <= 0) || isupper(*t))) || 
            (isdigit(*t) || (strchr(extra, *t) != NULL)))
          t++;
        else
        {
          // entry failed
          mark[i] = 0;
          n--;
          break;
        }
    }
  return n;
}


//= Only keep items with text length in range given (inclusive).
// can restrict count to being only alphanumerics (no punctuation or whitespace)
// a "hi" value of zero means anything longer than "lo" is acceptable
// returns number of boxes that are left

int jhcTxtBox::LengthOnly (int lo, int hi, int nopunc)
{
  int i, j, n, cnt, pass = 0;

  for (i = 0; i < valid; i++)
    if (Selected(i))
	  {
      // count how many real characters there are
      n = (int) strlen(text[i]);
      if (nopunc > 0)
	    {
        cnt = 0;
        for (j = 0; j < n; j++)
          if (isalnum(text[i][j]))
            cnt++;
        n = cnt;
	    }

      // invalidate text box if too few good chars
      if ((n < lo) || ((hi > 0) && (n > hi)))
        mark[i] = 0; 
	    else
        pass++;
	}
  return pass;
}


//= Change the marking on some particular text box to some value.

void jhcTxtBox::MarkN (int i, int val)
{
  if ((i > 0) && (i < total))
    mark[i] = val;
}


//= Mark all non-empty entries as valid again.
// restricts valid to range of non-blank entries as a side effect 
// returns number found

int jhcTxtBox::MarkAll (int val)
{
  int i, last = 0, n = 0;

  for (i = 0; i < valid; i++)
    if (text[i][0] != '\0')
    {
      mark[i] = val;
      last = i;
      n++;
    }
  valid = __min(valid, last + 1);
  return n;
}


//= Checks for non-empty text string and positive mark.

bool jhcTxtBox::Selected (int n) const
{
  if ((n >= 0) && (n < valid))
    if ((text[n][0] != '\0') && (mark[n] >= 1))
      return true;
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                               Box Merging                             //
///////////////////////////////////////////////////////////////////////////

//= Merge horizontally adjacent text fragments into a single line.
// box vertical centers must not be shifted too much wrt char height (shift)
// horizontal gap between boxes must not be too much wrt char width (gap)
// NOTE: alters and/or deletes original text boxes 
// NOTE: total text is limited to 200 characters per box
// returns number of remaining entries

int jhcTxtBox::MergeH (double gap, double shift, double sc)
{
  double mid0, cw0, mid, cw, ccnt;
  int i, j, n, rt0, ht0, lf, ht, dx, win, best, sp;

  // look through remaining valid entries
  for (i = 0; i < valid; i++)
    if (Selected(i))
      while (1)
      {
        // find middle of right side of this text box
        mid0 = area[i].RoiAvgY();
        rt0  = area[i].RoiX2();
        ht0  = area[i].RoiH();
        cw0  = area[i].RoiW() / (double) strlen(text[i]);

        // search for best linkage to the right only
        win = i;
        for (j = i + 1; j < valid; j++)
          if (((text[j])[0] != '\0') && (mark[j] > 0))
          {
            // see if this box is truly to right 
            lf = area[j].RoiX();
            if (lf <= rt0)
              continue;

            // get adjusted size of typical characters
            n = (int) strlen(text[j]);
            cw = area[j].RoiW() / n;
            ht = area[j].RoiH();
            if (n == 1)
            {
              ht = area[j].RoiMaxDim();    // isolated "-"
              cw = __max(cw, 0.5 * ht);    // isolated "|"
            }

            // check for font size and vertical offset
            if ((__max(ht, ht0) / (double) __min(ht, ht0)) > sc)
              continue;
            mid = area[j].RoiAvgY();
            if ((fabs(mid - mid0) / __min(ht, ht0)) > shift)
              continue;

            // measure gap width in terms of characters (even skinny "l")
            dx = lf - rt0;
            ccnt = dx / __min(cw, cw0);
            if (ccnt > gap)
              continue;
          
            // keep best gap
            if ((win <= i) || (dx < best))
            {
              win = j;
              best = dx;
              sp = ROUND(ccnt);
            }
          }

        // link best box by expanding base element
        if (win <= i)
          break;
        area[i].AbsorbRoi(area[win]);

        // append two pieces of text with some spaces in between
        sp = __max(1, sp);
        for (j = 0; j < sp; j++)
          strcat(text[i], " ");
        strcat(text[i], text[win]);

        // try again with new right side
        mark[win] = 0;
      }
  return CountOver(1);
}


//= Merge vertically adjacent text fragments into a single block.
// boxes must horizontally overlap by certain fraction of smaller (lap)
// vertical gap between boxes must not be too much wrt char height (gap)

int jhcTxtBox::MergeV (double gap, double overlap, double sc)
{
  double ch0, ch;
  int i, j, bot0, lf0, rt0, wid0, top, lf, rt, wid, dy, span, best, win, n = 0;

  // look through remaining valid entries
  for (i = 0; i < valid; i++)
    if (Selected(i))
      while (1)
      {
        // find span of item and bottom edge of box
        bot0 = area[i].RoiY(); 
        ch0  = area[i].RoiH() / (double) mark[i];
        lf0  = area[i].RoiX();
        rt0  = area[i].RoiLimX();
        wid0 = rt0 - lf0 + 1;

        // search for best linkage below only
        win = i;
        for (j = i + 1; j < valid; j++)
          if (((text[j])[0] != '\0') && (mark[j] > 0))
          {
            // make sure below
            top = area[j].RoiY2();
            if (top > bot0)
              continue;

            // check for font size compatibility
            ch = area[j].RoiH() / (double) mark[j];
            if ((__max(ch, ch0) / __min(ch, ch0)) > sc)
              continue;

            // check for reasonable vertical spacing
            dy = bot0 - top;
            if ((dy / __min(ch, ch0)) > gap)
              continue;

            // make sure the pieces overlap sufficiently horizontally
            lf = area[j].RoiX();
            rt = area[j].RoiLimX();
            wid = rt - lf + 1;
            span = __min(rt, rt0) - __max(lf, lf0);
            if ((span / (double) __min(wid, wid0)) < overlap)
              continue;

            // keep best gap
            if ((win <= i) || (dy < best))
            {
              win = j;
              best = dy;
            }
          }

        // link best box by expanding base element
        if (win <= i)
          break;
        area[i].AbsorbRoi(area[win]);

        // append two pieces of text with a single space in between
        strcat(text[i], " ");
        strcat(text[i], text[win]);

        // record lines of text so far then try again with new bottom
        mark[i] += mark[win];                     
        mark[win] = 0;
      }
  return CountOver(1);
}


///////////////////////////////////////////////////////////////////////////
//                             Box Splitting                             //
///////////////////////////////////////////////////////////////////////////

//= Attempt to interpret text boxes as single character labels.
// all must be less than "nchar" long and at least "frac" must be single
// splits strings into separate bounding boxes and forces capitalization

void jhcTxtBox::SplitSingle (double frac, int nchar)
{
  double cw;
  int i, j, len, cnt, cx, cy, ch, big = 0, uno = 0, n = 0;

  // scan through valid entries
  for (i = 0; i < valid; i++)
    if (((text[i])[0] != '\0') && (mark[i] == 1))  // no multi-line blocks
	  {
      // check number of real chars in current text block
      len = (int) strlen(text[i]);
      cnt = 0;
      for (j = 0; j < len; j++)
        if (isalnum((text[i])[j]))
          cnt++;
      if (cnt > nchar)
        big++;
      n++;

      // possibly capitalize if already single     
      if (cnt == 1)
	    {
        for (j = 0; j < len; j++)
          text[i][j] = toupper(text[i][j]);
        uno++;
	    }
	  }

  // see if high enough fraction are single to continue
  if ((big > 0) || (n <= 0) || ((uno / (double) n) < frac))
    return;

  // split entries as needed
  for (i = 0; i < valid; i++)
    if (((text[i])[0] != '\0') && (mark[i] == 1))  // no multi-line blocks
	  {
      // check number of real chars in current text block
      len = (int) strlen(text[i]);
      cnt = 0;
      for (j = 0; j < len; j++)
        if (isalnum((text[i])[j]))
          cnt++;
      if (cnt <= 1)
        continue;

      // get position and size of first box
      cx = area[i].RoiX();
      cy = area[i].RoiY();
      cw = area[i].RoiW() / (double) len;
      ch = area[i].RoiH();
        
      // make new boxes for other characters
      for (j = 1; j < len; j++)
        if (valid >= total)
          break;
	      else if (isalnum(text[i][j]))
	      {
          // peel off character and horizontal fraction of original box
          text[valid][0] = toupper(text[i][j]);
          text[valid][1] = '\0';
          area[valid].SetRoi(ROUND(cx + j * cw), cy, ROUND(cw), ch);
          mark[valid] = mark[i];
          valid++;
	      }
 
      // shrink original box to only first character
      text[i][0] = toupper(text[i][0]);
      text[i][1] = '\0';
      area[i].SetRoi(cx, cy, ROUND(cw), ch);
	  } 
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Draws bounding boxes around text fragments and optionally transcribes them.
// requires jhcDraw and jhcLabel as base classes
/*
void jhcTxtBox::ShowBoxes (jhcImg& dest, int label, int r, int g, int b)
{
  int i, n = Active();

  for (i = 0; i < n; i++)
    if (Selected(i))
    {
      RectEmpty(dest, *(BoxN(i)), 1, r, g, b);
      if (label > 0)
        LabelBox(dest, *(BoxN(i)), TxtN(i), 16, r, g, b);
    }
}
*/
