// jhcMatrix.cpp : generic 2D matrix and common operations
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

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "Interface/jhcMessage.h"

#include "Geometry/jhcMatrix.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcMatrix::~jhcMatrix ()
{
  delete [] made;
}


//= Default constructor initializes certain values.

jhcMatrix::jhcMatrix ()
{
  made = NULL;
  SetSize(0, 0);
}


//= Constructor makes a new instance with same size as some other matrix.

jhcMatrix::jhcMatrix (const jhcMatrix& ref)
{
  made = NULL;
  SetSize(ref.w, ref.h);
}


//= Constructor for a known size vector initializes certain values.

jhcMatrix::jhcMatrix (int rows)
{
  made = NULL;
  SetSize(1, rows);
}


//= Constructor for a known size matrix initializes certain values.

jhcMatrix::jhcMatrix (int cols, int rows)
{
  made = NULL;
  SetSize(cols, rows);
}


//= Set the size of the matrix to match another (contents not copied).

int jhcMatrix::SetSize (const jhcMatrix& ref)
{
  return SetSize(ref.w, ref.h);
}


//= Change vector size to something else and clear entries.

int jhcMatrix::SetSize (int rows)
{
  return SetSize(1, rows);
}


//= Change matrix size to something else and clear entries.
// Note: does NOT initialize any values (call Zero instead)
// returns 1 if successful, 0 if allocation failure

int jhcMatrix::SetSize (int cols, int rows)
{
#ifdef _DEBUG
  if ((cols < 0) || (rows < 0))
    Fatal("Bad size to jhcMatrix::SetSize");
#endif

  // save size
  w = cols;
  h = rows;
  n = w * h;

  // assume bound to normal array 
  vals = norm;
  if (n <= JMT_DIM)
    return 1;

  // make bigger array instead
  delete [] made;
  made = new double[n];
  vals = made;
  if (vals == NULL)
    return 0;
  return 1;
}


//= Fill matrix from an array of values in reading order: L->R, T->B.

void jhcMatrix::Load (const double *v)
{
  int i, j, n = 0;

  for (j = 0; j < h; j++)
    for (i = 0; i < w; i++)
      MSet(i, j, v[n++]);
}


//= Exactly copy the contents of another matrix into this one.

void jhcMatrix::Copy (const jhcMatrix& src)
{
  int i;

  if (&src == this)
    return;

#ifdef _DEBUG
  if (!SameSize(src)) 
    Fatal("Bad input to jhcMatrix::Copy");
#endif

  for (i = 0; i < n; i++)
    vals[i] = src.vals[i];
}


//= Makes an exact copy of some other matrix including size.

void jhcMatrix::Clone (const jhcMatrix& src)
{
  SetSize(src);
  Copy(src);
}


//= Clear all entries in matrix.
// if homogeneous then write a 1 in lower right corner

void jhcMatrix::Zero (double homo)
{
  int i;

  for (i = 0; i < n; i++)
    vals[i] = 0.0;
  if ((homo != 0.0) && (n > 0))
    vals[n - 1] = homo;
}


//= Put 1's on major diagonals of a square matrix.

void jhcMatrix::Identity ()
{
  double *v = vals;
  int i;

#ifdef _DEBUG
  if (w != h)
    Fatal("Non-square input to jhcMatrix::Identity");
#endif

  Zero();
  for (i = 0; i < w; i++, v += h)
    v[i] = 1.0;
}


//= Multiply all entries by some constant.

void jhcMatrix::Scale (double sc)
{
  int i;
 
  for (i = 0; i < n; i++)
    vals[i] *= sc;
}


//= Replace all entries by their absolute value.

void jhcMatrix::Abs ()
{
  int i;
 
  for (i = 0; i < n; i++)
    vals[i] = fabs(vals[i]);
}


//= Element-wise add of another matrix to self.

void jhcMatrix::Add (const jhcMatrix& ref)
{
  int i;

#ifdef _DEBUG
  if (!ref.SameSize(w, h))
    Fatal("Bad input to jhcMatrix::Add");
#endif

  for (i = 0; i < n; i++)
    vals[i] += (ref.vals)[i];
}


//= Fill self with the swapped rows and columns of the source.

void jhcMatrix::Transpose (const jhcMatrix& src)
{
  int i, j;

#ifdef _DEBUG
  if ((w != src.h) || (h != src.w) || (&src == this))
    Fatal("Bad input to jhcMatrix::Transpose");
#endif

  for (j = 0; j < h; j++)
    for (i = 0; i < w; i++)
      MSet0(i, j, src.MRef0(j, i));
}


///////////////////////////////////////////////////////////////////////////
//                      Assembly and Disassembly                         //
///////////////////////////////////////////////////////////////////////////

//= Fill an array with values from a particular row of this matrix.
// array assumed to be big enough to hold all values

void jhcMatrix::DumpRow (double *vals, int r) const
{
  int i;

#ifdef _DEBUG
  if ((r < 0) || (r >= h))
    Fatal("Bad input to jhcMatrix::DumpRow");
#endif

  for (i = 0; i < w; i++)
    vals[i] = MRef(i, r);
}


//= Fill an array with values from a particular column of this matrix.
// array assumed to be big enough to hold all values

void jhcMatrix::DumpCol (double *vals, int c) const
{
  int i;

#ifdef _DEBUG
  if ((c < 0) || (c >= w))
    Fatal("Bad input to jhcMatrix::DumpCol");
#endif

  for (i = 0; i < h; i++)
    vals[i] = MRef(c, i);
}


//= Load a particular row of this matrix with values from an array.
// array assumed to contain sufficient values

void jhcMatrix::LoadRow (int r, const double *vals)
{
  int i;

#ifdef _DEBUG
  if ((r < 0) || (r >= h))
    Fatal("Bad input to jhcMatrix::LoadRow");
#endif

  for (i = 0; i < w; i++)
    MSet(i, r, vals[i]);
}


//= Load a particular column of this matrix with values from an array.
// array assumed to contain sufficient values

void jhcMatrix::LoadCol (int c, const double *vals)
{
  int i;

#ifdef _DEBUG
  if ((c < 0) || (c >= w))
    Fatal("Bad input to jhcMatrix::LoadCol");
#endif

  for (i = 0; i < h; i++)
    MSet(c, i, vals[i]);
}


//= Set all entries in a particular row to the given value.

void jhcMatrix::FillRow (int r, double val)
{
  int i;

#ifdef _DEBUG
  if ((r < 0) || (r >= h))
    Fatal("Bad input to jhcMatrix::FillRow");
#endif

  for (i = 0; i < w; i++)
    MSet(i, r, val);
}


//= Set all entries in a particular column to the given value.

void jhcMatrix::FillCol (int c, double val)
{
  int i;

#ifdef _DEBUG
  if ((c < 0) || (c >= w))
    Fatal("Bad input to jhcMatrix::FillCol");
#endif

  for (i = 0; i < h; i++)
    MSet(c, i, val);
}


//= Load self as a vector using values in some row of matrix.
// must be correct size to receive entries

void jhcMatrix::GetRow (const jhcMatrix& src, int r)
{
  int i;

#ifdef _DEBUG
  if (!Vector(src.w) || (r < 0) || (r >= src.h) || (&src == this))
    Fatal("Bad input to jhcMatrix::GetRow");
#endif

  for (i = 0; i < h; i++)
    VSet0(i, src.MRef0(i, r));
}


//= Load self as a vector using values in some column of matrix.
// must be correct size to receive entries

void jhcMatrix::GetCol (const jhcMatrix& src, int c)
{
  int i;

#ifdef _DEBUG
  if (!Vector(src.h) || (c < 0) || (c >= src.w))
    Fatal("Bad input to jhcMatrix::GetCol");
#endif

  for (i = 0; i < w; i++)
    VSet0(i, src.MRef0(c, i));
}


//= Load one row of self using values from a vector.
// must be correct size to receive entries

void jhcMatrix::SetRow (int r, const jhcMatrix& src)
{
  int i;

#ifdef _DEBUG
  if (!Vector(src.w) || (r < 0) || (r >= h) || (&src == this))
    Fatal("Bad input to jhcMatrix::SetRow");
#endif

  for (i = 0; i < w; i++)
    MSet0(i, r, src.VRef0(i));
}


//= Load one column of self using values from a vector.
// must be correct size to receive entries

void jhcMatrix::SetCol (int c, const jhcMatrix& src)
{
  int i;

#ifdef _DEBUG
  if (!Vector(src.h) || (c < 0) || (c >= w))
    Fatal("Bad input to jhcMatrix::SetCol");
#endif

  for (i = 0; i < h; i++)
    MSet0(c, i, src.VRef0(i));
}


///////////////////////////////////////////////////////////////////////////
//                            Simple Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Check that the matrix has the same dimensions as some other matrix.
// returns 1 if the same, 0 if different

int jhcMatrix::SameSize (const jhcMatrix& tst) const
{
  return SameSize(tst.w, tst.h);
}


//= Check that the matrix has a specific number of rows and columns.
// returns 1 if the same, 0 if different

int jhcMatrix::SameSize (int c, int r) const
{
  if ((w != c) || (h != r))
    return 0;
  return 1;
}


//= Check that the item is a column vector.
// can optionally check that it has at least r rows 

int jhcMatrix::Vector (int rmin) const
{
  if ((w != 1) || (h <= 0))
    return 0;
  if ((rmin > 0) && (h < rmin))
    return 0;
  return 1;
}


//= Debugging functions shows values in matrix.

void jhcMatrix::Print (const char *tag) const
{
  int i, j;

  if (vals == NULL)
    Fatal("Bad input to jhcMatrix::Print");

  jprintf("\n");
  if ((tag != NULL) && (*tag != '\0'))
    jprintf("%s = \n", tag);
  for (j = 0; j < h; j++)
  {
    jprintf("  ");
    for (i = 0; i < w; i++)
      jprintf("% -10f ", MRef0(i, j));
    jprintf("\n");
  }
}


//= Shows values in vector, all in one line with brackets.
// can optionally insert text before this (extra space added)

void jhcMatrix::PrintVec (const char *tag, int cr) const
{
  int j;

  // print introduction
  if (tag != NULL)
  {
    jprint(tag);
    jprint(" = ");
  }

  // print values
  jprintf("[");
  for (j = 0; j < h; j++)
    jprintf(" %f", VRef0(j));
  jprintf(" ]");

  // end line
  if (cr > 0)
    jprintf("\n");
}


//= List contents to a multi-line string (needs big buffer).

char *jhcMatrix::List (char *buf, int ssz) const
{
  char item[20];
  int i, j;

  if (vals == NULL)
    Fatal("Bad input to jhcMatrix::List");

  *buf = '\0';
  for (j = 0; j < h; j++)
  {
    strcat_s(buf, ssz, "  ");
    for (i = 0; i < w; i++)
    {
      sprintf_s(item, "% -10f ", MRef0(i, j));
      strcat_s(buf, ssz, item);
    }
    strcat_s(buf, ssz, "\n");
  }
  return buf;
}


//= Generate a string form for a simple 3D vector.
// can supply a formatting command for floating point values

char *jhcMatrix::ListVec (char *buf, const char *fmt, int ssz) const
{
  char temp[40], res[20] = "%+4.2f";
  const char *prec = ((fmt == NULL) ? res : fmt);
  int i;

  if (buf == NULL)
    return NULL;
  if (!Vector(1))
  {
    strcpy_s(buf, ssz, "<bad dims>");
    return buf;
  }

  *buf = '\0';
  strcat_s(buf, ssz, "[");
  for (i = 0; i < h; i++)
  {
    if (i > 0)
      strcat_s(buf, ssz, " ");
    sprintf_s(temp, prec, VRef0(i));
    strcat_s(buf, ssz, temp);
  }
  strcat_s(buf, ssz, "]");
  return buf;
}


///////////////////////////////////////////////////////////////////////////
//                    Value Access and Modification                      //
///////////////////////////////////////////////////////////////////////////

//= Get pointer to some entry in the vector.

double *jhcMatrix::VPtrChk (int y) 
{
  if ((w != 1) || (y < 0) || (y >= h))
    Fatal("Bad input to jhcMatrix::VPtrChk");

  return VPtr0(y);
}


//= Get some entry in the vector.

double jhcMatrix::VRefChk (int y) const
{
  if ((w != 1) || (y < 0) || (y >= h))
    Fatal("Bad input to jhcMatrix::VRefChk");

  return VRef0(y);
}


//= Set some entry in the vector.

void jhcMatrix::VSetChk (int y, double v)
{
  if ((w != 1) || (y < 0) || (y >= h))
    Fatal("Bad input to jhcMatrix::VSetChk");

  VSet0(y, v);
}


//= Increment (or decrement) some entry in the vector.

void jhcMatrix::VIncChk (int y, double dv)
{
  if ((w != 1) || (y < 0) || (y >= h))
    Fatal("Bad input to jhcMatrix::VIncChk");

  VInc0(y, dv);
}


//= Get pointer to some entry in the matrix.

double *jhcMatrix::MPtrChk (int x, int y) 
{
  if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
    Fatal("Bad input to jhcMatrix::MPtrChk");

  return MPtr0(x, y);
}


//= Get some entry in the matrix.

double jhcMatrix::MRefChk (int x, int y) const
{
  if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
    Fatal("Bad input to jhcMatrix::MRefChk");

  return MRef0(x, y);
}


//= Set some entry in the matrix.

void jhcMatrix::MSetChk (int x, int y, double v)
{
  if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
    Fatal("Bad input to jhcMatrix::MSetChk");

  MSet0(x, y, v);
}


//= Increment some entry in the matrix.

void jhcMatrix::MIncChk (int x, int y, double dv)
{
  if ((x < 0) || (x >= w) || (y < 0) || (y >= h))
    Fatal("Bad input to jhcMatrix::MIncChk");

  MInc0(x, y, dv);
}


///////////////////////////////////////////////////////////////////////////
//                            Vector Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Returns the length of an aribtrary vector.
// not for use with homogenous coordinates

double jhcMatrix::LenVec () const
{
  return sqrt(Len2Vec());
}


//= Returns the squared length of an aribtrary vector.
// not for use with homogenous coordinates

double jhcMatrix::Len2Vec () const
{
  double v, sum = 0.0; 
  int i;

#ifdef _DEBUG
  if (!Vector(1))
    Fatal("Bad input to jhcMatrix::Len2Vec");
#endif

  for (i = 0; i < h; i++)
  {
    v = VRef0(i);
    sum += v * v;
  }
  return sum;
}


//= Returns the maximum coordinate in a vector.

double jhcMatrix::MaxVec () const
{
  double v, hi;
  int i;

#ifdef _DEBUG
  if (!Vector(1))
    Fatal("Bad input to jhcMatrix::MaxVec");
#endif

  hi = VRef(0);
  for (i = 1; i < h; i++)
  {
    v = VRef0(i);
    hi = __max(v, hi);
  }
  return hi;
}


//= Returns the minimum coordinate in a vector.

double jhcMatrix::MinVec () const
{
  double v, lo;
  int i;

#ifdef _DEBUG
  if (!Vector(1))
    Fatal("Bad input to jhcMatrix::MinVec");
#endif

  lo = VRef(0);
  for (i = 1; i < h; i++)
  {
    v = VRef0(i);
    lo = __min(v, lo);
  }
  return lo;
}


//= Return the dot product of self with some other vector.
// not for use with homogenous coordinates

double jhcMatrix::DotVec (const jhcMatrix& ref) const
{
  double sum = 0.0;
  int i;

#ifdef _DEBUG
  if (!Vector(1) || !SameSize(ref))
    Fatal("Bad input to jhcMatrix::DotVec");
#endif

  for (i = 0; i < h; i++)
    sum += VRef0(i) * ref.VRef0(i);
  return sum;
}


//= Set all elements of a vector to some value.

void jhcMatrix::FillVec (double val)
{
  int i;

#ifdef _DEBUG
  if (!Vector(1))
    Fatal("Bad input to jhcMatrix::FillVec");
#endif

  for (i = 0; i < h; i++)
    VSet0(i, val);
}


//= Increment each elements by corresponding amount found in input vector.
// only changes as many elements as vectors have in common

void jhcMatrix::IncVec (const jhcMatrix& ref)
{
  int i, n = __min(h, ref.h);

#ifdef _DEBUG
  if (!Vector(1) || !ref.Vector(1))
    Fatal("Bad input to jhcMatrix::IncVec");
#endif

  for (i = 0; i < n; i++)
    VInc0(i, ref.VRef0(i));
}


//= Multiply all elements by some factor.

void jhcMatrix::ScaleVec (double sc)
{
  int i;

  for (i = 0; i < h; i++)
    VSet0(i, VRef0(i) * sc);
}


//= Scale each elements by corresponding factor found in input vector.

void jhcMatrix::MultVec (const jhcMatrix& sc)
{
  int i;

#ifdef _DEBUG
  if (!Vector(1) || !SameSize(sc))
    Fatal("Bad input to jhcMatrix::MultVec");
#endif

  for (i = 0; i < h; i++)
    VSet0(i, VRef0(i) * sc.VRef0(i));
}


//= Fill self with element-wise sum of two vectors.
// not for use with homogenous coordinates

void jhcMatrix::AddVec (const jhcMatrix& a, const jhcMatrix& b)
{
  int i;

#ifdef _DEBUG
  if (!Vector(1) || !SameSize(a) || !SameSize(b))
    Fatal("Bad input to jhcMatrix::AddVec");
#endif

  for (i = 0; i < h; i++)
    VSet0(i, a.VRef0(i) + b.VRef0(i));
}


//= Fill self with element-wise difference of two vectors.
// not for use with homogenous coordinates

void jhcMatrix::DiffVec (const jhcMatrix& a, const jhcMatrix& b)
{
  int i;

#ifdef _DEBUG
  if (!Vector(1) || !SameSize(a) || !SameSize(b))
    Fatal("Bad input to jhcMatrix::DiffVec");
#endif

  for (i = 0; i < h; i++)
    VSet0(i, a.VRef0(i) - b.VRef0(i));
}


///////////////////////////////////////////////////////////////////////////
//                    Homegeneous Coordinate Functions                   //
///////////////////////////////////////////////////////////////////////////

//= Returns the length of a homogeneous coordinate 3D vector.

double jhcMatrix::LenVec3 () const
{
  return sqrt(Len2Vec3());
}


//= Returns the squared length of a homogeneous coordinate 3D vector.

double jhcMatrix::Len2Vec3 () const
{
  double x, y, z; 

#ifdef _DEBUG
  if (!Vector(3))
    Fatal("Bad input to jhcMatrix::Len2Vec3");
#endif

  x = VRef0(0);
  y = VRef0(1);
  z = VRef0(2);
  return(x * x + y * y + z * z);
}


//= Returns the length of the projection of the vector on the XY plane.

double jhcMatrix::PlaneVec3 () const
{
  double x, y; 

#ifdef _DEBUG
  if (!Vector(3))
    Fatal("Bad input to jhcMatrix::PlaneVec3");
#endif

  x = VRef0(0);
  y = VRef0(1);
  return sqrt(x * x + y * y);
}


//= Returns the maximum coordinate in a 3D vector.

double jhcMatrix::MaxVec3 () const
{
#ifdef _DEBUG
  if (!Vector(3))
    Fatal("Bad input to jhcMatrix::MaxVec3");
#endif

  return __max(VRef0(0), __max(VRef0(1), VRef0(2)));
}


//= Returns the maximum absolute coordinate in a 3D vector.
// like a length for an angular vector = RotDiff3 against Zero

double jhcMatrix::MaxAbs3 () const
{
#ifdef _DEBUG
  if (!Vector(3))
    Fatal("Bad input to jhcMatrix::MaxAbs3");
#endif

  return __max(fabs(VRef0(0)), __max(fabs(VRef0(1)), fabs(VRef0(2))));
}


//= Returns the minimum coordinate in a 3D vector.

double jhcMatrix::MinVec3 () const
{
#ifdef _DEBUG
  if (!Vector(3))
    Fatal("Bad input to jhcMatrix::MinVec3");
#endif

  return __min(VRef0(0), __min(VRef0(1), VRef0(2)));
}


//= Returns the length of the difference vector between two positions.

double jhcMatrix::PosDiff3 (const jhcMatrix& ref) const
{
  double dsq, sum;

#ifdef _DEBUG
  if (!Vector(4) || !ref.Vector(4))
    Fatal("Bad input to jhcMatrix::PosDiff3");
#endif

  dsq = VRef0(0) - ref.VRef0(0);
  dsq *= dsq;
  sum =  dsq;
  dsq = VRef0(1) - ref.VRef0(1);
  dsq *= dsq;
  sum += dsq;
  dsq = VRef0(2) - ref.VRef0(2);
  dsq *= dsq;
  sum += dsq;
  return((double) sqrt(sum));
}


//= Resolve a vector into a pan angle in the XY plane.

double jhcMatrix::PanVec3 () const
{
  return(R2D * atan2(VRef0(1), VRef0(0)));
}


//= Resolve a vector into a tilt angle relative to the Z axis.

double jhcMatrix::TiltVec3 () const
{
  return(R2D * atan2(VRef0(2), PlaneVec3()));
}


//= Return the dot product of self with some other 3D vector.

double jhcMatrix::DotVec3 (const jhcMatrix& ref) const
{
#ifdef _DEBUG
  if (!Vector(4) || !ref.Vector(4))
    Fatal("Bad input to jhcMatrix::DotVec3");
#endif

  return(VRef0(0) * ref.VRef0(0) + VRef0(1) * ref.VRef0(1) + VRef0(2) * ref.VRef0(2));
}


//= Return the dot product of self with some other vector including homogeneous components.
// useful for determining distance to plane represented as (a b c -d)

double jhcMatrix::DotVec4 (const jhcMatrix& ref) const
{
#ifdef _DEBUG
  if (!Vector(4) || !ref.Vector(4))
    Fatal("Bad input to jhcMatrix::DotVec4");
#endif

  return(VRef0(0) * ref.VRef0(0) + VRef0(1) * ref.VRef0(1) + VRef0(2) * ref.VRef0(2) + VRef0(3) * ref.VRef0(3));
}


//= Returns the angle (in degs) between unit vectors representing directions.

double jhcMatrix::DirUnit3 (const jhcMatrix& ref) const
{
  double dot;

#ifdef _DEBUG
  if (!Vector(4) || !ref.Vector(4))
    Fatal("Bad input to jhcMatrix::DirUnit3");
#endif

  dot = DotVec3(ref);
  return(R2D * acos(__max(-1.0, __min(dot, 1.0))));
}


//= Returns the angle (in degs) between general (non-unit) vectors.
// normalizes by length of vectors

double jhcMatrix::DirDiff3 (const jhcMatrix& ref) const
{
  double ndot;

#ifdef _DEBUG
  if (!Vector(4) || !ref.Vector(4))
    Fatal("Bad input to jhcMatrix::DirDiff3");
#endif

  ndot = DotVec3(ref) / sqrt(Len2Vec3() * ref.Len2Vec3());
  return(R2D * acos(__max(-1.0, __min(ndot, 1.0))));
}


//= Returns the maximum absolute angle difference of any single component.
// assumes vector elements are pan, tilt, and roll (ignores 4th element, if any)

double jhcMatrix::RotDiff3 (const jhcMatrix& ref) const
{
  double pd, td, rd;

#ifdef _DEBUG
  if (!Vector(3) || !ref.Vector(3))
    Fatal("Bad input to jhcMatrix::RotDiff3");
#endif

  pd = fabs(ang180(P() - ref.P()));
  td = fabs(ang180(T() - ref.T()));
  rd = fabs(ang180(R() - ref.R()));
  return __max(pd, __max(td, rd));
}


//= Normalizes angle to between +180 and -180 degrees.

double jhcMatrix::ang180 (double ang) const
{
  double a = ang;

  while (a > 180.0)
    a -= 360.0;
  while (a <= -180.0)
    a += 360.0;
  return a;
}


//= Find angular difference of self from reference vector in XY plane.
// vectors are XYZ elements not PTR elements, returns -180 to +180

double jhcMatrix::PanDiff3 (const jhcMatrix& ref) const
{
  return ang180(PanVec3() - ref.PanVec3());
}


//= Find angular difference of self from reference vector relative to XY plane.
// vectors are XYZ elements not PTR elements, returns -180 to +180

double jhcMatrix::TiltDiff3 (const jhcMatrix& ref) const
{
  return ang180(TiltVec3() - ref.TiltVec3());
}


//= Find pan and tilt angle to target position from self position.

void jhcMatrix::PanTilt3 (double& pan, double& tilt, const jhcMatrix& targ) const
{
  jhcMatrix tmp(4);

  tmp.DiffVec3(targ, *this);
  pan = tmp.PanVec3();
  tilt = tmp.TiltVec3();
}


//= Makes sure homogeneous component of vector is 1.
// often needed after perspective projection

void jhcMatrix::HomoDiv3 ()
{
  double iw;

#ifdef _DEBUG
  if (!Vector(4) || (VRef0(3) == 0.0))
    Fatal("Bad input to jhcMatrix::HomoDiv3");
#endif

  iw = 1.0 / VRef0(3);
  VSet0(0, iw * VRef0(0));
  VSet0(1, iw * VRef0(1));
  VSet0(2, iw * VRef0(2));
  VSet0(3, 1.0);
}


//= Set up a 2D column vector with specific values of X and Y.

void jhcMatrix::SetVec2 (double x, double y, double homo)
{
#ifdef _DEBUG
  if ((w != 1) || (h < 2) || (h > 3))
    Fatal("Bad input to jhcMatrix::SetVec2");
#endif

  VSet0(0, x);
  VSet0(1, y);
  if (h == 3)
    VSet0(2, homo);
}


//= Set up a 3D column vector with specific values of X, Y, and Z.

void jhcMatrix::SetVec3 (double x, double y, double z, double homo)
{
#ifdef _DEBUG
  if ((w != 1) || (h < 3) || (h > 4))
    Fatal("Bad input to jhcMatrix::SetVec3");
#endif

  VSet0(0, x);
  VSet0(1, y);
  VSet0(2, z);
  if (h == 4)
    VSet0(3, homo);
}


//= Make a unit vector with given pan angle in XY and given tilt angle wrt Z.

void jhcMatrix::SetPanTilt3 (double pan, double tilt, double homo)
{
  double prad = D2R * pan, trad = D2R * tilt;
  double cp = cos(prad), sp = sin(prad), ct = cos(trad), st = sin(trad);

  SetVec3(ct * cp, ct * sp, st); 
}



//= Extract components of vector into separate variables.
// returns homogeneous component (if any) 

double jhcMatrix::DumpVec3 (double& x, double& y, double& z) const
{
#ifdef _DEBUG
  if ((w != 1) || (h < 3) || (h > 4))
    Fatal("Bad input to jhcMatrix::DumpVec3");
#endif

  x = VRef0(0);
  y = VRef0(1);
  z = VRef0(2);
  if (h == 4)
    return VRef0(3);
  return 1.0;
}


//= Add two 3D column vectors together and put result in this one.

void jhcMatrix::AddVec3 (const jhcMatrix& a, const jhcMatrix& b)
{
#ifdef _DEBUG
  if (!Vector(3) || !a.Vector(3) || !b.Vector(3))
    Fatal("Bad input to jhcMatrix::AddVec3");
#endif

  VSet0(0, a.VRef0(0) + b.VRef0(0));
  VSet0(1, a.VRef0(1) + b.VRef0(1));
  VSet0(2, a.VRef0(2) + b.VRef0(2));
}


//= Add another 3D column vector to this one.
// returns 1 alway for convenience (e.g. increment sum for averaging)

int jhcMatrix::IncVec3 (const jhcMatrix& inc)
{
#ifdef _DEBUG
  if (!Vector(3) || !inc.Vector(3))
    Fatal("Bad input to jhcMatrix::IncVec3");
#endif

  VInc0(0, inc.VRef0(0));
  VInc0(1, inc.VRef0(1));
  VInc0(2, inc.VRef0(2));
  return 1;
}


//= Adjust a 3D column vector by some amount.
// returns 1 alway for convenience (e.g. increment sum for averaging)

int jhcMatrix::IncVec3 (double dx, double dy, double dz)
{
#ifdef _DEBUG
  if ((w != 1) || (h < 3) || (h > 4))
    Fatal("Bad input to jhcMatrix::IncVec3");
#endif

  VInc0(0, dx);
  VInc0(1, dy);
  VInc0(2, dz);
  return 1;
}


//= Build new vector which is old vector altered by given offset.
// self = ref + (dx dy dz)

void jhcMatrix::RelVec3 (const jhcMatrix& ref, double dx, double dy, double dz)
{
#ifdef _DEBUG
  if ((w != 1) || (ref.w != 1) || (ref.h != h) || (h < 3) || (h > 4))
    Fatal("Bad input to jhcMatrix::RelVec3");
#endif

  VSet0(0, ref.VRef0(0) + dx);
  VSet0(1, ref.VRef0(1) + dy);
  VSet0(2, ref.VRef0(2) + dz);
  if (h > 3)
    VSet0(3, ref.VRef0(3));
}


//= Add reference scaled by factor to self.
// self += f * inc

void jhcMatrix::AddFrac3 (const jhcMatrix& inc, double f)
{
#ifdef _DEBUG
  if ((w != 1) || (inc.w != 1) || (inc.h != h) || (h < 3) || (h > 4))
    Fatal("Bad input to jhcMatrix::AddFrac3");
#endif

  VInc0(0, f * inc.VRef0(0));
  VInc0(1, f * inc.VRef0(1));
  VInc0(2, f * inc.VRef0(2));
  if (h > 3)
    VSet0(3, inc.VRef0(3));
}


//= Set self to be reference vector plus some multiple of increment vector.
// self = ref + f * inc

void jhcMatrix::RelFrac3 (const jhcMatrix& ref, const jhcMatrix& inc, double f)
{
  Copy(ref);
  AddFrac3(inc, f);
}


//= Multiply all 3D coordinates of reference by some value.

void jhcMatrix::ScaleVec3 (const jhcMatrix& ref, double sc, double homo)
{
#ifdef _DEBUG
  if (!Vector(3) || !ref.Vector(3))
    Fatal("Bad input to jhcMatrix::ScaleVec3");
#endif

  SetVec3(sc * ref.VRef0(0), sc * ref.VRef0(1), sc * ref.VRef0(2), homo);
}


//= Multiply all 3D coordinates by some value.

void jhcMatrix::ScaleVec3 (double sc, double homo)
{
  ScaleVec3(*this, sc, homo);
}


//= Set self to a mixture of two vectors = (1 - f) * a + f * b.

void jhcMatrix::MixVec3 (const jhcMatrix& a, const jhcMatrix& b, double f)
{
  double cf = 1.0 - f;
#ifdef _DEBUG
  if (!Vector(3) || !a.Vector(3) || !b.Vector(3))
    Fatal("Bad input to jhcMatrix::MixVec3");
#endif
  if (f == 0.0)
    Copy(a);
  else if (f == 1.0)
    Copy(b);
  else
  {
    VSet0(0, cf * a.VRef0(0) + f * b.VRef0(0));
    VSet0(1, cf * a.VRef0(1) + f * b.VRef0(1));
    VSet0(2, cf * a.VRef0(2) + f * b.VRef0(2));
  }
}


//= Move self part of the way toward the target.

void jhcMatrix::MixVec3 (const jhcMatrix& target, double f)
{
  MixVec3(*this, target, f);
}


//= Replace all zero components of self with values from given vector.

void jhcMatrix::SubZero3 (const jhcMatrix& replace)
{
#ifdef _DEBUG
  if (!Vector(3) || !replace.Vector(3))
    Fatal("Bad input to jhcMatrix::SubZero3");
#endif

  if (VRef0(0) == 0.0)
    VSet0(0, replace.VRef0(0));
  if (VRef0(1) == 0.0)
    VSet0(1, replace.VRef0(1));
  if (VRef0(2) == 0.0)
    VSet0(2, replace.VRef0(2));
}


//= Clamp all components to be within +/- limits.

void jhcMatrix::ClampVec3 (const jhcMatrix& lim)
{
#ifdef _DEBUG
  if (!Vector(3) || !lim.Vector(3))
    Fatal("Bad input to jhcMatrix::ClampVec3");
#endif
  VSet0(0, __max(-lim.VRef0(0), __min(VRef0(0), lim.VRef0(0))));
  VSet0(1, __max(-lim.VRef0(1), __min(VRef0(1), lim.VRef0(1))));
  VSet0(2, __max(-lim.VRef0(2), __min(VRef0(2), lim.VRef0(2))));
}


//= Clamp all components to be within +/- a certain limit.

void jhcMatrix::ClampVec3 (double lim)
{
#ifdef _DEBUG
  if (!Vector(3))
    Fatal("Bad input to jhcMatrix::ClampVec3");
#endif
  VSet0(0, __max(-lim, __min(VRef0(0), lim)));
  VSet0(1, __max(-lim, __min(VRef0(1), lim)));
  VSet0(2, __max(-lim, __min(VRef0(2), lim)));
}


//= Fill self with negated version of input vector.

void jhcMatrix::FlipVec3 (const jhcMatrix& ref, double homo)
{
#ifdef _DEBUG
  if (!Vector(3) || !ref.Vector(3))
    Fatal("Bad input to jhcMatrix::FlipVec3");
#endif

  SetVec3(-ref.X(), -ref.Y(), -ref.Z(), homo);
}


//= Fill self with difference between input vector and self.
// useful for reflecting or rotating a point in an image

void jhcMatrix::CompVec3 (const jhcMatrix& wrt, double homo)
{
#ifdef _DEBUG
  if (!Vector(3) || !wrt.Vector(3))
    Fatal("Bad input to jhcMatrix::CompVec3");
#endif

  VSet0(0, wrt.VRef0(0) - VRef0(0));
  VSet0(1, wrt.VRef0(1) - VRef0(1));
  VSet0(2, wrt.VRef0(2) - VRef0(2));
  VSet0(3, homo);
}


//= Scale each elements by corresponding factor found in input vector.
// does not scale homogenous component

void jhcMatrix::MultVec3 (const jhcMatrix& sc, double homo)
{
#ifdef _DEBUG
  if (!Vector(4) || !sc.Vector(3))
    Fatal("Bad input to jhcMatrix::MultVec");
#endif

  SetVec3(sc.X() * X(), sc.Y() * Y(), sc.Z() * Z(), homo);
}


//= Fill self with element-wise difference of two 3D vectors.

void jhcMatrix::DiffVec3 (const jhcMatrix& a, const jhcMatrix& b, double homo)
{
#ifdef _DEBUG
  if (!Vector(4) || !a.Vector(4) || !b.Vector(4))
    Fatal("Bad input to jhcMatrix::DiffVec3");
#endif

  SetVec3(a.X() - b.X(), a.Y() - b.Y(), a.Z() - b.Z(), homo);
}


//= Fill self with unit vector pointing from b to a.
// returns the length of the difference vector

double jhcMatrix::DirVec3 (const jhcMatrix& a, const jhcMatrix& b, double homo)
{
  DiffVec3(a, b, homo);
  return UnitVec3();
}


//= Fill seld with "unit" vector pointing from pan, tilt, roll vector b to a.
// essentially divides through by largest component and saves ratios
// returns absolute magnitude of biggest component

double jhcMatrix::RotDir3 (const jhcMatrix& a, const jhcMatrix& b)
{
#ifdef _DEBUG
  if (!Vector(4) || !a.Vector(4) || !b.Vector(4))
    Fatal("Bad input to jhcMatrix::RotDir3");
#endif

  SetP(ang180(a.P() - b.P()));
  SetT(ang180(a.T() - b.T()));
  SetR(ang180(a.R() - b.R()));
  return RotUnit3();
}


//= Fill self with the cross product of two 3D vectors.
// Note: ignores 4th element of vector (e.g. homogeneous part)

void jhcMatrix::CrossVec3 (const jhcMatrix& a, const jhcMatrix& b, double homo)
{
  double x, y, z;

#ifdef _DEBUG
  if (!Vector(4) || !a.Vector(4) || !b.Vector(4))
    Fatal("Bad input to jhcMatrix::CrossVec3");
#endif

  x = a.VRef0(1) * b.VRef0(2) - a.VRef0(2) * b.VRef0(1);
  y = a.VRef0(2) * b.VRef0(0) - a.VRef0(0) * b.VRef0(2);
  z = a.VRef0(0) * b.VRef0(1) - a.VRef0(1) * b.VRef0(0);
  SetVec3(x, y, z, homo);
}


//= Rotate self around Z axis by a certain number of degrees.

void jhcMatrix::RotPan3 (double pan)
{
  double x, y, rads = D2R * pan, c = cos(rads), s = sin(rads);

#ifdef _DEBUG
  if (!Vector(4))
    Fatal("Bad input to jhcMatrix::RotPan3");
#endif

  x = c * VRef0(0) - s * VRef0(1); 
  y = s * VRef0(0) + c * VRef0(1); 
  VSet0(0, x);
  VSet0(1, y);
}


//= Rotate self away from Z axis by a certain number of degrees.

void jhcMatrix::RotTilt3 (double tilt)
{
  double xy0, z0, xy, z, f, rads = D2R * tilt, c = cos(rads), s = sin(rads);

#ifdef _DEBUG
  if (!Vector(4))
    Fatal("Bad input to jhcMatrix::RotTilt3");
#endif

  // generate 2D vector and rotate it
  xy0 = PlaneVec3();  
  z0 = VRef0(2);
  z  = c * z0 - s * xy0; 
  xy = s * z0 + c * xy0;

  // reconstruct separate x and y from new length
  if (xy0 == 0.0)
    SetVec3(xy, 0.0, z, VRef0(3));     // original was vertical
  else
  {
    f = xy / xy0;
    SetVec3(f * VRef0(0), f * VRef0(1), z, VRef0(3));
  }
}


//= Shows the 3 (or 4) values in the vector using given numeric format.
// can optionally insert text before this (extra space added)

void jhcMatrix::PrintVec3 (const char *tag, const char *fmt, int all4, int cr) const
{
  char txt[80];

  // print introduction
  if (tag != NULL)
  {
    jprint(tag);
    jprint(" = ");
  }

  // print values
  ListVec3(txt, fmt, all4, 80);
  jprintf(txt);

  // end line
  if (cr > 0)
    jprintf("\n");
}


//= Generate a string form for a simple 3D vector.
// can supply a formatting command for floating point values
// can optionally print the final 1 in homogeneous vectors

char *jhcMatrix::ListVec3 (char *txt, const char *fmt, int all4, int ssz) const
{
  char temp[80], res[20] = "%3.1f";
  const char *prec = ((fmt == NULL) ? res : fmt);

  if (txt == NULL)
    return NULL;
  if (!Vector(4))
  {
    strcpy_s(txt, ssz, "<bad dims>");
    return txt;
  }

  if (all4 > 0)
    sprintf_s(temp, "[%s %s %s : %s]", prec, prec, prec, prec);
  else
    sprintf_s(temp, "[%s %s %s]", prec, prec, prec);
  sprintf_s(txt, ssz, temp, VRef0(0), VRef0(1), VRef0(2), VRef0(3));  // not 1 in perspective
  return txt;
}


///////////////////////////////////////////////////////////////////////////
//                      Directions and Rotations                         //
///////////////////////////////////////////////////////////////////////////

//= Adjust first three components to lie within +/- 180 degrees.

void jhcMatrix::CycNorm3 ()
{
#ifdef _DEBUG
  if (!Vector(4))
    Fatal("Bad input to jhcMatrix::CycNorm3");
#endif

  SetP(ang180(P()));
  SetT(ang180(T()));
  SetR(ang180(R()));
}


//= Set self to a 3D unit vector based on input.
// returns original length of vector

double jhcMatrix::UnitVec3 (const jhcMatrix& ref, double homo)
{
  double len = ref.LenVec3();

#ifdef _DEBUG
  if (len < 0.0)
    Fatal("Bad input to jhcMatrix::UnitVec3");
#endif

  if (len > 0.0)
    ScaleVec3(ref, 1.0 / len, homo);
  return len;
}


//= Normalize self to be a 3D unit vector.
// returns original length of vector

double jhcMatrix::UnitVec3 (double homo)
{
  return UnitVec3(*this, homo);
}


//= Normalize all pan, tilt, roll values by one with largest magnitude.
// returns the largest absolute magnitude

double jhcMatrix::RotUnit3 ()
{
  double len = MaxAbs3();

#ifdef _DEBUG
  if (len < 0.0)
    Fatal("Bad input to jhcMatrix::RotUnit3");
#endif

  if (len > 0.0)
    ScaleVec3(1.0 / len, 0.0);
  return len;
}


//= Construct a unit vector with yaw in XY plane and pitch relative to Z.
// all angles are in degrees

void jhcMatrix::EulerVec3 (double yaw, double pitch, double homo)
{
  double yaw_r = D2R * yaw, pitch_r = D2R * pitch, flat = cos(pitch_r);

#ifdef _DEBUG
  if (!Vector(4))
    Fatal("Bad input to jhcMatrix::EulerVec3");
#endif

  SetVec3(flat * cos(yaw_r), flat * sin(yaw_r), sin(pitch_r), homo);
}


//= Connstruct a unit vector based on pan and tilt with 4th element being roll.

void jhcMatrix::EulerVec4 (const jhcMatrix& ptr)
{
  EulerVec3(ptr.VRef(0), ptr.VRef(1), ptr.VRef(2));
}


//= Determine angle of projection on XY plane.
// vector does not have to have unit length

double jhcMatrix::YawVec3 () const
{
#ifdef _DEBUG
  if (!Vector(4))
    Fatal("Bad input to jhcMatrix::YawVec3");
#endif

  if ((Y() == 0.0) && (X() == 0.0))
    return 0.0;
  return(R2D * atan2(Y(), X()));
}


//= Determine angle relative to Z axis.
// vector does not have to have unit length

double jhcMatrix::PitchVec3 () const
{
#ifdef _DEBUG
  if (!Vector(4))
    Fatal("Bad input to jhcMatrix::PitchVec3");
#endif

  return(R2D * atan2(Z(), (double) sqrt(X() * X() + Y() * Y())));
}


//= Fill self with a quaternion based on a rotation axis and angle.
// axis does not have to be a unit vector (quaternion will be normalized)
// first 3 quaternion elements are the vector part, fourth element is the scalar

void jhcMatrix::Quaternion (const jhcMatrix& axis, double degs)
{
  double ha = 0.5 * D2R * degs;

#ifdef _DEBUG
  if (!Vector(4) || !axis.Vector(3))
    Fatal("Bad input to jhcMatrix::Quaternion");
#endif

  ScaleVec3(axis, sin(ha) / axis.LenVec3());
  VSet0(3, cos(ha));
}


//= Fill self with quaternion based on angle scaled rotation axis.
// first 3 quaternion elements are the vector part, fourth element is the scalar

void jhcMatrix::Quaternion (const jhcMatrix& rot)
{
  double len, ha, s;
 
#ifdef _DEBUG
  if (!Vector(4) || !rot.Vector(3))
    Fatal("Bad input to jhcMatrix::Quaternion");
#endif

  // check for special case of no rotation
  len = rot.LenVec3();
  if (len == 0.0)
  {
    Zero(1.0);  // cos(0/2) = 1
    return;
  }

  // normalize vector and form unit quaternion
  ha = 0.5 * D2R * len;
  s = sin(ha) / len;
  VSet0(0, s * rot.X());
  VSet0(1, s * rot.Y());
  VSet0(2, s * rot.Z());
  VSet0(3, cos(ha));
}


//= Fill self with the rotation axis of the quaternion and return the angle.
// first 3 quaternion elements are the vector part, fourth element is the scalar
// resulting axis is a unit vector

double jhcMatrix::AxisQ (const jhcMatrix& q)
{
  double hcos;

#ifdef _DEBUG
  if (!Vector(3) || !q.Vector(4))
    Fatal("Bad input to jhcMatrix::AxisQ");
#endif

  // find cosine of half rotation angle from fourth component  
  hcos = q.VRef0(3);
  if (hcos == 1.0)
  {
    SetVec3(0.0, 0.0, 1.0);  // arbitrarily pick z axis
    return 0.0;
  }
  hcos = __max(-1.0, __min(hcos, 1.0));

  // use vector direction and get half rotation angle from scalar
  UnitVec3(q);
  return(2.0 * R2D * acos(hcos));
}


//= Fill self with a rotation axis scaled by the angle of the quaternion.
// first 3 quaternion elements are the vector part, fourth element is the scalar

void jhcMatrix::RotatorQ (const jhcMatrix& q)
{
  double hcos;

#ifdef _DEBUG
  if (!Vector(3) || !q.Vector(4))
    Fatal("Bad input to jhcMatrix::RotatorQ");
#endif

  // find cosine of half rotation angle from fourth component  
  hcos = q.VRef0(3);
  if (hcos == 1.0)
  {
    Zero(0.0);                        
    return;
  }
  hcos = __max(-1.0, __min(hcos, 1.0));

  // extract rotation axis from first 3 components then scale it
  ScaleVec3(q, 2.0 * R2D * acos(hcos) / q.LenVec3(), 0.0);  
}


//= Gives quaternion equivalent to first rotating by q1 then by q2.
// first 3 quaternion elements are the vector part, fourth element is the scalar
// convenient since composite rotation axis and angle are more explicit
// <pre>
//            +-                   -+ +-  -+
//            |  w2   z2  -y2   x2  | | x1 |
//  q1 * q2 = | -z2   w2   x2   y2  | | y1 |
//            |  y2  -x2   w2   z2  | | z1 |
//            | -x2  -y2  -z2   w2  | | w1 |
//            +-                   -+ +-  -+
// </pre>

void jhcMatrix::CascadeQ (const jhcMatrix& q1, const jhcMatrix& q2)
{
  double x1, y1, z1, w1, x2, y2, z2, w2; 

#ifdef _DEBUG
  if (!Vector(3) || !q1.Vector(4) || !q2.Vector(4))
    Fatal("Bad input to jhcMatrix::CascadeQ");
#endif

  // unpack quaternions for clarity (and in case this = q1 or q2)
  x1 = q1.X();
  y1 = q1.Y();
  z1 = q1.Z();
  w1 = q1.VRef0(3);
  x2 = q2.X();
  y2 = q2.Y();
  z2 = q2.Z();
  w2 = q2.VRef0(3);

  // do equivalent matrix multiply
  VSet0(0,  w2 * x1 + z2 * y1 - y2 * z1 + x2 * w1); 
  VSet0(1, -z2 * x1 + w2 * y1 + x2 * z1 + y2 * w1); 
  VSet0(2,  y2 * x1 - x2 * y1 + w2 * z1 + z2 * w1); 
  VSet0(3, -x2 * x1 - y2 * y1 - z2 * z1 + w2 * w1); 
}


//= Create a set of angles which is (1 - f) * a + f * b, all limited to +/- 180.
// self can be same as a or b vector without causing trouble

void jhcMatrix::CycMix3 (const jhcMatrix& a, const jhcMatrix& b, double f)
{
  double x = a.VRef0(0), y = a.VRef0(1), z = a.VRef0(2);

  CycDiff3(b, a);     
  ScaleVec3(f);
  IncVec3(x, y, z);  
  CycNorm3();
}


///////////////////////////////////////////////////////////////////////////
//                              Special Forms                            //
///////////////////////////////////////////////////////////////////////////

//= Set up square matrix to rotate various amounts around the X axis.
// only for 3x3 or 4x4 homogeneous

void jhcMatrix::RotationX (double degs)
{
  double a = D2R * degs, s = sin(a), c = cos(a);

#ifdef _DEBUG
  if ((w != h) || (w < 3) || (w > 4))
    Fatal("Bad input to jhcMatrix::RotationX");
#endif

  Identity();
  MSet0(1, 1,  c);      // middle row
  MSet0(2, 1,     -s);
  MSet0(1, 2,  s);      // bottom row
  MSet0(2, 2,      c);
}


//= Applies a rotation around X to the existing matrix (left multiplies it).
// composes rotation with current matrix instead of generating a rotation matrix

void jhcMatrix::RotateX (double degs)
{
  jhcMatrix rot(*this);

  rot.RotationX(degs);
  MatMat(rot, *this);
}


//= Set up square matrix to rotate various amounts around the Y axis.
// only for 3x3 or 4x4 homogeneous

void jhcMatrix::RotationY (double degs)
{
  double a = D2R * degs, s = sin(a), c = cos(a);

#ifdef _DEBUG
  if ((w != h) || (w < 3) || (w > 4))
    Fatal("Bad input to jhcMatrix::RotationY");
#endif

  Identity();
  MSet0(0, 0,  c);      // top row
  MSet0(2, 0,      s);
  MSet0(0, 2, -s);      // bottom row
  MSet0(2, 2,      c);
}


//= Applies a rotation around Y to the existing matrix (left multiplies it).
// composes rotation with current matrix instead of generating a rotation matrix

void jhcMatrix::RotateY (double degs)
{
  jhcMatrix rot(*this);

  rot.RotationY(degs);
  MatMat(rot, *this);
}


//= Set up square matrix to rotate various amounts around the Z axis.
// works for 2x2, 3x3, or 4x4 homogeneous

void jhcMatrix::RotationZ (double degs)
{
  double a = D2R * degs, s = sin(a), c = cos(a);

#ifdef _DEBUG
  if ((w != h) || (w < 2) || (w > 4))
    Fatal("Bad input to jhcMatrix::RotationZ");
#endif

  Identity();
  MSet0(0, 0,  c);      // top row
  MSet0(1, 0,     -s);
  MSet0(0, 1,  s);      // middle row
  MSet0(1, 1,      c);
}


//= Applies a rotation around Z to the existing matrix (left multiplies it).
// composes rotation with current matrix instead of generating a rotation matrix

void jhcMatrix::RotateZ (double degs)
{
  jhcMatrix rot(*this);

  rot.RotationZ(degs);
  MatMat(rot, *this);
}


//= Simultaneously rotate various amounts around the x, y, and z axes.
// if clr = 0 then translation/scaling part of homogeneous matrix is left unchanged
// only for 3x3 or 4x4 homogeneous

void jhcMatrix::Rotation (double xdegs, double ydegs, double zdegs, int clr)
{
  double ax = D2R * xdegs, sx = sin(ax), cx = cos(ax);
  double ay = D2R * ydegs, sy = sin(ay), cy = cos(ay);
  double az = D2R * zdegs, sz = sin(az), cz = cos(az);

#ifdef _DEBUG
  if ((w != h) || (w < 3) || (w > 4))
    Fatal("Bad input to jhcMatrix::Rotation");
#endif

  if ((w < 4) || (clr > 0))
    Identity();
  MSet0(0, 0,  cy * cz);                                                    // top 
  MSet0(1, 0,           -cx * sz + sx * sy * cz);
  MSet0(2, 0,                                     sx * sz + cx * sy * cz);
  MSet0(0, 1,  cy * sz);                                                    // middle
  MSet0(1, 1,            cx * cz + sx * sy * sz);
  MSet0(2, 1,                                    -sx * cz + cx * sy * sz);
  MSet0(0, 2, -sy);                                                         // bottom
  MSet0(1, 2,            sx * cy);
  MSet0(2, 2,                                     cx * cy);
}


//= Set up homogeneous 4x4 matrix to shift by certain amount along axes.
// if clr = 0 then rotation/scaling part of homogeneous matrix is left unchanged

void jhcMatrix::Translation (double dx, double dy, double dz, int clr)
{
#ifdef _DEBUG
  if ((w != h) || (w != 4))
    Fatal("Bad input to jhcMatrix::Translation");
#endif

  if (clr > 0)
    Identity();
  MSet0(3, 0, dx);
  MSet0(3, 1, dy);
  MSet0(3, 2, dz);
}


//= Set up matrix to translate coordinates relative to reference vector.

void jhcMatrix::Translation (const jhcMatrix& ref, int clr)
{
#ifdef _DEBUG
  if (!ref.Vector(3))
    Fatal("Bad input to jhcMatrix::Translation");
#endif

  return Translation(ref.X(), ref.Y(), ref.Z(), clr);
}


//= Applies a translation to the existing matrix (left multiplies it).
// composes translation with current matrix instead of generating a translation matrix

void jhcMatrix::Translate (double dx, double dy, double dz)
{
  jhcMatrix mv(*this);

  mv.Translation(dx, dy, dz, 1);
  MatMat(mv, *this);
}


//= Applies translation of points relative to given reference (alters matrix).

void jhcMatrix::Translate (const jhcMatrix& ref)
{
#ifdef _DEBUG
  if (!ref.Vector(3))
    Fatal("Bad input to jhcMatrix::Translate");
#endif

  return Translate(ref.X(), ref.Y(), ref.Z());
}


//= Set up homogeneous 4x4 matrix to magnify coordinates by some factor.

void jhcMatrix::Magnification (double fx, double fy, double fz)
{
#ifdef _DEBUG
  if ((w != 4) || (h != 4))
    Fatal("Bad input to jhcMatrix::Magnification");
#endif

  Identity();
  MSet0(0, 0, fx); 
  MSet0(1, 1, fy); 
  MSet0(2, 2, fz); 
}


//= Applies a magnification to the existing matrix.

void jhcMatrix::Magnify (double fx, double fy, double fz)
{
  int x;

#ifdef _DEBUG
  if ((w != 4) || (h != 4))
    Fatal("Bad input to jhcMatrix::Magnify");
#endif

  for (x = 0; x < 4; x++)
    MSet0(x, 0, fx * MRef0(x, 0)); 
  for (x = 0; x < 4; x++)
    MSet0(x, 1, fy * MRef0(x, 1)); 
  for (x = 0; x < 4; x++)
    MSet0(x, 2, fz * MRef0(x, 2)); 
}


//= Set up a homogeneous camera perspective projection matrix for world-to-camera transform.
// if clr = 0 then rotation/scaling/translation part of homogeneous matrix is unchanged
// the output coordinates and f are in the same units (e.g. pixels)
// need to call HomoDiv3 after this to get clean x and y coordinates

void jhcMatrix::Projection (double f, int clr)
{
#ifdef _DEBUG
  if ((w != 4) || (h != 4) || (f == 0.0))
    Fatal("Bad input to jhcMatrix::Projection");
#endif

  if (clr > 0)
  {
    Identity();
    MSet0(3, 3, 0.0);
  }
  MSet0(2, 3, 1.0 / f);
}


//= Apply perspective transform to some existing transform matrix (left multiply it).

void jhcMatrix::Project (double f)
{
  jhcMatrix pro(*this);

  pro.Projection(f, 1);
  MatMat(pro, *this);
}
 

///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Left multiply the transpose of a vector to give another vector.
// vector result goes into self

void jhcMatrix::MatVec (const jhcMatrix& mat, const jhcMatrix& vec) 
{
#ifdef _DEBUG
  if (!Vector() || !vec.Vector() || !mat.SameSize(vec.h, h))
    Fatal("Bad input to jhcMatrix::MatVec");
#endif

  // disjoint vector case  
  if (this != &vec)
  {
    MatVec0(mat, vec);
    return;
  }

  // update in place case
  jhcMatrix tmp(w, h);
  tmp.MatVec0(mat, vec);
  Copy(tmp);
} 


//= Left multiply the transpose of a vector without checking sizes.
// assumes output vector is not the same as the input vector
// vector result goes into self, slightly faster than full version
// Note: only uses first w elements of vector (e.g. 3 out of 4)

void jhcMatrix::MatVec0 (const jhcMatrix& mat, const jhcMatrix& vec) 
{
  double a;
  int i, j, mh = mat.h, mw = mat.w;

  for (j = 0; j < mh; j++)
  {
    a = 0.0;
    for (i = 0; i < mw; i++)
      a += mat.MRef0(i, j) * vec.VRef0(i);
    VSet0(j, a);
  }
} 


//= Multiply two matrices to yield a new matrix (in self).

void jhcMatrix::MatMat (const jhcMatrix& lf, const jhcMatrix& rt)
{
#ifdef _DEBUG
  if (!SameSize(rt.w, lf.h) || (rt.h != lf.w))
    Fatal("Bad input to jhcMatrix::MatMat");
#endif

  // disjoint matrix case  
  if ((this != &lf) && (this != &rt))
  {
    mm_core(lf, rt);
    return;
  }

  // update in place case
  jhcMatrix tmp(w, h);
  tmp.mm_core(lf, rt);
  Copy(tmp);
}


//= Left multiply a matrix to yield a new matrix without checking sizes.
// assumes self is not the same as the left or right matrices

void jhcMatrix::mm_core (const jhcMatrix& lf, const jhcMatrix& rt) 
{
  double v;
  int i, j, k, rh = rt.h;

  for (i = 0; i < w; i++)
    for (j = 0; j < h; j++)
    {
      v = 0.0;
      for (k = 0; k < rh; k++)
        v += lf.MRef0(k, j) * rt.MRef0(i, k);
      MSet0(i, j, v);
    }
}


//= Get the inverse of a square matrix into self.
// return 1 if successful, 0 if singular

int jhcMatrix::Invert (const jhcMatrix& ref)
{
#ifdef _DEBUG
  if ((w != h) || !SameSize(ref))
    Fatal("Bad input to jhcMatrix::Invert");
#endif

  // disjoint case
  if (this != &ref)
    return inv_core(ref);

  // update in place case
  jhcMatrix tmp(w, h);
  if (tmp.inv_core(ref) <= 0)
    return 0;
  Copy(tmp);
  return 1;
}


//= Get the inverse of a matrix without checking if square.
// uses Gauss-Jordan elimination with full pivoting (see Numerical Recipes)
// assumes ref is not the same as self, returns 0 if singular

int jhcMatrix::inv_core (const jhcMatrix& ref)
{
  jhcMatrix a(ref);
  int *fixed;
  double val, big, swap, recip, f;
  int d, i, j, row, diag;

  // original matrix "a" goes to identity, self "b" reduced in place to inverse
  a.Copy(ref);
  Identity();

  // no diagonal positions used yet
  fixed = new int[w];
  for (d = 0; d < w; d++)
    fixed[d] = 0;

  // loop over diagonal positions
  for (d = 0; d < w; d++)
  {
    // search over whole matrix for max value (full pivoting)
    big = 0.0;
    for (j = 0; j < h; j++)
      if (fixed[j] <= 0)              
        for (i = 0; i < w; i++)
          if (fixed[i] <= 0) 
          {
            // check value in potential unused diagonal position
            val = fabs(a.MRef0(i, j));  
            if (val >= big)
            {
              big  = val;
              diag = i;
              row  = j;
            }
          }

    // fail if nothing > zero found, else mark biggest as used
    if (big <= 0.0)
      return 0;
    fixed[diag] = 1;                   

    // swap rows to put pivot value on diagonal (column swap implicit)
    if (row != diag)
      for (i = 0; i < w; i++)
      {
        swap = a.MRef0(i, row);
        a.MSet0(i, row, a.MRef0(i, diag));
        a.MSet0(i, diag, swap);
        swap = MRef0(i, row);
        MSet0(i, row, MRef0(i, diag));
        MSet0(i, diag, swap);
      }

    // divide values on pivot row by the pivot value (yields 1 on diagonal)
    recip = 1.0 / a.MRef0(diag, diag);
    for (i = 0; i < w; i++)
    {
      a.MSet0(i, diag, recip * a.MRef0(i, diag));
      MSet0(i, diag, recip * MRef0(i, diag));
    }
 
    // do elimination by subtracting off scaled pivot row from all others
    for (j = 0; j < h; j++)
      if (j != diag)
      {
        f = a.MRef0(diag, j);        // make element on pivot's row be zero
        for (i = 0; i < w; i++)
        {
          a.MInc0(i, j, -f * a.MRef0(i, diag));
          MInc0(i, j, -f * MRef0(i, diag));
        }
      }
  }

  // cleanup 
  delete [] fixed;
  return 1;
}


//= Get the determinant of a square matrix.

double jhcMatrix::Det () const
{
  double cf, sum = 0.0;
  int i, j, pick, col;
  
#ifdef _DEBUG
  if (w != h)
    Fatal("Bad input to jhcMatrix::Det");
#endif

  // special cases for small matrices
  if (w == 1)
    return MRef0(0, 0);
  if (w == 2)
    return(MRef0(0, 0) * MRef0(1, 1) - MRef0(1, 0) * MRef0(0, 1));
  if (w == 3)
    return(MRef0(0, 0) * MRef0(1, 1) * MRef0(2, 2) +
           MRef0(1, 0) * MRef0(2, 1) * MRef0(0, 2) + 
           MRef0(2, 0) * MRef0(0, 1) * MRef0(1, 2) -
           MRef0(2, 0) * MRef0(1, 1) * MRef0(0, 2) - 
           MRef0(1, 0) * MRef0(0, 1) * MRef0(2, 2) -
           MRef0(0, 0) * MRef0(2, 1) * MRef0(1, 2));
 
  // general case with cofactors from first row
  jhcMatrix minor(w - 1, h - 1);
  for (pick = 0; pick < w; pick++)
  {
    // get cofactor with correct sign
    if ((cf = MRef0(pick, 0)) == 0.0)
      continue;
    if ((pick & 0x01) != 0)
      cf = -cf;

    // build minor matrix omitting first row and relevant column
    col = 0;
    for (i = 0; i < w; i++)
      if (i != pick)
      {
        for (j = 1; j < h; j++)
          minor.MSet0(col, j - 1, MRef0(i, j));
        col++;
      } 

    // recursively combine determinants of minors weight by cofactors
    sum += cf * minor.Det();
  }
  return sum;
}
