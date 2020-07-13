// jhcScan3D.cpp : finds and tracks people using a single scanning sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2015 IBM Corporation
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

#include "Interface/jhcMessage.h"

#include "People/jhcScan3D.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcScan3D::jhcScan3D ()
{
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcScan3D::~jhcScan3D ()
{
}


//= Reset state for the beginning of a sequence.

//void jhcScan3D::Reset ()
//{
//}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcScan3D::Defaults (const char *fname)
{
  int ok = 1;

  ok &= empty_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcScan3D::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= eps.SaveVals(fname);
  return ok;
}


//= Parameters used for something.

int jhcScan3D::empty_params (const char *fname)
{
  jhcParam *ps = &eps;
  int ok;

  ps->SetTag("empty_vals", 0);
//  ps->NextSpec4( &n, 10, "Empty value");  

  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
//                            Debuging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Show current head locations on color or depth input image.

int jhcScan3D::ShowHeads (jhcImg& dest, int raw, double sz)
{
  jhcRoi box;
  double wx, wy, wz;
  int i, n, id, col;

  if (!dest.Valid(1, 3) || (dest.XDim() != sf.XDim()) || (dest.YDim() != sf.YDim()))
    return Fatal("Bad images to jhcScan3D::ShowHeads");

  // possibly show raw detections (no numbers)
  if (raw > 0)
  {
    n = NumRaw();
    for (i = 0; i < n; i++)
    {
      RawHeadBeam(wx, wy, wz, i);
      sf.ImgCylinder(box, wx, wy, wz, sz, sz);
      RectEmpty(dest, box, 1, 255, 0, 255); 
      if (raw > 1)
        LabelBox(dest, box, i, 16, 255, 0, 255);
    }
    return 1;
  }

  // show tracked heads instead (always show numbers)
  n = NumTracked();
  for (i = 0; i < n; i++)
    if ((id = (dude[i]).TrackID()) > 0)
    {
      TrackedHeadBeam(wx, wy, wz, i);
      sf.ImgCylinder(box, wx, wy, wz, sz, sz);
      col = (id % 6) + 1;
      RectEmpty(dest, box, 3, -col);
      LabelBox(dest, box, id, 16, -col);
    }
  return 1;
}
