// jhcVideoSrc.h : video input stream base class
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

#ifndef _JHCVIDEOSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVIDEOSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"
#include "Data/jhcName.h"
#include "Data/jhcImg.h"


//= Video input stream base class.
// virtual class describing what messages a video source must handle
// extended to handle simple audio extraction as well
// it does not make sense to create instances of this class directly
// includes temporal region-of-interest (tROI) = start, stop, and step
//   these are used as defaults in calls to Seek and Get
// valid frames numbers start at 1, nframes is the last frame available
// if "noisy" is non-zero, will report errors and status to the user
//
// <pre>
// Derived classes should implement at least the following:
//   jhcVSrc (const char *fname, int index =0)
//     builds video source given name or specification
//     possibly builds frame index also (arg not needed for live types)
//     binds: ok, w, h, d, aspect, freq, and nframes
//   iGet (jhcImg &dest, int *advance)
//     retrieves the next available video frame (-1 = broken, 0 = fail, 1 = ok)
//     adjusts number of frames since last call (typically "advance" = 1)
//   iSeek (int number) = optional
//     set up to read sequentially starting at a certain frame number
//     returns 1 if successful else 0 if failed
// </pre>

class jhcVideoSrc : public jhcName
{
// PROTECTED MEMBER VARIABLES
protected:
  int w, h, d, w2, h2, d2, jumped, ok, phase, adim, ach, asps;
  int old, previous, nextread, nframes, anum;
  double aspect, freq, aspect2, freq2, flen, flen2, dsc, dsc2;
  char msg[20], kind[40], label[500];
  UC8 *daux;
  int naux;


// PUBLIC MEMBER VARIABLES
public:
  //= Playback control parameter set.
  // includes noisy, PauseNum, FirstFrame, LastFrame, DispRate, Increment, and ByKey
  jhcParam play;    
  int noisy;         /** Send user messages and ask questions.    */
  int PauseNum;      /** Pause every N frames and wait for OK.    */
  int PauseStart;    /** Start pausing at this frame number.      */
  int FirstFrame;    /** Video selection start (frame number).    */
  int LastFrame;     /** Video selection end (frame number).      */
  int Increment;     /** Used to play every Nth frame only.       */
  double DispRate;   /** Playback slowdown factor.                */
  int ByKey;         /** Play I-frames only (i.e. 15x for MPEG).  */
  int Shift;         /** Amount to downshift pixels (for Kinect). */


// PUBLIC MEMBER FUNCTIONS
public:
  // basic operations and miscellaneous functions common to sub-types
  // need virtual destructor to handle cases where some derived class is
  //   cast to this base one (makes sure specialized destructor is called)
  jhcVideoSrc ();
  virtual ~jhcVideoSrc () {};                            /** Placeholder for more specific classes. */

  // setting and testing image sizes
  void Defaults ();
  void AdjSize (int *xmax, int *ymax, int *bw =NULL);
  int SameFormat (jhcImg& tst, int src =0) const;
  jhcImg *SizeFor (jhcImg& dest, int src =0);
  const char *SizeTxt (int src =0);
  void SetAspect (double a =0.0) {aspect = a;}           /** Change aspect ratio of pixels in image */

  // member variable access (read only)
  int Valid () const  {if (ok > 0) return 1; return 0;}  /** Whether video source is operational.         */
  int Dual () const   {if (d2 > 0) return 1; return 0;}  /** Whether secondary stream is operational.     */
  int Status () const {return ok;}                       /** Possibly returns cause of error (if any).    */
  int GetKey () const {return ByKey;}                    /** If the video is being played by key frames.  */
  int Step () const   {return Increment;}                /** Frame number advance between each image.     */
  int Next () const   {return nextread;}                 /** Tells frame number of next frame to be read. */
  int Last () const   {return previous;}                 /** Tells frame number of last frame retrieved.  */
  int Frames () const {return nframes;}                  /** Returns total length of video (if known).    */
  int Start (); 
  int End ();
  int Advance ();

  // details of images retrieved
  int XDim (int src =0) const                                         /** Width of returned frames.         */
    {return((src > 0) ? w2 : w);}  
  int YDim (int src =0) const                                         /** Height of returned frames.        */
    {return((src > 0) ? h2 : h);}  
  int Fields (int src =0) const                                       /** Color depth of returned frames.   */
    {return((src > 0) ? d2 : d);}                       
  double Ratio (int src =0);
  double Rate (int src =0) const                                      /** Native video frame rate.          */
    {return((src > 0) ? freq2 : freq);}   
  double Focal (int src =0) const                                     /** Focal length in pixels.           */     
    {return((src > 0) ? flen2 : flen);} 
  double Scaling (int src =0) const                                   /** Value scaling (depth adjust).     */ 
    {return((src > 0) ? dsc2 : dsc);} 
  double AdjRate (int src =0) const                                   /** Frame viewing rate.               */
    {return(Rate(src) / (DispRate * Increment));}   
  int *Dims (int *v) const {v[0] = w; v[1] = h; v[2] = d; return v;}  /** Fill array with frame dimensions. */
  void Dims (int *x, int *y) const {*x = w; *y = h;};                 /** Extract frame width and height.   */
  const char *StrClass () const {return((const char *) kind);}        /** Name of class actually used.      */
  int IsClass (const char *cname) const;
  virtual const char *FrameName (int idx_wid =-1, int full =0);

  // for auxilliary data stored with images
  virtual int AuxCnt () const {return naux;}
  virtual const UC8 *AuxData () const {return daux;}

  // setting and testing temporal properties
  void SetStep (double secs);
  int StepTime (double rate =-1.0, int src =0);
  double Duration (int start =0, int stop =0);
  char *Interval (char *ans, int start, int stop, int style, int ssz);
  virtual int TimeStamp ();
  int Stepping (); 

  // temporal properties - convenience
  template <size_t ssz>
    char *Interval (char (&ans)[ssz], int start, int stop, int style =0)
      {return Interval(ans, start, stop, style, ssz);}
  
  // basic operations
  int Rewind (int rev_up =0);
  int Seek (int number);
  int Seek (double secs);
  int Get (jhcImg& dest, int src =0, int block =1);
  int DualGet (jhcImg& dest, jhcImg& dest2);

  // core working functions that should be implemented by subtypes
  // generally size cannot be changed and key frame (I-frame) access is invalid 
  virtual void SetStep (int offset, int key =0);
  virtual void SetRate (double fps) {}                     /** Request certain frame rate. */
  virtual void SetSize (int xmax, int ymax, int bw =0) {}  /** Request return image size.  */
  virtual void Prefetch (int doit =1) {}                   /** Allow frame prefetching.    */
  virtual void Close () {ok = 0;}                          /** Shut down image source.     */

  // for detailed configuration of source properties (e.g. pan, exposure, etc.)
  virtual int GetVal (int *val, const char *tag) {return 0;}      /** Get some framegrabber property value. */
  virtual int SetVal (const char *tag, int val) {return 0;}       /** Set some framegrabber property value. */
  virtual int GetDef (int *vdef, const char *tag, int *vmin =NULL, 
                      int *vmax =NULL, int *vstep =NULL) 
    {return 0;}                                                   /** Get default value for framegrabber property. */
  virtual int SetDef (const char *tag =NULL, int servo =0) 
    {return 0;}                                                   /** Set framegrabber property to default value.  */           
  double GetValF (const char *tag);
  double SetValF (const char *tag, double frac);

  // audio extensions
  int ABits () {return adim;}                                   /** Returns bits per audio sample.    */
  int AChan () {return ach;}                                    /** Returns number of audio channels. */
  int ARate () {return asps;}                                   /** Returns audio sampling rate.      */
  int ALast () {return anum;}                                   /** Index of last audio sample read.  */
  const char *AudioTxt ();
  int AGet (US16 *snd, int n, int ch =0) {return 0;}  /** Get a number of audio samples.    */
  int AFrame (US16 *snd, int ch =0) {return 0;}       /** Get audio associated with video.  */

  // audio configuration
  virtual int SetABits (int n) {return adim;}    /** Change the audio sample resolution.  */
  virtual int SetAChan (int ch) {return ach;}    /** Change the number of audio channels. */
  virtual int SetARate (int sps) {return asps;}  /** Change the audio sampling rate.      */


// PRIVATE MEMBER FUNCTIONS
private:
  // core working functions that should be implemented by subtypes
  // generally seeks just happen while dual frame and audio access always fails 
  virtual int iSeek (int number) {return 1;}                             /** Actually go to a particular frame.   */
  virtual int iGet (jhcImg& dest, int *advance, int src, int block) =0;  /** Get the next scheduled frame.        */
  virtual int iDual (jhcImg& dest, jhcImg& dest2) {return 0;}            /** Get next pair of frames from source. */
  virtual int iAGet (US16 *snd, int n, int ch) {return 0;}               /** Get a number of audio samples.       */

  // convenient mounting points for messages
  int UserCheck ();
  int DimsErrMsg (jhcImg& dest, int src =0);
  int GenErrMsg ();
  int EndFileMsg ();
  int EndSelMsg ();
  int GenFailMsg ();

};


#endif
