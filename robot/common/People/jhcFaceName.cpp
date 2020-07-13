// jhcFaceName.cpp : assigns names to all people tracks based on face recognition
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

#include "Interface/jhcMessage.h"

#include "jhcFaceName.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcFaceName::~jhcFaceName ()
{
}


//= Default constructor initializes certain values.

jhcFaceName::jhcFaceName ()
{
  midx = 320.0;
  Defaults();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for selecting and updating faces of people.

int jhcFaceName::name_params (const char *fname)
{
  jhcParam *ps = &nps;
  int ok;

  ps->SetTag("face_name", 0);
  ps->NextSpec4( &idth,  3, "ID repeat for name copy");
  ps->NextSpec4( &fix,  10, "ID unsure for update (0 = never)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.
// loads depth-based person finder/track also if local <= 0

int jhcFaceName::Defaults (const char *fname, int local)
{
  int ok = 1;

  ok &= name_params(fname);
  ok &= fr.Defaults(fname);
  ok &= jhcHeadGaze::Defaults(fname);
  if ((local <= 0) && (s3 != NULL))
    ok &= s3->Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.
// saves depth-based person finder/track also if local <= 0

int jhcFaceName::SaveVals (const char *fname, int local)
{
  int ok = 1;

  ok &= nps.SaveVals(fname);
  ok &= fr.SaveVals(fname);
  ok &= jhcHeadGaze::SaveVals(fname);
  if ((local <= 0) && (s3 != NULL))
    ok &= s3->SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// will reset depth-based person finder/track if local <= 0

void jhcFaceName::Reset (int local)
{
  int i;

  // reset person name data and counts
  for (i = 0; i < pmax; i++)
  {
    name[i][0] = '\0';
    pend[i][0] = '\0';
    pcnt[i] = 0;
    ucnt[i] = 0;
  }

  // reset subject of interest and last new identification
  focus = -1;
  spot = -1;                 

  // reset base class and face reco DNN
  jhcHeadGaze::Reset();
  fr.Reset();

  // possibly reset person finder/tracker
  if ((local <= 0) && (s3 != NULL))
    s3->Reset();
}


//= Change the position and orientation (p, t, r) of the single sensor.

void jhcFaceName::SetCam (const jhcMatrix& pos, const jhcMatrix& dir)
{
  SetAttn(pos);
  if (s3 != NULL)
    s3->SetCam(0, pos, dir);
}


//= Do all visual processing necessary for a single sensor system.
// assumes camera 0 is up to date, or SetCam has been called
// also assumes attention point is up to date or set with 

int jhcFaceName::Analyze (const jhcImg& col, const jhcImg& d16)
{
  // sanity check
  if (s3 == NULL)
    return Fatal("Unbound person detector in jhcFaceName::Analyze");

  // find people from depth image
  s3->rasa = 1;
  s3->Ingest(d16);
  s3->Analyze();

  // look for faces associated with heads
  ScanRGB(col, d16);
  DoneRGB();

  // identify people from faces
  midx = 0.5 * col.XDim();
  FindNames();
  return 1;
}


//= Keep trying to identify any faces found and update the database if needed.
// harvests results of last cycle (if any) then picks new face to recognize
// updates info in jhcBodyData structure associated with relevant track
// more useful than Analyze in situations where multiple sensors are being fused
// assumes "fr" (jhcFRecoDLL) has had its database loaded already
// NOTE: must first run s3->Ingest and s3->Analyze then ScanRGB and DoneRGB

int jhcFaceName::FindNames (int trk)
{
  int i, n = s3->PersonLim(trk);

  // clear local member data on any new head tracks 
  n = __min(n, pmax);
  for (i = 0; i < n; i++)
   if (!s3->PersonOK(i, trk))
    {
      pend[i][0] = '\0';        
      pcnt[i] = 0;      
      ucnt[i] = 0;   
    }

  // if last recognition done then pick a new face to try
  if (update_name(trk) > 0)
    if ((focus = query_track(trk)) >= 0)
      fr.Submit(GetCrop(focus), GetFace(focus));
  return 1;
}


//= Try to recognize face associated with some track to get name.
// person under investigation was stored in variable "focus"
// returns 1 if all action finished, 0 if waiting on recognition still

int jhcFaceName::update_name (int trk)
{
  jhcBodyData *p;
  const char *who;
  int reco;

  // nothing newly named or updates
  spot = -1;
  tweak = -1;

  // see if recognizer done with last face submitted (if any)
  if (focus < 0)
    return 1;
  if ((reco = fr.Check()) <= -2)       // still processing
    return 0;
  if (reco <= 0)                       // no name to work with
    return 1;

  // update consecutive count for same name
  who = fr.Name();
  if (strcmp(who, pend[focus]) == 0)
    pcnt[focus] += 1;
  else
    pcnt[focus] = 1;

  // possibly promote pending name to confident name (if none yet)
  strcpy_s(pend[focus], who);
  if ((pcnt[focus] >= idth) && (name[focus][0] == '\0'))
  {
    spot = focus;
    strcpy_s(name[focus], who);
    p = s3->RefPerson(focus, trk);     // copy to track if no tag yet
    if ((p->tag)[0] == '\0')
      strcpy_s(p->tag, who);                       
  }

  // update number of consecutive "unsure" identifications
  if ((pcnt[focus] >= idth) && (reco == 1))
    ucnt[focus] += 1;
  else
    ucnt[focus] = 0;                   // clamp to zero if ID changing

  // possibly add most recent example to list of vectors
  // ID must be reliable and long list of unsure IDs
  if (pcnt[focus] >= idth) 
    if ((fix > 0) && (ucnt[focus] >= fix))
    {
      tweak = focus;
      fr.TouchUp(name[focus]);
      ucnt[focus] = 0;                 // wait before adding again
    }
  return 1;
}


//= Choose some face among current detections to try to identify next.
// concentrates on closest un-named person since reco can be slow (93 ms on CPU = 3 frames)
// "pend" is most recent proposed name, "pcnt" is how many times in a row this has been chosen
// "ucnt" is number of consecutive likely but non-confident recognitions of this person
// returns tracker index, -1 if nothing

int jhcFaceName::query_track (int trk) 
{
  int i, n = s3->PersonLim(trk), best = 0, win = -1;

  // look for biggest un-named frontal head
  n = __min(n, pmax);
  for (i = 0; i < n; i++)
    if (s3->PersonOK(i, trk) && !s3->Named(i, trk) && Frontal(i))
      if (GetSize(i) > best)
      {
        win = i;
        best = GetSize(i);
      }

  // otherwise look for frontal head with lowest pcnt
  if (win < 0)
    for (i = 0; i < n; i++)
      if (s3->PersonOK(i, trk) && Frontal(i))
        if ((win < 0) || (pcnt[i] < best))
        {
          win = i;
          best = pcnt[i];
        }
  return win;
}
