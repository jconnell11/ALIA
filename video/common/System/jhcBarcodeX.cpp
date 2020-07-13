// jhcBarcodeX.cpp : wrapper around core barcode reading class
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2007 IBM Corporation
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

#include "jhcBarcodeX.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcBarcodeX::jhcBarcodeX ()
{
  Defaults();
}


//= Set sizes of internal images based on a reference image.

void jhcBarcodeX::SizeFor (jhcImg& ref)
{
  SetSize(ref.XDim(), ref.YDim(), ref.Fields());
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

void jhcBarcodeX::Defaults (const char *fname)
{
  slice_params(fname);
  edge_params(fname);
  bar_params(fname);
}


//= Write current processing variable values to a file.

void jhcBarcodeX::SaveVals (const char *fname)
{
  sps.SaveVals(fname);
  eps.SaveVals(fname);
  bps.SaveVals(fname);
}


//= Parameters used for getting intensity slice.

void jhcBarcodeX::slice_params (const char *fname)
{
  jhcParam *p = &sps;

  p->SetTag("upc_slice", 0);
  p->NextSpec4( &steps,   9, "Number of offset slices");
  p->NextSpec4( &off,    40, "Spacing of slices (pels)");
  p->NextSpec4( &cols,    3, "Colors to try (G, R, B)");
  p->NextSpec4( &dirs,    4, "Directions (H, V, D1, D2)");
  p->Skip(2);

  p->NextSpec4( &mode,    3, "Pattern to use for demo");
  p->LoadDefs(fname);
  p->RevertAll();
}


//= Parameters used for finding edges of bars in slice.

void jhcBarcodeX::edge_params (const char *fname)
{
  jhcParam *p = &eps;

  p->SetTag("upc_edge", 0);
  p->NextSpec4( &sm,      1, "Amount to smooth scan");        // was 4, 2, 1
  p->NextSpec4( &dmax,   20, "Barcode border uniformity");
  p->NextSpec4( &wmin,   15, "Barcode border min width");     // was 20, 30
  p->NextSpec4( &bmin,  200, "Overall barcode min width");
  p->NextSpec4( &dbar,    5, "Minimum slope for edge");       // was 5, 10, 20, 50
  p->NextSpec4( &bdiff,  30, "Minimum change for edge");      // was 20, 30, 50, 100

  p->LoadDefs(fname);
  p->RevertAll();
}


//= Parameters used for interpreting edges as barcode.

void jhcBarcodeX::bar_params (const char *fname)
{
  jhcParam *p = &bps;

  p->SetTag("upc_bars", 0);
  p->NextSpec4( &badj,    1, "Automatic border adjust");
  p->NextSpec4( &eadj,    1, "Automatic edge adjust");
  p->NextSpec4( &est,     2, "Bit width estimation mode");
  p->NextSpec4( &pod,     2, "Bar pair decoding mode");
  p->NextSpec4( &cvt,     0, "Convert 6 digit to 10 digit");

  p->LoadDefs(fname);
  p->RevertAll();
}




