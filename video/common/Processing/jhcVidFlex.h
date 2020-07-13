// jhcVidFlex.h : utilities for adapting to video sizes and rates
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2004-2011 IBM Corporation
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

#ifndef _JHCVIDFLEX_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVIDFLEX_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Utilities for adapting to video sizes and rates.

class jhcVidFlex
{
// PRIVATE MEMBER VARIABLES
private:
  // reference values
  int rw, rh;
  double rrate, rsize;

  // current video values
  int vw, vh;
  double vrate, vsize;

  // conversion factors
  double hf, ihf, af, tf, itf, sc, hf2, af2;


// PUBLIC MEMBER VARIABLES
public:
  int noisy;  /** Whether to display debugging messages. */


// PUBLIC MEMBER FUNCTIONS
public:
  jhcVidFlex ();
  void Report (const char *msg, ...) const;
  void Report (int level, const char *msg, ...) const;
  
  // configuration
  void FlexRef (int w, int h, double fps, double size =1.0);
  void FlexSize (int w, int h, double fps =0.0);
  void FlexSize (const jhcImg& ref, double fps =0.0);
  void FlexRate (double fps);
  void FlexScale (double size);

  // resizing utilities
  int msc (int dim) const;
  int psc (int pels) const;
  int asc (int area) const;
  int mfsc (int dim) const;
  int pfsc (int pels) const;
  int afsc (int area) const;
  int tsc (int frames) const;
  double dsc (double decay) const;
  double bsc (double frac) const;
  double vsc (double vel) const;


// PRIVATE MEMBER FUNCTIONS
private:
  void adj_time ();
  void adj_space ();
  void adj_feat ();

};


#endif




