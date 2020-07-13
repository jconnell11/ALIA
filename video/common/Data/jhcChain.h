// jhcChain.h : datastructure for recording linked edges in images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2007 IBM Corporation
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

#ifndef _JHCCHAIN_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCHAIN_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcArr.h"
#include "Data/jhcImg.h"


//= Datastructure for recording linked edges in images.
// an array of points representing one or more contours
// each point has special kind tag and index of next point in chain
// use "SetLen(n)" to initialize arrays for use as lists

class jhcChain 
{
private:
  int traced, asz;
  int total, valid;
  double aspect;
  int *kind;
  int *link, *above;
  double *xpos, *ypos;

public:
  int len;  /** Mark a->len = n for arrays of jhcChain elements. */ 

public:
  int Size () {return total;}    /** Maximum number of points allowed in segment. */
  int Active () {return valid;}  /** Current number of valid points in segment.   */
  void Clear () {valid = 0;}      /** Invalidate all current points.               */

  int GetMark (int i);
  double GetX (int i);
  double GetY (int i);
  int GetLink (int i);
  void SetMark (int i, int val);
  void SetX (int i, double val);
  void SetY (int i, double val);
  void SetLink (int i, int val);

  int BoundChk (int i, char *fcn =NULL);
  int GetMarkChk (int i, int def =0);
  double GetXChk (int i, double def =-1.0);
  double GetYChk (int i, double def =-1.0);
  int GetLinkChk (int i, int def =-1);
  int SetMarkChk (int i, int val);
  int SetXChk (int i, double val);
  int SetYChk (int i, double val);
  int SetLinkChk (int i, int val);

public:
  ~jhcChain ();
  jhcChain ();
  jhcChain (jhcChain& ref);
  jhcChain (int n);

  jhcChain *SetSize (jhcChain& ref);
  jhcChain *SetSize (int n);

  int Copy (jhcChain& ref);
  int Append (double x, double y, int val =0);
  int AddPt (double x, double y, int val, int next =-1);
  int AddLink (int p1, int p2);
  
  int FindPts (jhcImg& src, int th);

  int Trace (jhcChain& raw, int samp =4, int size =50, int no_bd =1);
  int Relax (double frac =0.5, int passes =1);
  int HistTurn (jhcArr& h, double lo, int squash =1);

  int DrawPts (jhcImg& dest, int ej =1, int squash =1, 
               int r =255, int g =255, int b =255);
  int DrawSegs (jhcImg& dest, int r =255, int g =255, int b =255);

private:
  int GetMark0 (int i) {return kind[i];}   /** Whether on image boundary or interior.  */ 
  double GetX0 (int i) {return xpos[i];}   /** X coordinate of a certain point.        */
  double GetY0 (int i) {return ypos[i];}   /** Y coordinate of a certain point.        */
  int GetLink0 (int i) {return link[i];}  /** Index of next point in segment.         */
  void SetMark0 (int i, int val) {kind[i] = val;}   /** Set type of point.             */
  void SetX0 (int i, double val) {xpos[i] = val;}   /** Set X coordinate of point.     */
  void SetY0 (int i, double val) {ypos[i] = val;}   /** Set Y coordinatge of point.    */
  void SetLink0 (int i, int val) {link[i] = val;}  /** Connect this point to another. */

  void AboveSize (int x);
  void InitChain (int first =0);
  void DeallocChain ();

  int AbovePt (int x);
  void AboveSet (int x, int i);
  int DoPattern (int west, int nw, int ne, int sw, int se, 
                  int x, int y, int mark);
};


#endif




