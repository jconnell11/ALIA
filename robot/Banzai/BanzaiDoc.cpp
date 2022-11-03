// BanzaiDoc.cpp : top level GUI framework runs robot and reasoner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#include "stdafx.h"
#include "Banzai.h"

#include "BanzaiDoc.h"

#include <direct.h>                  // for _getcwd in Windows
#include <conio.h>

#include <mmdeviceapi.h>
#include <endpointvolume.h>

#define SAFE_RELEASE(x) if(x) { x->Release(); x = NULL; } 


// JHC: for filtered video file open

extern CBanzaiApp theApp;


// JHC: for basic functionality

#include "Data/jhcImgIO.h"             // common video
#include "Interface/jhcMessage.h"
#include "Interface/jhcPickString.h"
#include "Interface/jhcPickVals.h"
#include "Interface/jhcString.h"
#include "Interface/jms_x.h"
#include "Processing/jhcFilter.h"


// whether to do faster background video capture

#define FASTVID 1    // some cameras need zero?


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBanzaiDoc

IMPLEMENT_DYNCREATE(CBanzaiDoc, CDocument)

BEGIN_MESSAGE_MAP(CBanzaiDoc, CDocument)
	//{{AFX_MSG_MAP(CBanzaiDoc)
	ON_COMMAND(ID_FILE_CAMERA, OnFileCamera)
	ON_COMMAND(ID_FILE_CAMERAADJUST, OnFileCameraadjust)
	ON_COMMAND(ID_FILE_OPENEXPLICIT, OnFileOpenexplicit)
	ON_COMMAND(ID_PARAMETERS_VIDEOCONTROL, OnParametersVideocontrol)
	ON_COMMAND(ID_PARAMETERS_IMAGESIZE, OnParametersImagesize)
	ON_COMMAND(ID_TEST_PLAYVIDEO, OnTestPlayvideo)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_OPENVIDEO, OnFileOpenvideo)
	ON_COMMAND(ID_PARAMETERS_SAVEDEFAULTS, OnParametersSavedefaults)
	ON_COMMAND(ID_PARAMETERS_LOADDEFAULTS, OnParametersLoaddefaults)
	//}}AFX_MSG_MAP
  ON_COMMAND(ID_FILE_KINECTSENSOR, &CBanzaiDoc::OnFileKinectsensor)
  ON_COMMAND(ID_FILE_KINECTHIRES, &CBanzaiDoc::OnFileKinecthires)
  ON_COMMAND(ID_FILE_SAVESOURCE, &CBanzaiDoc::OnFileSavesource)
  ON_COMMAND(ID_UTILITIES_PLAYDEPTH, &CBanzaiDoc::OnUtilitiesPlaydepth)
  ON_COMMAND(ID_UTILITIES_PLAYBOTH, &CBanzaiDoc::OnUtilitiesPlayboth)
  ON_COMMAND(ID_ANIMATION_IDLE, &CBanzaiDoc::OnAnimationIdle)
  ON_COMMAND(ID_ANIMATION_NEUTRAL, &CBanzaiDoc::OnAnimationNeutral)
  ON_COMMAND(ID_ARM_GOTOPOSE, &CBanzaiDoc::OnArmGotopose)
  ON_COMMAND(ID_UTILITIES_TEST, &CBanzaiDoc::OnUtilitiesTest)
  ON_COMMAND(ID_ARM_SWINGJOINT, &CBanzaiDoc::OnArmSwingjoint)
  ON_COMMAND(ID_ARM_SWINGPARAMS, &CBanzaiDoc::OnArmSwingparams)
  ON_COMMAND(ID_ARM_LIMP, &CBanzaiDoc::OnArmLimp)
  ON_COMMAND(ID_ARM_HANDFORCE, &CBanzaiDoc::OnArmHandforce)
  ON_COMMAND(ID_FORCE_DRAGHAND, &CBanzaiDoc::OnForceDraghand)
  ON_COMMAND(ID_FORCE_DRAGROBOT, &CBanzaiDoc::OnForceDragrobot)
  ON_COMMAND(ID_DEMO_DEMOOPTIONS, &CBanzaiDoc::OnDemoDemooptions)
  ON_COMMAND(ID_DEMO_TEXTFILE, &CBanzaiDoc::OnDemoTextfile)
  ON_COMMAND(ID_DEMO_INTERACTIVE, &CBanzaiDoc::OnDemoInteractive)
  ON_COMMAND(ID_PARAMETERS_MOVECMD, &CBanzaiDoc::OnParametersMovecmd)
  ON_COMMAND(ID_PARAMETERS_TURNCMD, &CBanzaiDoc::OnParametersTurncmd)
  ON_COMMAND(ID_PARAMETERS_BASEPROGRESS, &CBanzaiDoc::OnParametersBaseprogress)
  ON_COMMAND(ID_PARAMETERS_BASERAMP, &CBanzaiDoc::OnParametersBaseramp)
  ON_COMMAND(ID_DEMO_RESETROBOT, &CBanzaiDoc::OnDemoResetrobot)
  ON_COMMAND(ID_PARAMETERS_LIFTCMD, &CBanzaiDoc::OnParametersLiftcmd)
  ON_COMMAND(ID_PARAMETERS_LIFTRAMP, &CBanzaiDoc::OnParametersLiftramp)
  ON_COMMAND(ID_PARAMETERS_BATTERYLEVEL, &CBanzaiDoc::OnParametersBatterylevel)
  ON_COMMAND(ID_PARAMETERS_GRABCMD, &CBanzaiDoc::OnParametersGrabcmd)
  ON_COMMAND(ID_PARAMETERS_GRABRAMP, &CBanzaiDoc::OnParametersGrabramp)
  ON_COMMAND(ID_PARAMETERS_ARMHOME, &CBanzaiDoc::OnParametersArmhome)
  ON_COMMAND(ID_GROUNDING_HANDCMD, &CBanzaiDoc::OnGroundingHandcmd)
  ON_COMMAND(ID_GROUNDING_WRISTCMD, &CBanzaiDoc::OnGroundingWristcmd)
  ON_COMMAND(ID_GROUNDING_NECKCMD, &CBanzaiDoc::OnGroundingNeckcmd)
  ON_COMMAND(ID_PROFILING_ARMRAMP, &CBanzaiDoc::OnProfilingArmramp)
  ON_COMMAND(ID_RAMP_NECKRAMP, &CBanzaiDoc::OnRampNeckramp)
  ON_COMMAND(ID_DEPTH_PERSONMAP, &CBanzaiDoc::OnDepthPersonmap)
  ON_COMMAND(ID_DEPTH_TRACKHEAD, &CBanzaiDoc::OnDepthTrackhead)
  ON_COMMAND(ID_PEOPLE_SPEAKING, &CBanzaiDoc::OnPeopleSpeaking)
  ON_COMMAND(ID_ATTENTION_ENROLLPHOTO, &CBanzaiDoc::OnAttentionEnrollphoto)
  ON_COMMAND(ID_ATTENTION_ENROLLLIVE, &CBanzaiDoc::OnAttentionEnrolllive)
  ON_COMMAND(ID_PEOPLE_SOCIALEVENTS, &CBanzaiDoc::OnPeopleSocialevents)
  ON_COMMAND(ID_PEOPLE_SOCIALMOVE, &CBanzaiDoc::OnPeopleSocialmove)
  ON_COMMAND(ID_UTILITIES_EXTRACTWORDS, &CBanzaiDoc::OnUtilitiesExtractwords)
  ON_COMMAND(ID_UTILITIES_CHKGRAMMAR, &CBanzaiDoc::OnUtilitiesChkgrammar)
  ON_COMMAND(ID_ENVIRON_FLOORMAP, &CBanzaiDoc::OnEnvironFloormap)
  ON_COMMAND(ID_NAVIGATION_UPDATING, &CBanzaiDoc::OnNavigationUpdating)
  ON_COMMAND(ID_ENVIRON_INTEGRATED, &CBanzaiDoc::OnEnvironIntegrated)
  ON_COMMAND(ID_NAV_CAMCALIB, &CBanzaiDoc::OnNavCamcalib)
  ON_COMMAND(ID_NAV_GUIDANCE, &CBanzaiDoc::OnNavGuidance)
  ON_COMMAND(ID_ENVIRON_LOCALPATHS, &CBanzaiDoc::OnEnvironLocalpaths)
  ON_COMMAND(ID_ENVIRON_DISTANCES, &CBanzaiDoc::OnEnvironDistances)
  ON_COMMAND(ID_ENVIRON_GOTO, &CBanzaiDoc::OnEnvironGoto)
  ON_COMMAND(ID_PEOPLE_VISIBILITY, &CBanzaiDoc::OnPeopleVisibility)
  ON_COMMAND(ID_DEMO_ATTN, &CBanzaiDoc::OnDemoAttn)
  ON_COMMAND(ID_OBJECTS_TRACKOBJS, &CBanzaiDoc::OnObjectsTrackobjs)
  ON_COMMAND(ID_OBJECTS_SURFACEMAP, &CBanzaiDoc::OnObjectsSurfacemap)
  ON_COMMAND(ID_OBJECTS_DETECT, &CBanzaiDoc::OnObjectsDetect)
  ON_COMMAND(ID_OBJECTS_SHAPE, &CBanzaiDoc::OnObjectsShape)
  ON_COMMAND(ID_OBJECTS_TRACK, &CBanzaiDoc::OnObjectsTrack)
  ON_COMMAND(ID_OBJECTS_FILTER, &CBanzaiDoc::OnObjectsFilter)
  ON_COMMAND(ID_SURFACE_PICKTABLE, &CBanzaiDoc::OnSurfacePicktable)
  ON_COMMAND(ID_OBJECTS_PLANEFIT, &CBanzaiDoc::OnObjectsPlanefit)
  ON_COMMAND(ID_NAV_DEPTHFOV, &CBanzaiDoc::OnNavDepthfov)
  ON_COMMAND(ID_OBJECTS_SURFACEZOOM, &CBanzaiDoc::OnObjectsSurfacezoom)
  ON_COMMAND(ID_DEMO_STATICPOSE, &CBanzaiDoc::OnDemoStaticpose)
  ON_COMMAND(ID_SURFACE_HYBRIDSEG, &CBanzaiDoc::OnSurfaceHybridseg)
  ON_COMMAND(ID_OBJECTS_COLORPARAMS, &CBanzaiDoc::OnObjectsColorparams)
  ON_COMMAND(ID_SURFACE_COLOROBJS, &CBanzaiDoc::OnSurfaceColorobjs)
  ON_COMMAND(ID_UTILITIES_VALUERULES, &CBanzaiDoc::OnUtilitiesValuerules)
  ON_COMMAND(ID_VISUAL_DIST, &CBanzaiDoc::OnVisualDist)
  ON_COMMAND(ID_VISUAL_DIMSBINS, &CBanzaiDoc::OnVisualDimsbins)
  ON_COMMAND(ID_VISUAL_COLORFINDING, &CBanzaiDoc::OnVisualColorfinding)
  ON_COMMAND(ID_VISUAL_HUETHRESHOLDS, &CBanzaiDoc::OnVisualHuethresholds)
  ON_COMMAND(ID_VISUAL_PRIMARYCOLORS, &CBanzaiDoc::OnVisualPrimarycolors)
  ON_COMMAND(ID_VISUAL_OBJCOMPARISON, &CBanzaiDoc::OnVisualObjcomparison)
  ON_COMMAND(ID_PEOPLE_CHEST, &CBanzaiDoc::OnPeopleChest)
  ON_COMMAND(ID_PEOPLE_HEAD, &CBanzaiDoc::OnPeopleHead)
  ON_COMMAND(ID_PEOPLE_SHOULDER, &CBanzaiDoc::OnPeopleShoulder)
  ON_COMMAND(ID_ROOM_HEADTRACK, &CBanzaiDoc::OnRoomHeadtrack)
  ON_COMMAND(ID_UTILITIES_BATTERYFULL, &CBanzaiDoc::OnUtilitiesBatteryfull)
  ON_COMMAND(ID_OBJECTS_SHAPEBINS, &CBanzaiDoc::OnObjectsShapebins)
  ON_COMMAND(ID_ROOM_PERSONMAP, &CBanzaiDoc::OnRoomPersonmap)
  ON_COMMAND(ID_MOOD_ACTIVITYLEVEL, &CBanzaiDoc::OnMoodActivitylevel)
  ON_COMMAND(ID_MOOD_ENERGYLEVEL, &CBanzaiDoc::OnMoodEnergylevel)
  ON_COMMAND(ID_MOOD_INTERACTIONLEVEL, &CBanzaiDoc::OnMoodInteractionlevel)
  ON_COMMAND(ID_ROOM_ROBOTDIMS, &CBanzaiDoc::OnRoomRobotdims)
  ON_COMMAND(ID_SOCIAL_SACCADES, &CBanzaiDoc::OnSocialSaccades)
  ON_COMMAND(ID_MOTION_CALIBWRIST, &CBanzaiDoc::OnMotionCalibwrist)
  ON_COMMAND(ID_MOTION_CALIBARM, &CBanzaiDoc::OnMotionCalibarm)
  ON_COMMAND(ID_MANIPULATION_HANDEYE, &CBanzaiDoc::OnManipulationHandeye)
  ON_COMMAND(ID_MANIPULATION_GOTOVIA, &CBanzaiDoc::OnManipulationGotovia)
  ON_COMMAND(ID_MANIPULATION_ADJUSTZ, &CBanzaiDoc::OnManipulationAdjustz)
  ON_COMMAND(ID_MANIPULATION_MOVEOBJ, &CBanzaiDoc::OnManipulationMoveobj)
  ON_COMMAND(ID_OBJECTS_GRASPPOINT, &CBanzaiDoc::OnObjectsGrasppoint)
  ON_COMMAND(ID_OBJECTS_TRAJECTORYCTRL, &CBanzaiDoc::OnObjectsTrajectoryctrl)
  ON_COMMAND(ID_OBJECTS_MOTIONDONE, &CBanzaiDoc::OnObjectsMotiondone)
  ON_COMMAND(ID_DETECTION_GAZESURFACE, &CBanzaiDoc::OnDetectionGazesurface)
  ON_COMMAND(ID_SOCIAL_SURFFIND, &CBanzaiDoc::OnSocialSurffind)
  ON_COMMAND(ID_TABLE_SURFACE, &CBanzaiDoc::OnTableSurface)
  ON_COMMAND(ID_OBJECTS_SURFHEIGHT, &CBanzaiDoc::OnObjectsSurfheight)
  ON_COMMAND(ID_OBJECTS_SURFLOCATION, &CBanzaiDoc::OnObjectsSurflocation)
  ON_COMMAND(ID_MOTION_INVKINEMATICS, &CBanzaiDoc::OnMotionInvkinematics)
  ON_COMMAND(ID_OBJECTS_WORKSPACE, &CBanzaiDoc::OnObjectsWorkspace)
  ON_COMMAND(ID_MANIPULATION_DEPOSIT, &CBanzaiDoc::OnManipulationDeposit)
  ON_COMMAND(ID_OBJECTS_EMPTYSPOT, &CBanzaiDoc::OnObjectsEmptyspot)
  ON_COMMAND(ID_GRAB_BODYSHIFT, &CBanzaiDoc::OnGrabBodyshift)
  ON_COMMAND(ID_ROOM_FORKCALIB, &CBanzaiDoc::OnRoomForkcalib)
  ON_COMMAND(ID_UTILITIES_WEEDGRAMMAR, &CBanzaiDoc::OnUtilitiesWeedgrammar)
      END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBanzaiDoc construction/destruction

CBanzaiDoc::CBanzaiDoc()
{
  // JHC: move console and chat windows
  prt.SetTitle("ALIA console", 1);
  SetWindowPos(GetConsoleWindow(), HWND_TOP, 5, 5, 673, 1000, SWP_SHOWWINDOW);
  chat.Launch(1250, 505);                    // was 50 5 then 693 580

  // JHC: assume app called with command line argument fo file to open
  cmd_line = 1;
  strcpy_s(rname, "saved.bmp");

  // break out components
  eb = &(ec.body);

  // video parameters
  eb->BindVideo(&v);
  v.Shift = 2;

  // load configuration file(s)
  _getcwd(cwd, 200);
  sprintf_s(ifile, "%s\\Banzai_vals.ini", cwd);
  swing_params(ifile);
  interact_params(ifile);
  ec.Defaults(ifile);      // load defaults on start
}


// load parameters for testing single joint trajectories

int CBanzaiDoc::swing_params (const char *fname)
{
  jhcParam *ps = &jps;
  int ok;

  ps->SetTag("jt_swing", 0);
  ps->SetTitle("Pick joint movement parameters");
  ps->NextSpec4( &jnum,    1,   "Joint number");
  ps->NextSpecF( &acc,   180.0, "Acceleration (dps^2)");
  ps->NextSpecF( &slope,  10.0, "Servo slope (degs)");
  ps->NextSpecF( &start,  60.0, "Initial angle");
  ps->NextSpecF( &chg,   -90.0, "Angle change");
  ps->NextSpecF( &rate,    1.0, "Motion rate");

  ps->NextSpecF( &fchk,    3.0, "Motion lead factor");
  ps->NextSpecF( &gap,     0.3, "Time between swings");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


CBanzaiDoc::~CBanzaiDoc()
{
  if (cmd_line <= 0)
  {
    ips.SaveVals(ifile);
    ec.SaveVals(ifile);    // save defaults on exit
    jps.SaveVals(ifile);
  }
}


BOOL CBanzaiDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

  // change this value to non-zero for externally distributed code
  // cripple = -1 for full debugging
  //         =  0 for normal full control, expiration warning
  //         =  1 for restricted operation, expiration warning
  //         =  2 for restricted operation, expiration enforced
  cripple = 0;
  ver = ec.Version();
  LockAfter(4, 2023, 11, 2022);

  // JHC: if this function is called, app did not start with a file open
  // JHC: initializes display object which depends on document
  cmd_line = 0;
  if (d.Valid() <= 0)   
    d.BindTo(this);

	return TRUE;
}


// JHC: possibly run start up demo if called with command line file
// RunDemo is called from main application Banzai.cpp
// will have already called OnOpenDocument to set up video

void CBanzaiDoc::RunDemo()
{
  if (cmd_line <= 0)
    return;
  if (d.Valid() <= 0)   
    d.BindTo(this);

  // start autonomous interaction or exit program if disabled
  if (rob >= 2)
    OnDemoInteractive();
  else
    OnCloseDocument();       
}


// only allow demo code to run for a short while 
// make sure clock can't be reset permanently
// more of an annoyance than any real security

int CBanzaiDoc::LockAfter (int mon, int yr, int smon, int syr)
{
  char *tail;
  int cyr, cmon;

  // provide "backdoor" - override if directly in "jhc" directory
  if ((tail = strrchr(cwd, '\\')) != NULL)
    if (strcmp(tail, "\\jhc") == 0)
      if (cripple > 0)
        cripple = 0;

  // determine current month and year
  CTime today = CTime::GetCurrentTime();
  cyr = today.GetYear();
  cmon = today.GetMonth();

  // see if past expiration date (or before issue date)
  if ((cyr > yr)  || ((cyr == yr)  && (cmon > mon)) ||
      (cyr < syr) || ((cyr == syr) && (cmon < smon))) 
  {
    if (cripple > 1)
      Fatal("IBM Banzai %4.2f\nExpired as of %d/%d\njconnell@us.ibm.com",
            ver, mon, yr);
    Complain("IBM Banzai %4.2f\nOut-of-date as of %d/%d\njconnell@us.ibm.com", 
             ver, mon, yr);
  }
  return 1;
}


// what to do for functions that have been disabled

int CBanzaiDoc::LockedFcn ()
{
  if (cripple <= 0)
    return 0;
  Tell("Function not user-accessible in this version");
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
// CBanzaiDoc serialization

void CBanzaiDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


/////////////////////////////////////////////////////////////////////////////
// CBanzaiDoc diagnostics

#ifdef _DEBUG
void CBanzaiDoc::AssertValid() const
{
	CDocument::AssertValid();
}


void CBanzaiDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
//                               Video Source                              //
/////////////////////////////////////////////////////////////////////////////

// JHC: have to go into ClassWizard to override this function
// user has already chosen a file name so open it, init display
// this is also the function called by selecting from the MRU list

BOOL CBanzaiDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
  jhcString fn(lpszPathName);
  char alt[250];
  const char *start;
  char *ch = alt;

  // defeat command line argument at startup
  if (strchr("!?", fn.ch[fn.Len() - 1]) != NULL)
    return TRUE;
  
  // possibly convert text back from "safe" form in MRU list
  if ((start = strstr(fn.ch, "=> ")) != NULL) 
  {
    strcpy_s(alt, start + 3);
    while (*ch != '\0')
      if (*ch == ';')
        *ch++ = ':';
      else if (*ch == '|')
        *ch++ = '/';
      else
        ch++;
    fn.Set(alt);
  }

  // open source based on passed (modified) string
  d.Clear(1, "Configuring source ...");
	if (v.SetSource(fn.ch) <= 0)
    d.StatusText("");
  else
  {
    ShowFirst();	
    set_tilt(fn.ch);
  }
	return TRUE;
}


// JHC: use default video driver

void CBanzaiDoc::OnFileCamera() 
{
  jhcString mru;

  d.Clear(1, "Configuring camera ...");
  if (v.SetSource("*.dx") <= 0)  // was vfw
  {
    d.StatusText("");
    return;
  }
  sprintf_s(mru.ch, "C:/=> %s", v.File());
  mru.C2W();
  theApp.AddToRecentFileList(mru.Txt());
	ShowFirst();
}


// JHC: let user pick driver from menu and select all options

void CBanzaiDoc::OnFileCameraadjust() 
{
  jhcString mru;

  d.Clear(1, "Configuring camera ...");
  if (v.SetSource("*.dx+") <= 0)  // was vfw
  {
    d.StatusText("");
    return;
  }
  sprintf_s(mru.ch, "C:/=> %s", v.File());
  mru.C2W();
  theApp.AddToRecentFileList(mru.Txt());
  ShowFirst();
}


// Connect to combined color and depth sensor

void CBanzaiDoc::OnFileKinectsensor()
{
  jhcString mru;

  d.Clear(1, "Configuring Kinect sensor ...");
  if (v.SetSource("0.kin") <= 0)  
  {
    d.StatusText("");
    return;
  }
  sprintf_s(mru.ch, "C:/=> %s", v.File());
  mru.C2W();
  theApp.AddToRecentFileList(mru.Txt());
	ShowFirst();
}


// Get slow high-resolution color and fast depth 

void CBanzaiDoc::OnFileKinecthires()
{
  jhcString mru;

  d.Clear(1, "Configuring Kinect sensor ...");
  if (v.SetSource("0.kin2") <= 0)  
  {
    d.StatusText("");
    return;
  }
  sprintf_s(mru.ch, "C:/=> %s", v.File());
  mru.C2W();
  theApp.AddToRecentFileList(mru.Txt());
	ShowFirst();
}


// JHC: like normal Open but filters files for video types

void CBanzaiDoc::OnFileOpenvideo() 
{
  jhcString fname("");

  d.Clear(1, "Configuring video source ...");
  if (v.SelectFile(fname.ch, 500) <= 0)
  {
    d.StatusText("");
    return;
  }
  ShowFirst();
  fname.C2W();
  theApp.AddToRecentFileList(fname.Txt());
  set_tilt(fname.ch);
}


// Set default neck tilt angle based on file name

void CBanzaiDoc::set_tilt (const char *fname) 
{
  const char *sep;
  char tilt[10];
  int deg10;

  if ((sep = strstr(fname, "_t")) != NULL)
  {
    strncpy_s(tilt, sep + 2, ((sep[2] == '-') ? 4 : 3));
    if (sscanf_s(tilt, "%d", &deg10) == 1)
      eb->tdef = -0.1 * deg10;
  }
}


// JHC: let user type a file name, wildcard pattern, or vfw spec
// e.g. repeat foo.ras, show all *.bmp, choose Bt848\*.vfw
//      http://62.153.249.21/live/wildpark/live300k.asx

void CBanzaiDoc::OnFileOpenexplicit() 
{
  jhcString mru;
  char *ch = mru.ch + 6;

  // ask user for text then try opening source
  d.Clear(1, "Configuring video source ...");
  if (v.AskSource() <= 0)
  {
    d.StatusText("");
    return;
  }
  ShowFirst();

  // convert text into "safe" form for MRU list
  sprintf_s(mru.ch, "C:/=> %s", v.File());
  while (*ch != '\0')
    if (*ch == ':')
      *ch++ = ';';
    else if (*ch == '/')
      *ch++ = '|';
    else
      ch++;
  mru.C2W();
  theApp.AddToRecentFileList(mru.Txt());
}


/////////////////////////////////////////////////////////////////////////////
//                              Video Utilities                            //
/////////////////////////////////////////////////////////////////////////////

// JHC: show first frame of a new video source

void CBanzaiDoc::ShowFirst()
{
  jhcImg col, d8;

  // make sure some stream and set size for receiving images
  if (!v.Valid())
    return;
  eb->BindVideo(&v);

  // adjust pause and playback for list of images
  v.PauseNum = 0;
  if (v.IsClass("jhcListVSrc") > 0)
  {
    v.PauseNum = 1;
    v.DispRate = 0.001;
  }

  // try to read images
  v.Rewind();
  eb->UpdateImgs();
  v.Rewind();

  // get and show pretty color
  eb->SmallSize(col);
  eb->ImgSmall(col);
  d.Clear();
  d.ShowGrid(col, 0, 0, 0, v.Name());
  if (v.Dual() > 0)
  {
    // depth also
    eb->DepthSize(d8);
    eb->Depth8(d8);
    d.ShowGrid(d8, 1, 0, 0, "Depth");
  }
  d.StatusText("Ready");
}


// JHC: ask user for start, step. rate, etc. 

void CBanzaiDoc::OnParametersVideocontrol() 
{
  d.StatusText("Configuring video source ...");
  if (v.AskStep() <= 0)
    d.StatusText("");
  else
    ShowFirst();
}


// JHC: ask user for sizes and whether monochrome

void CBanzaiDoc::OnParametersImagesize() 
{
  d.StatusText("Configuring video source ...");
  if (v.AskSize() <= 0)
    d.StatusText("");
  else
    ShowFirst();
}


// JHC: see if video source is valid, if not try opening camera

int CBanzaiDoc::ChkStream (int dual)
{
  if (v.Valid() && ((dual <= 0) || v.Dual()))
    return 1;
  d.StatusText("Configuring Kinect sensor ...");
  v.noisy = ((cmd_line > 0) ? 0 : 1);
  if (v.SetSource("0.kin") <= 0)
  {
    d.StatusText("");
    return 0;
  }
  v.PauseNum = 0;
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                              Video Playback                             //
/////////////////////////////////////////////////////////////////////////////

// JHC: make up proper return image then start playback
// can use this as a template for all application functions

void CBanzaiDoc::OnTestPlayvideo() 
{
  if (ChkStream(0) <= 0)
    return;

  // get correct image sizes
  eb->BindVideo(&v);

  // loop over selected set of frames  
  d.Clear(1, "Live color image ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get frame, pause if right mouse click
      if (eb->UpdateImgs() <= 0)
        break;

      // show frame on screen
      d.ShowGrid(eb->Color(), 0, 0, 0, "%d: %s  --  Color", v.Last(), v.FrameName());
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
  res.Clone(eb->Color());	
  sprintf_s(rname, "%s_color.bmp", v.FrameName());
}


// Play just the depth images from a dual source

void CBanzaiDoc::OnUtilitiesPlaydepth()
{
  jhcImg d8;

  if (ChkStream() <= 0)
    return;

  // get correct image sizes
  eb->BindVideo(&v);
  eb->DepthSize(d8);

  // loop over selected set of frames  
  d.Clear(1, "Live depth image ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get frame, pause if right mouse click
      if (eb->UpdateImgs() <= 0)
        break;
      eb->Depth8(d8);

      // show frame on screen
      d.ShowGrid(d8, 0, 0, 0, "%d: %s  --  Depth", v.Last(), v.FrameName());
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
  res.Clone(d8);	
  sprintf_s(rname, "%s_depth.bmp", v.FrameName());
}


// Play both color and depth from current source

void CBanzaiDoc::OnUtilitiesPlayboth()
{
  jhcImg d8;

  if (ChkStream() <= 0)
    return;

  // get correct image sizes
  eb->BindVideo(&v);
  eb->DepthSize(d8);

  // loop over selected set of frames  
  d.Clear(1, "Color and depth ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get frame, pause if right mouse click
      if (eb->UpdateImgs() <= 0)
        break;
      eb->Depth8(d8);

      // show frame on screen
      d.ShowGrid(eb->Color(), 0, 0, 0, "%d: %s", v.Last(), v.FrameName());
      d.ShowGrid(d8, 1, 0, 0, "Depth");
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
}


/////////////////////////////////////////////////////////////////////////////
//                               Saving Images                             //
/////////////////////////////////////////////////////////////////////////////

// JHC: save last displayed image at user selected location
// should generally change this to some more meaningful image

void CBanzaiDoc::OnFileSaveAs() 
{
  jhcString sel, idir, rn(rname);
  CFileDialog dlg(FALSE, NULL, rn.Txt());  
  jhcImgIO fio;
  jhcImg d8;

  // show result first
  if (!res.Valid())
    return;
  d.Clear();
  d.ShowGrid(res, 0, 0, 0, "Last result");

  // pop standard file choosing box starting at results directory
  sprintf_s(idir.ch, "%s\\results", cwd);
  idir.C2W();
  (dlg.m_ofn).lpstrInitialDir = idir.Txt();
  if (dlg.DoModal() != IDOK)
    return;

  // show main image
  sel.Set((dlg.m_ofn).lpstrFile);
  fio.Save(sel.ch, res, 1);
  d.ShowGrid(res, 0, 0, 0, "Saved as %s", fio.Name());
}


// Save most recent sensor inputs

void CBanzaiDoc::OnFileSavesource()
{
  jhcString sel, idir, init("situation.bmp");
  CFileDialog dlg(FALSE);
  jhcImgIO fio;
  jhcName name;
  jhcImg col, d8;

  // get pretty images from Celia body
  eb->SmallSize(col);
  eb->DepthSize(d8);
  eb->ImgSmall(col);
  eb->Depth8(d8);

  // show result first
  d.Clear();
  d.ShowGrid(col, 0, 0, 0, "Last input");
  d.ShowGrid(d8,  1, 0, 0, "Depth");

  // append tilt angle
  sprintf_s(init.ch, "situation_t%d.bmp", ROUND(-10.0 * (eb->neck).Tilt()));
  init.C2W();
  

  // pop standard file choosing box starting at environ directory
  sprintf_s(idir.ch, "%s\\environ", cwd);
  idir.C2W();
  (dlg.m_ofn).lpstrInitialDir = idir.Txt();
  (dlg.m_ofn).lpstrFile = init.Txt();
  if (dlg.DoModal() != IDOK)
    return;
  sel.Set((dlg.m_ofn).lpstrFile);
  name.ParseName(sel.ch);

  // save raw images then indicate name for each
  fio.SaveDual(name.File(), eb->Color(), eb->Range());
  d.ShowGrid(col, 0, 0, 0, "Saved as: %s", name.Name());
  d.ShowGrid(d8,  1, 0, 0, "Saved as: %s", fio.Name());
}


/////////////////////////////////////////////////////////////////////////////
//                            Motion Profiling                             //
/////////////////////////////////////////////////////////////////////////////

// Motion profiling for change of gaze direction

void CBanzaiDoc::OnRampNeckramp()
{
	jhcPickVals dlg;

  dlg.EditParams((eb->neck).rps); 
}


// Trapezoidal profiling for lift stage

void CBanzaiDoc::OnParametersLiftramp()
{
	jhcPickVals dlg;

  dlg.EditParams((eb->lift).fps); 
}


// Trapezoidal profiling for arm positioning

void CBanzaiDoc::OnProfilingArmramp()
{
	jhcPickVals dlg;

  dlg.EditParams((eb->arm).tps); 
}


// Speed of finger motion and force adjustment step

void CBanzaiDoc::OnParametersGrabramp()
{
	jhcPickVals dlg;

  dlg.EditParams((eb->arm).fps); 
}


// Set parameters for joint angle search

void CBanzaiDoc::OnMotionInvkinematics()
{
	jhcPickVals dlg;

  dlg.EditParams((eb->arm).ips); 
}


// Trapezoidal profiling for translation and rotation

void CBanzaiDoc::OnParametersBaseramp()
{
	jhcPickVals dlg;

  dlg.EditParams((eb->base).mps); 
}


/////////////////////////////////////////////////////////////////////////////
//                            Ballistic Grounding                          //
/////////////////////////////////////////////////////////////////////////////


// Set battery notification level, drop detection, etc. 

void CBanzaiDoc::OnParametersBatterylevel()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).eps); 
}


// Parameters for changing gaze direction of head

void CBanzaiDoc::OnGroundingNeckcmd()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).nps);
}


// How to interpret words in fork lift stage commands

void CBanzaiDoc::OnParametersLiftcmd()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).lps); 
}


// How to interpret words in translation commands

void CBanzaiDoc::OnParametersMovecmd()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).tps); 
}


// How to interpret words in rotation commands

void CBanzaiDoc::OnParametersTurncmd()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).rps); 
}


// How to determine if a translation or rotation has failed

void CBanzaiDoc::OnParametersBaseprogress()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).pps); 
}


// Set pose for extend arm and hand

void CBanzaiDoc::OnParametersArmhome()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).aps); 
}


// Parameters for changing position of hand

void CBanzaiDoc::OnGroundingHandcmd()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).hps);
}


// Parameters for changing orientation of hand

void CBanzaiDoc::OnGroundingWristcmd()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).wps);
}


// Parameters for holding and releasing objects

void CBanzaiDoc::OnParametersGrabcmd()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.ball).gps); 
}


/////////////////////////////////////////////////////////////////////////////
//                         Application Parameters                          //
/////////////////////////////////////////////////////////////////////////////

// save parameters to file

void CBanzaiDoc::OnParametersSavedefaults() 
{
  jhcString sel, init(ifile), idir(cwd);
  CFileDialog dlg(FALSE, NULL, init.Txt());

  (dlg.m_ofn).lpstrInitialDir = idir.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Initialization Files\0*.ini\0Text Files\0*.txt\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() == IDOK)
  {
    sel.Set((dlg.m_ofn).lpstrFile);   
    ips.SaveVals(sel.ch);
    ec.SaveVals(sel.ch);            // change to relevant object
    jps.SaveVals(sel.ch); 
  }
}


// load parameters from file

void CBanzaiDoc::OnParametersLoaddefaults() 
{
  jhcString sel, init(ifile), idir(cwd);
  CFileDialog dlg(TRUE, NULL, init.Txt());

  (dlg.m_ofn).lpstrInitialDir = idir.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Initialization Files\0*.ini\0Text Files\0*.txt\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() == IDOK)
  {
    sel.Set((dlg.m_ofn).lpstrFile);
    interact_params(sel.ch);
    ec.Defaults(sel.ch);            // change to relevant object
    swing_params(sel.ch);
  }
}


/////////////////////////////////////////////////////////////////////////////
//                               Arm Functions                             //
/////////////////////////////////////////////////////////////////////////////

// Pick joint and rate as well as start and end angles for swing test 

void CBanzaiDoc::OnArmSwingparams()
{
	jhcPickVals dlg;

  dlg.EditParams(jps);
}


// Move some joint back and forth to set acceleration

void CBanzaiDoc::OnArmSwingjoint()
{
  jhcEliArm *arm = &(eb->arm);
	jhcPickVals dlg;
  jhcMatrix a0(7), a(7);
  jhcMatrix *goal;
//  jhcArr targ, err;
  jhcArr pos, vel;
  DWORD t0;
  double mt;
  int w0 = d.gwid, h0 = d.ght, ms = 33, cyc = 4, i0 = -1; 
  int sz, n, mn, gn, top, bot, stop, j, i;
//  int top2, bot2;

  // start up robot
  d.Clear(1, "Swinging joint ...");
  d.ResetGrid(3, 640, 200);
  d.StringGrid(0, 0, "Initializing robot ...");  
  if (eb->Reset(1) <= 0)
    if (AskNot("Problem with robot hardware. Continue?") <= 0)
      return;
  eb->InitPose();
  jnum = __max(0, __min(jnum, 7));

  // change to starting angle
  d.ClearGrid(0, 0);
  d.StringGrid(0, 0, "Setting start angle ...");
  arm->ArmConfig(a0);
  a0.VSet(jnum, start);
  arm->SetConfig(a0);
  
  // adjust controller parameters
  (arm->jt[jnum]).astd = acc;
  (arm->jt[jnum]).SetStiff(slope);

  // see how long move is expected to take
  a.Copy(a0);
  a.VInc(jnum, chg);
  mt = arm->CfgTime(a, a0, rate);
  mn = ROUND(0.5 * 1000.0 * mt / ms);
  gn = ROUND(0.5 * 1000.0 * gap / ms);
  n = mn + gn;

  // build performance arrays
  sz = cyc * n;
//  targ.InitSize(sz);
//  err.InitSize(sz);
  pos.InitSize(sz);
  vel.InitSize(sz);
  
  // loop over selected set of frames  
  d.ResetGrid();
  d.gwid = 1000;
  t0 = jms_now();
  try
  {
    while (d.AnyHit() == 0)
    {
      // get current sensor values
      if (eb->Update(0, 0) <= 0)
        break;

      // figure out elapsed time
      i = ROUND(0.5 * (jms_now() - t0) / (double) ms);
      if (i >= sz)
        break;

      // generate move command 
      if (((i / n) & 0x01) == 0)
        goal = &a;
      else
        goal = &a0;
      arm->CfgTarget(*goal, rate);

      // get command vs. actual position
      for (j = i0 + 1; j <= i; j++)
      {
//        targ.ASet(j, ROUND(100.0 * arm->CtrlPos(jnum)));
//        err.ASet( j, ROUND(100.0 * (arm->JtAng(jnum) - (arm->jt[jnum]).RampPos())));
        pos.ASet( j, ROUND(100.0 * arm->JtAng(jnum)));
        vel.ASet( j, ROUND(100.0 * arm->CtrlVel(jnum)));
      }

      // check error in stop regions
      stop = 0;
/*
      for (j = 0; j < sz; j++)
        if ((j % n) > mn)
          stop = __max(stop, abs(err.ARef(j)));
*/

      // graph target and response
      d.ght = 360;
      top = pos.MaxVal();
//      top2 = targ.MaxVal();
//      top = __max(top, top2);
      bot = pos.MinVal();
//      bot2 = targ.MinVal();
//      bot = __min(bot, bot2);
      if (bot < 0)
        top = __min(-top, bot);
      d.GraphGrid(pos, 0, 0, top, 5, "%s  --  Command vs. actual position ", arm->JtName(jnum));
      d.GraphVal(0, top);
//      d.GraphOver(targ, top, 4);
      for (j = 1; j <= cyc; j++)
      {
        d.GraphMark(j * n, 2);
        d.GraphMark(j * n - gn);
      }
/*
      // graph position error and 1 degree line
      d.ght = 120;
      top = err.MaxAbs();
      top = __max(100, top);
      d.GraphGrid(err, 0, 1, -top, 5, "Position error (max = %4.2f, final = %4.2f degs)", 
                                      0.01 * top, 0.01 * stop);
      d.GraphVal(100, -top, 8);
      d.GraphVal(0, -top, 8);
      d.GraphVal(-100, -top, 8);
      for (j = 1; j <= cyc; j++)
      {
        d.GraphMark(j * n, 2);
        d.GraphMark(j * n - gn);
      }
*/
      // graph trajectory velocity
      d.ght = 120;
      top = vel.MaxAbs();
      d.GraphBelow(vel, -top, 4, "Command velocity (%4.2f secs)", mt); 
      d.GraphVal(0, -top);
      for (j = 1; j <= cyc; j++)
      {
        d.GraphMark(j * n, 2);
        d.GraphMark(j * n - gn);
      }

      // send command
      eb->Issue(fchk);
      jms_sleep(ms - 8);
      i0 = i;
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.gwid = w0;
  d.ght = h0;
  d.StatusText("Stopped.");
}


// Establish target pose then achieve it form various starting points

void CBanzaiDoc::OnArmGotopose()
{
  jhcEliArm *arm = &(eb->arm);
  jhcMatrix a0(7), a(7), a2(7), rates(6), err(6);
  char diff[80], goal[80], start[80];
  double mt0, mt, t, tol = 2.0;
  int hit, n, ms = 33, state = 1; 

  // start up robot
  d.Clear(1, "Pose trajectories ...");
  d.ResetGrid(3, 640, 240);
  d.StringGrid(0, 0, "0: Initializing robot ...");  
  if (eb->Reset(1) <= 0)
    if (AskNot("Problem with robot hardware. Continue?") <= 0)
      return;
  (eb->neck).Freeze();
  arm->Limp();

  // loop over selected set of frames  
  eb->Beep();
  try
  {
    while (1)
    {
      // check for user interactions the get current sensor values
      if ((hit = d.AnyHit()) != 0)
      {
        if ((hit <= -3) || (state == 5))   // quit
          break;
        if (state < 6)                     // cycle 3-6
          state++;
        else
          state = 3;
      }
      if (eb->Update(0, 0) <= 0)
        break;

      // figure out what to do
      if (state <= 1)
      {
        // manhandle arm to goal
        arm->Limp();
        arm->ArmConfig(a0);
        a0.ListVec(goal, "%5.1f", 80);
        d.StringGrid(0, 0, "%d: Move arm to goal pose -- Hit any key to continue ...", state);
        d.StringBelow("Goal = %s", goal);
      }
      else if (state == 2)
      {
        // remember goal pose
        arm->CfgTarget(a0);
        a0.ListVec(goal, "%5.1f", 80);
        d.StringGrid(0, 0, "%d: Frozen at goal -- Hit any key to continue ...", state);
        d.StringBelow("Goal = %s", goal);
      }
      else if (state == 3)
      {
        // manhandle arm to start
        arm->Limp();
        arm->ArmConfig(a);
        a.ListVec(start, "%5.1f", 80);
        d.StringGrid(0, 0, "%d: Shift arm to start position -- Hit any key to continue ...", state);
        d.StringBelow("Start = %s", start);
      }
      else if (state == 4)
      {
        // remember start and pick joint rates to achieve goal pose
        n = 0;
        arm->CfgTarget(a);
        a.ListVec(start, "%5.1f", 80);
        mt0 = arm->CfgTime(a0, a);
        arm->CfgRate(rates, a0, a, mt0);
        mt = arm->CfgTime(a0, a, rates);
        d.StringGrid(0, 0, "%d: Frozen at start -- Hit any key to continue ...", state);
        d.StringBelow("Start = %s", start);
        d.StringBelow("Expect %4.2f seconds to goal", mt);
      }
      else if (state == 5)
      {
        // if not done then drive toward goal
        t = 0.001 * ms * n++;
        arm->ArmConfig(a2);
        arm->CfgErr(err, a0, 0);
        err.ListVec(diff, "%5.1f", 80); 
        if ((arm->CfgOffset(a0) <= tol) || (t > (10.0 * mt)))
          state++;
        else
          arm->CfgTarget(a0, rates);
        d.StringGrid(0, 0, "%d: Moving to goal -- Hit any key to EXIT ...", state);
        d.StringBelow("Joint errors: %s", diff);
        d.StringBelow("Elapsed %4.2f seconds -- %4.2f expected", t, mt);
      }
      else 
      {
        // goal achieved
        arm->CfgTarget(a2);
        d.StringGrid(0, 0, "%d: Finished with goal -- Hit any key to continue ...", state);
        d.StringBelow("Joint errors: %s", diff);
        d.StringBelow("Elapsed %4.2f seconds -- %4.2f expected", 0.001 * ms * n, mt);
      }

      // send command
      eb->Issue();
      jms_sleep(ms);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
}


/////////////////////////////////////////////////////////////////////////////
//                             Force Functions                             //
/////////////////////////////////////////////////////////////////////////////

// Show direction and magnitude of endpoint force

void CBanzaiDoc::OnArmHandforce()
{
  jhcEliArm *arm = &(eb->arm);
  jhcImg fxy, fz;
  jhcMatrix a0(7), fraw(4), fdir(4);
  double rng = 16.0, pel = 0.1, box = 4.0, z0 = -10.0, ang = 30.0;
  int mid, k, th, ht, y, dot = 13, wid = 70, ms = 33;

  // make up display images
  mid = ROUND(rng / pel);
  k = 2 * mid;
  fxy.SetSize(k, k);
  fz.SetSize(wid, k);
  th = ROUND(2.0 * box / pel);

  // start up robot
  d.Clear(1, "Hand forces ...");
  d.ResetGrid(3, 320, 50);
  d.StringGrid(0, 0, "Initializing robot ...");  
  if (eb->Reset(1) <= 0)
    if (AskNot("Problem with robot hardware. Continue?") <= 0)
      return;

  // change to starting angle
  d.ClearGrid(0, 0);
  d.StringGrid(0, 0, "Pointing arm forward ...");
  eb->InitPose();
  arm->ArmConfig(a0);
  a0.VSet(1, ang);
  arm->SetConfig(a0);
  
  // loop over selected set of frames  
  eb->Beep();
  try
  {
    while (d.AnyHit() == 0)
    {
      // get current sensor values
      if (eb->Update(0, 0) <= 0)
        break;
      arm->ForceVect(fraw, z0, 1);
      arm->ForceVect(fdir, z0);

      // build xy force display image with axes and dot (raw is cross)
      fxy.FillArr(255);
      DrawLine(fxy, mid, 0, mid, k, 3, 0);
      DrawLine(fxy, 0, mid, k, mid, 3, 0);
      RectCent(fxy, mid, mid, th, th, 0.0, 1, 0);
      BlockCent(fxy, mid + ROUND(fdir.X() / pel), mid + ROUND(fdir.Y() / pel), dot, dot, 50);
      Cross(fxy, mid + fraw.X() / pel, mid + fraw.Y() / pel, dot, dot, 1, 215); 

      // build z force display image with bar and mid line (raw is cross)
      fz.FillArr(255);
      ht = ROUND(fdir.Z() / pel);
      if (fdir.Z() >= 0.0)
        RectFill(fz, 0, mid, wid, ht, 128);
      else
        RectFill(fz, 0, mid + ht, wid, -ht, 200);
      DrawLine(fz, 0, mid, wid, mid, 3, 0);
      y = mid + ROUND(box / pel);
      DrawLine(fz, 0, y, wid, y, 1, 0);
      y = mid - ROUND(box / pel);
      DrawLine(fz, 0, y, wid, y, 1, 0);
      Cross(fz, 0.5 * wid, mid + fraw.Z() / pel, dot, dot, 1, 215);
      
      // show forces
      d.ShowGrid(fxy, 0, 0, 2, "Force:  X = %5.1f    Y = %5.1f", fdir.X(), fdir.Y());
      d.ShowGrid(fz,  1, 0, 2, "wt = %4.1f", -fdir.Z());
      d.StringBelow("Z = %5.1f", fdir.Z() + z0);

      // maintain pose
      arm->ArmConfig(a0);
      eb->Issue();
      jms_sleep(ms);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
}


// Pose arm by pushing on hand

void CBanzaiDoc::OnForceDraghand()
{
  jhcEliArm *arm = &(eb->arm);
  jhcMatrix fdir(4), mv(4);
  double z0 = -10.0, xyth = 4.0, zth = 4.0;
  double xymv = 2.0, zmv = 2.0;
  int ms = 33;

  // start up robot
  d.Clear(1, "Drag arm ...");
  d.ResetGrid(3, 320, 50);
  d.StringGrid(0, 0, "Initializing robot ...");  
  if (eb->Reset(1) <= 0)
    if (AskNot("Problem with robot hardware. Continue?") <= 0)
      return;

  // change to starting angle
  d.ClearGrid(0, 0);
  d.StringGrid(0, 0, "Assuming neutral pose ...");
  eb->InitPose();

  // loop over selected set of frames  
  eb->Beep();
  try
  {
    while (d.AnyHit() == 0)
    {
      // get current sensor values
      if (eb->Update(0, 0) <= 0)
        break;
      arm->ForceVect(fdir, z0);

      // move in direction of pull
      mv.Zero();
      if (fabs(fdir.X()) > xyth)
        mv.SetX((fdir.X() > 0.0) ? xymv : -xymv);
      if (fabs(fdir.Y()) > xyth)
        mv.SetY((fdir.Y() > 0.0) ? xymv : -xymv);
      if (fabs(fdir.Z()) > zth)
        mv.SetZ((fdir.Z() > 0.0) ? zmv : -zmv);
        
      // list forces
      d.ClearGrid(0, 0);
      d.StringGrid(0, 0, "Move arm  X = %3.1f  :  Y = %3.1f  :  Z = %3.1f", fdir.X(), fdir.Y(), fdir.Z());

      // maintain pose
      arm->ShiftTarget(mv, 0.5);
      eb->Issue();
      jms_sleep(ms);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
}


// Steer base by pulling on hand

void CBanzaiDoc::OnForceDragrobot()
{
  jhcEliArm *arm = &(eb->arm);
  jhcEliLift *lift = &(eb->lift);
  jhcEliBase *base = &(eb->base);
  jhcMatrix a0(7), fdir(4);
  double rrate, trate, zrate, dz, trans, rot;
  double sh = 15.0, elb = 30.0, z0 = -10.0, xyth = 4.0, zth = 4.0;
  double xyrng = 32.0, zrng = 48.0, mv = 6.0, turn = 15.0, elev = 1.0;
  int ms = 33;

  // start up robot
  d.Clear(1, "Drag robot ...");
  d.ResetGrid(3, 320, 50);
  d.StringGrid(0, 0, "Initializing robot ...");  
  if (eb->Reset(1) <= 0)
    if (AskNot("Problem with robot hardware. Continue?") <= 0)
      return;

  // change to starting angle
  d.ClearGrid(0, 0);
  d.StringGrid(0, 0, "Pointing arm forward ...");
  eb->InitPose();
  arm->ArmConfig(a0);
  a0.VSet(0, sh);
  a0.VSet(1, elb);
  arm->SetConfig(a0);

  // loop over selected set of frames  
  eb->Beep();
  try
  {
    while (d.AnyHit() == 0)
    {
      // get current sensor values
      if (eb->Update(0, 0) <= 0)
        break;
      arm->ForceVect(fdir, z0);

      // move in direction of pull
      dz = 0.0;

      // turn proportional to pull
      rot = ((fdir.X() > 0.0) ? -turn : turn);
      rrate = fabs(fdir.X()) / xyrng;

      // drive proportional to pull
      trans = ((fdir.Y() > 0.0) ? mv : -mv);
      trate = fabs(fdir.Y()) / xyrng;

      // lift proportional to pull (with deadband and mode switching)
      dz = 0.0;
      zrate = 1.0;
      if ((fabs(fdir.Z()) > zth) && (fabs(fdir.X()) <= xyth) && (fabs(fdir.Y()) <= xyth))
      {
        dz = ((fdir.Z() > 0.0) ? elev : -elev);
        zrate = (fabs(fdir.Z()) - zth) / zrng;
      }
        
      // list forces
      d.ClearGrid(0, 0);
      d.StringGrid(0, 0, "Move robot  X = %3.1f  :  Y = %3.1f  :  Z = %3.1f", fdir.X(), fdir.Y(), fdir.Z());

      // maintain pose
      lift->LiftShift(dz, 0.5);
      base->TurnTarget(rot, rrate);
      base->MoveTarget(trans, trate);
      eb->Issue();
      jms_sleep(ms);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
}


/////////////////////////////////////////////////////////////////////////////
//                           Animation Functions                           //
/////////////////////////////////////////////////////////////////////////////

// Force a robot hardware reset but no motion

void CBanzaiDoc::OnDemoResetrobot()
{
  // bind video
  d.Clear(1);
  d.StringGrid(0, 0, "Reset robot");
  if ((cam > 0) && (ChkStream() > 0))
    eb->BindVideo(&v);
  else
    eb->BindVideo(NULL);

  // start up robot
  d.StatusText("Initializing robot ...");
  if (eb->Reset(1, 1) <= 0)
    Complain("Problem with robot hardware");
  else
    Tell("Robot %d hardware reset", eb->BodyNum());
  d.StatusText("Done");  
}


// Assume default pose for all actuators
// also adjusts to canonical height

void CBanzaiDoc::OnAnimationNeutral()
{
  d.Clear(1);
  d.StatusText("Initializing pose ...");
  if (eb->Reset(1, 1) <= 0)
    Complain("Problem with robot hardware");
  else if (eb->InitPose() <= 0)
    Complain("Could not achieve starting pose");
  else
    Tell("Robot in standard pose");
  d.StatusText("Done");  
}


// Set all arm servos to passive

void CBanzaiDoc::OnArmLimp()
{
  // bind video
  d.Clear(1);
  d.StringGrid(0, 0, "Passive actuators");
  eb->Limp();
  Tell("All actuators powered off");
  d.StatusText("Done");  
}


// Robot is fully charged so recalibrate battery capacity

void CBanzaiDoc::OnUtilitiesBatteryfull()
{
  int n;

  if ((n = (ec.body).ResetVmax()) > 0)
    Tell("Robot %d battery capacity will be recalibrated on next run", n);
  else
    Complain("Robot body currently unknown");
}


// Show simple evidence of power on status

void CBanzaiDoc::OnAnimationIdle()
{
  double wid, mid = 1.5, dev = 0.4, tol = 0.2, rate = 0.2;  
  double tilt, tilt0 = -40.0, nod = 10.0, nrate = 0.2;
  int state = 0, ms = 33;

  // start up robot
  d.Clear(1, "Idle animation ...");
  d.ResetGrid(3, 640, 200);
  d.StringGrid(0, 0, "Initializing robot ...");  
  if (eb->Reset(1) <= 0)
    if (AskNot("Problem with robot hardware. Continue?") <= 0)
      return;
  eb->InitPose();

  // loop over selected set of frames  
  try
  {
    while (d.AnyHit() == 0)
    {
      // get current sensor values
      d.StringGrid(0, 0, "Breathing  --  Click left to exit ...");
      if (eb->Update(0, 0) <= 0)
        break;

      if (state <= 0)
      {
        // actually open the gripper
        wid = mid + dev;
        tilt = tilt0 + nod;
        (eb->arm).WidthTarget(wid, rate, 5);
        (eb->neck).TiltTarget(tilt, nrate, 5);
        if ((eb->arm).WidthErr(wid) < tol)
          state = 1;
      }
      else 
      {
        // actually close gripper
        wid = mid - dev;
        tilt = tilt0;
        (eb->arm).WidthTarget(wid, rate, 5);
        (eb->neck).TiltTarget(tilt, nrate, 5);
        if ((eb->arm).WidthErr(wid) < tol)
          state = 0;
      }

      // send command
      eb->Issue();
      jms_sleep(ms);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
  d.Clear();
}


/////////////////////////////////////////////////////////////////////////////
//                          Internal Modulation                            //
/////////////////////////////////////////////////////////////////////////////

// Adjust thresholds for boredom and overwhelmed

void CBanzaiDoc::OnMoodActivitylevel()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.mood).bps); 
}


// Adjust thresholds for loneliness

void CBanzaiDoc::OnMoodInteractionlevel()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.mood).sps); 
}


// Adjust threshold for tiredness

void CBanzaiDoc::OnMoodEnergylevel()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.mood).tps); 
}


/////////////////////////////////////////////////////////////////////////////
//                         Linguistic Interaction                          //
/////////////////////////////////////////////////////////////////////////////

// parameters for overall interaction

int CBanzaiDoc::interact_params (const char *fname)
{
  jhcParam *ps = &ips;
  int ok;

  ps->SetTag("banzai_opt", 0);
  ps->NextSpec4( &cam,        0, "Camera available");
  ps->NextSpec4( &rob,        0, "Body (none, yes, autorun)");
  ps->NextSpec4( &(ec.spin),  0, "Speech (no, w7, web, w11)");  
  ps->NextSpec4( &(ec.amode), 2, "Attn (none, ends, front, only)");
  ps->NextSpec4( &(ec.tts),   0, "Vocalize output");
  ps->NextSpec4( &fsave,      0, "Face model update");

  ps->NextSpec4( &(ec.vol),   1, "Load baseline volition");
  ps->NextSpec4( &(ec.acc),   0, "Skills (none, load, update)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


// Configure for voice, camera, etc.

void CBanzaiDoc::OnDemoDemooptions()
{
	jhcPickVals dlg;

  dlg.EditParams(ips); 
}


// Adjust attention timing and thought rate 

void CBanzaiDoc::OnDemoAttn()
{
	jhcPickVals dlg;

  dlg.EditParams(ec.tps); 
}


// Neck angles and lift height to use if no physical robot

void CBanzaiDoc::OnDemoStaticpose()
{
	jhcPickVals dlg;

  dlg.EditParams(eb->sps); 
}


// Read successive inputs from a text file

void CBanzaiDoc::OnDemoTextfile()
{
  jhcString sel, test;
  CFileDialog dlg(TRUE);
  HWND me = GetForegroundWindow();
  char in[200] = "";
  const jhcImg *icam = NULL;
  FILE *f;
  int sp0 = ec.spin;

  // select file to read 
  sprintf_s(test.ch, "%s\\test\\trial.tst", cwd);
  test.C2W();
  (dlg.m_ofn).lpstrFile = test.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Test Files\0*.tst\0Text Files\0*.txt\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() != IDOK)
    return;
  sel.Set((dlg.m_ofn).lpstrFile);    
  if (fopen_s(&f, sel.ch, "r") != 0)
    return;

  // possibly check for video
  if ((cam > 0) && (ChkStream() > 0))
    (ec.body).BindVideo(&v);
  else
    (ec.body).BindVideo(NULL);

  // reset all required components
  system("cls");
  jprintf_open();
  ec.spin = 0;
  d.StatusText("Initializing robot ...");
  if (ec.Reset(rob) <= 0)
  {
    jprintf_close();
    return;
  }
  ec.SetPeople("VIPs.txt");
  chat.Reset(0, "log");
  SetForegroundWindow(chat);
  if (next_line(in, 200, f))
    chat.Inject(in);

jtimer_clr();
  // keep taking sentences until ESC
  d.Clear(1, "File input (ESC to quit) ...");
  d.ResetGrid(0, 640, 480);
  d.StringGrid(0, 0, ">>> NO IMAGES <<<");
#ifndef _DEBUG
  try
#endif
  {
    while (chat.Interact() >= 0)
    {
      // check for keystroke input 
      if (ec.Accept(chat.Get(in), chat.Done()))
        if (next_line(in, 200, f))
          chat.Inject(in);

      // compute response
      if (ec.Respond() <= 0)
        break;

      // show interaction
      if ((ec.body).NewFrame())
      {
        d.ShowGrid((ec.rwi).HeadView(), 0, 0, 0, "Visual attention (%3.1f\")", ((ec.rwi).tab).ztab);
        d.ShowGrid((ec.rwi).MapView(),  1, 0, 2, "Overhead navigation map  %s", (ec.rwi).NavGoal());
      }
      (ec.stat).Memory(&d, 0, 1, ec.Sensing());
      chat.Post(ec.NewInput(), 1);
      chat.Post(ec.NewOutput());
    }
  }
#ifndef _DEBUG
  catch (...){Tell("Unexpected exit!");}
#endif

  // cleanup
  ec.Done(fsave);
  ec.spin = sp0;
  jprintf_close();
  fclose(f);
jtimer_rpt();
  d.StatusText("Stopped."); 
  chat.Mute();
  SetForegroundWindow(me);

  // result caching
  icam = (ec.rwi).HeadView();
  if ((icam != NULL) && icam->Valid())
  {
    res.Clone(*icam);
    sprintf_s(rname, "script_cam.bmp");
  }
}


//= Gets cleaned up next line from file (removes whitespace and comment from end).
// returns true if txt string filled with something new

bool CBanzaiDoc::next_line (char *txt, int ssz, FILE *f) const
{
  char *end;
  int n;

  // clear string then try to read a new one from file
  *txt = '\0';
  while (fgets(txt, ssz, f) != NULL)
  {
    // strip off comment part
    if ((end = strstr(txt, "//")) != NULL)
      *end = '\0';

    // erase from end until non-space found
    for (n = (int) strlen(txt) - 1; n >= 0; n--)
    {
      if (strchr(" \n", txt[n]) == NULL)
        return true;
      txt[n] = '\0';
    }
  }
  return false;
}


// Send commands and provide advice using dialog
// *** STANDARD DEMO ***

void CBanzaiDoc::OnDemoInteractive()
{
  HWND me = GetForegroundWindow();
  char in[200] = "";
  jhcImg col(640, 480, 3);
  const jhcImg *inav = NULL;

  // possibly check for video
  (ec.body).BindVideo(NULL);
  if (cam > 0)
  { 
    if (ChkStream() > 0)
      (ec.body).BindVideo(&v);
    else if (cmd_line > 0)
    {
      ec.SpeakError("I can't see anything");
      d.StatusText("Failed");
      return;
    }
  }

  // reset all required components
  system("cls");
  jprintf_open();
  d.StatusText("Initializing robot ...");
  if (ec.Reset(rob) <= 0)
  {
    if (cmd_line > 0)
      ec.SpeakError("My body is not working");
    else
      Complain("Robot not functioning properly");
    d.StatusText("Failed.");
    return;
  }
  ec.SetPeople("VIPs.txt");
  chat.Reset(0, "log");
  SetForegroundWindow(chat);

  // announce start and input mode
  if (ec.spin > 0)
    d.Clear(1, "Voice input (ESC to quit) ...");
  else
    d.Clear(1, "Text input (ESC to quit) ...");
  d.ResetGrid(0, 640, 480);
  d.StringGrid(0, 0, ">>> NO IMAGES <<<");

jtimer_clr();
  // keep taking sentences until ESC
#ifndef _DEBUG
  try
#endif
  {
    while (chat.Interact() >= 0)
    {
      // get inputs and compute response
      ec.Accept(chat.Get(in), chat.Done());
      if (ec.Respond() <= 0)
        break;

      // show robot sensing and action and any communication
      if ((ec.body).NewFrame())
      {
        d.ShowGrid((ec.rwi).HeadView(), 0, 0, 0, "Visual attention (%3.1f\")", ((ec.rwi).tab).ztab);
        d.ShowGrid((ec.rwi).MapView(),  1, 0, 2, "Overhead navigation map  %s", (ec.rwi).NavGoal());
      }
      (ec.stat).Memory(&d, 0, 1, ec.Sensing());
      chat.Post(ec.NewInput(), 1);
      chat.Post(ec.NewOutput());
    }
  }
#ifndef _DEBUG
  catch (...){Tell("Unexpected exit!");}
#endif

  // cleanup
  ec.Done(fsave);
  jprintf_close();
jtimer_rpt();
  d.StatusText("Stopped."); 
  chat.Mute();
  SetForegroundWindow(me);

  // result caching
  inav = (ec.rwi).MapView();
  if ((inav != NULL) && inav->Valid())
  {
    FalseClone(res, *inav);
    sprintf_s(rname, "interact_nav.bmp");
  }
}


/////////////////////////////////////////////////////////////////////////////
//                               Heads & Faces                             //
/////////////////////////////////////////////////////////////////////////////

// Set the size and resolution of the area in which to look for people

void CBanzaiDoc::OnRoomPersonmap()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).s3).mps); 
}


// Parameters controlling detection of separated people

void CBanzaiDoc::OnPeopleChest()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).s3).bps); 
}


// Characteristics needed for human heads

void CBanzaiDoc::OnPeopleHead()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).s3).hps); 
}


// Person verfication by shoulder analysis

void CBanzaiDoc::OnPeopleShoulder()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).s3).sps); 
}


// Detections needed to validate and max movement distance

void CBanzaiDoc::OnRoomHeadtrack()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).s3).tps); 
}


// Angular range where heads are likely to be detected

void CBanzaiDoc::OnPeopleVisibility()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.rwi).vps); 
}


// Parameters governing local mapping gazes during movement

void CBanzaiDoc::OnSocialSaccades()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.rwi).sps); 
}


// Parameters for detecting and selecting people

void CBanzaiDoc::OnPeopleSocialevents()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.soc).aps); 
}


// Parameters controlling approach and following of people

void CBanzaiDoc::OnPeopleSocialmove()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.soc).mps); 
}


// Supply an external mugshot for some person

void CBanzaiDoc::OnAttentionEnrollphoto()
{
  jhcFRecoDLL *fr = &(((ec.rwi).fn).fr);
  jhcFFind *ff = &(((ec.rwi).fn).ff);
  char person[80] = "";
  jhcString sel, init, idir(cwd);
  CFileDialog dlg(TRUE);
  jhcPickString pick;
  jhcName fname;
  jhcImgIO jio;
  jhcImg mug, mug4;
  jhcRoi det;
  jhcImg *src;
  FILE *out;
  const jhcFaceVect *inst = NULL;

  // get ID to use
  if (pick.EditString(person, 0, "Person name") <= 0)
    return;
  if (*person == '\0')
    return;
  sprintf_s(init.ch, "%s.bmp", person);
  init.C2W(); 

  // get sample image containing face
  (dlg.m_ofn).lpstrInitialDir = idir.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Image Files\0*.bmp;*.jpg\0All Files (*.*)\0*.*\0");
  (dlg.m_ofn).lpstrFile = init.Txt();
  if (dlg.DoModal() != IDOK)
    return;
  sel.Set((dlg.m_ofn).lpstrFile);
  if (jio.LoadResize(mug, sel.ch) <= 0)
    return;
  fname.ParseName(sel.ch);

  // find biggest face and try to enroll it
  ff->Reset();
  fr->Reset();
  fr->LoadDB("VIPs.txt");
  (ec.vip).Load("VIPs.txt");
  src = Image4(mug4, mug);
  if (ff->FindBest(det, *src, 20, 400, 0.0) > 0)
  {
    inst = fr->Enroll(person, *src, det);
    RectEmpty(*src, det, 3, 255, 0, 255);
  }
  res.Clone(*src);
  sprintf_s(rname, "enroll_box.bmp");

  // show results
  d.Clear();
  d.ShowGrid(*src, 0, 0, 0, fname.Base());
  if (inst == NULL)
    return;
  d.ShowGrid(inst->thumb, 0, 1, 0, person);

  // update database
  fr->SaveDude(person);
  if ((ec.vip).Canonical(person) == NULL)
    if (Ask("Add to VIP list?") > 0)
      if (fopen_s(&out, "VIPs.txt", "a") == 0)
      {
        fprintf(out, "%s\n", person);
        fclose(out);
      }
}


// Take a picture of some particular person

void CBanzaiDoc::OnAttentionEnrolllive()
{
  jhcFRecoDLL *fr = &(((ec.rwi).fn).fr);
  jhcFFind *ff = &(((ec.rwi).fn).ff);
  char person[80] = "";
  jhcPickString pick;
  jhcImg now, mark;
  jhcRoi box;
  FILE *out;
  const jhcFaceVect *inst = NULL;
  int ok = 0;

  // check video
  if (ChkStream() <= 0)
    return;
  v.SizeFor(now);
  ff->Reset();
  fr->Reset();
  fr->LoadDB("VIPs.txt");
  (ec.vip).Load("VIPs.txt");

  // get ID to use
  if (pick.EditString(person, 0, "Person name") <= 0)
    return;
  if (*person == '\0')
    return;

  // loop over selected set of frames  
  d.Clear(1, "Enroll live ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get next frame from source
      if (v.Get(now) < 1)
        break;

      // run face finder 
      ok = ff->FindBest(box, now, 20, 400, 0.0);
 
      // draw box around face
      mark.Clone(now);
      if (ok > 0)
        RectEmpty(mark, box, 5, -3);

      // show frame on screen
      d.ShowGrid(mark, 0, 0, 0, "%s  --  Hit any to capture", person);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
  res.Clone(mark);
  sprintf_s(rname, "enroll_box.bmp");

  // actually enroll face (no box) as instance for person
  d.ShowGrid(mark, 0, 0, 0, "%s  %s", person, ((ok <= 0) ? "--  No face detected!" : ""));
  if (ok <= 0)
    return;
  if ((inst = fr->Enroll(person, now, box)) == NULL)
    return;
  d.ShowGrid(inst->thumb, 0, 1, 0, "Enrolled");

  // update database
  fr->SaveDude(person);
  if ((ec.vip).Canonical(person) == NULL)
    if (Ask("Add to VIP list?") > 0)
      if (fopen_s(&out, "VIPs.txt", "a") == 0)
      {
        fprintf(out, "%s\n", person);
        fclose(out);
      }
}


// Continuously aim Kinect at closest detected head

void CBanzaiDoc::OnDepthTrackhead()
{
  jhcStare3D *s3 = &((ec.rwi).s3);
  jhcImg map, parts, col;
  jhcMatrix cam(4), dir(4), targ(4);
  double pan = 0.0, tilt = 0.0, dist = 0.0, side = 50.0, turn = 30.0;
  int index, id, sp0 = ec.spin;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system
  ec.spin = 0;
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);

  // local images
  parts.SetSize(s3->map);
  s3->dbg = 1;

  // loop over selected set of frames  
  d.Clear(1, "Head track ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get new sensor data and analyze it
      if ((ec.rwi).Update() <= 0)
        break;

      // if no head then return to default
      if ((index = s3->Closest()) < 0)
        (eb->neck).GazeTarget(0.0, -15.0);    
      else
      {
        // find target angles
        (eb->neck).HeadPose(cam, dir, (eb->lift).Height());
        s3->Head(targ, index);
        cam.PanTilt3(pan, tilt, targ);
        pan -= 90.0;
        dist = targ.PlaneVec3();

        // move robot head and possibly feet
        (eb->neck).GazeTarget(pan, tilt);
        if (pan > side)
          (eb->base).TurnTarget(turn);
        else if (pan < -side)
          (eb->base).TurnTarget(-turn);
      }

      // make pretty pictures
      map.Clone(s3->map);
      col.Clone(eb->Color());
      s3->CamZone(map, 0);
      if (index >= 0)
      {
        id = s3->PersonID(index);
        s3->ShowID(map, id, 1, 0, 7);
        s3->ShowIDCam(col, id);
      }
      s3->HeadLevels(parts);
      s3->ShowHeads(parts, s3->dude, s3->NumPotential(), 0, 8.0, -7);

      // show pair and overhead map
      if (index >= 0)
        d.ShowGrid(col, 0, 0, 0, "Selected head (%+d %+d) @ %3.1f in\n", ROUND(pan), ROUND(tilt), dist);
      else
        d.ShowGrid(col, 0, 0, 0, "No heads");
      d.ShowGrid(map,   1, 0, 2, "Overhead map");
      d.ShowBelow(parts, 2, "Heads and shoulders");

      // send any commands and start collecting next input
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  ec.Done();
  s3->dbg = 0;
  ec.spin = sp0;
  d.StatusText("Stopped.");  
  FalseClone(res, map);
  sprintf_s(rname, "%s_pmap.bmp", v.FrameName());
}


// Beeps if user is gazing at robot

void CBanzaiDoc::OnDepthPersonmap()
{
  jhcStare3D *s3 = &((ec.rwi).s3);
  jhcFaceName *fn = &((ec.rwi).fn);
  jhcEliBase *b = (ec.rwi).base;
  jhcImg map, col;
  jhcMatrix pos(4), dir(4);
  const jhcBodyData *p;
  int i, sp0 = ec.spin;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system
  ec.spin = 0;
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);
  ec.SetPeople("VIPs.txt");
  (eb->neck).Limp();

  // loop over selected set of frames  
  d.Clear(1, "Head gaze ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get new sensor data and analyze it
      if ((ec.rwi).Update() <= 0)
        break;

      // announce eye contact (GazeMax better than FrontMax)
      if (fn->GazeMax() >= 1)
      {
        b->ForceLED(1);
        PlaySound(TEXT("beep.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP | SND_LOOP);
      }
      else
      {
        b->ForceLED(0);
        PlaySound(NULL, NULL, SND_ASYNC);
      }

      // see if new person found or new face example added
      if ((i = fn->JustNamed()) >= 0)
      {
        p = s3->GetPerson(i);
        jprintf("Just determined person %d is %s\n", i, p->tag);
      }
      if ((i = fn->JustUpdated()) >= 0)
      {
        p = s3->GetPerson(i);
        jprintf("+ Added new image for %s\n", p->tag);
      }   

      // make pretty pictures
      map.Clone(s3->map);
      s3->CamLoc(map, 0);
      s3->AllHeads(map);
      fn->AllGaze(map);
      col.Clone(eb->Color());
      s3->HeadsCam(col, 0, 1, 0, 8.0, 3);
      fn->FacesCam(col);

      // show pair and overhead map
      d.ShowGrid(map, 0, 0, 2, "Overhead gaze angle");
      d.ShowGrid(col, 1, 0, 0, "Faces wrt heads");

      // send any commands and start collecting next input
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  b->ForceLED(0);
  PlaySound(NULL, NULL, SND_ASYNC);
  ec.Done();
  ec.spin = sp0;
  d.StatusText("Stopped."); 
  res.Clone(col); 
  sprintf_s(rname, "%s_heads.bmp", v.FrameName());
}


// Determine which person is talking

void CBanzaiDoc::OnPeopleSpeaking()
{
  jhcStare3D *s3 = &((ec.rwi).s3);
  jhcSpeaker *tk = &((ec.rwi).tk);
  jhcImg map, col;
  jhcMatrix pos(4), dir(4);
  int spk, sp0 = ec.spin;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system
  ec.spin = 1;
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  ec.Reset(rob);
  ec.SetPeople("VIPs.txt");
  (eb->neck).Limp();

  // loop over selected set of frames  
  d.Clear(1, "Sound source ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get new sensor data and analyze it
      ec.UpdateSpeech();
      if ((ec.rwi).Update(ec.SpeechRC()) <= 0)
        break;
      spk = tk->Speaking();

      // make pretty overhead picture
      map.Clone(s3->map);
      s3->CamLoc(map, 0);
      s3->AllHeads(map);
      if (spk >= 0)
        s3->ShowID(map, spk);
      if (ec.SpeechRC() > 0)
        tk->SoundMap(map);

      // make pretty front picture
      col.Clone(eb->Color());
      s3->HeadsCam(col);
      if (spk >= 0)
        s3->ShowIDCam(col, spk);
      if (ec.SpeechRC() > 0)
        tk->SoundCam(col, 0, 0, 2);
      tk->SoundCam(col);

      // show pair and overhead map
      if (spk <= 0)
        d.ShowGrid(map, 0, 0, 2, "Overhead direction");
      else
        d.ShowGrid(map, 0, 0, 2, "Overhead direction  --  speaker id = %d", spk);
      d.ShowGrid(col, 1, 0, 0, "Speaker and others  --  watching");

      // send any commands and start collecting next input
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  ec.Done();
  ec.spin = sp0;
  d.StatusText("Stopped."); 
  FalseClone(res, map); 
  sprintf_s(rname, "%s_beam.bmp", v.FrameName());
}


/////////////////////////////////////////////////////////////////////////////
//                           Grammar Construction                          //
/////////////////////////////////////////////////////////////////////////////

// Get preliminary rules from a list of properties and values

void CBanzaiDoc::OnUtilitiesValuerules()
{
  jhcString sel, test;
  CFileDialog dlg(TRUE);
  char *end;
  int n, skip = (int) strlen(cwd) + 1;

  // select file to read 
  sprintf_s(test.ch, "%s\\KB0\\kernel.vals", cwd);
  test.C2W();
  (dlg.m_ofn).lpstrFile = test.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Value lists\0*.vals\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() != IDOK)
    return;

  // remove extension then look for terms
  sel.Set((dlg.m_ofn).lpstrFile);    
  if ((end = strrchr(sel.ch, '.')) != NULL)
    *end = '\0';
  if ((n = (ec.net).AutoVals(sel.ch)) > 0)
    Tell("Analyzed %d categories to generate files:\n\n%s0.rules  +  %s_v0.rules", n, sel.ch + skip, sel.ch + skip);
}


// Get preliminary grammar terms from operators and rules

void CBanzaiDoc::OnUtilitiesExtractwords()
{
  jhcString sel, test;
  CFileDialog dlg(TRUE);
  char *end;
  int n, skip = (int) strlen(cwd) + 1;

  // select file to read 
  sprintf_s(test.ch, "%s\\KB2\\interaction.ops", cwd);
  test.C2W();
  (dlg.m_ofn).lpstrFile = test.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Operators and Rules\0*.ops;*.rules\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() != IDOK)
    return;

  // remove extension then look for terms
  sel.Set((dlg.m_ofn).lpstrFile);    
  if ((end = strrchr(sel.ch, '.')) != NULL)
    *end = '\0';
  if ((n = (ec.net).HarvestLex(sel.ch)) > 0)
    Tell("Extracted %d terms to: %s0.sgm", n, sel.ch + skip);
}


// Refine grammar terms for consistent morphology

void CBanzaiDoc::OnUtilitiesChkgrammar()
{
  jhcString sel, test;
  CFileDialog dlg(TRUE);
  int err;

  // select file to read 
  sprintf_s(test.ch, "%s\\language\\vocabulary.sgm", cwd);
  test.C2W();
  (dlg.m_ofn).lpstrFile = test.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Grammar Files\0*.sgm\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() != IDOK)
    return;

  // try generating derived terms and do basic inversion testing
  sel.Set((dlg.m_ofn).lpstrFile);    
  if ((err = ((ec.net).mf).LexDeriv(sel.ch)) < 0)
    return;
  if (err > 0)
  {
    Tell("Adjust original =[XXX-morph] section to fix %d problems", err);
    return;
  }

  // try regenerating base words from derived terms
  if ((err = ((ec.net).mf).LexBase("derived.sgm", 1, sel.ch)) < 0)
    return;
  if (err > 0)
    Tell("Adjust original =[XXX-morph] section to fix %d problems", err);
  else
    Tell("Looks good but examine \"derived.sgm\" then \"base_words.txt\"\n\nAdjust original =[XXX-morph] section to fix any problems");
}


// Find unused non-terminals in grammar

void CBanzaiDoc::OnUtilitiesWeedgrammar()
{
  int n;

  (ec.body).BindVideo(NULL);
  ec.Reset(0);
  ec.SetPeople("VIPs.txt");
  n = (ec.vc).WeedGram((ec.gr).Expansions());
  Tell("%d unused non-terminals listed in file: orphans.txt", n);
}


/////////////////////////////////////////////////////////////////////////////
//                                Navigation                               //
/////////////////////////////////////////////////////////////////////////////

// Parameters for ground obstacle map around robot

void CBanzaiDoc::OnNavigationUpdating()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).nav).eps); 
}


// Specify rough footprint of robot on floor and map fading

void CBanzaiDoc::OnRoomRobotdims()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).nav).gps); 
}


// Parameters for synthetic sensors and obstacle avoidance

void CBanzaiDoc::OnNavGuidance()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).nav).nps); 
}


// Define active part of Kinect sensing cone (only for navigation)

void CBanzaiDoc::OnNavDepthfov()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).nav).kps); 
}


// Adjust height from controller using hand measurements

void CBanzaiDoc::OnRoomForkcalib()
{
  jhcEliLift *lift = &(eb->lift);
  char in[20];
  double ht0, ht1, cmd0 = 10.0, cmd1 = 30.0, htol = 0.2;
  int stable, pmax0, pmin0, v0, v1, shift, span;

  // start up robot (no background loop)
  eb->BindVideo(NULL);
  if (eb->Reset(1, 1) <= 0)
    return;
  pmax0 = lift->pmax;
  pmin0 = lift->pmin;

  // go to lower height
  printf("Setting shelf to nomimal %3.1f inches ...", cmd0);
  stable = 0;
  while (stable++ < 10)
  {
    lift->Update();
    if (lift->LiftErr(cmd0) > htol)
      stable = 0;
    lift->LiftTarget(cmd0);
    lift->Issue();
  }

  // solict low measurement
  printf("\nDistance from bottom of shelf to floor (x to quit): ");
  gets_s(in);
  if (sscanf_s(in, "%lf", &ht0) != 1)
  {
    printf("\nQuit!\n");
    return;
  }
  lift->Update();
  v0 = lift->RawFB();                  // movement has stopped  

  // go to upper height
  printf("\nSetting shelf to nomimal %3.1f inches ...", cmd1);
  stable = 0;
  while (stable++ < 10)
  {
    lift->Update();
    if (lift->LiftErr(cmd1) > htol)
      stable = 0;
    lift->LiftTarget(cmd1);
    lift->Issue();
  }

  // solict high measurement
  printf("\nDistance from bottom of shelf to floor (x to quit): ");
  gets_s(in);
  if (sscanf_s(in, "%lf", &ht1) != 1)
  {
    printf("\nQuit!\n");
    return;
  }
  lift->Update();
  v1 = lift->RawFB();                  // movement has stopped  

  // do adjustment and gauge change
  eb->Limp();
  lift->AdjustRaw(ht0, v0, ht1, v1);
  shift = abs(lift->pmin - pmin0);
  span = abs((lift->pmax - lift->pmin) - (pmax0 - pmin0));
  printf("\n");

  // talk to user about what to do
  if ((shift > 5) || (span > 10))
  {
    printf("Feedback range adjusted to [%d %d] vs old [%d %d]\n", lift->pmin, lift->pmax, pmin0, pmax0);
    if ((shift > 100) || (span > 200)) 
      printf("  *** maybe potentiometer has come unglued? ***\n");
    if (Ask("Save new values to %s?", eb->LastCfg()))
    {
      lift->SaveCfg(eb->LastCfg());
      printf("Done.\n");
      return;
    }
  }

  // otherwise revert to original values
  lift->pmax = pmax0;
  lift->pmin = pmin0;
  printf("Values unchanged.\n");
}


// Refine sensor tilt, roll, and height based on floor

void CBanzaiDoc::OnNavCamcalib()
{
  HWND me = GetForegroundWindow();
  jhcLocalOcc *nav = &((ec.rwi).nav);
  jhcImg mask;
  jhcMatrix pos(4), dir(4);
  char fname[80];
  double tsum = 0.0, rsum = 0.0, hsum = 0.0, dev = 4.0, tilt = -45.0;
  double err, t, r, h, dt = 0.0, dr = 0.0, dh = 0.0, tol = 0.1, htol = 0.1;
  int cnt = 0;

  // make sure video is working then initialize robot
  if (ChkStream() <= 0)
    return;
  eb->BindVideo(&v);
  if (eb->Reset(1, 1) <= 0)
    return;
  (eb->neck).SetNeck(0.0, tilt);

  // configure map for tight range around floor
  nav->mw = 1.5 * nav->dej;
  nav->mh = nav->dej;
  nav->x0 = 0.75 * nav->dej;
  nav->y0 = 0.0;
  nav->jhcOverhead3D::Reset();
  mask.SetSize(nav->map2);

  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Camera calibration ...");
  try 
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get frame, pause if right mouse click
      if (eb->Update() <= 0)
        break;
 
      // find areas near floor and estimate camera corrections
      (eb->neck).HeadPose(pos, dir, (eb->lift).Height());
      nav->SetCam(0, 0.0, 0.0, pos.Z(), 90.0, dir.T(), dir.R(), 1.2 * nav->dej);
      err = nav->EstPose(t, r, h, eb->Range(), 0, dev);

      // accumulate statistics
      if (err > 0.0)
      {
        t = -t;
        tsum += t;
        rsum += r;
        hsum += h;
        cnt++;
        dt = tsum / cnt;
        dr = rsum / cnt;
        dh = hsum / cnt;
      }
 
      // make pretty pictures
      if (err > 0.0)
        nav->EstDev(mask, 2.0, dev);
      else
        Between(mask, nav->map2, 1, 254, 128);
      nav->CamZone(nav->map2, 0);

      // show results
      d.ShowGrid(nav->map2, 0, 0, 2, "Overhead map  --  adjust dt = %+4.2f, dr = %+4.2f, dh = %+4.2f", dt, dr, dh);
      if (err > 0.0)
        d.ShowGrid(mask, 0, 1, 2, "Corrected (%4.2f) --  estimated tilt = %4.2f, roll = %4.2f, ht = %4.2f)", err, t, r, h);
      else
        d.ShowGrid(mask, 0, 1, 2, "Pixels considered  --  BAD FIT");

      // prompt for new sensors
      eb->Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  eb->Limp();
  d.StatusText("Stopped.");  

  // possibly new save values
  if (cnt <= 0)
    Complain("Never found floor!");
  else if ((fabs(dt) <= tol) && (fabs(dr) <= tol) && (fabs(dh) < htol))
    Tell("No adjustment needed");
  else  
  {
    // ask about individual changes
    if (fabs(dt) > tol)
      if (Ask("Adjust tilt by %+4.2f degrees?", dt)) 
        ((eb->neck).jt[1]).cal += dt;
    if (fabs(dr) > tol)
      if (Ask("Adjust roll by %+4.2f degrees?", dr)) 
        (eb->neck).roll += dr;
    if (fabs(dh) > htol)
      if (Ask("Adjust height by %+3.1f inches?", dh)) 
        (eb->neck).nz0 += dh;

    // save values to file (some may not have changed)
    eb->CfgFile(fname, 1);
    if (Ask("Save calibration for robot %d ?", eb->BodyNum()))
      (eb->neck).SaveCfg(fname);
  }

  // clean up
  FalseClone(res, mask);
  sprintf_s(rname, "%s_cal.bmp", v.FrameName());
}


// Show instantaneous Kinect height map with neck limp

void CBanzaiDoc::OnEnvironFloormap()
{
  HWND me = GetForegroundWindow();
  jhcLocalOcc *nav = &((ec.rwi).nav);
  jhcImg map2, fw;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);
  fw.SetSize(nav->map);
  eb->Limp();

  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Depth projection ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;

      // make pretty pictures
      map2.Clone(nav->map);
      nav->ScanBeam(map2);
      nav->RobotMark(map2, 0);
      Threshold(fw, nav->map, 254, 255);         // white walls
      MarkTween(fw, nav->dev, 78, 178, 50);      // blue floor
      MarkTween(fw, nav->dev, 1, 1, 128);        // green missing

      // show overhead map and input image
      d.ShowGrid(map2, 0, 0, 2, "Raw overhead map  --  pan %3.1f, tilt %3.1f", (eb->neck).Pan(), (eb->neck).Tilt());
      d.ShowGrid(fw,   1, 0, 2, "Walls, floor, and missing  --  adj tilt %4.2f, adj roll %4.2f", nav->TiltDev(), nav->RollDev());

      // prompt for new sensors
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.StatusText("Stopped.");  

  // clean up
  FalseClone(res, map2);
  sprintf_s(rname, "%s_hts.bmp", v.FrameName());
}


// Show floor map integrated over time including base movement 

void CBanzaiDoc::OnEnvironIntegrated()
{
  HWND me = GetForegroundWindow();
  jhcLocalOcc *nav = &((ec.rwi).nav);
  jhcImg obs2, cf2;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);
  eb->Limp();

  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Local map ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;

      // make pretty pictures
      obs2.Clone(nav->obst);
      nav->ScanBeam(obs2);
      nav->RobotDir(obs2, 0);
      nav->RobotMark(obs2, 0);
      cf2.Clone(nav->conf);
      nav->Doormat(cf2, 0);

      // show overhead map and input image
      d.ShowGrid(obs2, 0, 0, 2, "Floor, obstacles, and potential dropoffs");
      d.ShowGrid(cf2,  1, 0, 2, "Confidence and doormat area (%4.2f)", nav->known);

      // prompt for new sensors
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.StatusText("Stopped.");  

  // clean up
  FalseClone(res, obs2);
  sprintf_s(rname, "%s_obst.bmp", v.FrameName());
}


// Show distances robot can go in various orientations

void CBanzaiDoc::OnEnvironLocalpaths()
{
  HWND me = GetForegroundWindow();
  jhcLocalOcc *nav = &((ec.rwi).nav);
  jhcImg path;
  int i, dev, nd, hnd, nd2;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);
  eb->Limp();
  nd  = nav->ndir;
  hnd = nd / 2;
  nd2 = 2 * nd;

  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Paths ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;

      // make pretty pictures      
      for (i = 0; i < nav->ndir; i++)
        nav->RobotBody(nav->spin[i]);
      path.Clone(nav->spin[hnd]);

      // show results
      d.ShowGrid(path, 0, 0, 2, "straight = F %3.1f, B %3.1f", nav->dist[nd], nav->dist[0]);
      d.ShowGrid(nav->spin[0], hnd - 2, 0, 2, "rt 90.0 degs = F %3.1f, B %3.1f", nav->dist[hnd], nav->dist[nd + hnd]);
      for (dev = 1; dev < hnd; dev++)
        d.ShowGrid(nav->spin[hnd - dev], dev - 1, 1, 2, "rt %3.1f degs = F %3.1f,  B %3.1f", 
                                                        dev * nav->Step(), nav->dist[nd - dev], nav->dist[nd2 - dev]); 
      for (dev = 1; dev < hnd; dev++)
        d.ShowGrid(nav->spin[hnd + dev], dev - 1, 2, 2, "lf %3.1f degs = F %3.1f, B %3.1f", 
                                                        dev * nav->Step(), nav->dist[nd + dev], nav->dist[dev]); 

      // prompt for new sensors
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.StatusText("Stopped.");  

  // clean up
  FalseClone(res, path);
  sprintf_s(rname, "%s_path.bmp", v.FrameName());
}


// Show valid forward and backward motions on large maps

void CBanzaiDoc::OnEnvironDistances()
{
  HWND me = GetForegroundWindow();
  jhcLocalOcc *nav = &((ec.rwi).nav);
  jhcImg fwd, rev;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);
  eb->Limp();

  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Sensors ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;

      // make pretty pictures
      fwd.Clone(nav->obst);
      nav->Dists(fwd, 0);
      nav->RobotBody(fwd, 0);
      rev.Clone(nav->obst);
      nav->Paths(rev, 0, 0);
      nav->RobotBody(rev, 0);

      // show overhead map and input image
      d.ShowGrid(fwd, 0, 0, 2, "Raw center ranges");
      d.ShowGrid(rev, 1, 0, 2, "Achievable motions (some turns not possible)");

      // prompt for new sensors
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.StatusText("Stopped.");  

  // clean up
  FalseClone(res, fwd);
  sprintf_s(rname, "%s_sensor.bmp", v.FrameName());
}


// Pick fixed map location for robot to travel toward

void CBanzaiDoc::OnEnvironGoto()
{
  HWND me = GetForegroundWindow();
  jhcLocalOcc *nav = &((ec.rwi).nav);
  jhcImg map;
  jhcMatrix z(4);
  char label[80] = "";
  double p0 = 60.0, t0 = -40.0, tol = 2.0, arrive = 4.0, tsp = 0.7;
  double cx, cy, ipp, circ, ix, iy, err, tx, ty, d0 = -1.0;
  int mx, my, mbut = 0, step = 0;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (eb->Reset(1, 0) <= 0)
  {
    d.StatusText("Failed.");
    return;
  }
  (ec.rwi).Reset(1, 0);
  z.Zero();
  (eb->arm).ShiftTarget(z);

  // local image sizes
  map.SetSize(nav->map);
  cx = 0.5 * map.XLim();
  cy = 0.5 * map.XLim();
  ipp = nav->ipp;
  circ = arrive / ipp;

  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Go to location ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime(), 3, 0, ""))
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;
      if (step >= 21)
        (eb->base).AdjustXY(tx, ty);

      // look right
      if ((step >= 0) && (step < 10))
      {
        err = (eb->neck).GazeErr(-p0, t0);
        sprintf_s(label, "Look right ... %3.1f", err);
        if (err < tol)
          step++;
        else 
          (eb->neck).GazeTarget(-p0, t0, 0.5);
      }

      // look left 
      if ((step >= 10) && (step < 20))
      {
        err = (eb->neck).GazeErr(p0, t0);
        sprintf_s(label, "Look left ... %3.1f", err);
        if (err < tol)
          step++;
        else 
          (eb->neck).GazeTarget(p0, t0, 0.5);
      }
      if (step == 20)
      {
        strcpy_s(label, "*** CLICK ON TARGET LOCATION ***");
        if (mbut > 0)
        {
          strcpy_s(label, "Moving toward target  -  %3.1f in away  %s");
          tx = (mx - cx) * ipp;
          ty = (my - cy) * ipp;
          step++;
        }
      }

      // pick steering and speed
      if (step >= 21)
      {
        d0 = sqrt(tx * tx + ty * ty);
        (ec.rwi).SeekLoc(tx, ty, tsp, 100);
      }

      // make pretty pictures
      nav->LocalMap(map);
      if (step >= 21)
      {
        // show current goal location and direct path
        ix = cx + tx / ipp;
        iy = cy + ty / ipp;
        DrawLine(map, cx, cy, ix, iy, 3, -6);
        CircleEmpty(map, ix, iy, circ, 3, -5);
      }
      if (step >= 20)                  
      {
        // show sensor beams when ready for target click
        nav->Paths(map);
        nav->RobotBody(map);
        nav->Tail(map);
      }

      // show overhead map with navigation data
      d.ShowGrid(map, 0, 0, 2, label, d0);

      // prompt for new sensors
      (ec.rwi).Issue();

      // check for mouse event in image or if motion done
      mbut = d.MouseRel0(mx, my);
      if ((mbut < -1) || (mbut == 3))
        break;
      if ((step >= 21) && (d0 <= arrive))
        break;
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.StatusText("Stopped.");  

  // clean up
  FalseClone(res, map);
  sprintf_s(rname, "%s_goto.bmp", v.FrameName());
  if ((step >= 21) && (d0 <= arrive))
    Tell("Arrived");
}


/////////////////////////////////////////////////////////////////////////////
//                             Object Detection                            //
/////////////////////////////////////////////////////////////////////////////

// Parameters controlling surface height finding in wide depth view

void CBanzaiDoc::OnSocialSurffind()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).tab).hps);      
}


// Parameters governing localization of surface patches

void CBanzaiDoc::OnTableSurface()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).tab).cps);      
}


// Parameters for qualitative height of surfaces 

void CBanzaiDoc::OnObjectsSurfheight()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.sup).hps);    
}


// Parameters for qualitative position and direction of surfaces

void CBanzaiDoc::OnObjectsSurflocation()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.sup).lps);    
}


// Map size and resolution for detecting objects on surface

void CBanzaiDoc::OnObjectsSurfacemap()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).sobj).mps);    
}


// Parameters for mapping and filling support surface

void CBanzaiDoc::OnObjectsSurfacezoom()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).sobj).zps);    
}


// Parameters for fitting plane to surface

void CBanzaiDoc::OnObjectsPlanefit()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).sobj).pps);    
}


// Parameters for finding objects protruding from surface

void CBanzaiDoc::OnObjectsDetect()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).sobj).dps);
}


// Parameters for finding nearly flat objects on surfaces

void CBanzaiDoc::OnObjectsColorparams()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).sobj).gps);
}


// Parameters for size compensation and temporal update rate

void CBanzaiDoc::OnObjectsShape()
{
	jhcPickVals dlg;

  dlg.EditParams(((ec.rwi).sobj).sps);  
}


// Set parameters for tracking objects on table

void CBanzaiDoc::OnObjectsTrack()
{
	jhcPickVals dlg;

  dlg.EditParams((((ec.rwi).sobj).pos).tps);
}


// Set parameters for updating object tracks

void CBanzaiDoc::OnObjectsFilter()
{
	jhcPickVals dlg;

  dlg.EditParams((((ec.rwi).sobj).pos).fps);
}


// Set distance thresholds for categories and alerts

void CBanzaiDoc::OnVisualDist()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.svis).rps);
}


// Set object shape thresholds for categories

void CBanzaiDoc::OnObjectsShapebins()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.svis).sps);
}


// Set object size thresholds for categories

void CBanzaiDoc::OnVisualDimsbins()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.svis).dps);
}


// Adjust value comparison and spatial zones

void CBanzaiDoc::OnVisualObjcomparison()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.svis).cps);
}


// Parameters for analyzing hues and intensities

void CBanzaiDoc::OnVisualColorfinding()
{
	jhcPickVals dlg;

  dlg.EditParams((((ec.rwi).sobj).pp).cps);
}


// Hue value boundaries between colors

void CBanzaiDoc::OnVisualHuethresholds()
{
	jhcPickVals dlg;

  dlg.EditParams((((ec.rwi).sobj).pp).hps);
}


// Threshold for qualitative color predicates

void CBanzaiDoc::OnVisualPrimarycolors()
{
	jhcPickVals dlg;

  dlg.EditParams((((ec.rwi).sobj).pp).nps);
}


// Find best surface from full range height map

void CBanzaiDoc::OnSurfacePicktable()
{
  jhcEliGrok *rwi = &(ec.rwi);
  jhcStare3D *s3 = &(rwi->s3);
  jhcTable *tab = &(rwi->tab);
  jhcImg surf, map2;
  jhcRoi box;
  double hw = (rwi->nav).rside, g0 = (eb->neck).gaze0;
  double mx, hp0, x0, y0, cx, cy, dist, hpel;
  int rect, gh0 = d.ght;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (eb->neck).gaze0 = -45.0;
  rwi->Reset(rob, 0);
  eb->Limp();

  // local images
  surf.SetSize(tab->wmap);
  mx = surf.RoiAvgX();

  // loop over selected set of frames  
  d.ght = 500;
  d.Clear(1, "Potential surfaces ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get images and body data then process it
      hp0 = tab->hpref;
      if (rwi->Update() <= 0)
        break;
      dist = 0.0;

      // make pretty pictures
      map2.Clone(tab->wmap);
      if (tab->dpref > 0.0)
      {
        CircleEmpty(map2, mx, 0.0, (tab->dpref - tab->margin) / tab->wipp, 1);
        CircleEmpty(map2, mx, 0.0, (tab->dpref + tab->margin) / tab->wipp, 1);
      }
      Threshold(surf, tab->wbin, 128, 200);
      if (tab->tsel >= 0)
      {
        // valid surface sensing area and travel corridor 
        (rwi->s3).BeamEmpty(surf, tab->SurfHt(), 1, -7);
        dist = (ec.man).surf_gap();
        rect = ROUND((dist + (ec.man).prow) / tab->wipp);
        hpel = hw / tab->wipp;
        RectEmpty(surf, ROUND(mx - hpel), 0, ROUND(2.0 * hpel), rect, 1, -2); 

        // show centroid on depth map
        (tab->wlob).BlobCentroid(&cx, &cy, tab->tsel);
        Cross(map2, cx, cy, 17, 17, 3, -2); 

        // ray from camera position to edge
        x0 = tab->hx / tab->wipp + mx;
        y0 = tab->hy / tab->wipp;
        (tab->wlob).BlobCentroid(&cx, &cy, tab->tsel);
        CircleEmpty(surf, x0, y0, 5.0, 1, -3);
        DrawLine(surf, x0, y0, tab->ex, tab->ey, 1, -3);
      }

      // show overhead map and selected surface
      d.ShowGrid(map2, 1, 0, 2, "Person height map  --  surface centroid");
      if (dist <= 0.0)
        d.ShowGrid(surf, 0, 0, 2, "Selected surface  --  true edge not seen!");
      else
        d.ShowGrid(surf, 0, 0, 2, "Selected surface  --  advance up to %3.1f\" for grab", dist);

      // show various height planes considered
      d.GraphGridV(tab->hhist, 2, 0, 0, 0, "Height clusters = %3.1f in", tab->ztab);
      d.GraphValV(200, 0, 4);
      d.GraphMarkV(s3->I2Z(tab->ztab), 1);
      d.GraphMarkV(tab->i2z(hp0), 2, 0.5);
     
      // prompt for new sensors
      rwi->Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  rwi->Stop();
  d.ght = gh0;
  (eb->neck).gaze0 = g0;
  d.StatusText("Stopped.");  

  // clean up
  FalseClone(res, surf);
  sprintf_s(rname, "%s_surf.bmp", v.FrameName());
}


// Optimize head angles for detected surface

void CBanzaiDoc::OnDetectionGazesurface()
{
  jhcEliGrok *rwi = &(ec.rwi);
  jhcSurfObjs *sobj = &(rwi->sobj);
  jhcTable *tab = &(rwi->tab);
  jhcImg omap, surf;
  jhcRoi box;
  jhcMatrix head(4), edge(4);
  double mx, x0, y0, cx, cy, g0 = (eb->neck).gaze0, pan = 0.0, tilt = -45.0;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (eb->neck).gaze0 = tilt;
  rwi->Reset(rob, 0);
  eb->Limp();

  // local images
  surf.SetSize(tab->wmap);
  mx = surf.RoiAvgX();

  // loop over selected set of frames  
  d.Clear(1, "Gazing toward surface ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get images and body data then process it
      if (rwi->Update() <= 0)
        break;

      // pick direction to look
      (eb->neck).HeadLoc(head, (eb->lift).Height());
      tab->SurfEdge(edge);
      (eb->neck).AimFor(pan, tilt, edge, (eb->lift).Height());
      (eb->neck).GazeFix(pan, tilt, 0.5);

      // make pretty pictures
      omap.Clone(sobj->map);
      (sobj->blob).DrawOutline(omap);
      surf.CopyArr(tab->wmap);
      if (tab->tsel >= 0)
      {
        (tab->wlob).GetRoi(box, tab->tsel);
        RectEmpty(surf, box, 1, -7);
        x0 = tab->hx / tab->wipp + mx;
        y0 = tab->hy / tab->wipp;
        (tab->wlob).BlobCentroid(&cx, &cy, tab->tsel);
        CircleEmpty(surf, x0, y0, 5.0, 1, -3);
        DrawLine(surf, x0, y0, cx, cy, 1, -3);
      }

      // show overhead map and selected surface
      d.ShowGrid(surf, 0, 0, 2, "Selected surface at %3.1f\"", sobj->ztab);
      d.ShowGrid(omap, 1, 0, 2, "Overhead objects (tilt %3.1f)", (eb->neck).Tilt());
     
      // prompt for new sensors
      rwi->Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  rwi->Stop();
  (eb->neck).gaze0 = g0;
  d.StatusText("Stopped.");  

  // clean up
  FalseClone(res, omap);
  sprintf_s(rname, "%s_zoom.bmp", v.FrameName());
}


// Detect protrusions from a flat surface

void CBanzaiDoc::OnObjectsTrackobjs()
{
  HWND me = GetForegroundWindow();
  jhcSurfObjs *sobj = &((ec.rwi).sobj);
  jhcImg map2, scr, obj2;
  int n = -1;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);
  (eb->neck).SetTilt(-50.0);
//  eb->Limp();

  // local images
  scr.SetSize(sobj->map);
  obj2.SetSize(sobj->map);

  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Depth objects ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;

      // make pretty pictures
      map2.Clone(sobj->map);
      n = (sobj->blob).DrawOutline(map2);
      Scramble(scr, sobj->cc);                                  
      Threshold(obj2, sobj->top, 50);
      SubstOver(obj2, scr, scr, 0);

      // show overhead map and input image
      d.ShowGrid(map2,      0, 0, 2, "Overhead height  --  %d object%c", n, ((n != 1) ? 's' : ' '));
      d.ShowGrid(sobj->det, 1, 0, 2, "Deviations from flat");

      d.StringGrid(2, 0, "Tilt adj \t%+4.2f", sobj->TiltDev());
      d.StringBelow("Roll adj \t%+4.2f", sobj->RollDev());
      d.StringBelow("Ht adj \t%+4.2f", sobj->HtDev());

      d.ShowGrid(sobj->top, 0, 1, 2, "Supporting plane @ %3.1f\" (adjusted)", sobj->ztab + sobj->HtDev());
      d.ShowGrid(obj2,      1, 1, 2, "Bumps and surface");

      // prompt for new sensors
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.StatusText("Stopped.");  

  // clean up
  FalseClone(res, map2);
  sprintf_s(rname, "%s_tall.bmp", v.FrameName());
}


// Find nearly flat objects based on color differences

void CBanzaiDoc::OnSurfaceColorobjs()
{
  HWND me = GetForegroundWindow();
  jhcSurfObjs *sobj = &((ec.rwi).sobj);
  jhcImg pat2, scr, obj2;
  int n;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);
  (eb->neck).SetTilt(-50.0);
//  eb->Limp();

  // local images
  scr.SetSize(sobj->map);
  obj2.SetSize(scr);
  pat2.SetSize(scr, 3);

jtimer_clr();
  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Color objects ...");
  v.Rewind(1);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;

      // make pretty pictures
      Scramble(scr, sobj->gcc);
      Threshold(obj2, sobj->bgnd, 0);
      UnderGate(obj2, obj2, sobj->rim, sobj->bgth, 1);
      SubstOver(obj2, scr, scr, 0);
      pat2.CopyArr(sobj->pat);
      n = (sobj->glob).DrawOutline(pat2);

      // show results
      d.ShowGrid(pat2,       0, 0, 0, "Overhead color --  %d object%c", n, ((n != 1) ? 's' : ' '));
      d.ShowGrid(sobj->cdet, 1, 0, 2, "Deviations from bulk color");

      d.GraphGrid(sobj->wkhist, 2, 0, 0, 0, "Intensity values");
      d.GraphMark(sobj->wk0, 0);
      d.GraphMark(sobj->wk1, 6);
      d.StringBelow("bulk = %d - %d", sobj->wk0, sobj->wk1);

      d.ShowGrid(sobj->gray, 0, 1, 0, "Grayscale surface");
      d.ShowGrid(obj2,       1, 1, 2, "Anomalies and background");

      // prompt for new sensors
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.StatusText("Stopped.");  
jtimer_rpt();

  // clean up
  res.Clone(pat2);
  sprintf_s(rname, "%s_flat.bmp", v.FrameName());
}


// Segment and track objects on table based on height and color

void CBanzaiDoc::OnSurfaceHybridseg()
{
  HWND me = GetForegroundWindow();
  jhcSurfObjs *sobj = &((ec.rwi).sobj);
  jhcImg col2, pat2, pass;
  jhcArr chist(90);
  jhcMatrix loc(4);
  char main[200] = "", alt[200] = "";
  double ang;
  int mbut, mx, my, n, id, item = -1;

  // make sure video is working
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (ec.rwi).Reset(rob, 0);
  (eb->neck).SetTilt(-50.0);
//  eb->Limp();

  // local images
  pat2.SetSize(sobj->pat);
  v.SizeFor(col2);
  pass.SetSize(col2);

jtimer_clr();
  // loop over selected set of frames  
  SetForegroundWindow(me);
  d.Clear(1, "Tracked objects ...");
  v.Rewind(1);
  try
  {
    while (1)
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;
      sobj->Spectralize(eb->Color(), eb->Range(), item, 1);

      // check collection of objects
      n = sobj->CntTracked();
      ang = sobj->World(loc, item);
      id = sobj->ObjID(item);

      // make pretty pictures of detections
      col2.CopyArr(eb->Color());
      sobj->ObjsCam(col2, 0, -1);
      pat2.Clone(sobj->pat);
      sobj->ShowAll(pat2);
      if (id > 0)
        XMark(pat2, sobj->MapX(item), sobj->MapY(item), 25, 2); 

      // extract color information
      pass.MaxRoi();
      pass.FillRGB(0, 0, 255);
      OverGateRGB(pass, eb->Color(), (sobj->pp).shrink, 128, 0, 0, 255);  
      (sobj->pp).QuantColor(chist);
      (sobj->pp).MainColors(main);
      (sobj->pp).AltColors(alt);

      // show overhead map and input image
      d.ShowGrid(col2, 0, 0, 0, "Frontal camera  --  %d object%c", n, ((n != 1) ? 's' : ' '));
      d.ShowGrid(pass, 0, 1, 0, "Object pixels (for color)");

      d.GraphGrid(chist, 1, 1, 0, 0, "ROYGBP-KXW measures");
      d.StringBelow("Colors = %-80s", main);
      d.StringBelow("Some = %-80s", alt);

      if (id <= 0)
        d.ShowGrid(pat2, 1, 0, 0, "Overhead zoom  --  Click object for properties");
      else
        d.ShowGrid(pat2, 1, 0, 0, "Overhead zoom  --  Object %d @ (%3.1f %3.1f %3.1f)  =  size (%4.2f %4.2f %4.2f) x %1.0f degs",
                                  id, loc.X(), loc.Y(), loc.Z(), 
                                  sobj->Major(item), sobj->Minor(item), sobj->SizeZ(item), 
                                  ((sobj->Elongation(item) < 1.5) ? 0.0 : ang));

      // see if any object was clicked (inside last image = pat2)
      mbut = d.MouseRel0(mx, my);     
      if (d.MouseExit(mbut) > 0)
      {
        d.StringBelow("%100s", "");
        break;
      }
      d.StringBelow("Click R above to exit ...");
      if (mbut == 1)
        item = sobj->ClickN(mx, my);

      // prompt for new sensors
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.StatusText("Stopped.");  
jtimer_rpt();

  // clean up
  res.Clone(pat2);
  sprintf_s(rname, "%s_track.bmp", v.FrameName());
}


/////////////////////////////////////////////////////////////////////////////
//                              Manipulation                               //
/////////////////////////////////////////////////////////////////////////////

// Geometric parameters for generating grasp and via points 

void CBanzaiDoc::OnObjectsGrasppoint()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.man).gps);
}


// Additional parameters governing execution of trajectories

void CBanzaiDoc::OnObjectsTrajectoryctrl()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.man).cps);
}


// Position and angle tolerances for successful completion

void CBanzaiDoc::OnObjectsMotiondone()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.man).dps);
}


// Parameters governing choice of object deposit location

void CBanzaiDoc::OnObjectsEmptyspot()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.man).sps);
}


// Limits on object position to be in valid manipulation area

void CBanzaiDoc::OnObjectsWorkspace()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.man).wps);
}


// Parameters for jockeying robot to bring object into workspace

void CBanzaiDoc::OnGrabBodyshift()
{
	jhcPickVals dlg;

  dlg.EditParams((ec.man).ips);
}


// Click on joints in image to improve arm kinematics

void CBanzaiDoc::OnMotionCalibarm()
{
  char hint[4][80] = {"on center of shoulder", "on elbow axis spot", "on between wrist white spots", "when valid forearm tilt"};
  char dtxt[20];
  jhcImg mark, d8, dcol;
  jhcEliArm *arm = &((ec.body).arm);
  const jhcMatrix *pos = arm->Position(), *dir = arm->Direction();
  double ax0, ay0, scal, ecal, lcal, z, a0, a1, tilt, tsm = 360.0;
  double jx, jy, sx, sy, sz, ex, ey, lx, ly, wx, wy;
  int mx, my, mbut = 0, gap = 0, step = 0, any = 0;

  // connect video source
  if (ChkStream() <= 0)
    return;
  (ec.body).BindVideo(&v);

  // local images
  v.SizeFor(mark);
  dcol.SetSize(mark);
  d8.SetSize(mark, 1);

  // reset body
  d.StatusText("Initializing robot ...");
  if ((ec.body).Reset(1, 0) <= 0)
  {
    Complain("Robot not functioning properly");
    d.StatusText("Failed.");
    return;
  }
  (ec.rwi).Reset(1, 0);

  // remember original calibration values
  ax0 = arm->ax0;
  ay0 = arm->ay0;
  scal = (arm->jt[0]).cal;
  ecal = (arm->jt[1]).cal;
  lcal = (arm->jt[2]).cal;

  // start up background processing
  d.Clear(1, "Calibrating arm ...");
  try
  {
    while (1)
    {
      // update joint angles and Kinect images
      (ec.rwi).Update();

      // find slope of forearm in depth image
      (ec.rwi).ImgJt(lx, ly, 2);
      (ec.rwi).ImgJt(wx, wy, 3);
      tilt = ((ec.rwi).s3).LineTilt((ec.body).Range(), lx, ly, wx, wy, 0.2, 0.8);
      if (tilt >= 360.0)
        tsm = 360.0;
      else if (tsm >= 360.0)
        tsm = tilt;
      else
        tsm += 0.1 * (tilt - tsm);
      if (tilt >= 360.0)
        strcpy_s(dtxt, "BAD");       
      else
        sprintf_s(dtxt, "%3.1f", tsm);

      // draw arm on depth
      Night8(d8, (ec.body).Range(), 0);
      CopyMono(dcol, d8);
      (ec.rwi).Skeleton(dcol);

      // draw arm on color with some joint highlighted
      (ec.body).ImgBig(mark);
      (ec.rwi).Skeleton(mark);
      if (step <= 2)
      {
        (ec.rwi).ImgJt(jx, jy, ((step == 2) ? 3 : step));
        RectCent(mark, jx, jy, 20.0, 20.0, 0.0, 3, -4);
      }
      
      // see if user clicked somewhere
      if ((mbut == 3) && (gap >= 15))
        step++;                                                      // skip this step
      else if ((mbut == 1) && (gap >= 15))
      {
        if (step == 0)                 
        {
          sz = (ec.rwi).ImgJt(sx, sy, 0);
          ((ec.rwi).s3).WorldPt(arm->ax0, arm->ay0, z, mx, my, sz);  // move shoulder center 
        }
        else if (step == 1)            
        {
          (ec.rwi).ImgJt(sx, sy, 0);
          (ec.rwi).ImgJt(ex, ey, 1);
          a0 = R2D * atan2(ey - sy, ex - sx);
          a1 = R2D * atan2(my - sy, mx - sx);
          (arm->jt[0]).cal -= a1 - a0;                               // adjust shoulder zero angle
        }
        else if (step == 2)            
          (arm->jt[1]).cal -= (ec.rwi).ImgVeer(mx, my, 3, 1);        // adjust elbow zero angle
        else if (step == 3) 
        {
          if (tsm < 360.0)
            (arm->jt[2]).cal -= (tsm + 45.0) - arm->JtAng(2);        // adjust lift zero angle
          else
          {
            Complain("Cannot compute forearm tilt");
            any--;                                                   // cancel following increments
            step--;
          }
        }
        any++;                                                       // number of things adjusted
        step++;                                                      
      }

      // show results
      d.ShowGrid(dcol, 1, 0, 0, "Forearm tilt = %s %s", dtxt, ((step == 3) ? " <-- sampling" : ""));
      if (step < 4)
        d.ShowGrid(mark, 0, 0, 0, "%d  --  Click %s (R to skip) ...", step + 1, hint[step]);
      else
        d.ShowGrid(mark, 0, 0, 0, "DONE  --  hand (%3.1f %3.1f %3.1f) : pan %3.1f, tilt %3.1f, roll %3.1f",
                                  pos->X(), pos->Y(), pos->Z(), dir->P(), dir->T(), dir->R());

      // check for mouse event in image or if all joints done
      gap = ((mbut != 0) ? 0 : gap + 1);         
      mbut = d.MouseRel0(mx, my);
      if ((mbut < -1) || (step > 4))
        break;

      // send any commands and start collecting next input
      ((ec.body).neck).GazeTarget(0.0, -80.0);
      arm->Limp();
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.ShowGrid(mark, 0, 0, 0, "Adjusted arm skeleton");
  d.StatusText("Stopped.");  

  // save/restore values and cleanup
  if (any > 0)
  {
    if ((step >= 4) && 
        AskNot("Adjust shoulder (%+3.1f %+3.1f) zero %+3.1f\nelbow zero %+3.1f, lift zero %+3.1f", 
               arm->ax0 - ax0, arm->ay0 - ay0, (arm->jt[0]).cal - scal, (arm->jt[1]).cal - ecal, (arm->jt[2]).cal - lcal))
      arm->SaveCfg((ec.body).LastCfg());
    else
    {
      // revert to original values
      arm->ax0 = ax0;
      arm->ay0 = ay0;
      (arm->jt[0]).cal = scal;
      (arm->jt[1]).cal = ecal;
      (arm->jt[2]).cal = lcal;
    }
  }
  res.Clone(mark);
  strcpy_s(rname, "skeleton.bmp");
}


// Smash hand onto table to calibrate wrist angles

void CBanzaiDoc::OnMotionCalibwrist()
{
  char hint[200];
  jhcImg mark;
  jhcBumps *sobj = &((ec.rwi).sobj);
  jhcEliLift *fork = &((ec.body).lift);
  jhcEliNeck *neck = &((ec.body).neck);
  jhcEliArm *arm = &((ec.body).arm);
  const jhcMatrix *pos = arm->Position(), *dir = arm->Direction();
  double tilt = -70.0, hover = 1.0, flat = 0.9, fix = 0.9, atol = 0.5, htol = 0.1;
  double mid, rcal, pcal, tcal, az0, shelf, ix0, iy0, ix1, iy1, ipan, veer, bump;
  int mx0, my0, mx1, my1, mx, my, mbut = 0, gap = 0, step = 0, adj = 0, pass = 0;

  // connect video source
  d.Clear();
  d.StringGrid(0, 0, "Preparing hardware  --  please wait ...");
  if (ChkStream() <= 0)
    return;
  (ec.body).BindVideo(&v);
  v.SizeFor(mark);
  mid = 0.5 * mark.XDim();

  // reset body
  Complain("Make sure arm is clear of surface");
  d.StatusText("Initializing robot ...");
  if ((ec.body).Reset(1, 0) <= 0)
  {
    Complain("Robot not functioning properly");
    d.StatusText("Failed.");
    return;
  }
  (ec.rwi).Reset(1, 0);

  // save original values
  rcal = (arm->jt[3]).cal;
  pcal = (arm->jt[4]).cal;
  tcal = (arm->jt[5]).cal;
  az0  = arm->az0;

  // start up background processing
  d.Clear(1, "Calibrating wrist ...");
  try
  {
    while (1)
    {
      // update joint angles and Kinect images
      if ((ec.rwi).Update() <= 0)
        break;

      // adjust lift to be above table
      if (step == 0)
      {
        sprintf_s(hint, "Adjusting to surface (hover %3.1f)", fork->LiftErr(sobj->ztab, 0));
        shelf = sobj->ztab + hover;
        if (fork->LiftErr(shelf) > 0.5)
          fork->LiftTarget(shelf, 0.25);         // slower lets vision catch up
        else 
          step++;
      }

      // level hand and determine finger direction
      if (step == 1)
        sprintf_s(hint, "Flatten hand on surface then click on jaw axis: pan %3.1f, tilt %3.1f, roll %3.1f, above %3.1f", 
                         dir->P(), dir->T(), dir->R(), (pos->Z() + fork->Height()) - sobj->ztab);

      // determine finger direction
      if (step == 2)
        sprintf_s(hint, "Click between finger tips: pan %3.1f, tilt %3.1f, roll %3.1f, above %3.1f", 
                         dir->P(), dir->T(), dir->R(), (pos->Z() + fork->Height()) - sobj->ztab);

      // make adjustments one at a time, let settle, then iterate
      if (step == 3)
      {
        // find pan and height errors
        (ec.rwi).ImgJt(ix0, iy0, 6);                                     // find jaw to tips angle
        (ec.rwi).ImgJt(ix1, iy1, 7);   
        ipan = R2D * atan2(iy1 - iy0, ix1 - ix0);
        veer = R2D * atan2(my1 - my0, mx1 - mx0);                        // user supplied angle
        bump = (pos->Z() + fork->Height()) - (sobj->ztab + flat);        // known finger offset

        // make some gradual adjustment
        if (adj <= 0)                            
          (arm->jt[3]).cal += fix * dir->R();                            // flat gripper
        else if (adj == 1)                       
          (arm->jt[4]).cal += fix * (veer - ipan);
        else if (adj == 2)                       
          (arm->jt[5]).cal += fix * dir->T();                            // flat gripper
        else if (adj == 3)                       
          arm->az0 -= bump;
        else if (adj >= 4)                       
          if ((pass++ > 5) || ((fabs(veer) <= atol) && (fabs(dir->T()) <= atol) && 
                               (fabs(dir->R()) <= atol) && (fabs(bump) <= htol)))
            step++;                                                      
        if (++adj > 4)                                                   // adjust next value
          adj = 0;
      }        

      // draw arm on color image and possibly indicate actual pan angle
      (ec.body).ImgBig(mark);
      DrawLine(mark, mid, 0, mid, mark.YDim(), 1, -4);
      if (step == 2)
        Cross(mark, mx0, my0, 17, 17, 1, -6);
      else if (step >= 3)
        Ray(mark, mx0, my0, veer, 0.0, 1, -6);
      (ec.rwi).Skeleton(mark); 

      // show results
      if (step <= 2)
        d.ShowGrid(mark, 0, 0, 0, "%s ...", hint);
      else
        d.ShowGrid(mark, 0, 0, 0, "DO NOT MOVE -- errors: pan %+3.1f, tilt %+3.1f, roll %+3.1f, above %+3.1f ...", 
                                  dir->P() - veer, dir->T(), dir->R(), bump);
                               
      // check for mouse event in image
      if (step == 1)
        mbut = d.MouseRel0(mx0, my0);
      else if (step == 2)
        mbut = d.MouseRel0(mx1, my1);
      else
        mbut = d.MouseRel0(mx, my);
      if ((mbut < -1) || (mbut == 3) || (step >= 4))
        break;

      // advance state and record position if needed
      if ((step == 1) || (step == 2))
        if ((mbut == 1) && (gap >= 15))
          step++;
      gap = ((mbut != 0) ? 0 : gap + 1);    

      // send any commands and start collecting next input
      neck->GazeTarget(0.0, tilt);
      arm->Limp();
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  d.ShowGrid(mark, 0, 0, 0, "Adjusted gripper errors: pan %+3.1f, tilt %+3.1f, roll %+3.1f, above %+3.1f", 
                             dir->P() - veer, dir->T(), dir->R(), (pos->Z() + fork->Height()) - sobj->ztab);
  d.StatusText("Stopped.");  

  // cleanup
  if ((step >= 4) &&
      AskNot("Adjust pan %+3.1f, tilt %+3.1f, roll %+3.1f, z %+3.1f", 
             (arm->jt[4]).cal - pcal, (arm->jt[5]).cal - tcal, (arm->jt[3]).cal - rcal, arm->az0 - az0))
    arm->SaveCfg((ec.body).LastCfg());
  else
  {
    // revert to original values
    (arm->jt[3]).cal = rcal;
    (arm->jt[4]).cal = pcal;
    (arm->jt[5]).cal = tcal;
    arm->az0 = az0;
  }
  res.Clone(mark);
  strcpy_s(rname, "wrist_cal.bmp");
}


// Move whole arm to achieve good height over detected surface

void CBanzaiDoc::OnManipulationAdjustz()
{
  jhcEliLift *fork = &((ec.body).lift);
  double start = 30.0, tilt = -70.0, hover = -(ec.man).wz0;
  double tz, shelf, hp0, g0 = (eb->neck).gaze0;
  int gh0 = d.ght, step = 0;

  // make sure video is working
  d.Clear();
  d.StringGrid(0, 0, "Preparing hardware  --  please wait ...");
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }
  (eb->neck).gaze0 = tilt;
  (ec.rwi).Reset(rob, 0);

  // start up background processing
  d.ght = 500;
  d.Clear(1, "Adjusting arm height ...");
  try     
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // update joint angles and Kinect images
      hp0 = ((ec.rwi).tab).hpref;
      if ((ec.rwi).Update() <= 0)
        break;
      tz = ((ec.rwi).sobj).ztab;

      // set arm high enough for counter
      if (step <= 0)
      {
        if (fork->LiftErr(start) > 0.5)
          fork->LiftTarget(start);
        else
        {
          ((ec.rwi).tab).Reset();
          step++;
        }
      }

      // adjust lift to be above table
      if (step >= 1)
      {
        shelf = tz + hover;
        if (fork->LiftErr(shelf) > 0.5)
          fork->LiftTarget(shelf, 0.25);         // slower lets vision catch up
      }

      // show results
      if (step <= 0)
        d.ShowGrid((ec.body).Color(), 0, 0, 0, "Rising to table height ... %3.1f", fork->Height());
      else
      {
        // also show various height planes considered
        d.ShowGrid((ec.body).Color(),      0, 0, 0,    "Surface at %3.1f\"  --  hovering at %3.1f\"", tz, fork->LiftErr(tz, 0));
        d.GraphGridV(((ec.rwi).tab).hhist, 1, 0, 0, 0, "Height clusters = %3.1f in", ((ec.rwi).tab).ztab);
        d.GraphValV(200, 0, 4);
        d.GraphMarkV(((ec.rwi).s3).I2Z(((ec.rwi).tab).ztab), 1);
        d.GraphMarkV(((ec.rwi).tab).i2z(hp0), 2, 0.5);
      }

      // send any commands and start collecting next input
      ((ec.body).neck).GazeTarget(0.0, tilt);
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  (eb->neck).gaze0 = g0;
  d.ght = gh0;
  d.StatusText("Stopped.");  

  // cleanup
  res.Clone((ec.body).Color());
  strcpy_s(rname, "arm_hover.bmp"); 
}


// Report hand position relative to a selected object

void CBanzaiDoc::OnManipulationHandeye()
{
  jhcImg mark, pat2;
  jhcMatrix obj(4);
  jhcSurfObjs *sobj = &((ec.rwi).sobj);
  jhcEliArm *arm = &((ec.body).arm);
  const jhcMatrix *pos = arm->Position(), *dir = arm->Direction();
  double ang, over, g0 = (eb->neck).gaze0, tilt = -70.0;
  int mx, my, mbut = 0, item = -1;

  // make sure video is working
  d.Clear();
  d.StringGrid(0, 0, "Preparing hardware  --  please wait ...");
  if (ChkStream() <= 0)
    return;
  v.SizeFor(mark);

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }

  // start vision routines
  (eb->neck).gaze0 = tilt;
  (ec.rwi).Reset(rob, 0);
  pat2.SetSize(sobj->pat);

  // start up background processing
  d.Clear(1, "Hand-eye coordination ...");
  try     
  {
    while (1)
    {
      // update joint angles and Kinect images
      if ((ec.rwi).Update() <= 0)
        break;

      // get target information
      if (item >= 0)
      {
        ang = sobj->World(obj, item);
        over = (pos->Z() + ((ec.body).lift).Height()) - (sobj->ztab + sobj->SizeZ(item));
        sobj->Retain(item);
      }

      // make pretty pictures of detections
      pat2.Clone(sobj->pat);
      if (item < 0)
        sobj->ShowAll(pat2);
      else
        Cross(pat2, sobj->MapX(item), sobj->MapY(item), 150, 150, 3, -4);
      (ec.rwi).MapArm(pat2);

      // draw arm on color image
      (ec.body).ImgBig(mark);
      (ec.rwi).Skeleton(mark); 

      // show results
      d.ShowGrid(mark,   0, 0, 0, "Color image (%3.1f\" surface) %s", sobj->ztab, 
                                  ((item < 0) ? " --  Mouse on overhead projection  ==>" : ""));
      if (item < 0)
        d.ShowGrid(pat2, 1, 0, 0, "Click one of %d objects (R to exit) ...", sobj->CntTracked());
      else
        d.ShowGrid(pat2, 1, 0, 0, "Hand offset (%+3.1f %+3.1f) over %+3.1f  --  align %+3.1f, pinch %+3.1f (tilt %3.1f, roll %3.1f)", 
                                  pos->X() - obj.X(), pos->Y() - obj.Y(), over, 
                                  dir->P() - ang, arm->Width() - sobj->Minor(item), dir->T(), dir->R());

      // see if any object was clicked (inside last image = pat2)
      mbut = d.MouseRel0(mx, my);     
      if (d.MouseExit(mbut) > 0)
        break;
      if (mbut == 1)
        item = sobj->ClickN(mx, my);

      // send any commands and start collecting next input
      ((ec.body).neck).GazeTarget(0.0, tilt);
      arm->Limp();
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  (eb->neck).gaze0 = g0;
  d.StatusText("Stopped.");  

  // cleanup
  res.Clone(pat2);
  strcpy_s(rname, "hand_eye.bmp"); 
}


// Go to pre-grasp position relative to some object 

void CBanzaiDoc::OnManipulationGotovia()
{
  jhcImg mark, pat2;
  jhcMatrix grab(4), via(4), end(4), aim(4), perr(4), derr(4);
  jhcSurfObjs *sobj = &((ec.rwi).sobj);
  jhcEliArm *arm = &((ec.body).arm);
  const jhcMatrix *pos = arm->Position(), *dir = arm->Direction();
  double ht, ang, diff, ix, iy, wid = 0.0, g0 = (eb->neck).gaze0, tilt = -70.0;
  int mbut, mx, my, rc, item = -1, done = 0;

  // make sure video is working
  d.Clear();
  d.StringGrid(0, 0, "Preparing hardware  --  please wait ...");
  if (ChkStream() <= 0)
    return;
  v.SizeFor(mark);

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }

  // start vision routines
  (eb->neck).gaze0 = tilt;
  (ec.rwi).Reset(rob, 0);
  pat2.SetSize(sobj->pat);

  // start up background processing
  d.Clear(1, "Move hand to via ...");
  try     
  {
    while (1)
    {
      // update joint angles and Kinect images
      if ((ec.rwi).Update() <= 0)
        break;
      ht = ((ec.body).lift).Height();

      // get target info 
      sobj->Retain(item);
      if (item >= 0) 
      {
        // compute via point once (wid > 0 afterwards) ignoring obstacles
        if (wid == 0.0)
          if ((rc = (ec.man).pick_grasp(wid, ang, grab, item)) >= 0)
            via.SetVec3(grab.X(), grab.Y(), sobj->MaxZ(item) + (ec.man).over);

        // check current gripper alignment
        diff = dir->P() - ang;
        if (diff > 180.0)
          diff -= 360.0;
        else if (diff <= -180.0)
          diff += 360.0;
      }

      // make pretty pictures of detections
      pat2.Clone(sobj->pat);
      if (item < 0)
        sobj->ShowAll(pat2);
      else
      {
        sobj->ViewPels(ix, iy, grab.X(), grab.Y());
        Cross(pat2, ix, iy,  75,  75, 3, -6);
      }
      (ec.rwi).MapArm(pat2);

      // draw arm on color image
      (ec.body).ImgBig(mark);
      (ec.rwi).Skeleton(mark); 

      // show results
      d.ShowGrid(mark,   0, 0, 0, "Color image (%3.1f\" surface) %s", sobj->ztab, 
                                  ((item < 0) ? " --  Mouse on overhead projection  ==>" : ""));
      if (item < 0)
        d.ShowGrid(pat2, 1, 0, 0, "Click one of %d objects (R to exit) ...", sobj->CntTracked());
      else if (rc < 0)
        d.ShowGrid(pat2, 1, 0, 0, "Problem: %3.1f x %3.1f inches (R to exit) ...", sobj->Minor(item), sobj->Major(item));
      else
        d.ShowGrid(pat2, 1, 0, 0, "Hand offset (%+4.2f %+4.2f) over %+4.2f  --  align %+3.1f, pinch %+3.1f (tilt %3.1f, roll %3.1f) %s",
                                  pos->X() - via.X(), pos->Y() - via.Y(), (pos->Z() + ht) - via.Z(),
                                  diff, arm->Width() - wid, dir->T(), dir->R(), ((done > 0) ? " --  DONE" : ""));

      // see if graspable object selected 
      if ((item >= 0) && (rc >= 0))
      {
        // compute arm target and orientation
        end.RelVec3(via, 0.0, 0.0, -ht);
        aim.SetVec3(ang, -(ec.man).tip, 0.0);

        // stop moving if close to destination
        arm->ArmErr(perr, derr, end, aim);
        if ((perr.MaxAbs3() > (ec.man).ptol) || (arm->WidthErr(wid) > (ec.man).wtol))
        {
          arm->PosTarget(end);
          arm->DirTarget(aim);
          arm->WidthTarget(wid);
        }
        else
          done = 1;
      }

      // see if any object was clicked (inside last image = pat2)
      mbut = d.MouseRel0(mx, my);     
      if (d.MouseExit(mbut) > 0)
        break;
      if (mbut == 1)
        item = sobj->ClickN(mx, my);

      // send any commands and start collecting next input
      ((ec.body).neck).GazeTarget(0.0, tilt);
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  (eb->neck).gaze0 = g0;
  d.StatusText("Stopped.");  

  // cleanup
  res.Clone(pat2);
  strcpy_s(rname, "via.bmp"); 
}

// Shift object from one location to another
// does not adjust gaze, base, or fork lift stage

void CBanzaiDoc::OnManipulationMoveobj()
{
  char phase[11][40] = {"", "", "REACH", "ENCLOSE", "GRAB", "TRANSFER", 
                        "PLACE", "RELEASE", "BACKOFF", "TUCK", "DONE"};
  jhcImg mark, pat2;
  jhcMatrix obj(4), tpos(4);
  jhcArr ax(400), ay(400), az(400), tx(400), ty(400), tz(400);
  int div[9];
  jhcSurfObjs *sobj = &((ec.rwi).sobj);
  jhcManipulate *man = &(ec.man);
  const jhcMatrix *pos = (eb->arm).Position();
  double g0 = (eb->neck).gaze0, tilt = -70.0;
  double zinc2, xinc = -0.5 * (man->wx0 + man->wx1), top = 10.0;
  double yinc = -0.5 * (man->wy0 + man->wy1 - top), zinc = 1.0;      // grip z wrt table
  double x, y, wx, wy, wz, sx, sy, gx, gy, dx, dy, off, gx0, gy0;
  int h0 = d.ght, w0 = d.gwid, rng = ROUND(1000.0 * top), trail = 400;
  int fn = 0, nraw = 0, n = 0, gap = 0, item = -1, dest = 0, rc = 0, fat = 0;
  int i, mbut, mx, my, id, n0, end, start;

  // clear graph data
  ax.Fill(ROUND(1000.0 * xinc));
  tx.Fill(ROUND(1000.0 * xinc));
  ay.Fill(ROUND(1000.0 * yinc));
  ty.Fill(ROUND(1000.0 * yinc));
  az.Fill(ROUND(1000.0 * zinc));
  tz.Fill(ROUND(1000.0 * zinc));
  for (i = 0; i < 9; i++)
    div[i] = 0;

  // make sure video is working
  d.Clear();
  d.StringGrid(0, 0, "Preparing hardware  --  please wait ...");
  if (ChkStream() <= 0)
    return;
  v.SizeFor(mark);

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }

jprintf_open();
  // start vision routines
  (eb->neck).gaze0 = tilt;
  man->Reset(&(ec.atree));
  (ec.rwi).Reset(rob, 0);
  pat2.SetSize(sobj->pat);

  // start up background processing
  d.Clear(1, "Move visible object ...");
  try     
  {
    while (1)
    {
      // update joint angles and Kinect images
      if ((ec.rwi).Update() <= 0)
        break;

      // shift marked positions and arm history if base moves
      (eb->base).AdjustXY(sx, sy);
      (eb->base).AdjustXY(wx, wy);
      end = __min(fn, 400);
      for (i = 0; i < end; i++)
      {
        // commanded position
        x = 0.001 * tx.ARef(i) - xinc;
        y = 0.001 * ty.ARef(i) - yinc;
        (eb->base).AdjustXY(x, y);
        tx.ASet(i, ROUND(1000.0 * (x + xinc)));
        ty.ASet(i, ROUND(1000.0 * (y + yinc)));  

        // actual arm position
        x = 0.001 * ax.ARef(i) - xinc;
        y = 0.001 * ay.ARef(i) - yinc;
        (eb->base).AdjustXY(x, y);
        ax.ASet(i, ROUND(1000.0 * (x + xinc)));
        ay.ASet(i, ROUND(1000.0 * (y + yinc)));  
      }

      // record new hand command and actual position
      (eb->arm).PosGoal(tpos);
      tx.Scroll(fn, ROUND(1000.0 * (tpos.X() + xinc)));
      ax.Scroll(fn, ROUND(1000.0 * (pos->X() + xinc)));
      ty.Scroll(fn, ROUND(1000.0 * (tpos.Y() + yinc)));
      ay.Scroll(fn, ROUND(1000.0 * (pos->Y() + yinc)));
      zinc2 = zinc + (eb->lift).Height() - ((ec.rwi).tab).ztab; 
      tz.Scroll(fn, ROUND(1000.0 * (tpos.Z() + zinc2)));
      az.Scroll(fn, ROUND(1000.0 * (pos->Z() + zinc2)));
      if (++fn >= 400)
        for (i = 0; i < 9; i++)
          div[i] -= 1;
    
      // if enough info run manipulation sequences
      if ((item >= 0) && (dest > 0))
      {
        // find distance of object to goal location
        sobj->World(obj, item);
        dx = obj.X() - wx;
        dy = obj.Y() - wy;
        off = sqrt(dx * dx + dy * dy);

        // run controller (unless already failed)
        if ((rc >= 0) && (nraw != 10) && (nraw != 22))
        {
          // check for execution error or bad object
          rc = man->Move();
          fat = man->LastErr();
          n0 = n;
          nraw = man->State();
          n = __max(0, __min(nraw, 10));
     
          // record phase change markers for graphs
          if ((n > n0) || (rc < 0))
            div[n0] = fn + 1;          
          if (fat < 0)
            div[n] = fn + 2;
        }
      }

      // show detected objects or selected object and goal position
      pat2.Clone(sobj->pat);
      man->Workspace(pat2, -6);
      if ((item >= 0) && (dest > 0))
      {
        sobj->ViewPels(gx, gy, wx, wy);
        Cross(pat2, gx, gy, 75, 75, 3, -2);      // destination
      }
      if (item >= 0)
      {
        sobj->ViewPels(gx, gy, sx, sy);          // source
        Cross(pat2, gx, gy, 49, 49, 3, -4);
      }
      else
        sobj->ShowAll(pat2);
      (ec.rwi).MapArm(pat2, 0.0);

      // show path of grip point over time
      end = __min(fn, 400);
      start = __max(0, end - trail);
      for (i = start; i < end; i++)
      {
        sobj->ViewPels(gx, gy, 0.001 * ax.ARef(i) - xinc, 0.001 * ay.ARef(i) - yinc);
        if (i > start)
          DrawLine(pat2, gx0, gy0, gx, gy, 3, -1);
        gx0 = gx;
        gy0 = gy;
      }

      // draw objects and arm on color image
      (ec.body).ImgBig(mark);
      sobj->ObjsCam(mark, 0);
      (ec.rwi).Skeleton(mark); 

      // show basic view from robot
      if ((item < 0) || (dest <= 0))
        d.ShowGrid(mark, 0, 0, 0, "Color image (%3.1f\" surface)  --  Mouse on overhead projection  ==>", sobj->ztab);
      else
        d.ShowGrid(mark, 0, 0, 0, "Color image (%3.1f\" surface)  --  Move to (%3.1f %3.1f) --> Object %d at (%3.1f %3.1f)", 
                                   sobj->ztab, wx, wy, sobj->ObjID(item), obj.X(), obj.Y());

      // show arm X position graph (vertical)
      d.gwid = 480;
      d.ght  = 400;
      d.GraphGridV(ax, 0, 1, -rng, 0, "X position of grip point vs. command");
      for (i = 0; i < n; i++)
        d.GraphMarkV(div[i], (((i == 0) || (i == 8)) ? 2 : 6));
      if (rc < 0)
        d.GraphMarkV(div[n], 1);
      d.GraphOverV(tx, -rng, 5);
      d.GraphOverV(ax, -rng, 8);

      // show arm X position graph (horizontal)
      d.gwid = 400;
      d.ght  = 240;
      d.GraphAdj( ay, rng, 0, "Y position of grip point vs. command");
      for (i = 0; i < n; i++)
        d.GraphMark(div[i], (((i == 0) || (i == 8)) ? 2 : 6));
      if (rc < 0)
        d.GraphMark(div[n], 1);
      d.GraphOver(ty, rng, 5);
      d.GraphOver(ay, rng, 8);

      // show arm Z position graph (horizontal)
      d.GraphAdj( az, rng, 0, "Z position of grip point vs. command");
      d.GraphVal(ROUND(1000.0 * zinc), rng, 4);
      for (i = 0; i < n; i++)
        d.GraphMark(div[i], (((i == 0) || (i == 8)) ? 2 : 6));
      if (rc < 0)
        d.GraphMark(div[n], 1);
      d.GraphOver(tz, rng, 5);
      d.GraphOver(az, rng, 8);

      // show overhead view (last for mouse focus)
      if (item < 0) 
        d.ShowGrid(pat2, 1, 0, 0, "Click one of %d objects (R to exit) ...", sobj->CntTracked());
      else if (dest <= 0)
        d.ShowGrid(pat2, 1, 0, 0, "Click destination for object (R to exit) ...");
      else if (fat < 0)
        d.ShowGrid(pat2, 1, 0, 0, "Problem: object %3.1f x %3.1f inches (R to exit) ...", 
                                  sobj->Minor(item), sobj->Major(item));
      else
        d.ShowGrid(pat2, 1, 0, 0, "%s  --  dx %+4.1f, dy %+4.1f --> radial offset %3.1f\" %s", 
                                  phase[n], dx, dy, off, ((rc < 0) ? " --  failed!" : ""));

      // see if any object was clicked (inside last image = pat2)
      mbut = d.MouseRel0(mx, my);     
      if (d.MouseExit(mbut) > 0)
        break;
      if ((mbut == 1) && (gap >= 15))
      {
        if ((item < 0) || (fat > 0))
        {
          // select item and remember grasp point
          item = sobj->ClickN(mx, my);
          man->ForceItem(item);
          id = sobj->ObjID(item);
          sobj->World(sx, sy, item);
        }
        else if (dest <= 0) 
        {
          // select deposit point
          sobj->PelsXY(wx, wy, mx, my);
          wz = sobj->MapZ(mx, my, 5, sobj->ztab);
          man->ForceDest(wx, wy, wz);
          dest = 1;
        }
      }
      gap = ((mbut != 0) ? 0 : gap + 1);         // separation between clicks

      // send any commands and start collecting next input
      ((ec.body).neck).GazeTarget(0.0, tilt);
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  (eb->neck).gaze0 = g0;
  d.ght = h0;
  d.gwid = w0;
  d.StatusText("Stopped.");  
jprintf_close();

  // cleanup
  res.Clone(pat2);
  strcpy_s(rname, "move_obj.bmp"); 
}


// Show selected final location for some object given a relation to another

void CBanzaiDoc::OnManipulationDeposit()
{
  jhcPickString pick;
  char prompt[80], rel[40] = "left of";
  int side[4] = {180, 0, -90, 90};
  jhcSurfObjs *sobj = &((ec.rwi).sobj);
  jhcManipulate *man = &(ec.man);
  jhcImg map2, space2, align2, shrink2;
  double ix, iy, dx, dy, wid, len, ang, pan = 90.0, g0 = (eb->neck).gaze0, tilt = -70.0;
  int mbut, mx, my, cx, cy, ok, t = -1, rn = -1, a = -1, a2 = -1, gap = 0, phase = 0;

  // get destination location relation first
  while ((rn < 0) || (rn == jhcManipulate::ON))
  {
    if (pick.EditString(rel, 0, "Relation: down, between, etc.") <= 0)
      return;
    rn = man->txt2rnum(rel);
  }

  // make sure video is working
  d.Clear();
  d.StringGrid(0, 0, "Preparing hardware  --  please wait ...");
  if (ChkStream() <= 0)
    return;

  // initialize robot system (no ALIA)
  d.StatusText("Initializing robot ...");
  if (rob > 0)
    if (eb->Reset(1, 0) <= 0)
    {
      d.StatusText("Failed.");
      return;
    }

  // start vision routines
  (eb->neck).gaze0 = tilt;
  man->Reset(&(ec.atree));
  (ec.rwi).Reset(rob, 0);
  (eb->arm).Limp();

  // find middle of map images and clear intermediates
  cx = (sobj->XDim()) >> 1;
  cy = (sobj->YDim()) >> 1;
  man->set_size(sobj->map);
  (man->align).FillArr(0);
  (man->shrink).FillArr(0);

  // start up background processing
  d.Clear(1, "Deposit location ...");
  v.Rewind(1);
  try
  {
    while (1)
    {
      // get images and body data then process it
      if ((ec.rwi).Update() <= 0)
        break;

      // interact with space routines in jhcManipulate
      if (phase >= 0)
      {
        sobj->RetainAll();
        man->htrk = t;
      }
      man->free_space(t);
      if (phase >= 3)
      {
        man->dest_ref(ix, iy, t, rn, a, a2);
        pan = man->dest_ang(ix, iy, t, rn, a, a2);
        ok = man->pick_spot(dx, dy, ix, iy, pan, t, rn, a, a2); 
      }

      // copy intermediate images for annotation
      map2.Clone(sobj->map);
      space2.Clone(man->space);
      align2.Clone(man->align);
      shrink2.Clone(man->shrink);
  
      // visually mark important objects
      sobj->ShowAll(map2, 1, 0, 0);
      if (t >= 0)
        XMark(map2, sobj->MapX(t), sobj->MapY(t), 15, 1);
      if (a >= 0)
        Cross(map2, sobj->MapX(a), sobj->MapY(a), 25, 25, 1);
      if (a2 >= 0)
        Cross(map2, sobj->MapX(a2), sobj->MapY(a2), 25, 25, 1);

      // mark spatial relation reference point in intermediate images
      if (phase >= 3)
      {
        if ((rn >= 2) && (rn <= 5))
        {
          ang = sobj->ViewAngle(side[rn - 2]);
          Ray(space2, ix, iy, ang + man->sdev, 0.0, 1, 90);
          Ray(space2, ix, iy, ang - man->sdev, 0.0, 1, 90);
        }
        Cross(space2, ix, iy, 17, 17, 1, 200);
        Cross(align2, cx, cy, 17, 17, 1, 200);
        if ((rn >= 2) && (rn <= 5))
        {
          ang = sobj->ViewAngle(side[rn - 2]) + (90.0 - pan);
          Ray(shrink2, cx, cy, ang + man->sdev, 0.0, 1, 50); 
          Ray(shrink2, cx, cy, ang - man->sdev, 0.0, 1, 50); 
        }
        Cross(shrink2, cx, cy, 17, 17, 1, 200);
      }

      // mark chosen deposit location and orientation
      if ((phase >= 3) && (ok > 0))
      {
        wid = 1.2 * sobj->I2P(sobj->Minor(t));
        len = 1.2 * sobj->I2P(sobj->Major(t));
        RectCent(map2, dx, dy, len, wid, pan, 1);
        RectCent(space2, dx, dy, len, wid, pan, 3, -5);    
        XMark(shrink2, man->xpick, man->ypick, 11, 3, 128);
      }

      // assemble prompt based on phase
      if (strcmp(rel, "down") == 0)
      {
        if (phase <= 0)
          sprintf_s(prompt, "X down\"  <--  click on object to move ...");
        else if (phase >= 1)
          sprintf_s(prompt, "object %d down\"", sobj->ObjID(t));
      }
      else if (strcmp(rel, "between") == 0)
      {
        if (phase <= 0)
          sprintf_s(prompt, "X between Y and Z\"  --  click on object to move ...");
        else if (phase == 1)
          sprintf_s(prompt, "object %d between Y and Z\"  --  click on reference object ...", sobj->ObjID(t));
        else if (phase == 2)
          sprintf_s(prompt, "object %d between objects %d and Z\"  --  click on second reference object ...", sobj->ObjID(t), sobj->ObjID(a));
        else if (phase >= 3)
          sprintf_s(prompt, "object %d between objects %d and %d\"" , sobj->ObjID(t), sobj->ObjID(a), sobj->ObjID(a2));
      }
      else if (phase <= 0)
         sprintf_s(prompt, "X %s Y\"  --  click on object to move ...", rel);
      else if (phase == 1)
        sprintf_s(prompt, "object %d %s Y\"  --  click on reference object ...", sobj->ObjID(t), rel); 
      else if (phase >= 2)
        sprintf_s(prompt, "object %d %s object %d\"", sobj->ObjID(t), rel, sobj->ObjID(a));

      // show results of processing
      if (phase < 3)
        d.ShowGrid(space2, 1, 0, 2, "Unoccupied surface");
      else if (ok <= 0)
        d.ShowGrid(space2, 1, 0, 2, "Unoccupied surface  --  object %d does not fit!", sobj->ObjID(t)); 
      else
        d.ShowGrid(space2, 1, 0, 2, "Unoccupied surface and destination for object %d", sobj->ObjID(t)); 
      if (pan == 90.0)
        d.ShowGrid(align2, 0, 1, 2, "Aligned around reference location (no rotation)");
      else
        d.ShowGrid(align2, 0, 1, 2, "Aligned around reference location  --  rotated %sclockwise %3.1f degs", 
                                    (((90.0 - pan) >= 0) ? "counter" : ""), fabs(90.0 - pan));
      d.ShowGrid(shrink2,  1, 1, 2, "Shrunken by deposit footprint");
      d.ShowGrid(map2,     0, 0, 2, "Command:  \"Put %s", prompt);

      // see if any object was clicked (inside last image = map2)
      if (phase >= 3)
        break;
      mbut = d.MouseRel0(mx, my);     
      if (d.MouseExit(mbut) > 0)
        break;
      if ((mbut == 1) && (gap >= 15))
      {
        if (phase == 0)
        {
          if ((t = sobj->ClickN(mx, my)) >= 0)
            phase = ((strcmp(rel, "down") == 0) ? 3 : 1);      // no anchors needed
        }
        else if (phase == 1)
        {
          if ((a = sobj->ClickN(mx, my)) >= 0)
            phase = ((strcmp(rel, "between") != 0) ? 3 : 2);   // only one anchor needed
        }
        else if (phase == 2)
        {
          if ((a2 = sobj->ClickN(mx, my)) >= 0)
            phase = 3;
        }
      }
      gap = ((mbut != 0) ? 0 : gap + 1);         // separation between clicks

      // prompt for new sensors
      (ec.rwi).Issue();
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  (ec.rwi).Stop();
  (eb->neck).gaze0 = g0;
  d.StatusText("Stopped.");  

  // cleanup
  FalseClone(res, space2);
  sprintf_s(rname, "deposit %s.bmp", rel); 
}


/////////////////////////////////////////////////////////////////////////////
//                              Test Functions                             //
/////////////////////////////////////////////////////////////////////////////

// test function for current fragment of code

void CBanzaiDoc::OnUtilitiesTest()
{
//  Tell("No current function");

  (ec.body).BindVideo(NULL);
  ec.Reset(0);
  ec.SetPeople("VIPs.txt");

//  Tell("%d words in file: words.txt", (ec.vc).ListAll());

//  char txt[80] = "..tell me, is theRe icecream 'on' the fuzzy SURFace?!";
//  jprintf("Marked: <%s>\n", (ec.vc).MarkBad(txt));

/*
  char txt2[6][80] = {"grab the fuzzy llama!", 
                      "the violet block is on the rough plank",
                      "the aardvark is angry",
                      "aardvarks are angrier than bandicoots",
                      "aardvarks are the angriest animals",
                      "the lion's tongue is scratchy"};
  for (int i = 0; i < 6; i++)
    jprintf("--> <%s>\n", (ec.vc).MarkBad(txt2[i]));
*/


  char txt3[5][80] = {"whatcolor is th eobject i nthe front?", 
                     "he siad shew as ferocius",
                     "wht colr is the thign?",
                     "is ee an objcet"};
  for (int i = 0; i < 4; i++)
  {
    jprintf("fix: %s\n", txt3[i]);
    (ec.vc).FixTypos(txt3[i]);
    jprintf(" --> %s\n", (ec.vc).Fixed());
  }

}


