// jhcSpeaker.cpp : determines which person is speaking using mic arrays
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#include "People/jhcSpeaker.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSpeaker::~jhcSpeaker ()
{
}


//= Default constructor initializes certain values.

jhcSpeaker::jhcSpeaker ()
{
  int i;

  // no person tracker bound or foreign mic
  s3 = NULL;
  m0 = NULL;

  // assign logical unit numbers to local mics
  for (i = 0; i < amax; i++)
    mic[i].unit = i;
    
  // set up processing parameters and state
  Defaults();
  Reset();
}


//= Get serial port instance used to control attention light.
// returns NULL if LED not available via local microphones

jhcSerial *jhcSpeaker::AttnLED () 
{
  int i;

  for (i = 0; i < amax; i++)
    if ((mic[i].mport > 0) && (mic[i].light > 0))
      return &(mic[i].mcom);
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.
// do not call if using external microphone

int jhcSpeaker::Defaults (const char *fname)
{
  int ok = 1;

  ok &= LoadCfg(fname);
  if (m0 != NULL)
    return 1;
  ok &= mic[0].Defaults(fname, 0);
  MicDup();
  return ok;
}


//= Read just deployment specific values from a file.
// do not call if using external microphone

int jhcSpeaker::LoadCfg (const char *fname)
{
  int i, ok = 1;

  // clear microphone port numbers (= validity flags)
  for (i = 0; i < amax; i++)
    mic[i].mport = 0;
  if (m0 != NULL)
    return 1;

  // get new values from file
  for (i = 0; i < amax; i++)
    ok &= mic[i].LoadCfg(fname);
  return ok;
}


//= Write current processing variable values to a file.
// do not call if using external microphone

int jhcSpeaker::SaveVals (const char *fname) const
{
  int ok = 1;

  if (m0 != NULL)
    return 1;
  ok &= SaveCfg(fname);
  ok &= mic[0].SaveVals(fname, 0);
  return ok;
}


//= Write current deployment specific values to a file.
// do not call if using external microphone

int jhcSpeaker::SaveCfg (const char *fname) const
{
  int i, ok = 1;

  // ignore if external mic in use
  if (m0 != NULL)
    return 1;

  // store valid devices (good port numbers) but erase other lines
  for (i = 0; i < amax; i++)
    if (mic[i].mport > 0)
      ok &= mic[i].SaveCfg(fname);   
    else
      (mic[i].gps).RemVals(fname); 
  return ok;
}


//= Copy processing parameters from microphone 0 to all others.
// should call this any time parameters are adjusted

void jhcSpeaker::MicDup ()
{
  int i;
  
  for (i = 1; i < amax; i++)
    mic[i].CopyVals(mic[0]);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// assumes remote mic (if used) is reset separately
// NOTE: should call Bind with a person finder first 

void jhcSpeaker::Reset ()
{
  int i;

  // clear all array microphone processing
  if (m0 == NULL)
    for (i = 0; i < amax; i++)
      if (mic[i].mport > 0)
        mic[i].Reset();

  // no speaker currently found
  vth = 2;                             // was jhcDirMic::spej
  spk = -1;
  vcnt = 0;
  det = 0;
}


//= Estimate sound direction for all valid local arrays.
// this should be called before Analyze unless using remote mic
// "voice" is whether someone is speaking (voice dir separate from snd dir)

void jhcSpeaker::Update (int voice)
{
  int i;

  for (i = 0; i < amax; i++)
    if (mic[i].mport > 0)
      mic[i].Update(voice);
}


//= Perform bulk of processing on input image.
// "voice" is whether someone is speaking (voice dir separate from snd dir)
// assumes jhcStare3D person detector and tracker already updated
// assumes remote mic (if used) is already updated
// returns speaker ID if known, negative if none

int jhcSpeaker::Analyze (int voice)
{
  double best = -1.0;
  int i, n, win = -1;

  // sanity check
  if (s3 == NULL)
    return Fatal("Unbound person detector in jhcSpeaker::Analyze");

  // see if new speaker should be sought
  det = 0;
  if (voice <= 0)
  {
    vcnt = 0;
    spk = -1;                // forget previous speaker!
    return spk;
  }

  // start searching at vth cycles but latch the first speaker found
  if (++vcnt < vth)                         
    spk = -1;
  else if ((vcnt >= vth) && (spk >= 0))       
    return spk;

  // figure out who might be talking based on all mics
  if (m0 != NULL)
    win = pick_dude(*m0, best);
  else
    for (i = 0; i < amax; i++)
      if (mic[i].mport > 0)
        if ((n = pick_dude(mic[i], best)) >= 0)
          win = n;
          
  // record presumed speaker and whether newly determined
  if (win < 0)
    return -1;
  spk = s3->PersonID(win, 1);
  det = 1;
  return spk;
}


//= Pick overall best person based on beam offset distance.
// returns head number (not ID) if any suitable, else -1

int jhcSpeaker::pick_dude (const jhcDirMic& m, double& best) const
{
  const jhcMatrix *hd;
  double d;
  int i, n = s3->PersonLim(1), win = -1;

  // scan through all currently valid heads
  for (i = 0; i < n; i++)
    if (s3->PersonOK(i, 1))
    {
      // find closest beam point in world coords and see if valid
      hd = (const jhcMatrix *) s3->GetPerson(i, 1);
      if ((d = m.ClosestPt(NULL, *hd, 0, 1)) < 0.0)
        continue;

      // compare to best so far and save index
      if ((best < 0.0) || (d < best))
      {
        best = d;
        win = i;
      }
    }
  return win;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Show location of all valid microphone arrays on overhead map.

int jhcSpeaker::MicsMap (jhcImg& dest, int invert) const
{
  int i;

  // sanity check
  if (s3 == NULL)
    return Fatal("Unbound person detector in jhcSpeaker::MicsMap");
  if (!dest.SameFormat(s3->ParseWid(), s3->ParseHt(), 1))
    return Fatal("Bad input to jhcSpeaker::MicsMap");

  // find microphone(s) in map coords
  if (m0 != NULL)
    draw_mic(dest, *m0, invert);
  else
    for (i = 0; i < amax; i++)
      if (mic[i].mport > 0)
        draw_mic(dest, mic[i], invert);
  return 1;
}


//= Find microphone in map coords and draw oriented rectangle.

void jhcSpeaker::draw_mic (jhcImg& dest, const jhcDirMic& m, int invert) const
{
  jhcMatrix mid(4);
  double ht = 2.2, wid = 11.4;
  double sc, hh, hw0, x, y, hw, a, sa, ca, ws, wc, hc, hs;
  double nex, ney, sex, sey, swx, swy, nwx, nwy;

  // figure out mic array size in pixels
  sc = s3->I2P(1.0);
  hh  = 0.5 * sc * ht;
  hw0 = 0.5 * sc * wid;

  // get mic center in map coordinates
  mid.MatVec(s3->ToMap(), m.loc);
  x = ((invert <= 0) ? mid.X() : dest.XLim() - mid.X());
  y = ((invert <= 0) ? mid.Y() : dest.YLim() - mid.Y());

  // get tilt compensated width and pan angle
  hw = hw0 * cos(D2R * m.tilt);
  a = D2R * m.pan;
  if (invert > 0)
    a = PI - a;

  // figure out corner offset values
  sa = sin(a);
  ca = cos(a);
  wc = hw * ca;
  ws = hw * sa;
  hc = hh * ca;
  hs = hh * sa;

  // compute 4 corners
  nex = x + wc - hs;
  sex = x + wc + hs; 
  nwx = x - wc - hs;
  swx = x - wc + hs;
  ney = y + ws + hc;
  sey = y + ws - hc;
  nwy = y - ws + hc;
  swy = y - ws - hc;

  // connect into angled rectangle
  DrawLine(dest, nex, ney, sex, sey, 1, 255);
  DrawLine(dest, sex, sey, swx, swy, 1, 255);
  DrawLine(dest, swx, swy, nwx, nwy, 1, 255);
  DrawLine(dest, nwx, nwy, nex, ney, 1, 255);
}


//= Show sound direction as a beam from each microphone on overhead map.
// src: 0 = raw, 1 = smoothed, 3 = voice
// projects beam (really a cone) at microphone height 

int jhcSpeaker::SoundMap (jhcImg& dest, int invert, int src) const
{
  int i;

  // sanity check
  if (!dest.SameFormat(s3->ParseWid(), s3->ParseHt(), 1))
    return Fatal("Bad input to jhcSpeaker::SoundMap");

  // find microphone(s) in map coords
  if (m0 != NULL)
    map_beam(dest, *m0, invert, src);
  else
    for (i = 0; i < amax; i++)
      if (mic[i].mport > 0)
        map_beam(dest, mic[i], invert, src);
  return 1;
}


//= Show beam from a particular microphone on the overhead map.

void jhcSpeaker::map_beam (jhcImg& dest, const jhcDirMic& m, int invert, int src) const
{
  jhcMatrix pos(4), tip(4);
  double rads = D2R * (m.Dir(src) + m.pan + 90.0), len = 192.0;   // 16 feet
  double mx, my, tx, ty;

  // get base in screen coordinates
  pos.MatVec(s3->ToMap(), m.loc);
  mx = ((invert <= 0) ? pos.X() : dest.XLim() - pos.X());
  my = ((invert <= 0) ? pos.Y() : dest.YLim() - pos.Y());

  // find end of long ray
  tip.RelVec3(m.loc, len * cos(rads), len * sin(rads), 0.0);
  pos.MatVec(s3->ToMap(), tip);
  tx = ((invert <= 0) ? pos.X() : dest.XLim() - pos.X());
  ty = ((invert <= 0) ? pos.Y() : dest.YLim() - pos.Y());
  
  // mark on map
  DrawLine(dest, mx, my, tx, ty, 1, -2);
}
  

//= Show sound direction as an azimuth line from each microphone on some camera view.
// src: 0 = raw, 1 = smoothed, 3 = voice
// assumes source is 6 feet away and at microphone height

int jhcSpeaker::SoundCam (jhcImg& dest, int cam, int rev, int src) const
{
  int i;

  // check arguments
  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcSpeaker::SoundCam");
  s3->AdjGeometry(cam);

  // go through all valid microphones
  if (m0 != NULL)
    front_beam(dest, *m0, rev, src);
  else
    for (i = 0; i < amax; i++)
      if (mic[i].mport > 0)
        front_beam(dest, mic[i], rev, src);
  return 1;
}


//= Show direction on frontal color view based on a particular microphone.
// assumes geometry already adjusted for desired camera

void jhcSpeaker::front_beam (jhcImg& dest, const jhcDirMic& m, int rev, int src) const
{
  jhcMatrix tip(4), rel(4);
  double rads = D2R * (m.Dir(src) + m.pan + 90.0), len = 72.0;   // 6 feet
  double tx, ty, sc = dest.YDim() / (double) s3->InputH();

  // find end of long ray in screen coordinates
  tip.RelVec3(m.loc, len * cos(rads), len * sin(rads), 0.0);
  s3->BeamCoords(rel, tip);
  s3->ImgPt(tx, ty, rel, sc);
  if (rev > 0)
    tx = dest.XLim() - tx;

  // draw vertical line 
  if (src >= 2)
    DrawLine(dest, tx, 0.1 * dest.YDim(), tx, 0.9 * dest.YDim(), 3, -2);
  else
    DrawLine(dest, tx, 0.0, tx, dest.YDim(), 1, ((src >= 1) ? -3 : -4));
}


//= Show closest points consistent with sound beam on overhead map.
// src: 0 = raw, 1 = smoothed, 2 = latched, 3 = only selected speaker
// style: 0 = red lollipop, 1 = blue beam, 3 = yellow X

int jhcSpeaker::OffsetsMap (jhcImg& dest, int trk, int invert, int src, int style) const
{
  int i;

  // check arguments
  if (s3 == NULL)
    return Fatal("Unbound person detector in jhcSpeaker::OffsetsMap");
  if (!dest.SameFormat(s3->ParseWid(), s3->ParseHt(), 1))
    return Fatal("Bad input to jhcSpeaker::OffsetsMap");

  // go through all valid microphones
  if (m0 != NULL)
    map_off(dest, *m0, trk, invert, src, style);
  else
    for (i = 0; i < amax; i++)
      if (mic[i].mport > 0)
        map_off(dest, mic[i], trk, invert, src, style);
  return 1;
}


//= Show offset location in overhead map using proper style for a particular microphone.

void jhcSpeaker::map_off (jhcImg& dest, const jhcDirMic& m, int trk, int invert, int src, int style) const
{
  jhcMatrix lims(4), arr(4), hit(4), pt(4);
  const jhcMatrix *hd;
  double sz, circ = 6.0;
  int i, id, n = s3->PersonLim(trk);

  // ignore raw beam request if no samples received
  if ((style <= 0) && (m.cnt <= 0))
    return;
  lims.SetVec3(dest.XLim(), dest.YLim(), 0.0);

  // find microphone in map coords
  arr.MatVec(s3->ToMap(), m.loc);
  if (invert > 0)
    arr.CompVec3(lims);
  sz = s3->I2P(0.5 * circ);

  // possibly generate a beam for each person
  for (i = 0; i < n; i++)
    if ((id = s3->PersonID(i, 1)) > 0)
      if ((src <= 2) || (id == spk))
      {
        // find closest point to a person (if valid match)
        hd = (const jhcMatrix *) s3->GetPerson(i, trk);
        if (m.ClosestPt(&pt, *hd, src, 1) < 0.0)
          continue;

        // convert to map coordinates (and possibly invert display)
        hit.MatVec(s3->ToMap(), pt);
        if (invert > 0)
           hit.CompVec3(lims);

        // pick drawing method based on source
        if (style <= 0)
        {
          CircleEmpty(dest, hit.X(), hit.Y(), sz, 1, -1);
          DrawLine(dest, arr.X(), arr.Y(), hit.X(), hit.Y(), 1, -1);
        }
        else if (style == 1)
          DrawLine(dest, arr.X(), arr.Y(), hit.X(), hit.Y(), 3, -4);
        else
          XMark(dest, hit.X(), hit.Y(), 17, 3, -3);
      }
}


//= Show closest points consistent with sound beam on frontal view.

int jhcSpeaker::OffsetsCam (jhcImg& dest, int cam, int trk, int rev, int src, int style) const
{
  int i;

  // check arguments
  if (s3 == NULL)
    return Fatal("Unbound person detector in jhcSpeaker::OffsetsCam");
  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcSpeaker::OffsetsCam");
  s3->AdjGeometry(cam);

  // go through all valid microphones
  if (m0 != NULL)
    front_off(dest, *m0, trk, rev, src, style);
  else
    for (i = 0; i < amax; i++)
      if (mic[i].mport > 0)
        front_off(dest, mic[i], trk, rev, src, style);
  return 1;
}


//= Show offset location in frontal view of some camera using proper style for a particular microphone.
// assumes geometry already adjusted for desired camera

void jhcSpeaker::front_off (jhcImg& dest, const jhcDirMic& m, int trk, int rev, int src, int style) const
{
  jhcMatrix rel(4), pt(4);
  jhcRoi box;
  const jhcMatrix *hd;
  double cx, cy, sz, circ = 6.0;
  double sc = dest.YDim() / (double) s3->InputH();
  int i, id, xlim = dest.XLim(), n = s3->PersonLim(trk);

  // ignore raw beam request if no samples received
  if ((style <= 0) && (m.cnt <= 0))
    return;

  // possibly generate a beam for each person
  for (i = 0; i < n; i++)
    if ((id = s3->PersonID(i, 1)) > 0)
      if ((src <= 2) || (id == spk))
      {
        // find closest point to a person (if valid match)
        hd = (const jhcMatrix *) s3->GetPerson(i, trk);
        if (m.ClosestPt(&pt, *hd, src, 1) < 0.0)
          continue;

        // convert to map coordinates 
        s3->BeamCoords(rel, pt);
        s3->ImgPt(cx, cy, rel, sc);
        sz = 0.5 * circ * s3->ImgScale(pt, sc);
        if (rev > 0)
          cx = xlim - cx;

        // pick drawing method based on source
        if (style <= 0)
          CircleEmpty(dest, cx, cy, sz, 1, -1);
        else if (style == 1)
          DrawLine(dest, cx, cy - sz, cx, cy + sz, 5, -4);
        else
          XMark(dest, cx, cy, 17, 3, -3);
      }
}

