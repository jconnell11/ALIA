// jhcObjList.cpp : manages a collection of visible objects
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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
#include <string.h>

#include "Interface/jhcMessage.h"

#include "Objects/jhcObjList.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Configuration                       //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcObjList::jhcObjList ()
{
  item = NULL;    // objects currently seen in this image
  prev = NULL;    // objects seen in some previous image
  tell = NULL;    // pointer for walking down item list
  Defaults();
  Rewind();
}


//= Default destructor does necessary cleanup.

jhcObjList::~jhcObjList ()
{
  jhcVisObj *t2, *t;

  // get rid of current list
  t = item;
  while (t != NULL)
  {
    t2 = t->NextObj();
    delete t;
    t = t2;
  }

  // get rid of previous list
  t = prev;
  while (t != NULL)
  {
    t2 = t->NextObj();
    delete t;
    t = t2;
  }
}


//= Get ready for next video sequence.

void jhcObjList::Reset ()
{
  clr_list(prev);
  clr_list(item);
}


//= Read all relevant defaults variable values from a file.

int jhcObjList::Defaults (const char *fname)
{
  int ok = 1;

  ok &= col_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcObjList::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= cps.SaveVals(fname);
  return ok;
}


//= Parameters used for qualitative color naming based on hue.

int jhcObjList::col_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("obj_qcol", 0);
  ps->NextSpec4( clim,         2,   "Red-orange boundary");     // was 13 then 7
  ps->NextSpec4( clim + 1,    26,   "Orange-yellow boundary");
  ps->NextSpec4( clim + 2,    47,   "Yellow-green boundary");   // was 45 
  ps->NextSpec4( clim + 3,   120,   "Green-blue boundary");  
  ps->NextSpec4( clim + 4,   170,   "Blue-purple boundary");  
  ps->NextSpec4( clim + 5,   234,   "Purple-red boundary");  

  ps->NextSpecF( &agree,       0.5, "Bounding box overlap");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Parse the scene into objects after finding foreground regions.
// returns the number of valid objects currently in the list

int jhcObjList::ParseObjs (const jhcBlob& blob, const jhcImg& comps, const jhcImg& src)
{
  jhcVisObj *t, *t0 = NULL;
  int i, n = blob.Active(), cnt = 0;

  // erase old objects and remember input image size
  backup_items();
  clr_list(item);
  iw = src.XDim();
  ih = src.YDim();

  // add new selected components to list (possibly as a new entry)
  t = item;
  for (i = 0; i < n; i++)
    if (blob.GetStatus(i) > 0)
    {
      // make sure there is a unused object entry
      if (t == NULL)
      {
        t = new jhcVisObj;
        if (t0 == NULL)
          item = t;
        else
          t0->AddObj(t);
      }

      // add properties to object record
      t->Ingest(src, comps, blob, i, clim);
      get_grasp(t->gx, t->gy, t->gwid, t->gdir, blob, comps, i);

      // advance to next blob and next object on list
      t0 = t;
      t = t->NextObj();
      cnt++;
    }

  // propagate info from previous object list
  track_names();  
  Rewind();
  return cnt;
}


//= Copy current objects to list of previous objects.

void jhcObjList::backup_items ()
{
  jhcVisObj *t = prev;

  prev = item;          // swap two lists (not all elements used)
  item = t;
}


//= Clear out some list of objects.

void jhcObjList::clr_list (jhcVisObj *head)
{
  jhcVisObj *t = head;

  while (t != NULL)
  {
    t->Clear();
    t = t->NextObj();
  }
}


//= Copy over any names and markings from previous object list.

void jhcObjList::track_names ()
{
  jhcVisObj *win, *i, *p = prev;
  double lap, best;

  while ((p != NULL) && (p->valid >= 0))
  {
    // find best match for old object
    win = NULL;
    i = item;
    while ((i != NULL) && (i->valid >= 0))
    {
      lap = p->OverlapBB(*i);
      if (lap > agree)
        if ((win == NULL) || (lap > best))
        {
          win = i;
          best = lap;
        }
      i = i->NextObj();
    }

    // transfer selection state and semantic name
    if (win != NULL)
    {
      win->valid = p->valid;
      strcpy_s((win->part).name, (p->part).name);
    }
    p = p->NextObj();
  }
}


//= Determine where to grab object including finger opening and approach vector.
// uses point along axis up half the width of the object (expects a cylinder) 

int jhcObjList::get_grasp (double &x, double& y, double& wid, double& ang, 
                           const jhcBlob& blob, const jhcImg& comps, int i)
{
  double xm, ym, len, rads, off;

  if (i < 0)
    return 0;
  blob.ABox(xm, ym, len, wid, comps, i);
  if (blob.BlobAspect(i) < 1.2)                   // was 1.5
    ang = 90.0;
  else
    ang = 180.0 - blob.BlobAngle(i);
  rads = D2R * ang;
  off = 0.5 * (len - wid);
  x = xm - off * cos(rads);
  y = ym - off * sin(rads);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Object Description                          //
///////////////////////////////////////////////////////////////////////////

//= Tells the number of objects currently in list.
// should be the same as HoleCount if objects based on color separation

int jhcObjList::ObjCount () 
{
  jhcVisObj *t = item;
  int n = 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    n++;
    t = t->NextObj();
  }
  return n;
}


//= Set up to start enumerating objects from list.

void jhcObjList::Rewind ()
{
  tell = item;
}


//= Continue enumerating object from list.

jhcVisObj *jhcObjList::Next ()
{
  jhcVisObj *ans = tell;

  if ((tell == NULL) || (tell->valid < 0))
    return NULL;
  tell = tell->NextObj();
  return ans;
}


//= Get a particular object from the current list.

jhcVisObj *jhcObjList::GetObj (int n)
{
  jhcVisObj *ans = item;
  int i;

  if (n < 0)
    return NULL;

  for (i = 0; i < n; i++)
    if ((ans == NULL) || (ans->valid < 0))
      return NULL;
    else
      ans = ans->NextObj();
  return ans;
}


//= Find a named subpart of some object.

jhcVisPart *jhcObjList::ObjPart (int n, const char *sub)
{
  jhcVisObj *t = GetObj(n);

  if (t == NULL)
    return NULL;
  return t->GetPart(sub, 0);
}


//= Return a small binary image of object N inside its bounding box.
// can specify a particular subpart like "top" if desired

const jhcImg *jhcObjList::GetMask (int n, const char *sub) 
{
  const jhcVisPart *p = ObjPart(n, sub);

  if (p == NULL)
    return NULL;
  return &(p->mask);
}


//= Return a small color image of object N inside its bounding box.
// can specify a particular subpart like "top" if desired

const jhcImg *jhcObjList::GetCrop (int n, const char *sub) 
{
  const jhcVisPart *p = ObjPart(n, sub);

  if (p == NULL)
    return NULL;
  return &(p->crop);
}


//= Return the hue histogram for some object.

const jhcArr *jhcObjList::GetHist (int n, const char *sub)
{
  jhcVisPart *p = ObjPart(n, sub);

  if (p == NULL)
    return NULL;
  return &(p->hhist);
}


//= Get quantized colors as a displayable array.

const jhcArr *jhcObjList::GetCols (jhcArr& dest, int n, const char *sub)
{
  jhcVisPart *p = ObjPart(n, sub);
  double sc;
  int i, all = 0;

  // set up default answer
  dest.SetSize(9);
  dest.Fill(0);
  if (p == NULL)
    return &dest;

  // copy and normalize histogram bins
  for (i = 0; i < 9; i++)
    all += (p->cols)[i];
  sc = 100.0 / all;
  for (i = 0; i < 9; i++)
    dest.ASet(i, ROUND(sc * (p->cols)[i]));
  return &dest;
}


//= Get string with names of dominant colors for some object.

const char *jhcObjList::MainColor (int cnum, int n, const char *sub) 
{
  jhcVisPart *p = ObjPart(n, sub);

  if (p == NULL)
    return NULL;
  return p->Color(cnum);
}


//= Get string with names of non-dominant colors for some object.

const char *jhcObjList::SubColor (int cnum, int n, const char *sub) 
{
  jhcVisPart *p = ObjPart(n, sub);

  if (p == NULL)
    return NULL;
  return p->AltColor(cnum);
}


//= Tell image location of bottom, apparent width, and approach direction.

int jhcObjList::GraspPoint (double &x, double& y, double& wid, double& ang, int n)
{
  jhcVisObj *t = GetObj(n);

  if (t == NULL)
    return 0;

  x = t->gx;
  y = t->gy;
  wid = t->gwid;
  ang = t->gdir;
  return 1;
}


//= Finds given part of object and returns a recognition signature.

int jhcObjList::ModelVec (jhcArr& mod, int n, const char *sub)
{
  return ModelVec(mod, GetObj(n), sub);
}


//= Return a visual model of the part as a single vector.
// this is: sqrt(pels), gwid, 100*ecc, cols[0] ... cols[8] (as pct)

int jhcObjList::ModelVec (jhcArr& mod, jhcVisObj *t, const char *sub)
{
  jhcVisPart *p;
  double sc;
  int i, sum = 0;

  // set up default answer
  mod.SetSize(12);
  mod.Fill(0);
  if (t == NULL)
    return 0;
  p = t->GetPart(sub);
  if (p == NULL)
    return 0;

  // set size parameters
  mod.ASet(0, ROUND(sqrt((double) p->area)));
  mod.ASet(1, ROUND(t->gwid));
  mod.ASet(2, ROUND(100.0 * t->asp));

  // set color parameters
  for (i = 0; i < 9; i++)
    sum += p->cols[i];
  sc = 100.0 / sum;
  for (i = 0; i < 9; i++)
    mod.ASet(i + 3, ROUND(sc * p->cols[i]));
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                             Object Status                             //
///////////////////////////////////////////////////////////////////////////

//= How many objects are marked with the highest status.

int jhcObjList::NumChoices ()
{
  jhcVisObj *t = item;
  int n = 0, top = TopMark();

  if (top <= 0)
    return 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid == top)
      n++;
    t = t->NextObj();
  }
  return n;
}


//= Retrieve the highest object marking.

int jhcObjList::TopMark ()
{
  jhcVisObj *t = item;
  int mark = 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    mark = __max(mark, t->valid);
    t = t->NextObj();
  }
  return mark;
}


//= Retrieve the number for the best target.

int jhcObjList::TargetID ()
{
  jhcVisObj *t = item;
  int i = 0, top = TopMark();

  if (top <= 0)
    return -1;
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid == top)
      return i;
    i++;
    t = t->NextObj();
  }
  return -1;
}


//= Retrieve a pointer to the semantic name (if any) for the best target.
// Note: this string can be read AND written to

char *jhcObjList::TargetName ()
{
  jhcVisObj *t = TargetObj();

  if (t == NULL)
    return NULL;
  return t->BulkName();
}


//= Retrieve a pointer to the best target (if any).

jhcVisObj *jhcObjList::TargetObj ()
{
  jhcVisObj *t = item;
  int top = TopMark();

  if (top <= 0)
    return NULL;
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid == top)
      return t;
    t = t->NextObj();
  }
  return NULL;
}


//= Demotes highest marked objects and promotes secondary objects.

int jhcObjList::SwapTop ()
{
  jhcVisObj *t = item;
  int top = TopMark(), sec = top - 1, n = 0;

  if (top <= 0)
    return 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid == top)
      t->valid = sec;
    else if (t->valid == sec)
    {
      t->valid = top;
      n++;
    }
    t = t->NextObj();
  }
  return n;
}


//= Marks all objects as unsuitable.

void jhcObjList::ClearObjs ()
{
  jhcVisObj *t = item;

  while ((t != NULL) && (t->valid >= 0))
  {
    t->valid = 0;
    t = t->NextObj();
  }
}


//= Make all visible objects potential targets again.
// returns total number of possible targets

int jhcObjList::RestoreObjs ()
{
  jhcVisObj *t = item;
  int n = 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    t->valid = 1; 
    n++;
    t = t->NextObj();
  }
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                            Object Rejection                           //
///////////////////////////////////////////////////////////////////////////

//= Find and mark just the nearest object to a mouse click point on image.
// leaves selection intact if nothing found near pointing (mouse) position
// returns 1 if something found, 0 if nothing reasonably close

int jhcObjList::MarkNearest (int x, int y) 
{
  jhcRoi box;
  jhcVisPart *p;
  jhcVisObj *t = item, *win = NULL;
  double dx, dy, dist, best, grow = 1.0;  // was 0.5
  int top = 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid >= 0)
    {
      // keep track of highest marking
      top = __max(top, t->valid);

      // check if in (expanded) bounding box
      p = &(t->part);
      box.SetRoi(p->rx, p->ry, p->rw, p->rh);
      box.ResizeRoi(1.0 + grow);
      if (box.RoiContains(x, y) > 0)
      {
        // keep closest centroid if conflict
        dx = p->cx - x;
        dy = p->cy - y;
        dist = (double) sqrt(dx * dx + dy * dy);
        if ((win == NULL) || (dist < best))
        {
          win = t;
          best = dist;
        }
      }
    }
    t = t->NextObj();
  }

  // mark only the winner
  if (win == NULL)
    return 0;
  win->valid = top + 1;
  return 1;
}


//= Keeps only those things that are most like the given color.
// can also look at secondary colors if alt > 0

int jhcObjList::KeepColor (const char *want, int alt)
{
  if (HasOnlyColor(want, 0) > 0)
    return HasOnlyColor(want);
  if ((alt <= 0) || (HasMainColor(want, 0) > 0))
    return HasMainColor(want);
  return HasColor(want);
}


//= Disregard all things that are not solely the specified color.
// returns number of choices left

int jhcObjList::HasOnlyColor (const char *want, int rem)
{
  jhcVisObj *t = item;
  const char *col, *col2;
  int n = 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
    {
      // must have only one valid main color 
      col  = (t->part).Color(0);
      col2 = (t->part).Color(1);
      if ((col != NULL) && (col2 == NULL) && (_stricmp(want, col) == 0))
      {
        t->valid = 1;
        n++;
      }
      else if (rem > 0)
        t->valid = 0;
    }
    t = t->NextObj();
  }
  return n;
}


//= Disregard all things that are not partially the specified color.
// returns number of choices left

int jhcObjList::HasMainColor (const char *want, int rem)
{
  jhcVisObj *t = item;
  const char *col;
  int i, n = 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
      for (i = 0; i < 9; i++)
      {
        // check all main colors for some match
        col = (t->part).Color(i);
        if (col == NULL)
        {
          if (rem > 0)
            t->valid = 0;
          break;
        }
        if (_stricmp(want, col) == 0)
        {
          t->valid = 1;
          n++;
          break;
        }
      }
    t = t->NextObj();
  }
  return n;
}


//= Disregard all things that are not at all the specified color.
// returns number of choices left

int jhcObjList::HasColor (const char *want, int rem)
{
  jhcVisObj *t = item;
  const char *col;
  int i, n = 0;

  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
    {
      for (i = 0; i < 9; i++)
      {
        // check all main colors for some match
        col = (t->part).Color(i);
        if ((col == NULL) || (_stricmp(want, col) == 0))
          break;
      }

      if (col != NULL)
      {
        t->valid = 1;
        n++;
      }
      else
        for (i = 0; i < 9; i++)
        {
          // check all secondary colors for some match
          col = (t->part).AltColor(i);
          if (col == NULL)
          {
            if (rem > 0)
              t->valid = 0;
            break;
          } 
          if (_stricmp(want, col) == 0)
          {
            t->valid = 1;
            n++;
            break;
          }
        }
    }
    t = t->NextObj();
  }
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                            Object Preference                          //
///////////////////////////////////////////////////////////////////////////

//= Randomly pick one of the highest marked objects.

int jhcObjList::PickOne (int inc)
{
  jhcVisObj *t = item;
  int pick = 0, i = 0, n = 0;

  // choose among top marked objects
  pick = (int)(NumChoices() * rand() / (double) RAND_MAX);

  // figure out the object ID number for this choice
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
      if (n++ == pick)
      {
        if (inc > 0)
          t->valid += 1;
        return i;
      }
    i++;
    t = t->NextObj();
  }
  return -1;
}


//= Pick object with largest percentage of the given color.

int jhcObjList::MostColor (const char *want, int inc)
{
  jhcVisObj *t = item;
  char cname[9][20] = {"red", "orange", "yellow", "green", "blue", "purple", 
                       "black", "gray", "white"};
  double frac, best; 
  int n, i = 0, win = -1, top = TopMark(), sec = top - 1;

  if (top <= 0)
    return -1;

  // convert color name into bin number
  for (n = 0; n < 9; n++)
    if (_stricmp(want, cname[n]) == 0)
      break;
  if (n >= 9)
    return -1;

  // look for biggest proportion of object color in that bin
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
    {
      t->valid = 1;
      frac = (t->part).cols[n] / (double)((t->part).area2);
      if ((win < 0) || (frac > best))
      {
        best = frac;
        win = i;
      }
    }
    i++;
    t = t->NextObj();
  }

  // possibly boost the salience
  if ((win >= 0) && (inc > 0))
  {
    t = GetObj(win);
    t->valid += 1;
  }
  return win;
}


//= Pick the currently valid object with the biggest area.

int jhcObjList::Biggest (int inc)
{
  jhcVisObj *t = item;
  int a1, i = 0, win = -1;

  // find object with most pixels
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
    {
      t->valid = 1;
      if ((win < 0) || ((t->part).area > a1))
      {
        a1 = (t->part).area;
        win = i;
      }
    }
    i++;
    t = t->NextObj();
  }

  // possibly boost the salience
  if ((win >= 0) && (inc > 0))
  {
    t = GetObj(win);
    t->valid += 1;
  }
  return win;
}


//= Pick the currently valid object with the smallest area.

int jhcObjList::Littlest (int inc)
{
  jhcVisObj *t = item;
  int a0, i = 0, win = -1;

  // find object with least pixels
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
    {
      t->valid = 1;
      if ((win < 0) || ((t->part).area < a0))
      {
        a0 = (t->part).area;
        win = i;
      }
    }
    i++;
    t = t->NextObj();
  }

  // possibly boost the salience
  if ((win >= 0) && (inc > 0))
  {
    t = GetObj(win);
    t->valid += 1;
  }
  return win;
}


//= Pick the currently valid object with the most average size.

int jhcObjList::MediumSized (int inc)
{
  jhcVisObj *t;
  int mid, a0 = -1, a1 = -1, best = -1;
  int i = 0, win = -1;

  // find limits of whole set of objects
  t = item;
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
    {
      t->valid = 1;
      if ((a0 < 0) || ((t->part).area < a0))
        a0 = (t->part).area;
      if ((a1 < 0) || ((t->part).area > a1))
        a1 = (t->part).area;
    }
    t = t->NextObj();
  }
  mid = (a0 + a1) / 2;

  // find highly marked object nearest to middle of range
  t = item;
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
      if ((win < 0) || (abs((t->part).area - mid) < best))
      {
        best = abs((t->part).area - mid);
        win = i;
      }
    i++;
    t = t->NextObj();
  }

  // possibly boost the salience
  if ((win >= 0) && (inc > 0))
  {
    t = GetObj(win);
    t->valid += 1;
  }
  return win;
}


//= Pick the currently valid object closest to the left of the image.

int jhcObjList::Leftmost (int inc)
{
  jhcVisObj *t = item;
  double x0;
  int i = 0, win = -1;

  // find object with lowest x centroid
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
    {
      t->valid = 1;
      if ((win < 0) || ((t->part).cx < x0))
      {
        x0 = (t->part).cx;
        win = i;
      }
    }
    i++;
    t = t->NextObj();
  }

  // possibly boost the salience
  if ((win >= 0) && (inc > 0))
  {
    t = GetObj(win);
    t->valid += 1;
  }
  return win;
}


//= Pick the currently valid object closest to the right of the image.

int jhcObjList::Rightmost (int inc)
{
  jhcVisObj *t = item;
  double x1;
  int i = 0, win = -1;

  // find object with highest x centroid
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
    {
      t->valid = 1;
      if ((win < 0) || ((t->part).cx > x1))
      {
        x1 = (t->part).cx;
        win = i;
      }
    }
    i++;
    t = t->NextObj();
  }

  // possibly boost the salience
  if ((win >= 0) && (inc > 0))
  {
    t = GetObj(win);
    t->valid += 1;
  }
  return win;
}


//= Pick the currently valid object closest to the middle of the whole set.

int jhcObjList::InMiddle (int inc)
{
  jhcVisObj *t;
  double mx, x0 = -1.0, x1 = -1.0, best = -1.0;
  int i = 0, win = -1;

  // find limits of whole set of objects
  t = item;
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid >= 0)
    {
      t->valid = 1;
      if ((x0 < 0) || ((t->part).cx < x0))
        x0 = (t->part).cx;
      if ((x1 < 0) || ((t->part).cx > x1))
        x1 = (t->part).cx;
    }
    t = t->NextObj();
  }
  mx = 0.5 * (x0 + x1);

  // find highly marked object nearest to center
  t = item;
  while ((t != NULL) && (t->valid >= 0))
  {
    if (t->valid > 0)
      if ((win < 0) || (fabs((t->part).cx - mx) < best))
      {
        best = fabs((t->part).cx - mx);
        win = i;
      }
    i++;
    t = t->NextObj();
  }

  // possibly boost the salience
  if ((win >= 0) && (inc > 0))
  {
    t = GetObj(win);
    t->valid += 1;
  }
  return win;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Show selected grasp point as bar across object plus tail along approach.

int jhcObjList::DrawGrasp (jhcImg& dest, int n, int tail, int t, int r, int g, int b)
{
  double f = dest.YDim() / (double) ih;
  double xg, yg, wid, ang, c, s, wc, ws;

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcObjList::DrawGrasp");

  // find object axis, width, and grasp location
  if (GraspPoint(xg, yg, wid, ang, n) <= 0)
    return 0;
  xg *= f;
  yg *= f;
  wid *= f;

  // compute useful factors
  ang *= D2R;
  c = cos(ang);
  s = sin(ang);

  // draw width across grasp point
  wid *= 0.5;
  wc = wid * c;
  ws = wid * s;
  DrawLine(dest, xg - ws, yg + wc, xg + ws, yg - wc, t, r, g, b);

  // draw approach vector leading to grasp point
  return DrawLine(dest, xg, yg, xg - tail * c, yg - tail * s, t, r, g, b);
}


//= Use quantized colors to redraw object in small icon-sized image.

int jhcObjList::ColorObj (jhcImg& dest, int n)
{
  UC8 cols[6][3] = {{255, 0, 0}, {255, 128, 0}, {255, 255, 0}, {0, 255, 0}, {0, 128, 255}, {128, 0, 128}};
  jhcVisPart *p = ObjPart(n);

  if (p == NULL)
    return Fatal("Bad input to jhcObjList::ColorObj");
  dest.SetSize(p->crop);
  dest.FillRGB(0, 0, 255);

  // local variables
  int i, x, y, rw = dest.XDim(), rh = dest.YDim(), csk = dest.Skip(), sk = (p->hmsk).Skip();
  const UC8 *s = (p->shrink).PxlSrc(), *m = (p->hmsk).PxlSrc();
  const UC8 *h = (p->hue).PxlSrc(), *w = (p->wht).PxlSrc(), *b = (p->blk).PxlSrc();
  UC8 *d = dest.PxlDest();

  for (y = rh; y > 0; y--, d += csk, s += sk, m += sk, h += sk, w += sk, b += sk)
    for (x = rw; x > 0; x--, d += 3, s++, m++, h++, w++, b++)
      if (*s <= 0)
        continue;
      else if (*m > 0)
      {
        i = color_bin(*h);
        d[0] = cols[i][2];
        d[1] = cols[i][1];
        d[2] = cols[i][0];
      }
      else if (*w > 0)
      {
        d[0] = 255;
        d[1] = 255;
        d[2] = 255;
      }
      else if (*b > 0)
      {
        d[0] = 0;
        d[1] = 0;
        d[2] = 0;
      }
      else 
      {
        d[0] = 128;
        d[1] = 128;
        d[2] = 128;
      }
  return 1;
}


//= Divide hue range into 6 distinct colors (red, orange, yellow, green, blue, purple).

int jhcObjList::color_bin (int hue)
{
  int i;

  for (i = 0; i < 6; i++)
    if (hue <= clim[i])
      return i;
  return 0;                                   // red 
}


