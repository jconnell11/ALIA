// jhcTessOCR.cpp : uses Open Tesseract OCR to find and read diagram call-outs
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013 IBM Corporation
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
#include <windows.h>

#include "System/jhcTessOCR.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTessOCR::jhcTessOCR ()
{
  OSVERSIONINFO ver;

  // figure out where OCR system likely installed
  ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&ver);
  sprintf(prog, "C:\\Program Files%s\\Tesseract-OCR\\tesseract",  
          ((ver.dwMajorVersion >= 6) ? " (x86)" : ""));

  // maximum number of text fragments
  SetSize(500);
}


//= Default destructor does necessary cleanup.

jhcTessOCR::~jhcTessOCR ()
{
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Run OCR and remember text strings and positions in arrays.
// optionally blows up image before running OCR for better results
// psm: 3 = normal page parsing, 5 = single vertical block, 6 = single block
// setting norm > 0 cinverts HTML and filters out unlikely text strings
// returns number of text items found, negative for error

int jhcTessOCR::Analyze (const jhcImg& src, double mag, int psm, int norm)
{
  const jhcImg *img = &src;
  char cmd[200], tname[200], iname[200], base[40] = "jhc_tmp";
  int sdim = src.MaxDim(), lim = 5000;
  double m = (((mag * sdim) < lim) ? mag : (lim / (double) sdim));

  // no text boxes yet
  ClearAll();

  // possibly make larger version of source
  if (m != 1.0)
  { 
    big.SetSize(src, m);
//    FillFrom(big, src);         
    Bicubic(big, src);         
    img = &big;
  }

  // save good size image as file 
  sprintf(iname, "%s.bmp", base);
  jio.Save(iname, *img);

  // call tesseract to generate HOCR output
  sprintf(cmd, "\"%s\" %s %s -psm %d hocr", prog, iname, base, psm);
  system(cmd);

  // parse HTML to get text and bounding boxes
  sprintf(tname, "%s.html", base);
  parse_html(tname, img->YLim(), m, norm);
  return CountOver();
}


//= Parse HTML optical character recognition file to get text and areas.
// returns number of entries filled, negative if file problem

int jhcTessOCR::parse_html (const char *fname, int hmax, double mag, int norm)
{
  FILE *in;
  char frag[200], term[80], tag[40] = "id='word_", hdr[40] ="title=\"bbox";
  int bx[4];
  int i, n, x, y, w, h, tlen = (int) strlen(tag), seen = 0;

  // try opening file
  if ((in = fopen(fname, "r")) == NULL)
    return -1;

  // search for found words and positions in HOCR file
  saved = '\0';
  while (get_frag(frag, in, 0) > 0)
    if (strncmp(frag, tag, tlen) == 0)             // word marker
      if (sscanf(frag + tlen, "%d", &n) == 1)      // word number
      {
        // validate word number (0 based)
        n--;
        if ((n < 0) || (n >= Size()))
          n = Size() - 1;

        // ingest text box coordinates (top down Y)
        if (get_frag(frag, in, 0) <= 0)
          continue;
        for (i = 0; i < 4; i++)
          if (get_frag(frag, in, 0) <= 0)
            break;
          else if (sscanf(frag, "%d", bx + i) != 1)
            break;
        if (i < 4)
          continue;

        // read actual text and set up bounding box (bottom up Y)
        if (get_frag(term, in, 1) <= 0)
          continue;
        x = ROUND(bx[0] / mag);
        y = ROUND((hmax - bx[3]) / mag);
        w = ROUND((bx[2] - bx[0]) / mag);
        h = ROUND((bx[3] - bx[1]) / mag);
        if (SetItem(n, term, x, y, w, h, norm) > 0)
          seen++;
      }

  // clean up
  fclose(in);
  return(Active() - 1);
}


//= Get a fragment of an HTML file delimited by space, new line, > and <.
// can optionally stop once first terminating HTML tag is found
// returns 1 if successful, 0 if empty, negative if end of file

int jhcTessOCR::get_frag (char *txt, FILE *in, int term)
{
  char *f;
  char ch;

  while (1)
  {
    // start of new fragment
    f = txt;
    *f = '\0';
    ch = saved;
    saved = '\0';

    // trim off leading whitespace
    f = txt;
    if (ch == '\0')
    {
      ch = fgetc(in);
      if (feof(in) || ferror(in))
        return -1;
    }
    while (strchr(" \t\n", ch) != NULL)
    {
      ch = fgetc(in);
      if (feof(in) || ferror(in))
        return -1;
    }

    // transcribe up to end delimiter
    *f++ = ch;
    ch = fgetc(in);
    if (feof(in) || ferror(in))
      return 0;
    while (strchr("<> \t\n", ch) == NULL)
    {
      *f++ = ch;
      ch = fgetc(in);
      if (feof(in) || ferror(in))
        break;
    }

    // possibly copy end delimiter then terminate string
    if (ch == '<')
      saved = ch;
    else if (ch == '>')
      *f++ = ch;
    *f = '\0';

    // stop if not HTML tag or possibly if terminator
    if (txt[0] != '<')
      break;
    if ((term > 0) && (txt[1] == '/'))
    {
      *txt = '\0';
      return 0;
    }
  }

  // check if valid string
  return 1;
}
