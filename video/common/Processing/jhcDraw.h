// jhcDraw.h : create boxes, circles, etc, in images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#ifndef _JHCDRAW_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDRAW_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Create boxes, circles, etc, in images.

class jhcDraw
{
// PRIVATE MEMBER VARIABLES
private:
  int ej_clip;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcDraw ();
  int DrawClip (int doit);

  // color shifting
  void Color8 (UC8 *r, UC8 *g, UC8 *b, int i, int nf =3) const;
  UC8 ColorN (int i) const;
  int Scramble (jhcImg& dest, const jhcImg& src, int field =0) const;
  int ScrambleNZ (jhcImg& dest, const jhcImg& src, int field =0) const;
  int FalseColor (jhcImg& dest, const jhcImg& src) const;
  int FalseClone (jhcImg& dest, const jhcImg& src) const 
    {dest.SetSize(src, 3); return FalseColor(dest, src);}
  int IndexColor (jhcImg& dest, const jhcImg& src) const;

  // filled shapes
  int RectFill (jhcImg& dest, int left, int bot, int w, int h, 
                int r =255, int g =255, int b =255) const;
  int RectFill (jhcImg& dest, const int *specs, int r =255, int g =255, int b =255) const;
  int RectFill (jhcImg& dest, const jhcRoi& src, int r =255, int g =255, int b =255) const;
  int BlockCent (jhcImg& dest, int xc, int yc, int w, int h, 
                 int r =255, int g =255, int b =255) const;
  int BlockRot (jhcImg& dest, double xc, double yc, double w, double h, 
                double degs =0.0, int r =255, int g =255, int b =255, int set =0) const;
  int FillPoly4 (jhcImg& dest, double nwx, double nwy, double nex, double ney, 
                 double sex, double sey, double swx, double swy, int r =255, int g =255, int b =255, int set =0) const;
  int FillPoly4 (jhcImg& dest, const double x[], const double y[], int r =255, int g =255, int b =255, int set =0) const
    {return FillPoly4(dest, x[0], y[0], x[1], y[1], x[2], y[2], x[3], y[3], r, g, b, set);}
  int Clear (jhcImg& dest, int r =0, int g =0, int b =0) const;
  int Matte (jhcImg& dest, const jhcRoi& src, int r =0, int g =0, int b =0) const;
  int Matte (jhcImg& dest, int r =0, int g =0, int b =0)  const
    {return Matte(dest, dest, r, g, b);}  /** Fill everything except image ROI with some color */
  int CircleFill (jhcImg& dest, double xc, double yc, double radius, 
                  int r =255, int g =255, int b =255) const;
  int FillH (jhcImg& dest, double frac, int r =255, int g =255, int b =255) const;
  int FillV (jhcImg& dest, double frac, int r =255, int g =255, int b =255) const;

  // outline shapes
  int RectEmpty (jhcImg& dest, int left, int bot, int w, int h, 
                 int t =3, int r =255, int g =255, int b =255) const;
  int RectEmpty (jhcImg& dest, double lfrac, double bfrac, double wfrac, double hfrac, 
                 int t =3, int r =255, int g =255, int b =255) const;
  int RectEmpty (jhcImg& dest, const int *specs,
                 int t =3, int r =255, int g =255, int b =255) const;
  int RectEmpty (jhcImg& dest, const jhcRoi& s, 
                 int t =3, int r =255, int g =255, int b =255) const;
  int Border (jhcImg& dest, int t =1, int r =255, int g =255, int b =255) const;
  int BorderV (jhcImg& dest, int t =1, int v =255) const;
  int BorderH (jhcImg& dest, int t =1, int v =255) const;
  int BorderExt (jhcImg& dest) const;
  int EdgeDup (jhcImg& dest, int n =1) const;
  int SideDup (jhcImg& dest, int side, int n =1) const;
  int DrawSide (jhcImg& dest, int side, int t =1, int r =255, int g =255, int b =255) const; 
  int RectCent (jhcImg& dest, double xc, double yc, double w, double h, 
                double degs =0.0, int t =3, int r =255, int g =255, int b =255) const;
  int RectFrac (jhcImg& dest, double xcf, double ycf, double wf, double hf, 
                double degs =0.0, int t =3, int r =255, int g =255, int b =255) const;
  int CircleEmpty (jhcImg& dest, double xc, double yc, double radius, 
                   int t =3, int r =255, int g =255, int b =255) const;
  int EllipseEmpty (jhcImg& dest, double xc, double yc, double len, double wid, double ang, 
                    int t =3, int r =255, int g =255, int b =255) const;
  int Diamond (jhcImg& dest, double xc, double yc, int w, int h,
               int t =3, int r =255, int g =255, int b =255) const; 

  // lines and curves
  int DrawLine (jhcImg& dest, double x0, double y0, double x1, double y1,
                int t =1, int r =255, int g =255, int b =255) const; 
  int Ray (jhcImg& dest, double x0, double y0, double ang, double len =0.0, 
           int t =1, int r =255, int g =255, int b =255) const; 
  int DrawCorners (jhcImg& dest, const double x[], const double y[], int pts, 
                   int t =3, int r =255, int g =255, int b =255) const;
  int DrawPoly (jhcImg& dest, const int x[], const int y[], int pts, 
                int t =3, int r =255, int g =255, int b =255) const;
  int Cross (jhcImg& dest, double xc, double yc, int w =9, int h =9,
             int t =3, int r =255, int g =255, int b =255) const;
  int XMark (jhcImg& dest, double xc, double yc, int sz =9, 
             int t =3, int r =255, int g =255, int b =255) const;
  int Spline (jhcImg& dest, double x0, double y0, double x1, double y1, 
              double x2, double y2, double x3, double y3,
              int w =1, int r =255, int g =255, int b =255) const;
  int SplineStart (jhcImg& dest, double x1, double y1, double x2, double y2, double x3, double y3,
                   int w =1, int r =255, int g =255, int b =255) const;
  int SplineEnd (jhcImg& dest, double x0, double y0, double x1, double y1, double x2, double y2, 
                 int w =1, int r =255, int g =255, int b =255) const;
  int MultiSpline (jhcImg& dest, const int cx[], const int cy[], int n, 
                   int w =1, int r =255, int g =255, int b =255) const;
  int Ribbon (jhcImg& dest, const int cx[], const int cy[], int n, 
              const int cx2[], const int cy2[], int n2, 
              int w =1, int r =255, int g =255, int b =255) const;

  // masks
  int Outline (jhcImg& dest, const jhcImg& src, int th =0, int r =255, int g =255, int b =255) const; 
  int Emphasize (jhcImg& dest, const jhcImg& src, const jhcImg& data, int th =0, int dr =0, int dg =80, int db =0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  int MatteRGB (jhcImg& dest, const jhcRoi& src, int r, int g, int b) const;
  int RectFill_16 (jhcImg& dest, const jhcRoi& src, int val) const;

  double spline_mix (double v0, double v1, double v2, double v3, double t1, double t2, double t) const;

  bool chk_hood (const UC8 *s, int sln, int th) const;


};


#endif




