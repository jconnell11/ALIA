// jhcSmTrack.h : tracks objects in 3D with simple smoothing
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2018 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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

#ifndef _JHCSMTRACK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSMTRACK_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <string.h>

#include "Data/jhcArr.h"
#include "Data/jhcParam.h"


//= Tracks objects in 3D with simple smoothing.
// <pre>
// pos array has Limit() object entries:
//   0 = x center
//   1 = y center
//   2 = z center
// </pre>

class jhcSmTrack
{
// PRIVATE MEMBER VARIABLES
private:
  double **pos, **var, **dist;
  int *id, *cnt, *fwd, *back;
  int total, valid, last_id, stats;
  char name[40];


// PUBLIC MEMBER VARIABLES
public:
  // track labels
  char **tag;
  int *state;

  // whether xyz axes are fixed
  int axes;

  // debugging information
  double bin_sz;
  jhcArr move[3];

  // tracking parameters
  jhcParam tps;
  double close[3];
  double frac, dsf, daf, rival, fermi;

  // filtering parameters
  jhcParam fps;
  double noise[3], mix[3];
  int born, gone;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcSmTrack (int n =50);
  ~jhcSmTrack ();
  int SetSize (int n);
  void SetName (const char *txt) {strcpy_s(name, txt);}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  void SetTrack (double dx, double dy, double dz, double f, double sw, double aw, double rv, double lap);
  void SetFilter (double nx, double ny, double nz, double mx, double my, double mz, int b, int g);
  void CopyParams (const jhcSmTrack& ref);

  // track status 
  void Reset (int dbg =0);
  void NoMiss (int i);
  void ForcePos (int i, double x, double y, double z);

  // main functions
  void MatchAll (const double * const *detect, int n, int rem =1, const double * const *shp =NULL);
  void PairUp (int i, const double * const *detect, int j);
  void Prune ();
  double Dist2 (int i, const double *item) const;
  int Update (int i, const double *item);
  void Penalize (int i);
  int Clear (int i);

  // track information
  int Count () const;
  int Limit () const {return valid;}
  int MaxItems () const {return total;}
  int Valid (int i) const;
  int TrackFor (int j) const;
  int DetectFor (int i) const;
  int Coords (double& x, double& y, double& z, int i) const;
  const double *Coords (int i) const;
  double TX (int i) const {return(((i < 0) || (i >= valid)) ? 0.0 : pos[i][0]);}
  double TY (int i) const {return(((i < 0) || (i >= valid)) ? 0.0 : pos[i][1]);}
  double TZ (int i) const {return(((i < 0) || (i >= valid)) ? 0.0 : pos[i][2]);}
  double DistXY (int i) const;
  void ForceXYZ (int i, double x, double y, double z) 
    {if ((i < 0) || (i >= valid)) return; pos[i][0] = x; pos[i][1] = y; pos[i][2] = z;}


// PRIVATE MEMBER FUNCTIONS
private:
  void dealloc ();

  // parameters
  int track_params (const char *fname);
  int filter_params (const char *fname);

  // main functions
  void score_all (const double * const *detect, int n, const double * const *shp);
  double get_d2 (int i, const double *item) const;
  double get_d2s (int i, const double *item, const double *shp) const;
  double get_d2i (int i, const double *item, const double *shp) const;
  void greedy_pair (const double * const *detect, int n, int solid);
  bool disputed (const double *item, const double * const *shp) const;
  int add_track (const double *item);
  int shift_pos (int i, const double *item);
  int mark_hit (int i);
  int mark_miss (int i);
  void rem_item (int i);

};


#endif  // once




