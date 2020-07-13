// jhcPlaneEst.cpp : estimates best fitting plane for a collection of points
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

#include "Geometry/jhcPlaneEst.h"


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Load a bunch of points then do plane fitting on them.
// allows post-scaling of xyz coordinates by given factors
// returns 1 if successful, 0 if problem

int jhcPlaneEst::FitPts (const double *x, const double *y, const double *z, int n,
                         double xsc, double ysc, double zsc)
{
  int i;

  ClrStats();
  for (i = 0; i < n; i++)
    AddPoint(x[i], y[i], z[i]);
  return Analyze(xsc, ysc, zsc);
}


//= Get rid of all accumulated statistics.

void jhcPlaneEst::ClrStats ()
{
  // statistics
  num = 0.0;
  Sx  = 0.0;
  Sy  = 0.0;
  Sz  = 0.0;
  Sxx = 0.0;
  Syy = 0.0;
  Szz = 0.0;
  Sxy = 0.0;
  Sxz = 0.0;
  Syz = 0.0;

  // plane fit coefficients
  err = -1.0;
  a = 0.0;
  b = 0.0;
  c = 0.0;
}


//= Add a single point to accumulated statistics.

int jhcPlaneEst::AddPoint (double x, double y, double z)
{
  Sx += x;  
  Sy += y;  
  Sz += z;  
  Sxx += x * x;  
  Syy += y * y;  
  Szz += z * z;  
  Sxy += x * y;  
  Sxz += x * z;  
  Syz += y * z;  
  num += 1.0;
  return((int) num);
}


//= Resolve statistics into plane coefficients and average error.
// allows post-scaling of xyz coordinates by given factors
// returns 1 if successful, 0 if problem

int jhcPlaneEst::Analyze (double xsc, double ysc, double zsc)
{
  if (num < 3.0)
    return 0;
  scale_xyz(xsc, ysc, zsc);
  find_abc();
  find_err();
  return 1;
}


//= Scale dimensions of statistics before doing actual plane fit.
// sometimes this is more efficient than correcting each point

void jhcPlaneEst::scale_xyz (double xsc, double ysc, double zsc)
{
  Sx  *= xsc;
  Sy  *= ysc;
  Sz  *= zsc;
  Sxx *= xsc * xsc;
  Syy *= ysc * ysc;
  Szz *= zsc * zsc;
  Sxy *= xsc * ysc;
  Sxz *= xsc * zsc;
  Syz *= ysc * zsc;
}


//= Determine plane fitting coefficients from statistics.
// does a least-squares plane fit to the z coordinates of surface points
// using statistic sums to fit plane to all points so far
// returns average orthogonal distance of points from final plane
// <pre>
//
// plane equation:
//
//   A * x + B * y + C * z + D = 0
//   z = (-A/C) * x + (-B/C) * y - (D/C)
//
//   z = a * x + b * y + c
//
// where: a = -A/C, b = -B/C, c = -D/C
//
//
// residual error squared:
//
//   n * r^2 = sum[ (z - (ax + by + c))^2 ]
//           = sum[ z^2 - 2z(ax + by + c) + (ax + by + c)^2 ]
//           = sum[ z^2 - 2a(xz) - 2b(yz) - 2c(z) + (a^2x^2 + 2ax(by + c) + (by + c)^2) ] 
//           = sum[ z^2 - 2a(xz) - 2b(yz) - 2c(z) + a^2(x^2) + 2ab(xy) + 2ac(x) + b^2(y^2) + 2bc(y) + c^2 ]
//           = sum[ z^2 + a^2(x^2) + b^2(y^2) + 2[ab(xy) - a(xz) - b(yz) + c(a(x) + b(y) - z))] + c^2 ]   
//
//   n * r^2 = ( Szz + a^2 * Sxx + b^2 * Syy 
//               + 2 * [ab * Sxy - a * Sxz - b * Syz 
//                      + c * (a * Sx + b * Sy - Sz)] ) 
//             + n * c^2 
//
// where: Si = sum[ i ], Sij = sum[ i * j ]
//
//
// residual error squared minimum wrt a, b, c:
//
//   d(n * r^2)/da = 0 = sum[ 2 * (z - (ax + by + c)) * (-x) ] 
//                   0 = -2 * sum[ z * x - (a * x * x + b * x * y + c * x) ]
//     --> Sxz = a * Sxx + b * Sxy + c * Sx       
//
//   d(n * r^2)/db = 0 = sum[ 2 * (z - (ax + by + c)) * (-y) ]
//                   0 = -2 * sum[ z * y - (a * x * y + b * y * y + c * y) ]
//     --> Syz = a * Sxy + b * Syy + c * Sy       
//
//   d(n * r^2)/dc = 0 = sum[ 2 * (z - (ax + by + c)) * -1 ] = 0
//                   0 = -2 * sum[ z - (a * x + b * y + c) ]
//     -->  Sz = a * Sx + b * Sy + n * c       
//
// in matrix form:
//
//   | Sxx  Sxy  Sx | | a |   | Sxz |
//   | Sxy  Syy  Sy | | b | = | Syz |
//   | Sx   Sy   n  | | c |   | Sz  |
//
//
// solving for a, b, c by inverting the matrix of moments:
//
//   | a |    | Sxx  Sxy  Sx |-1 | Sxz |
//   | b | =  | Sxy  Syy  Sy |   | Syz |
//   | c |    | Sx   Sy   n  |   | Sz  |
//
//   | a |     1    |  ( nSyy - SySy )  -( nSxy - SxSy )  ( SySxy - SxSyy ) | | Sxz |
//   | b | =  --- * | -( nSxy - SxSy )   ( nSxx - SxSx ) -( SySxx - SxSxy ) | | Syz |
//   | c |    det   |  (SySxy - SxSyy)  -(SySxx - SxSxy)  (SxxSyy - SxySxy) | | Sz  |
//
//   where det = Sxx * (nSyy - SySy) - Sxy * (nSxy - SxSy) + Sx * (SySxy - SxSyy)
//
// NOTE: direct inversion is about 7x faster than Gauss-Jordan elimination
// </pre>

void jhcPlaneEst::find_abc () 
{
  double m00, m10, m20, m01, m11, m21, m02, m12, m22, idet;

  // compute matrix entries
  m00 = num * Syy - Sy * Sy;
  m10 = Sx * Sy - num * Sxy; 
  m20 = Sy * Sxy - Sx * Syy;
  m01 = m10; 
  m11 = num * Sxx - Sx * Sx; 
  m21 = Sx * Sxy - Sy * Sxx;
  m02 = m20; 
  m12 = m21; 
  m22 = Sxx * Syy - Sxy * Sxy;

  // solve for plane parameters
  idet = 1.0 / (Sxx * m00 - Sxy * (num * Sxy - Sx * Sy) + Sx * m20);
  a = idet * (Sxz * m00 + Syz * m10 + Sz * m20);
  b = idet * (Sxz * m01 + Syz * m11 + Sz * m21);
  c = idet * (Sxz * m02 + Syz * m12 + Sz * m22);
}


//= Determine average orthogonal error based on plane coefficients.
// <pre>
//
//   n * r^2 = ( Szz + a^2 * Sxx + b^2 * Syy 
//               + 2 * [ab * Sxy - a * Sxz - b * Syz 
//                      + c * (a * Sx + b * Sy - Sz)] ) 
//             + n * c^2 
// </pre>

void jhcPlaneEst::find_err ()
{
  double nr2, std;

  // compute sum of squared errors up to constant
  nr2  = a * Sx + b * Sy - Sz;
  nr2 *= c;
  nr2 += a * b * Sxy - a * Sxz - b * Syz;
  nr2 *= 2.0;
  nr2 += Szz + a * a * Sxx + b * b * Syy;

  // divide by number of points, add offset, tip by viewing angle
  std = (nr2 / num) + c * c;
  std /= (a * a + b * b + 1.0);  
  err = sqrt(std);
} 


///////////////////////////////////////////////////////////////////////////
//                         Result Interpretation                         //
///////////////////////////////////////////////////////////////////////////

//= Computes tilt, roll, and height of camera at (cx cy cz) with given pan.
// uses the fact that (a b -1) is the normal vector for the plane
// <pre>
//   z = ax + by + c --> Ax + By + Cz + D = 0 with A = a, B = b, C = -1, D = c
// </pre>
// roll is rotation around xy pan direction, then tilt is rotation wrt xy plane
// height is the orthogonal distance of the camera (x y z) from the plane
// returns 1 if successful, 0 if problem

int jhcPlaneEst::Pose (double& t, double& r, double& h, 
                       double p, double x, double y, double z) const
{
  double rads = D2R * p, cosp = cos(rads), sinp = sin(rads);

  if (num < 3.0)
    return 0;

  t = -R2D * atan( a * cosp + b * sinp);
  r =  R2D * atan(-a * sinp + b * cosp);
  h = (z - (a * x + b * y + c)) / sqrt(a * a + b * b + 1.0);
  return 1;
}