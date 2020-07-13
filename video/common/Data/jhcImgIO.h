// jhcImgIO.h : interface to the image file reader functions
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

#ifndef _JHCIMGIO_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCIMGIO_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>             // needed for FILE type

#include "Data/jhcName.h"
#include "Data/jhcImg.h"


//= Interface to the image file reader functions.

class jhcImgIO_0 : public jhcName
{
// PUBLIC MEMBER VARIABLES
public:
  char pathdef[250];  /** Path default = dsk + dir (e.g. "C:/foo/").      */
  char dskdef[20];    /** Disk specification default (e.g. "C:").         */
  char dirdef[200];   /** Directory specification default (e.g. "/foo/"). */
  char extdef[20];    /** File extension default (e.g. ".bmp").           */
  char err_str[250];  /** Possibly contains explanation of last error.    */
  int quality;        /** For compression: 0 - 100 valid range.           */


// PUBLIC MEMBER FUCNTIONS
public:
  jhcImgIO_0 ();
  void SetDir (const char *path){strcpy0(dirdef, path, 200);};  /** Set the default directory.      */
  void SetExt (const char *end){strcpy0(extdef, end, 20);};     /** Set the default file extension. */
  void SaveDir (const char *file_spec);
  void SaveSpec (const char *file_spec);
  void BuildName (const char *file_spec, int full);
  void NickName (char *name, const char *file_spec, int ssz);

  // convenience
  template <size_t ssz>
    void NickName (char (&name)[ssz], const char *file_spec)
      {return NickName(name, file_spec, ssz);}

  // generic functions
  jhcImg *SizeFor(jhcImg& dest, const char *file_spec, int full =-1);
  int Specs (int specs[], const char *file_spec, int full =-1);
  int Specs (int *w, int *h, int *f, const char *file_spec, int full =-1);
  int LoadResize (jhcImg& dest, const char *file_spec, int full =-1,
                  int limit =0, UC8 *aux_data =NULL);
  int Load (jhcImg& dest, const char *file_spec, int full =-1,
            int limit =0, UC8 *aux_data =NULL);
  int Save (const char *file_spec, const jhcImg& src, int full =-1,
            int extras =0, const UC8 *aux_data =NULL);
  int LoadDual (jhcImg& col, jhcImg& dist, const char *cname,
                int limit =0, UC8 *aux_data =NULL);
  int SaveDual (const char *cname, const jhcImg& col, const jhcImg& dist,
                int extras =0, const UC8 *aux_data =NULL);

  // Microsoft bitmap format
  int ReadBmpHdr (int *w, int *h, int *f, FILE *in, UC8 *map =NULL);  
  int LoadBmp (jhcImg& dest, FILE *in);
  int SaveBmp (FILE *out, const jhcImg& src);

  // Sun raster format
  int ReadRasHdr (int *w, int *h, int *f, FILE *in);  
  int LoadRas (jhcImg& dest, FILE *in);
  int SaveRas (FILE *out, const jhcImg& src);

  // Portable Gray Map format
  int ReadPgmHdr (int *w, int *h, int *f, FILE *in);  
  int LoadPgm (jhcImg& dest, FILE *in);
  int SavePgm (FILE *out, const jhcImg& src);

  // old IBM Video Capture Adapter card
  int ReadVcaHdr (int *w, int *h, int *f, FILE *in);  
  int LoadVca (jhcImg& dest, FILE *in);


// PROTECTED MEMBER FUCNTIONS
protected:
  virtual int ReadAltHdr (int *w, int *h, int *f, const char *fname);  
  virtual int LoadAlt (jhcImg& dest, const char *fname);
  virtual int SaveAlt (const char *fname, const jhcImg& src);


// PRIVATE MEMBER FUCNTIONS
private:
  // auxilliary data
  int read_aux (UC8 *aux_data, FILE *in, int limit);
  void write_aux (FILE *out, const UC8 *aux_data, int extras);

  // general utilities
  UL32 read32 (FILE *in);
  US16 read16 (FILE *in);
  void write32 (FILE *out, UL32 val);
  void write16 (FILE *out, US16 val);
  UL32 read32b (FILE *in);
  US16 read16b (FILE *in);
  void write32b (FILE *out, UL32 val);
  void write16b (FILE *out, US16 val);
  int create_subdirs ();

};


/////////////////////////////////////////////////////////////////////////////

//= Allows transparent insertion of JPEG class over top of normal class.
// define global macro JHC_JPEG to allow use of non-IBM image reader code

#if defined(JHC_MSIO)
  #include "Data/jhcImgMS.h"
#elif defined(JHC_JPEG)
  #include "Data/jhcJPEG.h"
#else
  typedef jhcImgIO_0 jhcImgIO;
#endif


#endif  // once



