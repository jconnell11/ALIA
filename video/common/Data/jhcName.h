// jhcName.h : handles standard parsing of file names
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2013 IBM Corporation
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

#ifndef _JHCNAME_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNAME_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Handles standard parsing of file names.

class jhcName
{
// PROTECTED MEMBER VARIABLES
protected:
  char FileName[500], FileNoExt[500], JustDir[500], DiskSpec[50], Flavor[50];
  char *Ext, *BaseName, *BaseExt, *DirNoDisk;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcName ();
  jhcName (const char *name);

  // read only access
  const char *File0 () const     {return((*FileName == '\0') ? NULL : FileName);}
  const char *File () const      {return FileName;}   /** Full file spec passed in (e.g. "C:/imgs/foo.bmp"). */
  const char *Trimmed () const   {return FileNoExt;}  /** Full path without extension (e.g. "C:/imgs/foo").  */
  const char *Dir () const       {return JustDir;}    /** Just the directory portion (e.g. "C:/imgs/").      */
  const char *Disk () const      {return DiskSpec;}   /** Just the disk specification (e.g. "C:").           */
  const char *Kind () const      {return Flavor;}     /** The extensions without a dot (e.g. "bmp").         */
  const char *Extension () const {return Ext;}        /** Just the extension (e.g. ".bmp").                  */
  const char *Base () const      {return BaseName;}   /** Just the file name portion (e.g. "foo").           */
  const char *Name () const      {return BaseExt;}    /** Just file name and extension (e.g. "foo.bmp").     */
  const char *Path () const      {return DirNoDisk;}  /** The path without the disk (e.g. "/imgs/").         */

  // basic functionality
  int IsFlavor (const char *spec) const;
  int HasWildcard () const;
  int Remote (const char *service =NULL) const;
  void ParseName (const char *name);
  char *strcpy0 (char *dest, const char *src, int dest_size) const;


// PROTECTED MEMBER FUNCTIONS
protected:
  // creation and configuration
  void Init ();

  // basic functionality
  void NoRestarts (char *dest, const char *src, int maxlen) const;
  char *LastMark (char *path) const;
  int NextSubDir (char *spec, const char *full, int last, int ssz) const;

  // convenience
  template <size_t ssz>
    int NextSubDir (char (&spec)[ssz], const char *full, int last =0) const
      {return NextSubDir(spec, full, last, ssz);}

};


#endif




