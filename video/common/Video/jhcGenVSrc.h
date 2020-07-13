// jhcGenVSrc.h : wrapper to homogenize other video sources
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2019 IBM Corporation
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

#ifndef _JHCGENVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGENVSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Video/jhcVideoSrc.h"


//= Wrapper to homogenize other video sources.
// This is a shell that internally contains another video source.
// A lot of member variables are duplicated, but can use with "new".
// Usually create one of these with a file or framegrabber name. 
// Can also reinitialize to a different file after creation.
// Uses global registry object "jvreg" to find correct source class.
// NOTE: Global macro JHC_GVID must be declared to get types to self-register.

class jhcGenVSrc : public jhcVideoSrc
{
// PROTECTED MEMBER VARIABLES
protected:
  int xlim, ylim, Mono;
  jhcVideoSrc *gvid;
  char def_ext[20];


// PUBLIC MEMBER VARIABLES
public:
  int index;  /** When to make index (if applicable, e.g. MPEG). */


// PUBLIC MEMBER FUNCTIONS
public:
  ~jhcGenVSrc ();
  jhcGenVSrc ();
  jhcGenVSrc (const char *name);
  void Default (const char *ext);

  // basic construction and operation
  int SetSource (const char *name);
  void Release ();
  void Prefetch (int doit =1)
    {if (gvid != NULL) gvid->Prefetch(doit);}
  void Close () {Release();}

  // pass-through configuration functions
  void SetStep (int offset, int key =0);
  void SetRate (double fps);
  void SetSize (int xmax, int ymax, int bw =0);
  int SetVal (const char *tag, int val) 
    {return((gvid == NULL) ? -1 : gvid->SetVal(tag, val));}
  int SetDef (const char *tag =NULL, int servo =0) 
    {return((gvid == NULL) ? -1 : gvid->SetDef(tag, servo));}
  int SetABits (int n)
    {if (gvid != NULL) adim = gvid->SetABits(n); return adim;}
  int SetAChan (int ch)
    {if (gvid != NULL) ach = gvid->SetAChan(ch); return ach;}
  int SetARate (int sps)
    {if (gvid != NULL) asps = gvid->SetARate(sps); return asps;}

  // pass-through property functions
  const char *StrClass ();
  int BaseClass (const char *cname);
  int StepTime (double rate =-1.0, int src =0);
  int TimeStamp ()
    {return((gvid == NULL) ? 0 : gvid->TimeStamp());}
  const char *FrameName (int idx_wid =-1, int full =0)
    {return((gvid == NULL) ? NULL : gvid->FrameName(idx_wid, full));}
  int GetVal (int *val, const char *tag)
    {return((gvid == NULL) ? -1 : gvid->GetVal(val, tag));}
  int GetDef (int *vdef, const char *tag, int *vmin =NULL, int *vmax =NULL, int *vstep =NULL)
    {return((gvid == NULL) ? -1 : gvid->GetDef(vdef, tag, vmin, vmax, vstep));}
  int AuxCnt () const 
    {return((gvid != NULL) ? gvid->AuxCnt() : 0);}
  const UC8 *AuxData () const 
    {return((gvid!= NULL) ? gvid->AuxData() : NULL);}


// PROTECTED MEMBER FUNCTIONS
protected:
  // initializers for various encapsulated types
  void init_vals ();
  int new_source (const char *hint =NULL);

#ifdef _LIB
  int RegisterAll ();  // make sure types get registered for static libraries
#endif


// PRIVATE MEMBER FUNCTIONS
private:
  // core functionality
  int iSeek (int number);
  int iGet (jhcImg& dest, int *advance, int src, int block);
  int iDual (jhcImg& dest, jhcImg& dest2);
  int iAGet (US16 *snd, int n, int ch);


};


#endif
