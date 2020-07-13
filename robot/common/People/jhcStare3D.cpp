// jhcStare3D.cpp : finds and tracks people using a multiple fixed sensors
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2020 IBM Corporation
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

#include "People/jhcStare3D.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcStare3D::jhcStare3D ()
{
  w2b.SetSize(4, 4);
  strcpy_s(name, "s3d");
  sm_bg = 7;                // background thread parameters
  pmin_bg = 10;
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcStare3D::~jhcStare3D ()
{
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcStare3D::Defaults (const char *fname)
{
  int ok = 1;

  ok &= jhcOverhead3D::Defaults(fname);
  ok &= jhcTrack3D::Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcStare3D::SaveVals (const char *fname, int geom) const
{
  int ok = 1;

  ok &= jhcOverhead3D::SaveVals(fname, geom);
  ok &= jhcTrack3D::SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcStare3D::Reset (double ftime)
{
  // configure base classes
  jhcOverhead3D::Reset();
  jhcTrack3D::Reset(ftime);

  // make jhcParse3D consistent with jhcOverhead3D
  jhcParse3D::SetScale(ztab + zlo, ztab + zhi, IPP());  
  jhcParse3D::SetView(0.0, MX0(), MY0()); 
  jhcParse3D::MapSize(map);
}


//= Find and track all people and hands.
// assumes all depth images have already been loaded with Ingest
// and that all necessary blanking and editing has been done on map
// returns index limit for collection of heads (not number tracked)

int jhcStare3D::Analyze (int sm, int pmin)
{
/*
  double tcut;
  int by = ROUND(W2Y(-0.5 * blank));
  
  // adjust for changing table height if needed
  if (ztab > 0.0)
  {
    tcut = ztab + poff;
    alev = __max(alev0, tcut); 
    ch   = __max(ch0, tcut);
  }

  // clean up overhead depth then assess candidates
  if (blank > 0.0)
    RectFill(map, 0, by, XDim(), YDim() - by, 0);  
*/
  Interpolate(sm, pmin);
  return TrackPeople(map2);  
}


//= How many raw or tracked heads found.

int jhcStare3D::CntValid (int trk) const
{
  if (trk > 0)
    return CntTracked();   // valid count, not potential number
  return NumRaw();
}


//= Iteration limit on person jhcBodyData array.

int jhcStare3D::PersonLim (int trk) const
{
  return((trk > 0) ? NumPotential() : NumRaw());
}


//= See if a particular person index is valid.

bool jhcStare3D::PersonOK (int i, int trk) const
{
  if (trk <= 0) 
    return((i >= 0) && (i < NumRaw()) && (raw[i].id > 0));
  return((i >= 0) && (i < NumPotential()) && (dude[i].id > 0));
}


//= Get the numeric label associated with a particular index number.

int jhcStare3D::PersonID (int i, int trk) const
{
  if (i < 0)
    return -1;
  if (trk <= 0) 
    return((i >= NumRaw()) ? -1 : raw[i].id);
  return((i >= NumPotential()) ? -1 : dude[i].id);
}


//= See if there is a text tag associated with a particular index number.

bool jhcStare3D::Named (int i, int trk) const
{
  if (i < 0)
    return false;
  if (trk <= 0) 
    return((i < NumRaw()) && ((raw[i]).tag[0] != '\0'));
  return((i < NumPotential()) && ((dude[i]).tag[0] != '\0'));
}


//= Retrieve data for a person with a particular index number.

const jhcBodyData *jhcStare3D::GetPerson (int i, int trk) const
{
  if ((i < 0) || (i >= PersonLim(trk)))
    return NULL;
  return((trk > 0) ? dude + i : raw + i);
} 


//= Retrieve modifiable record for a person with a particular index number.

jhcBodyData *jhcStare3D::RefPerson (int i, int trk)
{
  if ((i < 0) || (i >= PersonLim(trk)))
    return NULL;
  return((trk > 0) ? dude + i : raw + i);
} 


//= Retrieve data for a person with a particular ID.

const jhcBodyData *jhcStare3D::GetID (int id, int trk) const
{
  const jhcBodyData *item = ((trk > 0) ? dude : raw);
  int i, n = PersonLim(trk);

  if (id < 0)
    return NULL;
  for (i = 0; i < n; i++)
    if (item[i].id == id)
      return &(item[i]);
  return NULL;
}


//= Retrieve modifiable record for a person with a particular ID.

jhcBodyData *jhcStare3D::RefID (int id, int trk)
{
  jhcBodyData *item = ((trk > 0) ? dude : raw);
  int i, n = PersonLim(trk);

  if (id < 0)
    return NULL;
  for (i = 0; i < n; i++)
    if (item[i].id == id)
      return &(item[i]);
  return NULL;
}


//= Starting with a person ID find the equivalent tracking index.

int jhcStare3D::TrackIndex (int id, int trk) const
{
  jhcBodyData *item = ((trk > 0) ? dude : raw);
  int i, n = PersonLim(trk);

  if (id < 0)
    return NULL;
  for (i = 0; i < n; i++)
    if (item[i].id == id)
      return i;
  return -1;
}


//= Find the ID for the person containing some real-world point.
// marks one or the other hand as "busy" (assumes it is touching some object)
// returns 0 if no one is there

int jhcStare3D::PersonTouch (double wx, double wy, int trk) 
{
  jhcMatrix pos(4), ref(4);
  jhcBodyData *item = ((trk > 0) ? dude : raw);
  double d0 = -1.0, d1 = -1.0;
  int i, tag, n = PersonLim(trk);

  // find person blob that overlaps given point
  if ((tag = BlobAt(wx, wy)) <= 0)
    return 0;
  for (i = 0; i < n; i++)
    if ((item[i].bnum == tag) || (item[i].alt == tag))
      break;
  if (i >= n)
    return 0;

  // detemine distance of each hand to reference
  ref.SetVec3(wx, wy, 0.0);
  if (item->HandPos(pos, 0) > 0)
    d0 = ref.PosDiff3(pos);
  if (item->HandPos(pos, 1) > 0)
    d1 = ref.PosDiff3(pos);

  // mark one or the other hand (or perhaps neither)
  if ((d0 >= 0.0) && ((d1 < 0.0) || (d1 >= d0)))
    item->busy[0] = 1;    
  else if ((d1 >= 0.0) && ((d0 < 0.0) || (d0 > d1)))
    item->busy[1] = 1;    
  return item->id;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Find person closest in 3D to camera origin in projection space.
// returns tracker index not person ID

int jhcStare3D::Closest (int trk) const
{
  jhcMatrix pos(4);
  double d2, best;
  int i, n = PersonLim(trk), win = -1;

  for (i = 0; i < n; i++)
    if (PersonOK(i, trk))
    {
      Head(pos, i, trk);
      d2 = pos.Len2Vec3();
      if ((win < 0) || (d2 < best))
      {
        win = i;
        best = d2;
      }
    }
  return win;
}


//= Gives position of center of person's head.
// returns 1 if okay, 0 if invalid

int jhcStare3D::Head (jhcMatrix& full, int i, int trk) const
{
  const jhcBodyData *guy = GetPerson(i, trk);

  if (guy == NULL)
    return 0;
  full.Copy(*guy);
  return 1;
}


//= Returns estimated person height (in inches).
// returns negative if invalid

double jhcStare3D::Height (int i, int trk) const
{
  const jhcBodyData *guy = GetPerson(i, trk);

  if (guy == NULL)
    return -1.0;
  return(guy->Z() + edn);
}


//= Gives position of particular hand of some person.
// returns 1 if okay, 0 if invalid

int jhcStare3D::Hand (jhcMatrix& full, int i, int rt, int trk) const
{
  const jhcBodyData *guy = GetPerson(i, trk);

  if (guy == NULL)
    return 0;
  return guy->HandPos(full, rt);
}


//= Gives height (in inches) over surface for particular hand of some person.
// returns -1.0 if invalid, else zero or positive value

double jhcStare3D::HandOver (int i, int rt, int trk) const
{
  const jhcBodyData *guy = GetPerson(i, trk);
  int side = __max(0, __min(rt, 1));

  if (guy == NULL)
    return -1.0;
  return guy->sep[side];
}


//= Gives table intersection point of ray from some hand of some person.
// returns 1 if okay, 0 if invalid

int jhcStare3D::Target (jhcMatrix& full, int i, int rt, int trk, double zlev) const
{
  const jhcBodyData *guy = GetPerson(i, trk);

  if (guy == NULL)
    return 0;
  return guy->RayHit(full, rt, zlev);
}


//= Gives wall with fixed Y intersection point of ray from some hand of some person.
// returns 1 if okay, 0 if invalid

int jhcStare3D::TargetY (jhcMatrix& full, int i, int rt, int trk, double yoff) const
{
  const jhcBodyData *guy = GetPerson(i, trk);

  if (guy == NULL)
    return 0;
  return guy->RayHitY(full, rt, yoff);
}


//= Gives wall with fixed X intersection point of ray from some hand of some person.
// returns 1 if okay, 0 if invalid

int jhcStare3D::TargetX (jhcMatrix& full, int i, int rt, int trk, double xoff) const
{
  const jhcBodyData *guy = GetPerson(i, trk);

  if (guy == NULL)
    return 0;
  return guy->RayHitX(full, rt, xoff);
}


//= Gives bounding box around head of some person in some camera (not reversed).
// returns 1 if okay, 0 if invalid

int jhcStare3D::HeadBoxCam (jhcRoi& box, int i, int cam, int trk, double sc)
{
  jhcMatrix rel(4);
  const jhcBodyData *guy = GetPerson(i, trk);
  double sz = 8.0;

  if (guy == NULL)
    return 0;
  AdjGeometry(cam);
  BeamCoords(rel, *guy);
  ImgCylinder(box, rel, sz, sz, sc);
  return 1;
}


//= Find the display tag associated with a particular ID.
// returns NULL if bad ID given

const char *jhcStare3D::GetName (int id, int trk) const
{
  const jhcBodyData *guy = GetID(id, trk);

  if (guy == NULL) 
    return NULL;
  return guy->tag;
}


//= Set the display tag for a person with a particular ID.
// will accept a name of "" to clear previous name
// tries to keep two tracks from having the same name
// returns 1 if changed, 0 if bad id, negative for problem

int jhcStare3D::SetName (int id, const char *name, int trk)
{
  jhcBodyData *p, *guy = RefID(id, trk);
  int i, n = PersonLim(trk);

  // check for errors (like already having a name)
  if (name == NULL)
    return -1;
  if (guy == NULL) 
    return 0;

  // check if some other track already has same name
  for (i = 0; i < n; i++)
  {
    p = RefPerson(i, trk);
    if ((p->id > 0) && (_stricmp(p->tag, name) == 0))
      p->tag[0] = '\0';
  }

  // set the name as requested
  strcpy_s(guy->tag, name);
  return 1;
}


//= Convenience function to get the semantic node associated with some ID.

const void *jhcStare3D::GetNode (int id, int trk) const
{
  const jhcBodyData *guy = GetID(id, trk);

  if (guy == NULL)
    return NULL;
  return guy->node;
}


//= Convenience function to set the semantic node associated with some ID.

int jhcStare3D::SetNode (void *n, int id, int trk)
{
  jhcBodyData *guy = RefID(id, trk);

  if (guy == NULL)
    return 0;
  guy->node = n;
  return 1;
}


//= Find tracking ID number for person whose node matches the one given.

int jhcStare3D::NodeID (const void *node, int trk) const
{
  const jhcBodyData *p;
  int i, n = PersonLim(trk);

  if (node != NULL)
    for (i = 0; i < n; i++)
      if ((p = GetPerson(i, trk)) != NULL)
        if (p->node == node)
          return p->id;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= In overhead view, draw a box of some color around a head having a particular ID.

int jhcStare3D::ShowID (jhcImg& dest, int id, int trk,  
                        int invert, int col, double sz, int style) 
{
  jhcRoi box;
  jhcMatrix pos(4);
  const jhcBodyData *p = GetID(id, trk);
  int hsz = ROUND(fabs(sz) / ParseScale());

  if (!dest.SameFormat(ParseWid(), ParseHt(), 1))
    return Fatal("Bad input to jhcParse3D::ShowID");
  if (p == NULL)
    return 0;

  pos.MatVec(w2m, *p);
  box.CenterRoi(ROUND(pos.X()), ROUND(pos.Y()), hsz, hsz);
  if (invert > 0)
    box.InvertRoi(dest.XDim(), dest.YDim());
  RectEmpty(dest, box, 3, -col);
  LabelBox(dest, box, label(*p, style), -16, -col);
  return 1;
}


//= In frontal view, draw a box of some color around a head having a particular ID.
// negative color skips building projection matrices again
// sz control head box size

int jhcStare3D::ShowIDCam (jhcImg& dest, int id, int cam, int trk, int rev, 
                           int col, double sz, int style) 
{
  jhcRoi box;
  jhcMatrix rel(4);
  const jhcBodyData *p = GetID(id, trk);
  double sz0 = fabs(sz), sc = dest.YDim() / (double) InputH();
  int w = dest.XDim(), c = abs(col);

  // make sure requested person is valid
  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcStare3D::ShowIDCam");
  if (p == NULL)
    return 0;

  // find image footprint of cylinder around head point
  if (sz >= 0.0)
    AdjGeometry(cam);
  BeamCoords(rel, *p);
  ImgCylinder(box, rel, sz0, sz0, sc);
  if (rev > 0)
    box.MirrorRoi(w);

  // draw colored box and ID number
  RectEmpty(dest, box, 3, -c); 
  LabelBox(dest, box, label(*p, style), -16, -c);
  return 1;
}


//= Show current head locations and numbers on color or depth input image.
// needs to know which sensor input to plot relative to
// can optionally show raw or tracked version 
// sz control head box size, negative chooses color based on ID

int jhcStare3D::HeadsCam (jhcImg& dest, int cam, int trk, int rev, double sz, int style) 
{
  int i, n = PersonLim(trk);

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcStare3D::HeadsCam");

  AdjGeometry(cam);
  for (i = 0; i < n; i++)
    PersonCam(dest, i, cam, trk, rev, -5, sz, style);
  return 1;
}


//= Show the head box for a particular person on color input image.
// negative color skips building projection matrices again
// sz control head box size, negative chooses color based on ID

int jhcStare3D::PersonCam (jhcImg& dest, int i, int cam, int trk, int rev, int col, double sz, int style)
{
  jhcRoi box;
  jhcMatrix rel(4);
  jhcBodyData *guy = RefPerson(i, trk); 
  double sz0 = fabs(sz), sc = dest.YDim() / (double) InputH();
  int id, w = dest.XDim(), c = abs(col);

  // make sure requested person is valid
  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcStare3D::PersonCam");
  if ((guy == NULL) || ((id = guy->id) <= 0))
    return 0;

  // find image footprint of cylinder around head point
  if (col >= 0)
    AdjGeometry(cam);
  BeamCoords(rel, *guy);
  ImgCylinder(box, rel, sz0, sz0, sc);
  if (rev > 0)
    box.MirrorRoi(w);

  // draw colored box and ID number 
  if (sz < 0.0)
    c = (id % 6) + 1;
  RectEmpty(dest, box, 3, -c); 
  LabelBox(dest, box, label(*guy, style), -16, -c);
  return 1;
}


//= Show current valid hand locations on color or depth input image.
// needs to know which sensor input to plot relative to
// can optionally show raw or tracked version 
// typically want to call this after ShowHands (smaller crosses)

int jhcStare3D::HandsCam (jhcImg& dest, int cam, int trk, int rev, double sz)
{
  jhcRoi box;
  jhcMatrix rel(4), hand(4);
  jhcBodyData *items = ((trk > 0) ? dude : raw);
  double ix, iy, sc = dest.YDim() / (double) InputH();
  int i, xlim = dest.XLim(), n = PersonLim(trk);

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcStare3D::HeadsCam");

  // set up projection from a particular camera
  AdjGeometry(cam);

  for (i = 0; i < n; i++)
  {
    // find image location of left fingertip and draw as X
    if (items[i].HandPos(hand, 0) > 0)
    {
      BeamCoords(rel, hand);
      ImgPt(ix, iy, rel, sc);
      if (rev > 0)
        ix = xlim - ix;
      XMark(dest, ix, iy, 25, 3, -4); 
    }

    // find image location of right fingertip and draw as +
    if (items[i].HandPos(hand, 1) > 0)
    {
      BeamCoords(rel, hand);
      ImgPt(ix, iy, rel, sc);
      if (rev > 0)
        ix = xlim - ix;
      Cross(dest, ix, iy, 33, 33, 3, -4); 
    }
  }
  return 1;
}


//= Show current point ray Z intersection locations on color or depth input image.
// needs to know which sensor input to plot relative to
// can optionally show raw or tracked version 

int jhcStare3D::RaysCam (jhcImg& dest, int cam, int trk, int rev, double zlev)
{
  jhcRoi box;
  jhcMatrix full(4), rel(4);
  jhcBodyData *items = ((trk > 0) ? dude : raw);
  double hx, hy, ex, ey, sc = dest.YDim() / (double) InputH();
  int i, side, xlim = dest.XLim(), n = PersonLim(trk);

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcStare3D::RaysCam");

  // set up projection from a particular camera
  AdjGeometry(cam);
  for (i = 0; i < n; i++)
    for (side = 0; side <= 1; side++)
      if (items[i].HandOK(side))
      {
        // find image location of hand 
        items[i].RayBack(full, side, flen);
        BeamCoords(rel, full);
        ImgPt(hx, hy, rel, sc);

        // find image location where ray intersects surface
        items[i].RayHit(full, side, zlev);
        BeamCoords(rel, full);
        ImgPt(ex, ey, rel, sc);

        // compensate for left-right reversal
        if (rev > 0)
        {
          hx = xlim - hx;
          ex = xlim - ex;
        }

        // connect with a line
        DrawLine(dest, hx, hy, ex, ey, 3, -3);
      }
  return 1;
}


//= Show current point ray Y intersection locations on color or depth input image.
// needs to know which sensor input to plot relative to
// can optionally show raw or tracked version 

int jhcStare3D::RaysCamY (jhcImg& dest, int cam, int trk, int rev, double yoff)
{
  jhcRoi box;
  jhcMatrix full(4), rel(4);
  jhcBodyData *items = ((trk > 0) ? dude : raw);
  double hx, hy, ex, ey, sc = dest.YDim() / (double) InputH();
  int i, side, xlim = dest.XLim(), n = PersonLim(trk);

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcStare3D::RaysCamY");

  // set up projection from a particular camera
  AdjGeometry(cam);
  for (i = 0; i < n; i++)
    for (side = 0; side <= 1; side++)
      if (items[i].HandOK(side))
      {
        // find image location of hand 
        items[i].RayBack(full, side, flen);
        BeamCoords(rel, full);
        ImgPt(hx, hy, rel, sc);

        // find image location where ray intersects surface
        items[i].RayHitY(full, side, yoff);
        BeamCoords(rel, full);
        ImgPt(ex, ey, rel, sc);

        // compensate for left-right reversal
        if (rev > 0)
        {
          hx = xlim - hx;
          ex = xlim - ex;
        }

        // connect with a line
        DrawLine(dest, hx, hy, ex, ey, 3, -3);
      }
  return 1;
}


//= Show current point ray X intersection locations on color or depth input image.
// needs to know which sensor input to plot relative to
// can optionally show raw or tracked version 

int jhcStare3D::RaysCamX (jhcImg& dest, int cam, int trk, int rev, double xoff)
{
  jhcRoi box;
  jhcMatrix full(4), rel(4);
  jhcBodyData *items = ((trk > 0) ? dude : raw);
  double hx, hy, ex, ey, sc = dest.YDim() / (double) InputH();
  int i, side, xlim = dest.XLim(), n = PersonLim(trk);

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcStare3D::RaysCamX");

  // set up projection from a particular camera
  AdjGeometry(cam);
  for (i = 0; i < n; i++)
    for (side = 0; side <= 1; side++)
      if (items[i].HandOK(side))
      {
        // find image location of hand 
        items[i].RayBack(full, side, flen);
        BeamCoords(rel, full);
        ImgPt(hx, hy, rel, sc);

        // find image location where ray intersects surface
        items[i].RayHitX(full, side, xoff);
        BeamCoords(rel, full);
        ImgPt(ex, ey, rel, sc);

        // compensate for left-right reversal
        if (rev > 0)
        {
          hx = xlim - hx;
          ex = xlim - ex;
        }

        // connect with a line
        DrawLine(dest, hx, hy, ex, ey, 3, -3);
      }
  return 1;
}

