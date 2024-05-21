// jhcListVSrc.cpp : makes a video from a file containing a list of names
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
// Copyright 2023 Etaoin Systems
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

#include "Video/jhcListVSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

#if defined(JHC_MSIO)
  #if defined(JHC_JRST)
    JREG_VSRC(jhcListVSrc, "txt cam lst bmp pgm 664 ras jpg jpeg tif tiff gif png img");
  #else
    JREG_VSRC(jhcListVSrc, "txt cam lst bmp pgm 664 ras jpg jpeg tif tiff gif png");
  #endif
#else
  #if defined(JHC_JRST)
    JREG_VSRC(jhcListVSrc, "txt cam lst bmp pgm 664 ras jpg jpeg tif tiff img");
  #elif defined(JHC_TIFF)
    JREG_VSRC(jhcListVSrc, "txt cam lst bmp pgm 664 ras jpg jpeg tif tiff");
  #elif defined(JHC_JPEG)
    JREG_VSRC(jhcListVSrc, "txt cam lst bmp pgm 664 ras jpg jpeg");
  #else
    JREG_VSRC(jhcListVSrc, "txt cam lst bmp pgm 664 ras");
  #endif
#endif

#endif  // JHC_GVID


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Destructor closes any open list of file names.

jhcListVSrc::~jhcListVSrc ()
{
  if (list != NULL)
    fclose(list);
  delete [] daux;          // better done in base class?
}


//= Construct source given a file name.
// index request ignored since simple array indexing suffices
// assumes 1 Hz display framerate

jhcListVSrc::jhcListVSrc (const char *name, int index)
{
  // make up space for auxilliary data
  daux = new UC8[256];

  // set up default values
  strcpy_s(kind, "jhcListVSrc");
  def = 0;
  frt = 0.0;
  list = NULL;
  aspect = 1.0;
  freq = 1.0;
  strcpy_s(list_name, name);
  ParseName(name);

  // make a list of names if needed, then start reading
  if (strchr(BaseName, '*') != NULL)
    make_list();
  if ((strcmp(Ext, ".txt") == 0) || (strcmp(Ext, ".cam") == 0) || (strcmp(Ext, ".lst") == 0))
    ok = read_list();
  else 
    ok = repeat_img();
}


//= Return either base filename or full filename for current image.

const char *jhcListVSrc::FrameName (int idx_wid, int full) 
{
  if (full > 0)
    return iname.File();
  return iname.Base();
}


//= Find the index of the frame whose image file name matches tag.
// returns -1 if no match, else gives proper starting frame number
// Note: leaves video source positioned so named image is next Get

int jhcListVSrc::FrameMatch (const char *tag)
{
  if (!Valid() || (tag == NULL) || (*tag == '\0'))
    return -1;

  // rewind video and scan image names
  Rewind();
  while (next_line() > 0)
  {
    // turn file line into image name
    jio.BuildName(entry, -1);
    iname.ParseName(jio.File());           

    // save number if matching name image found
    if (strcmp(iname.Base(), tag) == 0)
    {
      // unread last image name
      fseek(list, backup, SEEK_SET);
      return nextread;
    }
    nextread++;
  }

  // rewind for failure
  Rewind();
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                              Initialization                           //
///////////////////////////////////////////////////////////////////////////

//= Set up to repeat a single image many times.

int jhcListVSrc::repeat_img ()
{
  char dname[500];

  // check that main image exists
  nframes = 1;                              // default number of repeats  
  iname.ParseName(list_name);
  if (jio.Specs(&w, &h, &d, list_name, 1) <= 0)
    return 0;

  // see if there is an associated secondary (depth) image
  sprintf_s(dname, "%s_z.ras", iname.Trimmed());
  if (jio.Specs(&w2, &h2, &d2, dname, -1) > 0)
    kinect_geom();
  return 1;
}


//= Hack based on depth image size to set Kinect 1 vs 2 geometry.

void jhcListVSrc::kinect_geom ()
{
  // Kinect 2 geometry (h2 = 540)
  if (h2 > 500)
  {
    flen2 = 540.685;   // was 535
    dsc2  = 1.0;
    flen  = flen2;
    if (w > 1000)
      flen *= 2.0;
    return;
  }

  // Kinect 1 geometry (h2 = 480)
  flen2 = 525.0;
  dsc2  = 0.9659;
  flen  = flen2;
  if (w > 1000)
    flen *= 2.0;
}


//= Determine all files which match the given specification.
// creates a script file (because commands are int) then executes
// for DOS and Windows makes temporary batch file 
// e.g."\s3vvc5m3.bat" containing commands:
//   @echo off
//   <working-disk>:
//   cd <working-directory>
//   <pattern-disk>:
//   cd <pattern-directory>
//   dir /ON/B <base-pattern> > <place>

void jhcListVSrc::make_list ()
{
  char *start;
  char clean[250], script[250], list[250] = "tmp_list.lst";
  FILE *out;

  // create first line specifying target directory
  if (fopen_s(&out, list, "w") != 0)
    return;
  fprintf(out, "%s*\n", JustDir);
  fclose(out);

  // make sure pattern has no forward slashes
  strcpy_s(clean, FileName);
  start = clean;
  while (*start != '\0')
    if (*start == '/')
      *start++ = '\\';
    else
      start++;

  // create a new script file which will append name list
  sprintf_s(script, "dir /ON/B/-W/-P/A:-D \"%s\" >> %s", clean, list);
  system(script);
  strcpy_s(list_name, list);
  ParseName(list_name);
}


//= Create instructions to go to some directory.
// automatically handles change of disk if needed

void jhcListVSrc::shift_dir (FILE *out, char *path)
{
  char misc[250];
  char *rest;
  int n;

  // if no disk change required, just change directory
  strcpy_s(misc, path);
  if ((rest = strchr(misc, ':')) == NULL)
  {
    fprintf(out, "cd %s\n", path);
    return;
  }

  // else change disk first then directory
  *rest = '\0';
  fprintf(out, "%s:\n", misc); 
  rest++;
  n = (int) strlen(rest) - 1;
  if (rest[n] == '\\')
    rest[n] = '\0';                               
  fprintf(out, "cd %s\n", rest); 
}


//= Read a formatted list of images (binds "list" pointer).
// first line may be special framerate spec, e.g. ">FPS 30.0"
// first line may be spec default with "*" for file names
// assumes all images will be same size as first one

int jhcListVSrc::read_list ()
{
  int n = 1;
  char full[500], dname[500];

  // open file and check first line (or two)
  if (fopen_s(&list, list_name, "r") != 0)
    return 0;
  if (next_line() <= 0)
    return 0;

  // special framerate specifier found so parse and then read next line
  if (strncmp(entry, ">FPS ", 5) == 0)
  {
    if (sscanf_s(entry + 5, "%f", &frt) != 1)
      return 0;
    freq = frt;
    if (next_line() <= 0)
      return 0;
  }
  
  // if first line is a default specification
  // append listing file's directory unless a different disk is mentioned
  if (strchr(entry, '*') != NULL)
  {
    def = 1;
    if (strchr(entry, ':') != NULL) 
      strcpy_s(full, entry);
    else if ((strstr(entry, "//") != NULL) || (strstr(entry, "\\\\") != NULL))
      sprintf_s(full, "%s%s", DiskSpec, entry + 1);
    else
      sprintf_s(full, "%s%s", JustDir, entry); 
    jio.SaveSpec(full);
  }
  else 
    jio.SaveDir(FileName);

  // determine standard image size (must be at least one image in list)
  if (def != 0)
    if (next_line() <= 0)
      return 0;
  if (jio.Specs(&w, &h, &d, entry, -1) <= 0)
    return 0;
  iname.ParseName(jio.File());

  // see if there is an associated secondary (depth) image
  sprintf_s(dname, "%s_z.ras", iname.Trimmed());
  if (jio.Specs(&w2, &h2, &d2, dname, -1) > 0)
    kinect_geom();

  // count rest of lines to give total number of "frames"
  // then rewind file to access filenames in order (after default)
  while (next_line() > 0)
    n++;
  nframes = n;
  if (reset_list() <= 0)
    return 0;
  return 1;
}


//= Get back to first file name in list.

int jhcListVSrc::reset_list ()
{
  rewind(list);
  backup = ftell(list);
  if (frt != 0.0)
    if (next_line() <= 0)
      return 0;
  if (def != 0)
    if (next_line() <= 0)
      return 0;
  previous = 0;
  return 1;
}


//= Eliminates leading and trailing whitespace.
// allows embedded spaces but alters given string
// copies valid portion to "entry" variable

int jhcListVSrc::next_line ()
{
  int i = 0;
  char *begin;
  char line[500];

  // remember starting file position then scan for valid line
  backup = ftell(list);
  while (i == 0)
  {
    // find next non-blank line in file
    if (feof(list))
      return 0;
    if (fgets(line, 500, list) == NULL) 
      return 0;

    // prune whitespace from front
    begin = line;
    while (*begin != '\0')
      if (strchr(" \n\r\x0A", *begin) != NULL)
        begin++;
      else
        break;

    // prune whitespace from back
    for (i = (int) strlen(begin); i > 0; i--)
      if (strchr(" \n\r\x0A", begin[i - 1]) == NULL)
        break;
  }

  // valid portion of line found, so copy to output
  begin[i] = '\0';
  strcpy_s(entry, begin);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Retrieval                                //
///////////////////////////////////////////////////////////////////////////

//= Position file pointer so desired line is the next one read.

int jhcListVSrc::iSeek (int number)
{
  int now = previous + 1;
 
  // figure out where file pointer is now and if it needs to move
  if (jumped != 0) 
    now = nextread;
  if ((number == now) || (list == NULL))
    return 1;

  // if small number, start over from beginning of list
  if (number < now)
  {
    if (reset_list() <= 0)
      return 0;
    now = previous + 1;
  }

  // advance by appropriate number of name entries  
  while (now++ < number)
    if (next_line() <= 0)
      return 0;
  return 1;
}


//= Read next image on list, or the basic file if no list.
// ignores "block" argument since not a live source
// NOTE: resizes destination image to match file 

int jhcListVSrc::iGet (jhcImg &dest, int *advance, int src, int block)
{
  char dname[500];

  if (list != NULL)
  {
    // advance to next still image in list (if any)
    if (next_line() <= 0)
      return 0;

    // build frame name for primary image
    jio.BuildName(entry, -1);
    iname.ParseName(jio.File());   
  }

  // load pixels (and possibly auxilliary data)
  if ((src > 0) && (d2 > 0))
  {
    // convert name to background depth image instead
    sprintf_s(dname, "%s_z.ras", iname.Trimmed());
    naux = jio.LoadResize(dest, dname, 1, 256, daux);
  }
  else
    naux = jio.LoadResize(dest, iname.File(), 1, 256, daux);
  return naux;
}


//= See if there is an associated depth image.
// NOTE: resizes destination images to match files 

int jhcListVSrc::iDual (jhcImg& dest, jhcImg& dest2)
{
  char dname[500];
  int ans, n;

  // get basic (color) image
  ans = iGet(dest, &n, 0, 1);
  if (ans <= 0)
    return ans;

  // get secondary (depth) image
  if (d2 <= 0)
    return dest2.CopyArr(dest);
  sprintf_s(dname, "%s_z.ras", iname.Trimmed());
  return jio.LoadResize(dest2, dname, 1);
}

