// jhcVideoSrc.cpp : common functions for video stream base class
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2019 IBM Corporation
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
#include <string.h>

#include "Interface/jhcMessage.h"

#include "Video/jhcVideoSrc.h"


///////////////////////////////////////////////////////////////////////////
//                            Creation                                   //   
///////////////////////////////////////////////////////////////////////////

//= Constructor sets up defaults.

jhcVideoSrc::jhcVideoSrc ()
{
  // name of this class
  strcpy_s(kind, "jhcVideoSrc");
  ok = 0;

  // image size, shape, and frame rate of main stream
  w  = 0;
  h  = 0;
  d  = 0;
  aspect = 0.0;     // used to be 1
  freq = 30.0;      // typical for cameras
  flen = 554.3;     // 60 deg VGA
  dsc  = 1.0;       // no pixel adjustment

  // image size, shape, and frame rate of secondary stream
  w2 = 0;
  h2 = 0;
  d2 = 0;
  aspect2 = 0.0;
  freq2 = 30.0;
  flen2 = 554.3;
  dsc2  = 1.0;

  // depth image conversion
  Shift = 0;

  // frame stepping parameters
  ByKey = 0;
  old = 0;
  previous = 0;
  nextread = 1;
  nframes  = 0;
  jumped   = 1;

  // message initialization
  noisy = 0;      // need 0 for command line scripts
  phase = 0;
  *msg = '\0';
  *label = '\0';

  // auxilliary data in images
  daux = NULL;
  naux = 0;

  // audio properties
  adim = 0;
  ach = 0;
  asps = 0;
  anum = 0;

  // standard processing values
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                     Image Sizes and Defaults                          //   
///////////////////////////////////////////////////////////////////////////

//= Configure pointers and default values pairings.

void jhcVideoSrc::Defaults ()
{
  play.SetTag("vid_step");
  play.ClearAll();
  play.NextSpec4( &PauseNum,     0,   "Pause Interval");
  play.NextSpec4( &PauseStart,   0,   "Start of Pausing");
  play.NextSpec4( &FirstFrame,   0,   "Selection Start");      // was 1
  play.NextSpec4( &LastFrame,    0,   "Selection End");
  play.NextSpec4( &previous,     0,   "Last Frame Shown");
  play.NextSpec4( &nframes,      0,   "Length of Video");

  play.NextSpec4( &Increment,    1,   "Frame Increment");
  play.NextSpecF( &DispRate,     1.0, "Playback Slowdown");
  play.RevertAll();

  // some things are for display purposes only
  play.LockMatch( &previous, 1);
  play.LockMatch( &nframes, 1);
}


//= Request a size below some limits, return actual size.

void jhcVideoSrc::AdjSize (int *xmax, int *ymax, int *bw)
{
  int dw = ((xmax != NULL) ? *xmax : 0);
  int dh = ((ymax != NULL) ? *ymax : 0);
  int dm = (  (bw != NULL) ? *bw   : 0);

  SetSize(dw, dh, dm);
  if (xmax != NULL)
    *xmax = w;
  if (ymax != NULL)
    *ymax = h;
  if (bw != NULL)
    *bw = d;
}


//= Check if image is appropriate for receiving video data.

int jhcVideoSrc::SameFormat (jhcImg& tst, int src) const
{
  if ((tst.XDim() != XDim(src)) || (tst.YDim() != YDim(src)) || (tst.Fields() != Fields(src)))
    return 0;
  return 1;
}


//= Alter image so it is the correct size to receive data from stream.

jhcImg *jhcVideoSrc::SizeFor (jhcImg& dest, int src) 
{
  dest.SetSize(XDim(src), YDim(src), Fields(src));
  dest.SetRatio(Ratio(src));
  return &dest;
}


//= Report size of image in format "(w h) x f").

const char *jhcVideoSrc::SizeTxt (int src)
{
  sprintf_s(msg, "(%d %d) x %d", XDim(src), YDim(src), Fields(src));
  return msg;
}


///////////////////////////////////////////////////////////////////////////
//                          Source Properties                            //
///////////////////////////////////////////////////////////////////////////

//= Tell frame number where rewind will go.

int jhcVideoSrc::Start ()
{
  // forward playing
  if (Increment >= 0)
  {
    if (FirstFrame > 0)
      return FirstFrame;
    return 1;
  }

  // reverse playing
  if (LastFrame > 0) 
    return LastFrame;
  if (nframes > 0)
    return nframes;

  // if no limits
  return nextread;
}


//= Tell frame number where playback selection stops.
// zero means indeterminate end

int jhcVideoSrc::End ()
{
  // reverse playing
  if (Increment < 0)
  {
    if (FirstFrame > 0)
      return FirstFrame;
    return 1;
  }

  // forward playing
  if (LastFrame > 0) 
    return LastFrame;
  if (nframes > 0)
    return nframes;

  // if no limits
  return 0;
}


//= How many frames jumped over since last read.

int jhcVideoSrc::Advance ()
{
  if ((jumped != 0) || (previous <= 0))
    return 0;
  return((int)(previous - old));
}


//= Returns aspect ratio (h / w) of frame pixels.   
// if no aspect has been bound, then makes a guess based on dimensions

double jhcVideoSrc::Ratio (int src) 
{
  double *a = ((src > 0) ? &aspect2 : &aspect);
  int w0 = XDim(src), h0 = YDim(src);
  double den;

  if ((*a > 0.0) || (w0 <= 0) || (h0 <= 0))
    return *a;

  den = w0 / (double) h0;
  if ((den < 1.0) || (den > 3.0))
    *a = 1.0;                         // default
  else if (den >= 2.6)                  
    *a = (8.0 / 3.0) / den;           // special double wide stereo
  else if (den >= 1.6)
    *a = (16.0 / 9.0) / den;          // wide-screen 16:9 frame
  else 
    *a = (4.0 / 3.0) / den;           // standard 4:3 frame
  return *a;
}                            


//= Check to see if underlying stream is of some particular class.

int jhcVideoSrc::IsClass (const char *cname) const
{
  if (strcmp(kind, cname) == 0)
    return 1;
  return 0;
}


//= Get special name for frame, possibly appending frame index.
// if num_wid is negative then no frame index added
// if num_wid is positive then some leading zeros are added
// if full is non-zero then add proper directory and extension

const char *jhcVideoSrc::FrameName (int idx_wid, int full)
{
  char fmt[20], core[80];

  // get source name, possibly with frame index number tacked on
  if (idx_wid < 0)
    strcpy_s(core, Name());
  else
  {
    sprintf_s(fmt, "%%s_%%0%dd", idx_wid);    // create format string
    sprintf_s(core, fmt, Base(), previous);
  }

  // possibly attach directory and extension
  if (full <= 0)
    strcpy_s(label, core);
  else
    sprintf_s(label, "%s/%s%s", Dir(), core, Extension());
  return ((const char *) label);
}


///////////////////////////////////////////////////////////////////////////
//                        Temporal Properties                            //
///////////////////////////////////////////////////////////////////////////

//= Generally just take given increment and frob read pointer to match.

void jhcVideoSrc::SetStep (int offset, int key)
{
  int oldinc = (int) Increment;

  Increment = offset;
  ByKey = key;
  Seek(previous - oldinc + (int) Increment);
}


//= Advance a certain amount of real time on each step.

void jhcVideoSrc::SetStep (double secs)
{
  SetStep(ROUND(secs * freq), 0);
}


//= Determine how long display loop should wait between frames (ms).

int jhcVideoSrc::StepTime (double rate, int src)
{
  int incr = Increment;
  double tol = 0.5, r = ((rate >= 0.0) ? rate : DispRate);

  if (r <= 0.0)
    return 0;

  // special case for single frame videos
  if (nframes == 1)
    return 0;

// special VFW tweak - now handled in jhcDisplay::PaceOrHit (LoopHit)
//  if (IsFlavor("vfw"))
//    tol = -3.5;
//  else if ((jumped == 0) && (previous > 0))
//    incr = nextread - previous;

  if (incr < 0)
    incr = -incr;
  return((int)(1000.0 * incr * r / Rate(src) + tol));
}


//= Return length of whole file (or portion) in seconds, not frames.
// start = 0 defaults to FirstFrame, 1 means start of video 
// stop = 0 defaults to LastFrame, -1 defaults to end of video

double jhcVideoSrc::Duration (int start, int stop)
{
  int t1 = start, t2 = stop;

  if (stop == -1)
    t2 = nframes;
  else if (stop <= 0)
    t2 = LastFrame;
  if (start == 0)
    t1 = FirstFrame;
  else if (start < 0)
    start = 1;
  return((double)(t2 - t1) / freq);
}


//= Generate header string for database entry.
// styles:
//   0 = start-secs, start-us, end-secs, end-us
//   1 = start*90000, stop*90000
//   2 = start-secs thru stop-secs (N frames)

char *jhcVideoSrc::Interval (char *ans, int start, int stop, int style, int ssz)
{
  int isec0, isec1;
  double sec0, sec1;
  double fr = ((freq == 0.0) ? 1.0 : freq);

  sec0 = start / fr;
  isec0 = (int) sec0;
  sec1 =  (stop + 1) / fr;
  isec1 = (int) sec1;
  if (style == 0)
    sprintf_s(ans, ssz, "%4d, %06d, %4d, %06d",
              isec0, ROUND(1000000.0 * (sec0 - isec0)),
              isec1, ROUND(1000000.0 * (sec1 - isec1)));
  else if (style == 1)
    sprintf_s(ans, ssz, "%9d, %9d", 
              (int)(90000.0 * start     / fr), 
              (int)(90000.0 * (stop + 1) / fr));
  else if (style == 2)
    sprintf_s(ans, ssz, "%4d thru %4d (%4d frames)", start, stop, stop - start + 1);
  return ans;
}


//= Generate a time stamp in milliseconds of when last frame was "acquired".
// time zero is first frame, each succeeding frame takes 1000/freq ms

int jhcVideoSrc::TimeStamp ()
{
  if ((previous <= 0) || (freq <= 0))
    return 0;
  return((int)(1000.0 * (previous - 1) / freq + 0.5)); 
}


//= Whether the video is currently in frame-by-frame mode.
// useful for turning on debugging messages in certain parts 

int jhcVideoSrc::Stepping () 
{
  if ((PauseNum > 0) && (previous >= PauseStart) && (phase == 1)) 
    return 1; 
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                           Basic Operations                            //   
///////////////////////////////////////////////////////////////////////////

//= Change read pointer to appropriate boundary of selected region.
// can also start prefetch if rev_up is non-zero

int jhcVideoSrc::Rewind (int rev_up)
{
  int ans;

  ans = Seek(Start());
  if ((ans > 0) && (rev_up > 0))
    Prefetch(1);
  return ans;
}


//= Set up so next "Get" is a certain frame within the video.

int jhcVideoSrc::Seek (int number)
{
  int n = number;

  if (ok <= 0)
    return GenErrMsg();

  // clip to valid range and check for no movement
//  if ((n <= 0) || (nframes <= 0))                
  if (n <= 0)                          // can guess for streams
    n = 1;
  else if ((nframes > 0) && (n > nframes))
    n = nframes;
  if (n == nextread)
    return 1;

  // move real read positions and update local pointer
  if (iSeek(n) <= 0)
    return 0;
  nextread = n;        
  jumped = 1;
  phase = 0;
  return 1;
}


//= Go to location based on time not frame number.

int jhcVideoSrc::Seek (double secs)
{ 
  return Seek((int)(secs * freq + 0.5));
}


//= Get frame currently pointed to and set next read point.
// returns 0 if at end of selection, negative for some problem
// else returns total number of frames skipped over since last one
// (defaults to Increment if iGet method does not touch value)
// if block <= 0 then will return 0 until frame ready (live sources)

int jhcVideoSrc::Get (jhcImg& dest, int src, int block)
{
  int ans, n = Increment;

  // make sure blank image is compatible with video
  if (ok <= 0)
    return GenErrMsg();
  if (!dest.SameFormat(XDim(src), YDim(src), Fields(src)) && !IsClass("jhcListVSrc"))
    return DimsErrMsg(dest, src); 

  // no valid frame selection for streaming inputs (but can guess)
//  if (nframes <= 0)
//    FirstFrame = 0;

  // if end of video or selection reached, see if user wants to loop
  // setting LastFrame negative makes looping automatic at end of file
  if (((FirstFrame > 0) && (nextread < FirstFrame)) ||
      ((LastFrame > 0)  && (nextread > LastFrame)))
  {
    if (EndSelMsg() <= 0)
      return 0;
    Rewind();
  }
  else if ((nextread < 1) || ((nframes > 0) && (nextread > nframes)))
  {
    if ((LastFrame >= 0) && (EndFileMsg() <= 0))
      return 0;
    Rewind();
  }

  // call underlying function 
  if (UserCheck() != 1)
    return 0;
  dest.SetRatio(Ratio());
  dest.FullRoi();
  ans = iGet(dest, &n, src, block);
  if (ans < 0)
    return GenFailMsg();
  if (ans == 0)
    return 0;

  // update internal position counters
  // if a seek occurred (jumped = 1) then always force frame number to value
  old = previous;
  if (jumped != 0)
    previous = nextread;
  else
    previous += n;
  nextread = previous + Increment;
  jumped = 0;
  return 1;
}


//= Get next pair of frames from video source.
// returns 0 if at end of selection, negative for some problem
// else returns total number of frames skipped over since last one
// essentially the same as Get but calls iDual internally
// if video source has only one stream, dest2 is a copy of dest
// returns 1 if just depth (dest2), 2 if depth and color (dest)
// returns 0 if at end of selection, negative for some problem

int jhcVideoSrc::DualGet (jhcImg& dest, jhcImg& dest2)
{
  int ans, n = Increment;

  // make sure blank image is compatible with video
  if (ok <= 0)
    return GenErrMsg();
  if (!dest.SameFormat(w, h, d))
    return DimsErrMsg(dest); 
  if (!dest2.SameFormat(w2, h2, d2))
    return DimsErrMsg(dest2, 1); 

  // no valid frame selection for streaming inputs (but can guess)
//  if (nframes <= 0)
//    FirstFrame = 0;

  // if end of video or selection reached, see if user wants to loop
  // setting LastFrame negative makes looping automatic at end of file
  if (((FirstFrame > 0) && (nextread < FirstFrame)) ||
      ((LastFrame > 0)  && (nextread > LastFrame)))
  {
    if (EndSelMsg() <= 0)
      return 0;
    Rewind();
  }
  else if ((nextread < 1) || ((nframes > 0) && (nextread > nframes)))
  {
    if ((LastFrame >= 0) && (EndFileMsg() <= 0))
      return 0;
    Rewind();
  }

  // check for pause then prepare receiving images
  if (UserCheck() != 1)
    return 0;
  dest.SetRatio(Ratio());
  dest2.SetRatio(Ratio(1));

  // check if one or two internal streams
  if (Dual())
    ans = iDual(dest, dest2);
  else
  {
    ans = iGet(dest, &n, 0, 1);
    dest2.CopyArr(dest);
  }

  // see if internal function succeeded
  if (ans < 0)
    return GenFailMsg();
  if (ans == 0)
    return 0;

  // update internal position counters
  // if a seek occurred (jumped = 1) then always force frame number to value
  old = previous;
  if (jumped != 0)
    previous = nextread;
  else
    previous += n;
  nextread = previous + Increment;
  jumped = 0;
  return ans;
}


///////////////////////////////////////////////////////////////////////////
//                      Source Property Manipulation                     //   
///////////////////////////////////////////////////////////////////////////

//= Get the value of some property as a fraction of its range (0 to 1).
// returns -1 for bad property name (or unimplemented)

double jhcVideoSrc::GetValF (const char *tag)
{
  int val, vmin, vmax;

  if (GetDef(NULL, tag, &vmin, &vmax) <= 0)
    return -1.0;
  if (GetVal(&val, tag) <= 0)
    return -1.0;
  if (vmin == vmax)
    return 1.0;
  return((val - vmin) / (double)(vmax - vmin));
}


//= Set some property to a given fraction of its range (0 to 1).
// returns -1 for bad property else new fractional value of property 

double jhcVideoSrc::SetValF (const char *tag, double frac)
{
  int val, vmin, vmax;

  if (GetDef(NULL, tag, &vmin, &vmax) <= 0)
    return -1.0;
  val = ROUND(frac * (vmax - vmin) + vmin);
  if (SetVal(tag, val) <= 0)
    return -1.0;
  return GetValF(tag);
}


///////////////////////////////////////////////////////////////////////////
//                           Audio Extensions                            //
///////////////////////////////////////////////////////////////////////////

//= Report audio parameters in format "ch x d bits @ r Hz".

const char *jhcVideoSrc::AudioTxt ()
{
  sprintf_s(msg, "%d x %d bits @ %d Hz", ach, adim, asps);
  return msg;
}


///////////////////////////////////////////////////////////////////////////
//                              User Messages                            //   
///////////////////////////////////////////////////////////////////////////

//= Occasionally check to see if user wants to stop.

int jhcVideoSrc::UserCheck ()
{
  if ((noisy > 0) && (PauseNum > 0) && (nextread >= PauseStart))
  {
    if (phase >= PauseNum)
    {
      phase = 0;
      if (Ask("Processed through frame %ld. Continue?", previous) == 0)
        return 0;
    }
    phase++;
  }
  return 1;
}


//= Something wrong with underlying stream.

int jhcVideoSrc::GenErrMsg ()
{
  if (noisy > 0)
    Complain("Video stream %s is broken", FileName);
  return -1;
}


//= Output image wrong size.

int jhcVideoSrc::DimsErrMsg (jhcImg& dest, int src)
{
  if (noisy > 0)
    Complain("Receiving image (%d %d) x %d does not match video (%d %d) x %d!", 
              dest.XDim(), dest.YDim(), dest.Fields(), XDim(src), YDim(src), Fields(src));
  return -1;
}


//= End of video file reached.

int jhcVideoSrc::EndFileMsg ()
{
//  if (noisy > 0)
//    return AskNot("End of video. Loop?\nProcessed through frame %ld", 
//                   previous);
  return 0;
}


//= End of marked selection reached.

int jhcVideoSrc::EndSelMsg ()
{
  if (noisy > 0)
    return AskNot("End of video selection. Loop?\nProcessed through frame %ld", 
                   previous);
  return 0;
}


//= Requested frame could not be fetched.

int jhcVideoSrc::GenFailMsg ()
{
  if (noisy > 0)
    Complain("Failed after frame %ld !", previous);
  return -1;
}









