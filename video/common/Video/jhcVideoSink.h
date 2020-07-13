// jhcVideoSink.h : video output stream base class
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2004-2012 IBM Corporation
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

#ifndef _JHCVIDEOSINK_
#define _JHCVIDEOSINK_

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Data/jhcName.h"


//= Video output stream base class.
// virtual class describing what messages a video source must handle
// it does not make sense to create instances of this class directly

class jhcVideoSink : public jhcName
{
protected:
  int nextframe;
  int bound, ok, w, h, d; 
  double freq;

public:
  jhcVideoSink ();

  int Valid () {return ok;}                /** Whether file opened properly.  */
  int XDim () {return w;}                  /** Width of frames being saved.   */
  int YDim () {return h;}                  /** Height of frames being saved.  */
  int Fields () {return d;}                /** Depth of frames being saved.   */
  double Rate () {return((double) freq);}  /** Frame rate of output video.    */
  int Where () {return nextframe;}         /** Frame number for next save.    */
 
public:
  int SetSize (int x, int y, int f =3);
  int SetSize (const jhcImg& ref);
  int SetSpeed (double fps);
  int SetSink (const char *fname);
  int SetSpecs (jhcImg& ref, const char *fname, double fps =0.0);

  int Close ();
  int Open ();   
  int Open (const char *fname);
  int Put (const jhcImg& src) ; 
  int Put (UC8 *pixels);  

protected:
  virtual void iClose () =0;                /** Real shutdown implemented by subtype.  */
  virtual int iOpen () =0;                  /** File creation implemented by subtype.  */
  virtual int iPut (const jhcImg& src) =0;  /** Data recording implemented by subtype. */

};


#endif




