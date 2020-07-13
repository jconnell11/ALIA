// jhcImgIO.cpp : file I/O functions for images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2017 IBM Corporation
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

#ifndef _WIN32_WCE
  #include <direct.h>    // path and directory manipulation in Windows
#endif

#include "Data/jhcImgIO.h"


///////////////////////////////////////////////////////////////////////////
//                             Initialization                            //
///////////////////////////////////////////////////////////////////////////

//= Constructor sets up some default values.

jhcImgIO_0::jhcImgIO_0 ()
{
  *pathdef = '\0';
  *dskdef  = '\0';
  *dirdef  = '\0';
  *extdef  = '\0';
  *err_str = '\0';
  quality = 85;
}


//= Extract just directory part of file name to use as default.

void jhcImgIO_0::SaveDir (const char *file_spec)
{
  jhcName jn(file_spec);

  strcpy0(dskdef, jn.Disk(), 20);
  strcpy0(dirdef, jn.Path(), 200);
  sprintf_s(pathdef, "%s%s", dskdef, dirdef);
}


//= Extract both directory and extension from file name.

void jhcImgIO_0::SaveSpec (const char *file_spec)
{
  jhcName jn(file_spec);

  strcpy0(dskdef, jn.Disk(), 20);
  strcpy0(dirdef, jn.Path(), 200);
  strcpy0(extdef, jn.Extension(), 20);
  sprintf_s(pathdef, "%s%s", dskdef, dirdef);
}


//= Construct file name possibly using default directory.
// full = 1 means no change, 0 = always add dir and ext
// -1 means add dir and/or ext if they seem to be missing

void jhcImgIO_0::BuildName (const char *file_spec, int full)
{
  char name[250];

  if (full > 0)
  {
    ParseName(file_spec);
    return;
  }
  if (full == 0)
  {
    sprintf_s(name, "%s%s%s%s", dskdef, dirdef, file_spec, extdef);
    ParseName(name);
    return;
  }
  ParseName(file_spec);
  sprintf_s(name, "%s%s%s%s", 
            ((*DiskSpec == '\0') ? dskdef : ""),
            (((*DirNoDisk == '\0') || 
             ((*DirNoDisk != '\\') && (*DirNoDisk != '/'))) ? dirdef : ""),
            file_spec, 
            ((*Ext == '\0') ? extdef : ""));
  ParseName(name);
}


//= Strips off directory and / or extension if they match defaults.

void jhcImgIO_0::NickName (char *name, const char *full_path, int ssz)
{
  ParseName(full_path);
  if ((_stricmp(DiskSpec, dskdef) != 0) || (_stricmp(DirNoDisk, dirdef) != 0))
    strcpy_s(name, ssz, FileName);
  else if (_stricmp(Ext, extdef) != 0)
    strcpy_s(name, ssz, BaseExt);
  else
    strcpy_s(name, ssz, BaseName);
}


/////////////////////////////////////////////////////////////////////////////
//                             Generic Functions                           //
/////////////////////////////////////////////////////////////////////////////

//= Adjust image to be correct size for image contained in file.

jhcImg* jhcImgIO_0::SizeFor (jhcImg& dest, const char *file_spec, int full)
{
  int w, h, f;

  Specs(&w, &h, &f, file_spec, full);
  dest.SetSize(w, h, f);
  return &dest;
}


//= Find size of image described by file.
// appends default directory to name unless "full" is non-zero
// errors: 0 = could not open, -1 = bad format, -2 = bad pixel size, -3 = unsupported

int jhcImgIO_0::Specs (int *w, int *h, int *f, const char *file_spec, int full)
{
  int ans;
  FILE *in;

  // construct file name possibly using default directory
  // then fill in default values to return if file does not exist
  BuildName(file_spec, full);
  *w = 0;
  *h = 0;
  *f = 0;

  // try more exotic types first
  if ((ans = ReadAltHdr(w, h, f, FileName)) > 0)
    return ans;

  // try opening file
  if (fopen_s(&in, FileName, "rb") != 0)
    return 0;

  // dispatch to different types based on file extension
  if (IsFlavor("bmp"))
    ans = ReadBmpHdr(w, h, f, in, NULL);
  else if (IsFlavor("pgm"))
    ans = ReadPgmHdr(w, h, f, in);
  else if (IsFlavor("664"))
    ans = ReadVcaHdr(w, h, f, in);
  else
    ans = ReadRasHdr(w, h, f, in);        // "ras" or unknown extension or none

  // clean up
  if (ferror(in))
    ans = -6;
  fclose(in);
  return ans;
}


//= Like other Specs but uses a 3 element array for use with image creator.

int jhcImgIO_0::Specs (int specs[], const char *file_spec, int full)
{
  return Specs(specs, specs + 1, specs + 2, file_spec, full);
}


//= Like plain Load but resizes dest image to match size in file.

int jhcImgIO_0::LoadResize (jhcImg& dest, const char *file_spec, int full,
                            int limit, UC8 *aux_data)
{
  SizeFor(dest, file_spec, full);
  return Load(dest, file_spec, full, limit, aux_data);
}


//= Attempts to load file using format based on extension.
// appends default directory to name unless "full" is non-zero
// if extension is unknown, then tries Sun Raster format
// errors: 0 = no file, -4 = wrong size, -5 = wrong fields, -6 = read error
// can read in up to 65535 bytes of data at end of file
// returns number of extra bytes read PLUS ONE

int jhcImgIO_0::Load (jhcImg& dest, const char *file_spec, int full,
                      int limit, UC8 *aux_data)
{
  FILE *in;
  int ans, x = 0, y = 0, f = 0, n = 0;

  // check that image size matches file contents
  ans = Specs(&x, &y, &f, file_spec, full);
  if (ans > 0)
  {
    if ((x != dest.XDim()) || (y != dest.YDim()))
       ans = -4;
    else if (f != dest.Fields())
       ans = -5;
  }
  if (ans <= 0)
    return ans;

  // try more exotic types first
  if ((ans = LoadAlt(dest, FileName)) > 0)
    return ans;  

  // try opening file
  if (fopen_s(&in, FileName, "rb") != 0)
    return 0;

  // dispatch based on extension to correct pixel reader
  if (IsFlavor("bmp"))
    ans = LoadBmp(dest, in);
  else if (IsFlavor("pgm"))
    ans = LoadPgm(dest, in);
  else if (IsFlavor("664"))
    ans = LoadVca(dest, in);
  else
    ans = LoadRas(dest, in);        // "ras" or unknown extension or none

  // look for auxilliary data and read it into array
  // and check for any read errors
  if (ans > 0)
  {
    n = read_aux(aux_data, in, limit);
    if (ferror(in))
      n = -6;
  }

  // clean up
  fclose(in);
  return n;
}


//= Attempts to save file using format based on extension.
// appends default directory to name unless "full" is non-zero
// if extension is unknown, then uses Sun Raster format
// can add up to 65535 bytes of auxilliary data at end

int jhcImgIO_0::Save (const char *file_spec, const jhcImg& src, int full,
                      int extras, const UC8 *aux_data)
{
  FILE *out;
  int ans;

  // get full file name and make sure directory exists
  BuildName(file_spec, full);
  create_subdirs();

  // try more exotic types first
  if ((ans = SaveAlt(FileName, src)) > 0)
    return ans;

  // try opening file
  if (fopen_s(&out, FileName, "wb") != 0)
    return 0;

  // go through easy types
  if (IsFlavor("bmp"))
    ans = SaveBmp(out, src);
  else if (IsFlavor("pgm"))
    ans = SavePgm(out, src);
  else
    ans = SaveRas(out, src);        // "ras" or unknown extension or none

  // add in some auxilliary data bytes at the end
  if (ans > 0)
    write_aux(out, aux_data, extras);

  // check for any write errors
  fflush(out);
  if (ferror(out))
    ans = -1;
  fclose(out);
  return ans;
}


//= Variation loads both named color image and associated depth map.
// depth is always same base name plus "_z.ras"
// automatically resizes receiving arrays as needed
// can read in up to 65535 bytes of data at end of file
// returns number of extra bytes read PLUS ONE

int jhcImgIO_0::LoadDual (jhcImg& col, jhcImg& dist, const char *cname,
                          int limit, UC8 *aux_data)
{
  char zname[500];
  char *tail;
  int cnt, ok = 1;

  // build related name for depth map
  strcpy_s(zname, cname);
  if ((tail = strrchr(zname, '.')) != NULL)
    *tail = '\0';
  strcat_s(zname, "_z.ras");

  // load images (and possibly auxilliary data from color image)
  if ((cnt = LoadResize(col, cname, -1, limit, aux_data)) <= 0)
    ok = 0;
  if (LoadResize(dist, zname) <= 0)
    ok = 0;

  // check if everything ok
  if (ok <= 0)
    return ok;
  return cnt;
}


//= Variation saves color image to name as well as associated depth map.
// depth is always same base name plus "_z.ras"
// can add up to 65535 bytes of auxilliary data at end of color image

int jhcImgIO_0::SaveDual (const char *cname, const jhcImg& col, const jhcImg& dist,
                          int extras, const UC8 *aux_data)
{
  char zname[500];
  char *tail;
  int ok = 1;

  // build related name for depth map
  strcpy_s(zname, cname);
  if ((tail = strrchr(zname, '.')) != NULL)
    *tail = '\0';
  strcat_s(zname, "_z.ras");

  // save images
  if (Save(cname, col, -1, extras, aux_data) <= 0)
    ok = 0;
  if (Save(zname, dist) <= 0)
    ok = 0;
  return ok;
}


/////////////////////////////////////////////////////////////////////////////
//                         Windows BMP Images                              //
/////////////////////////////////////////////////////////////////////////////

//= Read header for BMP image file and fills in color map (if given).
// returns size of color table (possibly zero) PLUS ONE, negative is error

int jhcImgIO_0::ReadBmpHdr (int *w, int *h, int *f, FILE *in, UC8 *map)
{
  UC8 *m = map;
  UL32 col, sz, hdr;
  int i, val, mono = 1; 

  // read the file header (14 bytes)
  if (getc(in) != 'B')                         // first 2 bytes are ascii "BM"
    return -1;
  if (getc(in) != 'M')
    return -1;
  if ((sz = read32(in)) < 54)                 // whole file size
    return -1;
  read32(in);                                 // Reserved 1 + 2
  if ((hdr = read32(in)) < 54)                // combined size of both headers
    return -1;

  // read the bitmapinfo header (40 bytes)
  if (read32(in) != 40)                       // size of this header
    return -1;  
  if ((*w = read32(in)) == 0)                 // image width
    return -2;
  if ((*h = read32(in)) == 0)                 // image height (bottom up)
    return -2;
  if (read16(in) != 1)                        // number of planes
     return -2;
  if ((*f = (read16(in) >> 3)) == 0)          // bits per pixel
    return -2;  
  if (read32(in) != 0)                        // compression format (0 = none)
    return -3;
  read32(in);                                 // image size (typically 0 uncompressed)
  read32(in);                                 // X pels/meter
  read32(in);                                 // Y pels/meter (could use for aspect)
  col = read32(in);                           // colors used (0 can mean max)
  read32(in);                                 // important colors

  // check computed quantities
  if ((*w > 16384) || (*h > 16384) || ((*f != 1) && (*f != 3)))
    return -2;
  if (col > 256)
    return -3;
  if ((col == 0) && (*f == 1))
    col = 256;
  if (hdr < (54 + (col << 2)))
    return -3;

  // read the color table (if any)
  if (col > 0)
  {
    if (map == NULL)                          // just check if monochrome map
      for (i = col; i > 0; i--)
      {
        val = getc(in);                       // blue
        if (getc(in) != val)                  // green
          mono = 0;
        if (getc(in) != val)                  // red
          mono = 0;
        getc(in);                             // ignore 4th (should be zero)
      }
    else
      for (i = col; i > 0; i--)               // make exact copy of color table
      {
        val = getc(in);
        *m++ = (UC8) val;                     // blue
        if ((*m++ = (UC8) getc(in)) != val)   // green
          mono = 0;
        if ((*m++ = (UC8) getc(in)) != val)   // red
          mono = 0;
        getc(in);                             // ignore 4th (should be zero)
        *m++ = 0;                             
      }
  }
  if ((*f == 1) && (mono != 1))               // adjust for 8 bit color mapped
    *f = 3;

  // strip off any excess space in color table
  for (i = hdr - 54 - (col << 2); i > 0; i--)  
    getc(in);
  return((int)(col + 1));
}


//= Fill dest with image in Windows "bmp" format.
// assumes dest image size has already been checked

int jhcImgIO_0::LoadBmp (jhcImg& dest, FILE *in)
{
  int w = dest.XDim(), h = dest.YDim(), nf = dest.Fields();
  int x, y, f, n, val, ssk = w % 4, dsk = dest.Skip();
  UL32 i;
  UC8 *d = dest.PxlDest();
  UC8 map[1024];

  // only 8 bit monochrome and 24 bit color supported
  // if okay then get color map (if any) from header
  if ((nf != 1) && (nf != 3))
    return -5;
  n = ReadBmpHdr(&x, &y, &f, in, map) - 1;

  // read in pixel data, size of color table from header read
  if (n == 0)
    for (i = dest.PxlSize(); i > 0; i--) // read 24 bit color
      *d++ = (UC8) getc(in);
  else if (f == 1)
    for (i = dest.PxlSize(); i > 0; i--) // read 8 bit monochrome 
      *d++ = map[getc(in) << 2];
  else 
    for (y = h; y > 0; y--)              // read 8 bit color mapped
    {
      for (x = w; x > 0; x--)
      {
        val = getc(in) << 2;
        *d++ = map[val];
        *d++ = map[val + 1];
        *d++ = map[val + 2];
      }
      d += dsk;                          // line padding for 24 bits
      for (i = ssk; i > 0; i--)          // line padding for 8 bits
        getc(in);
    }
  return 1;
}


//= Save image out in Windows "bmp" format (aka "dib" Device Independent Bitmap).

int jhcImgIO_0::SaveBmp (FILE *out, const jhcImg& src)
{
  int i, w = src.XDim(), h = src.YDim(), f = src.Fields();
  int col = ((f == 1) ? 256 : 0);
  UL32 j, hdrs = 14 + (40 + (col << 2));
  UL32 bsize = h * ((((w * f) + 3) >> 2) << 2);
  const UC8 *s = src.PxlSrc();

  // write the file header (14 bytes)
  putc('B', out);                    // first 2 bytes are ascii "BM"
  putc('M', out);
  write32(out, hdrs + bsize);        // whole file size 
  write32(out, 0);                   // Reserved 1 + 2
  write32(out, hdrs);                // combined size of both headers

  // write the bitmapinfo header (40 bytes)
  write32(out, 40);                  // size of this header
  write32(out, w);                   // image width
  write32(out, h);                   // image height (bottom up)
  write16(out, 1);                   // number of planes
  write16(out, (US16)(f << 3));      // bits per pixel 
  write32(out, 0);                   // compression format (none)
  write32(out, bsize);               // image size (0 for uncompressed)
  write32(out, 0);                   // X pels/meter
  write32(out, 0);                   // Y pels/meter
  write32(out, col);                 // colors used (0 can mean max)
  write32(out, 0);                   // important colors (all)

  // write fake color table for monochrome images (0:B:G:R for each index)
  for (i = 0; i < col; i++)
  {
    putc(i, out);
    putc(i, out);
    putc(i, out);
    putc(0, out);                    // top byte of little-endian value
  }

  // dump data -- already in correct order and padded properly
  for (j = src.PxlSize(); j > 0; j--)
    putc(*s++, out);
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                           Sun Raster Images                             //
/////////////////////////////////////////////////////////////////////////////

//= Digests a Sun raster image header and returns image size and type.

int jhcImgIO_0::ReadRasHdr (int *w, int *h, int *f, FILE *in)
{
  int cols;

  if (read32b(in) != 0x59A66A95)    // magic ID number
    return -1;
  *w = read32b(in);                 // width
  *h = read32b(in);                 // height
  switch ((int) read32b(in))        // bits per pixel
  {
    case 8:                         // monochrome images
      *f = 1;
      break;
    case 16:                        // added for Kinect depth images
      *f = 2;
      break;
    case 24:                        // BGR color images
      *f = 3;
      break;
    default:
      return -2;
      break;
  }
  read32b(in);                      // total number of pixels
  read32b(in);                      // format (generally 1)
  read32b(in);                      // number of colors (ignore)
  cols = read32b(in);               // color table size
  while (cols-- > 0)                // ignore it all
    getc(in); 
  return 1;
}


//= Fill dest with image in Sun raster format.
// assumes image size has already been checked

int jhcImgIO_0::LoadRas (jhcImg& dest, FILE *in)
{
  int x, y, f;
  int w = dest.XDim(), h = dest.YDim(), nf = dest.Fields(), ln = dest.Line();
  UC8 *d, *d0 = dest.PxlDest() + (h - 1) * ln;

  // advance file pointer beyond header section (already checked)
  ReadRasHdr(&x, &y, &f, in);

  // read in pixels (happen to be in correct order: B G R)
  // however must change top down to bottom up scan
  for (y = h; y > 0; y--)
  {
    d = d0;
    for (x = w; x > 0; x--)
      for (f = nf; f > 0; f--)
        *d++ = (UC8) getc(in);
    d0 -= ln;
  }
  return 1;
}


//= Save image out in Sun raster format.

int jhcImgIO_0::SaveRas (FILE *out, const jhcImg& src)
{
  int x, y, f;
  int w = src.XDim(), h = src.YDim(), n = src.Fields(), ln = src.Line();
  const UC8 *s, *s0 = src.PxlSrc() + (h - 1) * ln;

  // generate proper header
  write32b(out, 0x59A66A95);
  write32b(out, w);
  write32b(out, h);
  write32b(out, 8 * n);
  write32b(out, w * h);
  write32b(out, 1);
  write32b(out, 0);
  write32b(out, 0);

  // write out pixels (happen to be in correct order: B G R)
  // however must change bottom up to top down scan
  for (y = h; y > 0; y--)
  {
    s = s0;
    for (x = w; x > 0; x--)
      for (f = n; f > 0; f--)
        putc(*s++, out);
    s0 -= ln;
  }
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                      Portable Gray Map Images                           //
/////////////////////////////////////////////////////////////////////////////

//= Digests a monochrome PGM image header and returns image size and type.

int jhcImgIO_0::ReadPgmHdr (int *w, int *h, int *f, FILE *in)
{
  // look for magic number
  if (getc(in) != 'P')
    return -1;
  if (getc(in) != '5')
    return -1;

  // get width and height
  if (fscanf_s(in, "%d", w) != 1)
    return -1;
  if (fscanf_s(in, "%d", h) != 1)
    return -1;

  // check for 8 bit monochrome data
  if (fscanf_s(in, "%d", f) != 1)
    return -1;
  if (*f != 255)
    return -2;    
  *f = 1;
  return 1;
}


//= Fill dest with monochrome image in PGM format.
// assumes image size has already been checked

int jhcImgIO_0::LoadPgm (jhcImg& dest, FILE *in)
{
  int x, y, f, w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  UC8 *d, *d0 = dest.PxlDest() + (h - 1) * ln;

  // advance file pointer beyond header section (already checked)
  ReadPgmHdr(&x, &y, &f, in);
  if (getc(in) == EOF)
    return -1;

  // read in pixels and change top down to bottom up scan
  for (y = h; y > 0; y--)
  {
    d = d0;
    for (x = w; x > 0; x--, d++)
      *d = (UC8) getc(in);
    d0 -= ln;
  }
  return 1;
}


//= Save monochrome image out in PGM format.

int jhcImgIO_0::SavePgm (FILE *out, const jhcImg& src)
{
  int x, y, w = src.XDim(), h = src.YDim(), ln = src.Line();
  const UC8 *s, *s0 = src.PxlSrc() + (h - 1) * ln;

  // generate proper header
  if (!src.Valid(1))
    return 0;
  fprintf(out, "P5\n%d %d\n255\n", w, h);

  // write out pixels and change bottom up to top down scan
  for (y = h; y > 0; y--)
  {
    s = s0;
    for (x = w; x > 0; x--, s++)
      putc(*s, out);
    s0 -= ln;
  }
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                       Old 16-bit IBM VCA Images                         //
/////////////////////////////////////////////////////////////////////////////

//= Check for enough pixels then return standard image size.

int jhcImgIO_0::ReadVcaHdr (int *w, int *h, int *f, FILE *in)
{
  long now, last;

  // standard dimensions (VGA color)
  *w = 640;
  *h = 480;
  *f = 3;

  // check that file is big enough
  now = ftell(in);
  fseek(in, 0, SEEK_END);
  last = ftell(in);
  fseek(in, 0, SEEK_SET);
  if ((last - now) < (640 * 480 * 2))
    return 0;
  return 1;
}


//= Fill dest with color image from Video Capture Adapter 664 format.
// assumes image size has already been checked
// NOTE: color rendition is off, perhaps needs gamma correction

int jhcImgIO_0::LoadVca (jhcImg& dest, FILE *in)
{
  int x, y, v, w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  UC8 *d, *d0 = dest.PxlDest() + (h - 1) * ln;

  // read in pixels and change top down to bottom up scan
  for (y = h; y > 0; y--, d0 -= ln)
  {
    d = d0;
    for (x = w; x > 0; x--, d += 3)
    {
      v =  getc(in);                     // little-endian
      v |= getc(in) << 8;
      d[0] = (UC8)((v & 0x000F) << 4);   // blue  (bottom 4 bits)
      d[1] = (UC8)((v & 0x03F0) >> 2);   // green (middle 6 bits)
      d[2] = (UC8)((v & 0xFC00) >> 8);   // red      (top 6 bits)
    }
  }   
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                     Placeholders for Extensions                         //
/////////////////////////////////////////////////////////////////////////////

//= Digests an alternative format image header and returns image size and type.

int jhcImgIO_0::ReadAltHdr (int *w, int *h, int *f, const char *fname)
{
  return -1;
}


//= Fill dest with image in some alternative format.

int jhcImgIO_0::LoadAlt (jhcImg& dest, const char *fname)
{
  return -1;
}


//= Save image out in some alternative format.

int jhcImgIO_0::SaveAlt (const char *fname, const jhcImg& src)
{
  return -1;
}


/////////////////////////////////////////////////////////////////////////////
//                          Auxilliary Data                                //
/////////////////////////////////////////////////////////////////////////////

//= Check for auxilliary data at end of file and read it into array.
// generally 2 bytes for size (MSB first) followed by other bytes in array
// a negative limit forces read of that many bytes
// return number of bytes actually read PLUS ONE

int jhcImgIO_0::read_aux (UC8 *aux_data, FILE *in, int limit)
{
  int f, n = 0;
  UC8 *a = aux_data;

  if ((limit != 0) && (aux_data != NULL))
  {
    if (limit < 0) 
      n = -limit;
    else
    {
      if ((n = getc(in)) == EOF)
        return 1;
      if ((f = getc(in)) == EOF)
        return 1;
      n = (n << 8) | f;
      if (n > limit)
        n = limit;
    }
    for (f = n; f > 0; f--)
      *a++ = (UC8) getc(in);
  }
  n++;
  return n;
}


//= Add in some auxilliary data bytes at the end of file and record number.
// generally 2 bytes for size (MSB first) followed by other bytes in array

void jhcImgIO_0::write_aux (FILE *out, const UC8 *aux_data, int extras)
{
  int f;
  const UC8 *a = aux_data;

  if ((extras > 0) && (aux_data != NULL))
  {
    putc((extras >> 8) & 0xFF, out);
    putc(extras & 0xFF, out);
    for (f = 0; f < extras; f++)
      putc(*a++, out);
  }
}


/////////////////////////////////////////////////////////////////////////////
//                          General Utilities                              //
/////////////////////////////////////////////////////////////////////////////

//= Reads 32 bit value, takes care of big-endian/little-endian problems.
// this version assumes values are stored in little-endian fashion

UL32 jhcImgIO_0::read32 (FILE *in)
{
  UL32 val;

  val =  read16(in);
  val |= read16(in) << 16; 
  return val;
}


//= Reads 32 bit value, takes care of big-endian/little-endian problems.
// this version assumes values are stored in little-endian fashion

US16 jhcImgIO_0::read16 (FILE *in)
{
  US16 val;

  val = (US16) getc(in);
  val |= (getc(in) << 8);
  return val;
}


//= Writes 32 bit value, takes care of big-endian/little-endian problems.
// this version assumes stores values in little-endian fashion

void jhcImgIO_0::write32 (FILE *out, UL32 val)
{
  write16(out, (US16)(val & 0xFFFF));
  write16(out, (US16)((val >> 16) & 0xFFFF));
}


//= Writes 16 bit value, takes care of big-endian/little-endian problems.
// this version assumes stores values in little-endian fashion

void jhcImgIO_0::write16 (FILE *out, US16 val)
{
  putc(val & 0xFF, out);
  putc((val >> 8) & 0xFF, out);
}


//= Reads 32 bit value, takes care of big-endian/little-endian problems.
// this version assumes values are stored in big-endian fashion

UL32 jhcImgIO_0::read32b (FILE *in)
{
  UL32 val;

  val =  read16b(in) << 16;
  val |= read16b(in); 
  return val;
}


//= Reads 32 bit value, takes care of big-endian/little-endian problems.
// this version assumes values are stored in big-endian fashion

US16 jhcImgIO_0::read16b (FILE *in)
{
  US16 val;

  val = (US16)(getc(in) << 8);
  val |= getc(in);
  return val;
}


//= Writes 32 bit value, takes care of big-endian/little-endian problems.
// this version assumes stores values in big-endian fashion

void jhcImgIO_0::write32b (FILE *out, UL32 val)
{
  write16b(out, (US16)((val >> 16) & 0xFFFF));
  write16b(out, (US16)(val & 0xFFFF));
}


//= Writes 16 bit value, takes care of big-endian/little-endian problems.
// this version assumes stores values in big-endian fashion

void jhcImgIO_0::write16b (FILE *out, US16 val)
{
  putc((val >> 8) & 0xFF, out);
  putc(val & 0xFF, out);
}


//= Make sure all subdirectories in path name exist, create them if they don't.
// assumes BuildName has already been called so full filename is defaulted
// should not really have to write this -- what's an operating system for?

int jhcImgIO_0::create_subdirs ()
{
#ifndef _WIN32_WCE        // Win CE does not have same file structure

  int i = 0;
  char old[250], sub[250], fname[250];

  // save current working directory then
  // remove any ".." specs in the parsed file name
  _getcwd(old, 250);
  _fullpath(fname, FileName, 250);

  while (1)
  {
    // pull out the next subdirectory in the path
    i = NextSubDir(sub, fname, i, 250);
    if (i < 0)
      break;

    // see if the subdirectory exists, else create it
    if (_chdir(sub) < 0)
      if (_mkdir(sub) < 0)
        return 0;
  }

  // restore old working directory
  _chdir(old);

#endif

  return 1;
}

