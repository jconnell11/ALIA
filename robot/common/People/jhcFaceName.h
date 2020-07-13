// jhcFaceName.h : assigns names to all people tracks based on face recognition
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#ifndef _JHCFACENAME_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFACENAME_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"               // common video
#include "Data/jhcParam.h"
#include "Face/jhcFRecoDLL.h"

#include "People/jhcHeadGaze.h"


//= Assigns names to all people tracks based on face recognition.
// looks at only one person per cycle, generally round robin
// continues after ID established to enhance database of mugshots
// uses jhcHeadGaze to look for faces associated with all tracks
// <pre>
// class tree:
//
//   jhcFaceName
//     jhcHeadGaze
//       jhcFrontal
//         +jhcFFindOCV          member variable
//       >jhcStare3D             bound pointer
//     +jhcFRecoDLL              member variable
//
// </pre>

class jhcFaceName : public jhcHeadGaze
{
// PRIVATE MEMBER VARIABLES
private:
  char name[pmax][80], pend[pmax][80];
  int pcnt[pmax], ucnt[pmax];
  double midx;
  int focus, tweak, spot;


// PUBLIC MEMBER VARIABLES
public:
  // face recognizer and database
  jhcFRecoDLL fr;

  // reco parameters
  jhcParam nps;
  int idth, fix;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcFaceName ();
  jhcFaceName ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL, int local =0);
  int SaveVals (const char *fname, int local =0);

  // main functions
  void Reset (int local =0);
  void SetCam (const jhcMatrix& pos, const jhcMatrix& dir);
  int Analyze (const jhcImg& col, const jhcImg& d16);
  int FindNames (int trk =1);
  int JustNamed () const {return spot;}
  int JustUpdated () const {return tweak;}
  const char *FaceName (int i) const 
    {return(((i >= 0) && (i < pmax)) ? name[i] : NULL);}


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int name_params (const char *fname);

  // main functions
  int query_track (int trk);
  int update_name (int trk);


};


#endif  // once




