// jhcObjList.h : manages a collection of visible objects
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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

#ifndef _JHCOBJLIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCOBJLIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcBlob.h"
#include "Data/jhcImg.h"
#include "Data/jhcParam.h"
#include "Processing/jhcDraw.h"

#include "Objects/jhcVisObj.h"


//= Manages a collection of visible objects.

class jhcObjList : private jhcDraw
{
// PRIVATE MEMBER VARIABLES
private:
  jhcVisObj *item, *prev, *tell;
  int iw, ih;


// PUBLIC MEMBER VARIABLES
public:
  // qualitative color
  jhcParam cps;
  int clim[6];
  double agree;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcObjList ();
  ~jhcObjList ();
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  void Reset ();

  // main functions
  int ParseObjs (const jhcBlob& blob, const jhcImg& comps, const jhcImg& src);

  // object properties
  int ObjCount ();
  void Rewind ();
  jhcVisObj *Next ();
  jhcVisObj *GetObj (int n);
  jhcVisPart *ObjPart (int n, const char *sub =NULL);
  const jhcImg *GetMask (int n =1, const char *sub =NULL);
  const jhcImg *GetCrop (int n =1, const char *sub =NULL);
  const jhcArr *GetHist (int n =1, const char *sub =NULL);
  const jhcArr *GetCols (jhcArr& dest, int n =1, const char *sub =NULL);
  const char *MainColor (int cnum, int n =1, const char *sub =NULL);
  const char *SubColor (int cnum, int n =1, const char *sub =NULL);
  int GraspPoint (double &x, double& y, double& wid, double& ang, int n =1);
  int ModelVec (jhcArr& mod, int n =1, const char *sub =NULL);
  int ModelVec (jhcArr& mod, jhcVisObj *t, const char *sub =NULL);

  // object status
  int NumChoices ();
  int TopMark ();
  int TargetID ();
  char *TargetName ();
  jhcVisObj *TargetObj ();
  int SwapTop ();
  void ClearObjs ();
  int RestoreObjs ();

  // object rejection
  int MarkNearest (int x, int y);
  int KeepColor (const char *want, int alt =0);
  int HasOnlyColor (const char *want, int rem =1);
  int HasMainColor (const char *want, int rem =1);
  int HasColor (const char *want, int rem =1);

  // object preferences
  int PickOne (int inc =1);
  int MostColor (const char *want, int inc =1);
  int Biggest (int inc =1);
  int Littlest (int inc =1);
  int MediumSized (int inc =1);
  int Leftmost (int inc =1);
  int Rightmost (int inc =1);
  int InMiddle (int inc =1);

  // debugging graphics
  int DrawGrasp (jhcImg& dest, int n =1, int tail =20, 
                 int t =1, int r =255, int g =0, int b =255);
  int ColorObj (jhcImg& dest, int n);


// PRIVATE MEMBER FUNCTIONS
private:
  int col_params (const char *fname);
  void backup_items ();
  void clr_list (jhcVisObj *head);
  void track_names ();
  int get_grasp (double &x, double& y, double& wid, double& ang, 
                 const jhcBlob& blob, const jhcImg& comps, int i);

  int color_bin (int hue);

};


#endif  // once




