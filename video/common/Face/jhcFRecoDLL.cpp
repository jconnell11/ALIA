// jhcFRecoDLL.cpp : holds gallery of faces and matches probe image to them
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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
#include <stdarg.h>
#include <string.h>

#include "Interface/jhcMessage.h"

#include "Face/jhcFRecoDLL.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcFRecoDLL::~jhcFRecoDLL ()
{
  freco_done();
  freco_cleanup();
  ClrDB();
  delete probe;
}


//= Default constructor initializes certain values.

jhcFRecoDLL::jhcFRecoDLL ()
{
  // member variables
  strcpy_s(dir, "faces");
  db = NULL;
  vsz = 256;

  // initialize DLL
  Defaults();
  freco_start();

  // record proper sizes for things
  vsz = freco_vsize();
  probe = new jhcFaceVect(vsz);
  (probe->thumb).SetSize(freco_mug_w(), freco_mug_h(), 3);

  // status
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for matching and database maintenance.

int jhcFRecoDLL::match_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("freco_dll", 0);
  ps->NextSpecF( &mth,       0.2, "Okay match distance");  
  ps->NextSpec4( &sure,      2,   "Top matches agree");
  ps->Skip();
  ps->NextSpec4( &vcnt,     12,   "Max examples (0 = no rem)");
  ps->NextSpec4( &boost,    10,   "Boost utility of top");
  ps->NextSpec4( &ucap,  50000,   "Maximum utility");          // about 3 min
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcFRecoDLL::Defaults (const char *fname, int data)
{
  int ok = 1;

  if ((fname != NULL) && (data > 0))
    if (LoadDB() < 0)
      ok = 0;
  ok &= match_params(fname);
  ok &= freco_setup(fname);
  return ok;
}


//= Write current processing variable values to a file.
// can optionally save current people names and vectors

int jhcFRecoDLL::SaveVals (const char *fname, int data)
{
  int ok = 1;

  if ((data > 0) && (db != NULL))
    if (SaveDB() < 0)
      ok = 0;
  ok &= mps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Set all status variables for start of run.

void jhcFRecoDLL::Reset ()
{
  win = NULL;
  best = NULL;
  ranked = 0;
  busy = 0;
}


//= Add the face in the bounding box as a new instance of some person.
// if "rem" <= 0 then disallows removal of old instance to preserve limit
// returns new vector if successful, NULL for problem

const jhcFaceVect *jhcFRecoDLL::Enroll (const char *name, const jhcImg& src, const jhcRoi& box, int rem)
{
  jhcFaceVect *v;
  jhcFaceOwner *dude = get_person(name);
  const UC8 *s = src.PxlSrc();
  int lf, rt, bot, top, iw = src.XDim(), ih = src.YDim();

  // get thumbnail for this face presentation
  v = new jhcFaceVect(vsz);
  (v->thumb).SetSize(freco_mug_w(), freco_mug_h(), 3);
  box.RoiLims(lf, bot, rt, top);
  freco_mug((v->thumb).PxlDest(), s, iw, ih, lf, rt, bot, top);

  // get signature vector (blocks) and add to person's list
//freco_vect_mug(v->data, (v->thumb).PxlSrc());
  freco_vect(v->data, s, iw, ih, lf, rt, bot, top);
  dude->AddVect(v, ((rem <= 0) ? 0 : vcnt));
  return v;
}


//= Copy and add some already formed recognition vector to a particular person.
// if "rem" <= 0 then disallows removal of old instance to preserve limit
// useful in conjunction with LastResult() function to allow backgrounding
// returns new vector if successful, NULL for problem

const jhcFaceVect *jhcFRecoDLL::Enroll (const char *name, const jhcFaceVect *ref, int rem)
{
  jhcFaceOwner *dude = get_person(name);
  jhcFaceVect *v = new jhcFaceVect(vsz);

  v->Copy(ref);
  dude->AddVect(v, ((rem <= 0) ? 0 : vcnt));
  return v;
}


//= Possibly add the last recognition result to instances for person if needed.
// returns new vector if added, else NULL if not needed or inappropriate

const jhcFaceVect *jhcFRecoDLL::TouchUp (const char *name)
{
  if (verdict != 1)
    return NULL;
  return Enroll(name, probe);
}


//= Get exisitng entry or create new one for person based on name.

jhcFaceOwner *jhcFRecoDLL::get_person (const char *name)
{
  jhcFaceOwner *last = NULL, *dude = NULL;

  // find proper person 
  while ((dude = NextDude(dude)) != NULL)
  {
    if (strcmp(dude->Who(), name) == 0)
      return dude;
    last = dude;
  }

  // make new person and add to end of list 
  dude = new jhcFaceOwner(name, vsz);
  if (last == NULL)
    db = dude;
  else
    last->next = dude;
  return dude;
}


//= Compare face in image to whole database and find best match.
// use Name() and Distance() with choice rank for additional details 
// returns 2 for sure, 1 for okay, 0 for poor or no match

int jhcFRecoDLL::Recognize (const jhcImg& src, const jhcRoi& box)
{
  const UC8 *s = src.PxlSrc();
  int lf, rt, bot, top, iw = src.XDim(), ih = src.YDim();

  box.RoiLims(lf, bot, rt, top);
  freco_mug((probe->thumb).PxlDest(), s, iw, ih, lf, rt, bot, top);
//freco_vect_mug(probe->data, (probe->thumb).PxlSrc());
  freco_vect(probe->data, s, iw, ih, lf, rt, bot, top);
  score_all(probe->data);
  return chk_sure();
}


//= Start recognition on some face with results harvested later by Check.
// always saves sample mugshot in member variable "probe"
// returns 1 if started, 0 or negative if problem

int jhcFRecoDLL::Submit (const jhcImg& src, const jhcRoi& box)
{
  const UC8 *s = src.PxlSrc();
  int lf, rt, bot, top, iw = src.XDim(), ih = src.YDim();

  if (busy > 0)
    return 0;
  box.RoiLims(lf, bot, rt, top);
  freco_mug((probe->thumb).PxlDest(), s, iw, ih, lf, rt, bot, top);
//freco_submit_mug((prob->thumb).PxlSrc());
  freco_submit(s, iw, ih, lf, rt, bot, top);
  busy = 1;
  return 1;
}


//= Check if recognition and scoring is done.
// always saves finished vector in member variable "probe"
// returns: 2 = sure, 1 = okay, 0 = poor or no match, -1 = nothing pending, -2 = still busy

int jhcFRecoDLL::Check ()
{
  if (busy <= 0)
    return -1;
  if (freco_check(probe->data, 0) <= 0)
    return -2;
  busy = 0;
  score_all(probe->data);
  return chk_sure();
}

  
//= Score every vector for every person.
// marks highest and saves pointer to person and vector

void jhcFRecoDLL::score_all (const double *query) 
{
  jhcFaceOwner *dude = NULL;
  jhcFaceVect *v;
  double dmin = -1.0;

  // set up default results (e.g. no database)
  win = NULL;
  best = NULL;
  ranked = 0;

  // scan through all people
  while ((dude = NextDude(dude)) != NULL)
  {
    // scan through all vectors
    v = NULL;
    while ((v = dude->NextVect(v)) != NULL)
    {
      // save score but assume not best
      v->dist = freco_dist(query, v->data);
      v->rank = 0;

      // see if best so far
      if ((dmin < 0.0) || (v->dist < dmin))
      {
        win = dude;
        best = v;
        dmin = v->dist;
      }
    } 
  }

  // go back and mark best choice
  if (best != NULL)
  {
    best->rank = 1;
    ranked = 1;
  }
}


//= Find however many other matches needed to test for sureness.
// confident if top N matches are all the same person, answer saved in "verdict"
// returns 2 for sure, 1 for okay, 0 for poor or no match

int jhcFRecoDLL::chk_sure ()
{
  jhcFaceVect *v = NULL;
  const char *n2;
  double d2;
  int i, u;

  // make sure at least one instance matches well
  verdict = 0;
  if ((best == NULL) || (best->dist > mth))
    return verdict;

  // check to see if successive instances belong to same person
  for (i = 1; i < sure; i++)
  {
    if ((n2 = Match(d2, i)) == NULL)         // assigns rank
      break;
    if ((d2 > mth) || (strcmp(n2, win->Who()) != 0))
      break;
  }

  // reward person instances actually involved in match decision
  while ((v = win->NextVect(v)) != NULL)
  {
    u = v->util;
    if ((v->rank > 0) && (v->rank <= sure))  // assigned above
      u += boost;
    else
      u--;                                   // penalize if not used
    v->util = __max(0, __min(u, ucap));
  }
      
  // report if sureness criteria met
  verdict = ((i >= sure) ? 2 : 1);
  return verdict;
}


///////////////////////////////////////////////////////////////////////////
//                            Result Browsing                            //
///////////////////////////////////////////////////////////////////////////

//= Get the name associated with the Nth match (0 = best).
// returns NULL if nothing matched 

const char *jhcFRecoDLL::Name (int i)
{
  const jhcFaceVect *v;
  const char *ans = NULL;

  v = mark_rank(&ans, i);
  return ans;
}


//= Get the distance associated with the Nth match (0 = best).
// returns negative if nothing matched 

double jhcFRecoDLL::Distance (int i)
{
  const jhcFaceVect *v;
  const char *ans;

  v = mark_rank(&ans, i);
  return((v != NULL) ? v->dist : -1.0);
}


//= Get the image index associated with the Nth match (0 = best)
// returns 0 if nothing matched

int jhcFRecoDLL::ImgNum (int i)
{
  const jhcFaceVect *v;
  const char *ans;

  v = mark_rank(&ans, i);
  return((v != NULL) ? v->inum : 0);
}


//= Get the name and distance associated with the Nth match (0 = best).
// returns NULL if nothing matched (dist is negative)

const char *jhcFRecoDLL::Match (double& d, int i)
{
  const jhcFaceVect *v;
  const char *ans = NULL;

  v = mark_rank(&ans, i);
  d = ((v != NULL) ? v->dist : -1.0);
  return ans;
}


//= Get the small image associated with the Nth match (0 = best).
// returns NULL if nothing matched

const jhcImg *jhcFRecoDLL::Mugshot (int i)
{
  const jhcFaceVect *v;
  const char *ans;

  v = mark_rank(&ans, i);
  return((v != NULL) ? &(v->thumb) : NULL);
}


//= Lookup or search for Nth best match and return vector with name.
// returns NULL if nothing found

jhcFaceVect *jhcFRecoDLL::mark_rank (const char **name, int i)
{
  jhcFaceOwner *dude;
  jhcFaceVect *top, *v;
  double low;

  // see if ranking already done
  if (ranked <= 0)
    return NULL;
  if (i < ranked)
    return find_rank(name, i);

  // rank some more choices
  while (ranked <= i)
  {
    // initialize search for this rank level
    top = NULL;
    *name = NULL;
    low = -1.0;

    // scan through vectors for next best match
    dude = NULL;
    while ((dude = NextDude(dude)) != NULL)
    {
      v = NULL;
      while ((v = dude->NextVect(v)) != NULL)
        if (v->rank <= 0)
          if ((low < 0.0) || (v->dist < low))
          {
            // remember best unranked one
            top = v;
            low = v->dist;
            *name = dude->Who();
          }
    }

    // go back and mark this instance (for later use)
    if (top == NULL)
      break;
    top->rank = ++ranked;    // mark with 1 higher than search
  }
  return top;                // name should already have been set
}


//= Lookup vector and name associated with the Nth match (0 = best).
// assumes already tested for i < ranked
// returns NULL if nothing found 

jhcFaceVect *jhcFRecoDLL::find_rank (const char **name, int i) const
{
  jhcFaceOwner *dude = NULL;
  jhcFaceVect *v;
  int ri = i + 1;            // marked as 1 higher than search

  // check simplest case of topmost match
  if (i <= 0)
  {
    *name = ((win != NULL) ? win->Who() : NULL);
    return best;
  }

  // seach for desired rank
  while ((dude = NextDude(dude)) != NULL)
  {
    v = NULL;
    while ((v = dude->NextVect(v)) != NULL)
      if (v->rank == ri)
      {
        *name = dude->Who();
        return v;
      }
  }
  return NULL;               // should never get here
}


///////////////////////////////////////////////////////////////////////////
//                           Database Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Tells the number of people currently in the database.
// can optionally restrict this to only those with at least one vector

int jhcFRecoDLL::NumPeople (int some) const
{
  const jhcFaceOwner *dude = NULL;
  int cnt = 0;

  while ((dude = NextDude(dude)) != NULL)
    if ((some <= 0) || (dude->NumVec() > 0))
      cnt++;
  return cnt;
}


//= Tells the total number of face signatures in the database.

int jhcFRecoDLL::TotalVects () const
{
  const jhcFaceOwner *dude = NULL;
  int cnt = 0;

  while ((dude = NextDude(dude)) != NULL)
    cnt += dude->NumVec();
  return cnt;
}


//= Get rid of all people currently in the database.
// always returns 0 for convenience

void jhcFRecoDLL::ClrDB ()
{
  jhcFaceOwner *rest, *dude = db;

  if (db == NULL)
    return;
  while (dude != NULL)
  {
    rest = dude->next;
    delete dude;
    dude = rest;
  }
  db = NULL;
}


//= Set subdirectory to be used for reading person data.

void jhcFRecoDLL::SetDir (const char *fmt, ...)
{
  va_list args;

  if (fmt == NULL)
    return;
  va_start(args, fmt); 
  vsprintf_s(dir, fmt, args);
}


//= Get recognition data for all people listed in file.
// erases database first unless "append" > 0
// names are one per line, no underscores needed
// lines starting with "//" are ignored
// should call DirDB() first to set vector file location
// returns number of people added, negative for error 

int jhcFRecoDLL::LoadDB (const char *fname, int append)
{
  FILE *in;
  jhcFaceOwner *dude = NULL, *last = NULL;
  jhcFaceVect *v;
  const char *fn = fname;
  char line[80], iname[200], def[80] = "people.txt";
  int n, cnt = 0;

  // possibly erase old data then try opening names file
  if (append <= 0)
    ClrDB();
  if (fname == NULL)
    fn = def;
  plist.ParseName(fn);
  if (fopen_s(&in, fn, "r") != 0)
    return -1;
  
  // find end of current list
  while ((dude = NextDude(dude)) != NULL)
    last = dude;

  // read each line for a valid name
  while (fgets(line, 80, in) != NULL)
    if (strncmp(line, "//", 2) != 0)
      if ((n = (int) strlen(line)) > 1)
      {
        // try to load data for person (initializes vectors)
        if (line[n - 1] == '\n')
          line[n - 1] = '\0';
        dude = new jhcFaceOwner(line, vsz);
        if (dude->Load(dir) < 0)
        {
          delete dude;
          continue;
        }

        // try to read all thumbnails for person
        v = NULL;
        while ((v = dude->NextVect(v)) != NULL)
          if (v->inum > 0)
          {
            // make up name and adjust ibig for person
            sprintf_s(iname, "%s/%s %03d.bmp", dir, dude->Who(), v->inum);
            LoadResize(v->thumb, iname);
            dude->ibig = __max(dude->ibig, v->inum);
          }
          
        // add new person to end of list
        if (last == NULL)
          db = dude;
        else
          last->next = dude;
        last = dude;
        cnt++;
      }

  // clean up
  fclose(in);
  return cnt;
}


//= Save a list of the names of all people as well as their vectors.
// if NULL is given for file, tries to uses same name as loaded
// top level list is suitable for ingestion by LoadDB()
// should call DirDB() first to set vector file location
// returns number of people with data, negative for error

int jhcFRecoDLL::SaveDB (const char *fname) 
{
  FILE *out;
  jhcFaceOwner *dude = NULL;
  jhcFaceVect *v;
  const char *fn = fname;
  char iname[200], def[80] = "people.txt";
  int cnt = 0;

  // try opening names file
  if (fname == NULL)
    fn = plist.File();
  if (*fn == '\0')
    fn = def;
  if (fopen_s(&out, fn, "w") != 0)
    return -1;

  // write out data for all people
  while ((dude = NextDude(dude)) != NULL)
  {
    // write names (even if no data)
    fprintf(out, "%s\n", dude->Who());

    // save all newly added thumbnails for person (inum added already)
    v = NULL;
    while ((v = dude->NextVect(v)) != NULL)
      if ((v->thumb).Valid() > 0)
      {
        sprintf_s(iname, "%s/%s %03d.bmp", dir, dude->Who(), v->inum);
        Save(iname, v->thumb);
      }
          
    // save recognition vectors (needs updated ibig and inum)
    if (dude->Save(dir) > 0)
      cnt++;
  }

  // cleanup 
  fclose(out);
  return cnt;
}


//= Save information for just the named person.
// returns number of mugshots written

int jhcFRecoDLL::SaveDude (const char *name)
{
  char iname[200];
  jhcFaceOwner *dude = NULL;
  jhcFaceVect *v = NULL;
  int cnt = 0;

  // see if there is anyone with that name
  while ((dude = NextDude(dude)) != NULL)
    if (strcmp(name, dude->Who()) == 0)
      break;
  if (dude == NULL)
    return 0;
  
  // save all newly added thumbnails for person (inum added already)
  while ((v = dude->NextVect(v)) != NULL)
    if ((v->thumb).Valid() > 0)
    {
      sprintf_s(iname, "%s/%s %03d.bmp", dir, dude->Who(), v->inum);
      Save(iname, v->thumb);
      cnt++;
    }
          
  // save recognition vectors (needs updated ibig and inum)
  dude->Save(dir);
  return cnt;
}
