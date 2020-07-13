// jhcFilter.cpp : attempts to removes compression and camera noise
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2003-2013 IBM Corporation
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

#include <math.h>
#include "Interface/jhcMessage.h"

#include "Processing/jhcFilter.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor sets variables to default values.

jhcFilter_0::jhcFilter_0 ()
{
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                              Image Clean Up                           //
///////////////////////////////////////////////////////////////////////////

//= Reset mixing coefficient and noises to default for first frame.

void jhcFilter_0::Reset ()
{
  nv[2] = 64.0;    // noise about 3 bits in red
  nv[1] = 64.0;    // noise about 3 bits in green
  nv[0] = 64.0;    // noise about 3 bits in blue
  f0 = 0.1;        // mixing value
  first = 1;
}


//= Set the sizes of internal arrays.

void jhcFilter_0::SetSize (const jhcImg& ref)
{
  est.SetSize(ref, 3);
  var.SetSize(ref, 3);
  Reset();
}


//= Use a Kalman filter at each pixel to give color smoothing.
// but now assume true process is a random walk in intensity
// <pre>
//   measurement:  M  = P + Vm          where Vm = variance in measurement
//       process:  P' = d * P + c       where c is expected jumpiness 
//                                        and d is a time decay constant 
// </pre>

int jhcFilter_0::Flywheel (const jhcImg& src, int init)  
{
  // check for correct arguments
  if ((init > 0) || !est.Valid())
    SetSize(src);
  if (!src.Valid(3) || !src.SameSize(est))
    return Fatal("Bad images to jhcFilter_0::Flywheel");
  est.MergeRoi(src);
  var.CopyRoi(est);

  // if this is the first image presented then initialize things
  if ((init > 0) || (first > 0))
  {
    est.CopyArr(src);
    var.FillRGB(BOUND(ROUND(nv[0])), BOUND(ROUND(nv[1])), BOUND(ROUND(nv[2])));
    first = 0;
    return 1;
  }

  // general ROI case, f = amount of noise reduction desired
  int diff, vm, k, val; 
  int rn, gn, bn, fi = ROUND(f0 * 256.0), cfi = 256 - fi; 
  int x, y, rw = est.RoiW(), rh = est.RoiH(), sk = est.Skip();
  const UC8 *m = src.RoiSrc(est);
  UC8 *p = est.RoiDest(), *v = var.RoiDest();

  // pre-multiply default noise values  
  bn = ROUND(256.0 * nv[0]);
  gn = ROUND(256.0 * nv[1]);
  rn = ROUND(256.0 * nv[2]);

  // independently examine all pixels in image
  for (y = rh; y > 0; y--, p += sk, v += sk, m += sk)
    for (x = rw; x > 0; x--, p += 3, v += 3, m += 3)
    {
      // BLUE - project from last step and add in new measurement 
      diff = m[0] - p[0];
      vm   = cfi * v[0] + fi * diff * diff;
      k    = (vm << 8) / (vm + bn);
      val  = ((p[0] << 8) + k * diff + 128) >> 8;
      p[0] = BOUND(val);
      val  = ((256 - k) * (vm >> 1) + 16384) >> 15;
      v[0] = BOUND(val);

      // GREEN - project from last step and add in new measurement 
      diff = m[1] - p[1];
      vm   = cfi * v[1] + fi * diff * diff;
      k    = (vm << 8) / (vm + gn);
      val  = ((p[1] << 8) + k * diff + 128) >> 8;
      p[1] = BOUND(val);
      val  = ((256 - k) * (vm >> 1) + 16384) >> 15;
      v[1] = BOUND(val);

      // RED - project from last step and add in new measurement 
      diff = m[2] - p[2];
      vm   = cfi * v[2] + fi * diff * diff;
      k    = (vm << 8) / (vm + rn);
      val  = ((p[2] << 8) + k * diff + 128) >> 8;
      p[2] = BOUND(val);
      val  = ((256 - k) * (vm >> 1) + 16384) >> 15;
      v[2] = BOUND(val);
    }
  return 1;
}


//= Original floating point version (3x slower).

int jhcFilter_0::Flywheel0 (const jhcImg& src)  
{
  // check for correct arguments
  if (!est.Valid())
    SetSize(src);
  if (!src.Valid(3) || !src.SameSize(est))
    return Fatal("Bad images to jhcFilter_0::Flywheel0");
  est.MergeRoi(src);
  var.CopyRoi(est);

  // if this is the first image presented then initialize things
  if (first > 0)
  {
    est.CopyArr(src);
    var.FillRGB(BOUND(ROUND(nv[0])), BOUND(ROUND(nv[1])), BOUND(ROUND(nv[2])));
    first = 0;
    return 1;
  }

  // general ROI case
  int val, diff, x, y, i, rw = est.RoiW(), rh = est.RoiH(), sk = est.Skip();
  const UC8 *m = src.RoiSrc(est);
  UC8 *p = est.RoiDest(), *v = var.RoiDest();
  double pm, vm, k, f = 0.1; 

  // independently examine all pixels in image
  for (y = rh; y > 0; y--, p += sk, v += sk, m += sk)
    for (x = rw; x > 0; x--, p += 3, v += 3, m += 3)
      for (i = 0; i < 3; i++)
      {
        // project from last step
        pm = (double) p[i];
        diff = abs(m[i] - p[i]);
        vm = (1.0 - f) * v[i] + f * diff * diff;

        // add in new measurement
        k    = vm / (vm + nv[i]);
        val  = ROUND(pm + k * (m[i] - pm));
        p[i] = BOUND(val);
        val  = ROUND((1.0 - k) * vm);
        v[i] = BOUND(val);
      }
  return 1;
}




