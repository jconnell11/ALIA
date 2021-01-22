// jhcFRecoDLL.h : holds gallery of faces and matches probe image to them
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCFRECODLL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFRECODLL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Data/jhcImgIO.h"
#include "Data/jhcParam.h"
#include "Face/jhcFaceOwner.h"

#include "Face/freco_nkr.h"       // generic fcns but specific lib file


//= Holds gallery of faces and matches probe image to them.
// NOTE: file "freco_nkr.wts" must be in executable directory

class jhcFRecoDLL : private jhcImgIO
{
// PRIVATE MEMBER VARIABLES
private:
  // configuration
  char dir[200];
  jhcName plist;

  // database of vectors
  jhcFaceOwner *db;
  int vsz;

  // pending background operation
  jhcFaceVect *probe;
  int busy;

  // status and results
  const jhcFaceOwner *win;
  jhcFaceVect *best;
  int verdict, ranked;


// PUBLIC MEMBER VARIABLES
public:
  // matching parameters
  jhcParam mps;
  int sure, vcnt, boost, ucap;
  double mth;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcFRecoDLL ();
  jhcFRecoDLL ();
  jhcFaceOwner *NextDude (const jhcFaceOwner *dude =NULL) const
    {return((dude == NULL) ? db : dude->next);}
  const jhcFaceVect *LastResult () const {return probe;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL, int data =1);
  int SaveVals (const char *fname, int data =1);

  // main functions
  void Reset ();
  const jhcFaceVect *Enroll (const char *name, const jhcImg& src, const jhcRoi& box, int rem =1);
  const jhcFaceVect *Enroll (const char *name, const jhcFaceVect *ref, int rem =1);
  const jhcFaceVect *TouchUp (const char *name);
  int Recognize (const jhcImg& src, const jhcRoi& box);
  int Submit (const jhcImg& src, const jhcRoi& box);
  int Submit (const jhcImg *src, const jhcRoi *box)
    {return(((src == NULL) || (box == NULL)) ? 0 : Submit(*src, *box));}
  int Check ();
  
  // result browsing
  const char *Name (int i =0);
  double Distance (int i =0); 
  int ImgNum (int i =0);
  const char *Match (double& d, int i =0);
  const jhcImg *Mugshot (int i =0);

  // database functions
  int NumPeople (int some =1) const;
  int TotalVects () const;
  void ClrDB ();
  void SetDir (const char *fmt, ...);
  const char *GetDir () const {return dir;}
  int LoadDB (const char *fname =NULL, int append =0);
  int SaveDB (const char *fname =NULL);
  int SaveDude (const char *name);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int match_params (const char *fname);

  // main functions
  jhcFaceOwner *get_person (const char *name);
  void score_all (const double *query);
  int chk_sure ();

  // result browsing
  jhcFaceVect *mark_rank (const char **name, int i);
  jhcFaceVect *find_rank (const char **name, int i) const;

  // database functions
  int save_vects ();


};


#endif  // once




