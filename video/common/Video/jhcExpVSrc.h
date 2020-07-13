// jhcExpVSrc.h : sequencer for presenting frames for analysis
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

#ifndef _JHCEXPVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCEXPVSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Processing/jhcGray.h"
#include "Processing/jhcResize.h"
#include "Video/jhcGenVSrc.h"


//= Sequencer for presenting frames for analysis.
// does Windows IO to control iteration over a video source
// more flexible resizing than basic video source types
// use standard Seek/Get vs. Rewind/Read for silent operation

class jhcExpVSrc : public jhcGenVSrc
{
// PRIVATE MEMBER VARIABLES
private:
  jhcGray jg;
  jhcResize jr;
  jhcImg base, qbase, mbase, base2, qbase2, mbase2;
  int mc;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam squash;

  //= Method used to reduce size (0 = sample). 
  // @see Processing.jhcResize#ForceSize 
  int Avg;  

  //= Which quadrant to extract (0 = full image).
  int Quad;  


// PUBLIC MEMBER FUNCTIONS
public:
  // basic creation
  jhcExpVSrc ();
  jhcExpVSrc (const char *name);

  // configuration
  int AskStep ();
  int AskSize ();
  int AskSource ();
  int SelectFile (char *choice, int ssz);
  void Defaults (char *fname =NULL);
  void SaveVals (char *fname);

  // configuration - convenience
  template <size_t ssz>
    int SelectFile (char (&choice)[ssz])
      {return SelectFile(choice, ssz);}

  // core functions
  int SetSource (const char *name);
  void SetSize (int xmax, int ymax, int bw =0); 


// PRIVATE MEMBER FUNCTIONS
private:
  // core functions
  int iSeek (int number); 
  int iGet (jhcImg& dest, int *advance, int src);
  int iDual (jhcImg& dest, jhcImg& dest2);

};


#endif




