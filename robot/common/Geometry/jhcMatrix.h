// jhcMatrix.h : generic 2D matrix and common operations
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

#ifndef _JHCMATRIX_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMATRIX_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdlib.h>           // needed for NULL !

#define JMT_DIM  36           // size of local array (no allocation needed)


//= Generic 2D matrix and common operations.
// can also be used to represent vectors

class jhcMatrix
{
// PRIVATE MEMBER VARIABLES
private:
  int w, h, n;
  double norm[JMT_DIM];
  double *made, *vals;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  ~jhcMatrix ();
  jhcMatrix ();
  jhcMatrix (const jhcMatrix& ref);
  jhcMatrix (int rows);
  jhcMatrix (int cols, int rows);
  int SetSize (const jhcMatrix& ref);
  int SetSize (int rows);
  int SetSize (int cols, int rows);
  void Load (const double *v);
  void Copy (const jhcMatrix& src);
  void Clone (const jhcMatrix& src);
  void Zero (double homo =0.0);
  void Identity ();
  void Scale (double sc);
  void Abs ();
  void Add (const jhcMatrix& ref);
  void Transpose (const jhcMatrix& src);

  // assembly and disassembly
  void DumpRow (double *vals, int r) const;
  void DumpCol (double *vals, int c) const;
  void LoadRow (int r, const double *vals);
  void LoadCol (int c, const double *vals);
  void FillRow (int r, double val);
  void FillCol (int c, double val);
  void GetRow (const jhcMatrix& src, int r =0);
  void GetCol (const jhcMatrix& src, int c =0);
  void SetRow (int r, const jhcMatrix& src);
  void SetCol (int c, const jhcMatrix& src);

  // simple functions
  int Cols () const {return w;}; 
  int Rows () const {return h;};
  int SameSize (const jhcMatrix& tst) const;
  int SameSize (int c, int r) const;
  int Vector (int rmin =0) const;
  void Print (const char *tag =NULL) const;
  void PrintVec (const char *tag =NULL, int cr =1) const;
  char *List (char *buf, int ssz) const;
  char *ListVec (char *buf, const char *fmt, int ssz) const;

  // simple functions - convenience
  template <size_t ssz>
    char *List (char (&buf)[ssz]) const
      {return List(buf, ssz);}
  template <size_t ssz>
    char *ListVec (char (&buf)[ssz], const char *fmt =NULL) const
      {return ListVec(buf, fmt, ssz);}

  // value access and modification
  double *VPtrChk (int y);
  double VRefChk (int y) const;
  void VSetChk (int y, double v);
  void VIncChk (int y, double dv);
  double *MPtrChk (int x, int y);
  double MRefChk (int x, int y) const;
  void MSetChk (int x, int y, double v);
  void MIncChk (int x, int y, double dv);

  // convenient macro-like definitions
#ifndef _DEBUG
  double *VPtr (int y)                {return VPtr0(y);}
  double VRef (int y) const           {return VRef0(y);}
  void VSet (int y, double v)         {VSet0(y, v);}
  void VInc (int y, double dv)        {VInc0(y, dv);}

  double *MPtr (int x, int y)         {return MPtr0(x, y);}
  double MRef (int x, int y) const    {return MRef0(x, y);}
  void MSet (int x, int y, double v)  {MSet0(x, y, v);}
  void MInc (int x, int y, double dv) {MInc0(x, y, dv);}
#else
  void *VPtr (int y)                  {return VPtrChk(y);}
  double VRef (int y) const           {return VRefChk(y);}
  void VSet (int y, double v)         {VSetChk(y, v);}
  void VInc (int y, double dv)        {VIncChk(y, dv);}

  void *MPtr (int x, int y)           {return MPtrChk(x, y);}
  double MRef (int x, int y) const    {return MRefChk(x, y);}
  void MSet (int x, int y, double v)  {MSetChk(x, y, v);}
  void MInc (int x, int y, double dv) {MIncChk(x, y, dv);}
#endif

  // vector functions
  double LenVec () const;
  double Len2Vec () const;
  double MaxVec () const;
  double MinVec () const;
  double DotVec (const jhcMatrix& ref) const;
  void FillVec (double val);
  void IncVec (const jhcMatrix& ref);
  void ScaleVec (double sc);
  void MultVec (const jhcMatrix& sc);
  void AddVec (const jhcMatrix& a, const jhcMatrix& b);
  void DiffVec (const jhcMatrix& a, const jhcMatrix& b);

  // homogeneous 3D coordinate vector utilities
  double X () const    {return VRef(0);}
  double Y () const    {return VRef(1);}
  double Z () const    {return VRef(2);}
  double Homo () const {return VRef(3);}
  int Pos () const     {return((VRef(3) == 1.0) ? 1 : 0);}
  int Dir () const     {return((VRef(3) == 0.0) ? 1 : 0);}
  void SetX (double v) {VSet(0, v);} 
  void SetY (double v) {VSet(1, v);} 
  void SetZ (double v) {VSet(2, v);} 
  void SetH (double v) {VSet(3, v);} 
  void IncX (double v) {VInc(0, v);} 
  void IncY (double v) {VInc(1, v);} 
  void IncZ (double v) {VInc(2, v);} 

  // hand pose vector consisting of 3 angles and gripper width
  double P () const    {return VRef(0);}
  double T () const    {return VRef(1);}
  double R () const    {return VRef(2);}
  double W () const    {return VRef(3);}
  void SetP (double v) {VSet(0, v);} 
  void SetT (double v) {VSet(1, v);} 
  void SetR (double v) {VSet(2, v);} 
  void SetW (double v) {VSet(3, v);} 
  void IncP (double v) {VInc(0, v);} 
  void IncT (double v) {VInc(1, v);} 
  void IncR (double v) {VInc(2, v);} 
  void IncW (double v) {VInc(3, v);} 

  // camera pose vector comprised of 6 doubles: PTRXYZ
  void GetDir6 (const double *p6) {SetP(p6[0]); SetT(p6[1]); SetR(p6[2]);}
  void GetPos6 (const double *p6) {SetX(p6[3]); SetY(p6[4]); SetZ(p6[5]);}
  void SetDir6 (double *p6) const {p6[0] = P(); p6[1] = T(); p6[2] = R();}
  void SetPos6 (double *p6) const {p6[3] = X(); p6[4] = Y(); p6[5] = Z();}

  // homogeneous 3D coordinate vector functions
  double LenVec3 () const;
  double Len2Vec3 () const;
  double PlaneVec3 () const;
  double MaxVec3 () const;
  double MaxAbs3 () const;
  double MinVec3 () const;
  double PosDiff3 (const jhcMatrix& ref) const;
  double PanVec3 () const;
  double TiltVec3 () const;
  double DotVec3 (const jhcMatrix& ref) const;
  double DotVec4 (const jhcMatrix& ref) const;
  double DirUnit3 (const jhcMatrix& ref) const;
  double DirDiff3 (const jhcMatrix& ref) const;
  double RotDiff3 (const jhcMatrix& ref) const;
  double PanDiff3 (const jhcMatrix& ref) const;
  double TiltDiff3 (const jhcMatrix& ref) const;
  void PanTilt3 (double& pan, double& tilt, const jhcMatrix& targ) const;
  void HomoDiv3 ();
  void SetVec2 (double x, double y, double homo =1.0);
  void SetVec3 (double x, double y, double z, double homo =1.0);
  void SetPanTilt3 (double pan, double tilt, double homo =1.0);
  double DumpVec3 (double& x, double& y, double& z) const;
  void AddVec3 (const jhcMatrix& a, const jhcMatrix& b);
  int IncVec3 (const jhcMatrix& inc);
  int IncVec3 (double dx, double dy, double dz);
  void RelVec3 (const jhcMatrix& ref, double dx, double dy, double dz);
  void AddFrac3 (const jhcMatrix& inc, double f =1.0);
  void RelFrac3 (const jhcMatrix& ref, const jhcMatrix& inc, double f =1.0);
  void ScaleVec3 (const jhcMatrix& ref, double sc, double homo =1.0);
  void ScaleVec3 (double sc, double homo =1.0);
  void MixVec3 (const jhcMatrix& a, const jhcMatrix& b, double f);
  void MixVec3 (const jhcMatrix& target, double f);
  void SubZero3 (const jhcMatrix& replace);
  void ClampVec3 (const jhcMatrix& lim);
  void ClampVec3 (double lim);
  void FlipVec3 (const jhcMatrix& ref, double homo =1.0);
  void CompVec3 (const jhcMatrix& wrt, double homo =1.0);
  void MultVec3 (const jhcMatrix& sc, double homo =1.0);
  void DiffVec3 (const jhcMatrix& a, const jhcMatrix& b, double homo =1.0);
  double DirVec3 (const jhcMatrix& a, const jhcMatrix& b, double homo =1.0);
  double RotDir3 (const jhcMatrix& a, const jhcMatrix& b);
  void CrossVec3 (const jhcMatrix& a, const jhcMatrix& b, double homo =1.0);
  void RotPan3 (double pan);
  void RotPan3 (const jhcMatrix& ref, double pan) {Copy(ref); RotPan3(pan);}
  void RotTilt3 (double tilt);
  void PrintVec3 (const char *tag =NULL, const char *fmt =NULL, int all4 =0, int cr =1) const;
  void PrintVec3i (const char *tag =NULL, const char *fmt =NULL, int all4 =0) const
    {PrintVec3(tag, fmt, all4, 0);}
  char *ListVec3 (char *txt, const char *fmt, int all4, int ssz) const;

  // 3D vector - convenience
  template <size_t ssz>
    char *ListVec3 (char (&txt)[ssz], const char *fmt =NULL, int all4 =0) const
      {return ListVec3(txt, fmt, all4, ssz);}

  // directions and rotations
  void CycNorm3 ();
  void CycDiff3 (const jhcMatrix& a, const jhcMatrix& b, double homo =1.0) 
    {DiffVec3(a, b, homo); CycNorm3();}
  double UnitVec3 (const jhcMatrix& ref, double homo =1.0);
  double UnitVec3 (double homo =1.0);
  double RotUnit3 (); 
  void EulerVec3 (double yaw, double pitch, double homo =0.0);
  void EulerVec4 (const jhcMatrix& ptr);
  double YawVec3 () const;
  double PitchVec3 () const;
  void Quaternion (const jhcMatrix& axis, double degs);
  void Quaternion (const jhcMatrix& rot);
  double AxisQ (const jhcMatrix& q);
  void RotatorQ (const jhcMatrix& q);
  void CascadeQ (const jhcMatrix& qlf, const jhcMatrix& qrt);
  void CycMix3 (const jhcMatrix& a, const jhcMatrix& b, double f);

  // special forms for 2x2, 3x3, and 4x4 homogeneous matrices
  void RotationX (double degs);
  void RotateX (double degs);
  void RotationY (double degs);
  void RotateY (double degs);
  void RotationZ (double degs);
  void RotateZ (double degs);
  void Rotation (double xdegs, double ydegs, double zdegs, int clr =1);
  void Translation (double dx, double dy, double dz, int clr =1);
  void Translation (const jhcMatrix& ref, int clr =1);
  void Translate (double dx, double dy, double dz);
  void Translate (const jhcMatrix& ref);
  void Magnification (double f) {Magnification(f, f, f);}
  void Magnification (double fx, double fy, double fz);
  void Magnify (double f) {Magnify(f, f, f);}
  void Magnify (double fx, double fy, double fz);
  void Projection (double f, int clr =1);
  void Project (double f);

  // main functions
  void MatVec (const jhcMatrix& mat, const jhcMatrix& vec);
  void MatVec0 (const jhcMatrix& mat, const jhcMatrix& vec);
  void MatMat (const jhcMatrix& lf, const jhcMatrix& rt);
  int Invert (const jhcMatrix& ref);
  double Det () const;


// PRIVATE MEMBER FUNCTIONS
private:
  double *VPtr0 (int y)                {return(vals + y);}
  double VRef0 (int y) const           {return vals[y];}
  void VSet0 (int y, double v)         {vals[y] = v;}
  void VInc0 (int y, double dv)        {vals[y] += dv;}

  double *MPtr0 (int x, int y)         {return(vals + x * h + y);}
  double MRef0 (int x, int y) const    {return vals[x * h + y];}
  void MSet0 (int x, int y, double v)  {vals[x * h + y] = v;}
  void MInc0 (int x, int y, double dv) {vals[x * h + y] += dv;}

  double ang180 (double ang) const;
  void mm_core (const jhcMatrix& lf, const jhcMatrix& rt);
  int inv_core (const jhcMatrix& ref);


};


#endif  // once




