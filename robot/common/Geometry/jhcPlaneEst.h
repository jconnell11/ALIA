// jhcPlaneEst.h : estimates best fitting plane for a collection of points
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 IBM Corporation
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

#ifndef _JHCPLANEEST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCPLANEEST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <math.h>


//= Estimates best fitting plane for a collection of point.

class jhcPlaneEst
{
// PRIVATE MEMBER VARIABLES
private:
  double num, Sx, Sy, Sz, Sxx, Syy, Szz, Sxy, Sxz, Syz;
  double err, a, b, c;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  jhcPlaneEst () {ClrStats();}
 
  // main functions
  int FitPts (const double *x, const double *y, const double *z, int n, 
              double xsc =1.0, double ysc =1.0, double zsc =1.0);
  void ClrStats ();
  int AddPoint (double x, double y, double z);
  int Analyze (double xsc =1.0, double ysc =1.0, double zsc =1.0);

  // result interpretation
  int Pose (double& t, double& r, double& h, 
            double p =0.0, double x =0.0, double y =0.0, double z =0.0) const;
  double CoefX () const  {return a;}
  double CoefY () const  {return b;}
  double Offset () const {return c;}
  double RMS () const    {return err;}
  double StdX () const   {return sqrt((Sxx - Sx * Sx) / (num * num));}
  double StdY () const   {return sqrt((Sxx - Sx * Sx) / (num * num));}
  int Pts () const       {return((int) num);}


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  void scale_xyz (double xsc, double ysc, double zsc);
  void find_abc ();
  void find_err ();

};


#endif  // once




