// MensEtDoc.cpp : top level GUI framework does something
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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
#include "MensEt.h"

#include "MensEtDoc.h"

#include <direct.h>                  // for _getcwd in Windows


// JHC: for filtered video file open

extern CMensEtApp theApp;


// JHC: for basic functionality

#include "Data/jhcImgIO.h"
#include "Interface/jhcMessage.h"
#include "Interface/jhcPickVals.h"
#include "Interface/jhcString.h"
#include "Interface/jms_x.h"

#include "Manus/jhcInteractFSM.h"


// whether to do faster background video capture

#define FASTVID 1    // some cameras need zero?


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMensEtDoc

IMPLEMENT_DYNCREATE(CMensEtDoc, CDocument)

BEGIN_MESSAGE_MAP(CMensEtDoc, CDocument)
	//{{AFX_MSG_MAP(CMensEtDoc)
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
  ON_COMMAND(ID_ENVIRONMENT_LOADGEOM, &CMensEtDoc::OnEnvironmentLoadgeom)
  ON_COMMAND(ID_ENVIRONMENT_SAVEGEOM, &CMensEtDoc::OnEnvironmentSavegeom)
  ON_COMMAND(ID_UTILITIES_TEST, &CMensEtDoc::OnUtilitiesTest)
  ON_COMMAND(ID_MANUS_DRIVEPARAMS, &CMensEtDoc::OnManusDriveparams)
  ON_COMMAND(ID_MANUS_RANGEPARAMS, &CMensEtDoc::OnManusRangeparams)
  ON_COMMAND(ID_MANUS_LIFTPARAMS, &CMensEtDoc::OnManusLiftparams)
  ON_COMMAND(ID_MANUS_GRIPPARAMS, &CMensEtDoc::OnManusGripparams)
  ON_COMMAND(ID_DEMO_OPTIONS, &CMensEtDoc::OnDemoOptions)
  ON_COMMAND(ID_DEMO_INTERACT, &CMensEtDoc::OnDemoInteract)
  ON_COMMAND(ID_DEMO_REMOTE, &CMensEtDoc::OnDemoRemote)
  ON_COMMAND(ID_PARAMETERS_REMOTEPARAMS, &CMensEtDoc::OnParametersRemoteparams)
  ON_COMMAND(ID_MANUS_CAMERAPARAMS, &CMensEtDoc::OnManusCameraparams)
  ON_COMMAND(ID_CAMERAPARAMS_DEWARP, &CMensEtDoc::OnCameraparamsDewarp)
  ON_COMMAND(ID_PROCESSING_GROUNDPLANE, &CMensEtDoc::OnProcessingGroundplane)
  ON_COMMAND(ID_PROCESSING_CLEANUP, &CMensEtDoc::OnProcessingCleanup)
  ON_COMMAND(ID_PARAMETERS_PATCHAREA, &CMensEtDoc::OnParametersPatcharea)
  ON_COMMAND(ID_PARAMETERS_FLOORPARAMS, &CMensEtDoc::OnParametersFloorparams)
  ON_COMMAND(ID_VISION_OBJECTS, &CMensEtDoc::OnVisionObjects)
  ON_COMMAND(ID_VISION_BOUNDARY, &CMensEtDoc::OnVisionBoundary)
  ON_COMMAND(ID_REFLEXES_COZYUP, &CMensEtDoc::OnReflexesCozyup)
  ON_COMMAND(ID_REFLEXES_ENGULF, &CMensEtDoc::OnReflexesEngulf)
  ON_COMMAND(ID_REFLEXES_ACQUIRE, &CMensEtDoc::OnReflexesAcquire)
  ON_COMMAND(ID_REFLEXES_DEPOSIT, &CMensEtDoc::OnReflexesDeposit)
  ON_COMMAND(ID_REFLEXES_STACK, &CMensEtDoc::OnReflexesStack)
  ON_COMMAND(ID_REFLEXES_OPEN, &CMensEtDoc::OnReflexesOpen)
  ON_COMMAND(ID_REFLEXES_CLOSE, &CMensEtDoc::OnReflexesClose)
  ON_COMMAND(ID_MANUS_WIDTHPARAMS, &CMensEtDoc::OnManusWidthparams)
  ON_COMMAND(ID_VISION_STACKGROW, &CMensEtDoc::OnVisionStackgrow)
  ON_COMMAND(ID_PARAMETERS_SHAPEPARAMS, &CMensEtDoc::OnParametersShapeparams)
  ON_COMMAND(ID_VISION_COLORDIFFS, &CMensEtDoc::OnVisionColordiffs)
  ON_COMMAND(ID_VISION_SIMILARREGIONS, &CMensEtDoc::OnVisionSimilarregions)
  ON_COMMAND(ID_FILE_SAVEINPUT, &CMensEtDoc::OnFileSaveinput)
  ON_COMMAND(ID_PARAMETERS_CLEANPARAMS, &CMensEtDoc::OnParametersCleanparams)
  ON_COMMAND(ID_VISION_FEATURES, &CMensEtDoc::OnVisionFeatures)
  ON_COMMAND(ID_PARAMETERS_QUANTPARAMS, &CMensEtDoc::OnParametersQuantparams)
  ON_COMMAND(ID_PARAMETERS_EXTRACTPARAMS, &CMensEtDoc::OnParametersExtractparams)
  ON_COMMAND(ID_PARAMETERS_PICKPARAMS, &CMensEtDoc::OnParametersPickparams)
  ON_COMMAND(ID_PARAMETERS_STRIPEDPARAMS, &CMensEtDoc::OnParametersStripedparams)
  ON_COMMAND(ID_DEMO_FILELOCAL, &CMensEtDoc::OnDemoFilelocal)
  ON_COMMAND(ID_PARAMETERS_SIZEPARAMS, &CMensEtDoc::OnParametersSizeparams)
  ON_COMMAND(ID_MOTION_DISTANCE, &CMensEtDoc::OnMotionDistance)
  ON_COMMAND(ID_MOTION_TRANSLATION, &CMensEtDoc::OnMotionTranslation)
  ON_COMMAND(ID_MOTION_ROTATION, &CMensEtDoc::OnMotionRotation)
  ON_COMMAND(ID_MOTION_LIFT, &CMensEtDoc::OnMotionLift)
  ON_COMMAND(ID_REFLEXES_INITPOSE, &CMensEtDoc::OnReflexesInitpose)
  ON_COMMAND(ID_DEMO_TIMING, &CMensEtDoc::OnDemoTiming)
  ON_COMMAND(ID_UTILITIES_EXTVOCAB, &CMensEtDoc::OnUtilitiesExtvocab)
  ON_COMMAND(ID_UTILITIES_TESTVOCAB, &CMensEtDoc::OnUtilitiesTestvocab)
  ON_COMMAND(ID_UTILITIES_TESTGRAPHIZER, &CMensEtDoc::OnUtilitiesTestgraphizer)
    ON_COMMAND(ID_DEMO_BASICMSGS, &CMensEtDoc::OnDemoBasicmsgs)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMensEtDoc construction/destruction

CMensEtDoc::CMensEtDoc()
{
  // JHC: move console and chat windows
  prt.SetTitle("ALIA console", 1);
  SetWindowPos(GetConsoleWindow(), HWND_TOP, 5, 5, 673, 1000, SWP_SHOWWINDOW);
  chat.Launch(1395, 505);              // was 1005 then 1250

  // direct pointers to useful parts
  fsm = (mc.rwi).fsm;
  ss = (mc.rwi).seg;  
  pp = (mc.rwi).ext;  

  // JHC: assume app called with command line argument of file to open
  cmd_line = 1;
  strcpy_s(rname, "saved.bmp");
  _getcwd(cwd, 200);

  // load configuration file(s)
  sprintf_s(cdir, "%s\\config", cwd);
  sprintf_s(ifile, "%s\\MensEt_vals.ini", cwd);
  interact_params(ifile);
  tais.Defaults(ifile);
  mc.Defaults(ifile);        // load defaults on start

  // load proper calibration then share body
  (mc.body).LoadCfg(cdir, tid);
  tais.Bind(&(mc.body));
}


CMensEtDoc::~CMensEtDoc()
{
  if (cmd_line <= 0)
  {
    ips.SaveVals(ifile);
    tais.SaveVals(ifile);
    mc.SaveVals(ifile);      // save defaults on exit
    (mc.body).SaveCfg(cdir);
  }
}


BOOL CMensEtDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

  // change this value to non-zero for externally distributed code
  // cripple = -1 for full debugging
  //         =  0 for normal full control, expiration warning
  //         =  1 for restricted operation, expiration warning
  //         =  2 for restricted operation, expiration enforced
  cripple = 0;
  ver = mc.Version(); 
  LockAfter(5, 2023, 12, 2022);

  // JHC: if this function is called, app did not start with a file open
  // JHC: initializes display object which depends on document
  cmd_line = 0;
  if (d.Valid() <= 0)   
    d.BindTo(this);

  return TRUE;
}


// JHC: possibly run start up demo if called with command line file
// RunDemo is called from main application MensEt.cpp
// will have already called OnOpenDocument to set up video

void CMensEtDoc::RunDemo()
{
  if (cmd_line <= 0)    // comment out for file arguments
    return;
  if (d.Valid() <= 0)   
    d.BindTo(this);

  OnDemoInteract();
//  OnCloseDocument();    // possibly auto-exit when done
}


// only allow demo code to run for a short while 
// make sure clock can't be reset permanently
// more of an annoyance than any real security

int CMensEtDoc::LockAfter (int mon, int yr, int smon, int syr)
{
  char *tail;

  // provide "backdoor" - override if directly in "jhc" directory
  if ((tail = strrchr(cwd, '\\')) != NULL)
    if (strcmp(tail, "\\jhc") == 0)
      if (cripple > 0)
        cripple = 0;

  // see if past expiration date (or before issue date)
  if (jms_expired(mon, yr, smon, syr))
  {
    if (cripple > 1)
      Fatal("IBM MensEt %4.2f\nExpired as of %d/%d\njconnell@us.ibm.com",
            ver, mon, yr);
    Complain("IBM MensEt %4.2f\nOut-of-date as of %d/%d\njconnell@us.ibm.com", 
             ver, mon, yr);
  }
  return 1;
}


// what to do for functions that have been disabled

int CMensEtDoc::LockedFcn ()
{
  if (cripple <= 0)
    return 0;
  Tell("Function not user-accessible in this version");
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
// CMensEtDoc serialization

void CMensEtDoc::Serialize(CArchive& ar)
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
// CMensEtDoc diagnostics

#ifdef _DEBUG
void CMensEtDoc::AssertValid() const
{
	CDocument::AssertValid();
}


void CMensEtDoc::Dump(CDumpContext& dc) const
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

BOOL CMensEtDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
  jhcString fn(lpszPathName);
  char alt[250];
  const char *start;
  char *ch = alt;

  // defeat command line argument at startup
//  if (strchr("!?", fn.ch[n - 1]) != NULL)
//    return TRUE;
  
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
    ShowFirst();
	return TRUE;
}


// JHC: use default video driver

void CMensEtDoc::OnFileCamera() 
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

void CMensEtDoc::OnFileCameraadjust() 
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


// JHC: like normal Open but filters files for video types

void CMensEtDoc::OnFileOpenvideo() 
{
  jhcString fname("images/situation.bmp");

  d.Clear(1, "Configuring video source ...");
  if (v.SelectFile(fname.ch, 500) <= 0)
  {
    d.StatusText("");
    return;
  }
  ShowFirst();
  fname.C2W();
  theApp.AddToRecentFileList(fname.Txt());
}


// JHC: let user type a file name, wildcard pattern, or vfw spec
// e.g. repeat foo.ras, show all *.bmp, choose Bt848\*.vfw
//      http://62.153.249.21/live/wildpark/live300k.asx

void CMensEtDoc::OnFileOpenexplicit() 
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

void CMensEtDoc::ShowFirst()
{
  if (!v.Valid())
    return;

  v.SizeFor(now);
  d.Clear();
  v.Rewind();
  if (v.Get(now) == 1)
  {
    d.ShowGrid(now, 0, 0, 0, v.File());
    v.Rewind();
  }
  d.StatusText("Ready");
  res.Clone(now);
  sprintf_s(rname, "frame_%d.bmp", v.Last());
}


// JHC: ask user for start, step. rate, etc. 

void CMensEtDoc::OnParametersVideocontrol() 
{
  d.StatusText("Configuring video source ...");
  if (v.AskStep() <= 0)
    d.StatusText("");
  else
    ShowFirst();
}


// JHC: ask user for sizes and whether monochrome

void CMensEtDoc::OnParametersImagesize() 
{
  d.StatusText("Configuring video source ...");
  if (v.AskSize() <= 0)
    d.StatusText("");
  else
    ShowFirst();
}


// JHC: see if video source is valid, if not try opening camera

int CMensEtDoc::ChkStream (int dw, int dh)
{
  int ans;

  // always rebuild SQ13 camera receiver
  if (!v.Valid() || v.IsClass("jhcOcv3VSrc"))
  {
    d.StatusText("Configuring camera ...");
    if ((ans = v.SetSource("http://192.168.25.1:8080/?action=stream.ocv3")) <= 0)
    {
      d.StatusText("");
      return ans;
    }
  }

  // possible change image sizes based on dialog box settings
  if ((dw > 0) || (dh > 0))
    v.SetSize(dw, dh);
  v.SizeFor(now);
  v.SizeFor(dnow, 1);
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                              Video Playback                             //
/////////////////////////////////////////////////////////////////////////////

// JHC: make up proper return image then start playback
// can use this as a template for all application functions

void CMensEtDoc::OnTestPlayvideo() 
{
  const jhcImg *src;
  int specs[3] = {0, 0, 0};

  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);

  // loop over selected set of frames  
  d.Clear(1, "Live image ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get frame, pause if right mouse click
      if ((mc.body).UpdateImg() <= 0)
        break;
      src = (mc.body).Raw();

      // clear if different sized image (jhcListVSrc)
      if (!src->SameFormat(specs))
      {
        d.Clear(0);
        src->Dims(specs);
      }

      // show frame on screen
      d.ShowGrid(src, 0, 0, 0, "%ld: %s", v.Last(), v.Name());
    }
  }
  catch (...){Tell("Unexpected exit!");}
  v.Prefetch(0);
  d.StatusText("Stopped.");  
  res.Clone(now);
  sprintf_s(rname, "frame_%d.bmp", v.Last());
}


/////////////////////////////////////////////////////////////////////////////
//                               Saving Images                             //
/////////////////////////////////////////////////////////////////////////////

// Save most recent raw input image

void CMensEtDoc::OnFileSaveinput()
{
  jhcString sel, sn;
  CFileDialog dlg(FALSE);
  jhcImgIO fio;

  // show source image first
  d.Clear();
  d.ShowGrid((mc.body).Raw(), 0, 0, 0, "Last input");

  // pop standard file choosing box starting at video directory
  sprintf_s(sn.ch, "%s\\images\\situation.bmp", cwd);
  sn.C2W();
  (dlg.m_ofn).lpstrFile = sn.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Image Files\0*.bmp;*.jpg\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() != IDOK)
    return;

  // show main image
  sel.Set((dlg.m_ofn).lpstrFile);
  fio.Save(sel.ch, *((mc.body).Raw()), 1);
  d.ShowGrid((mc.body).Raw(), 0, 0, 0, "Saved as %s", fio.Name());
}


// JHC: save last displayed image at user selected location
// should generally change this to some more meaningful image

void CMensEtDoc::OnFileSaveAs() 
{
  jhcString sel, rn;
  CFileDialog dlg(FALSE);
  jhcImgIO fio;

  // show result first
  d.Clear();
  d.ShowGrid(res, 0, 0, 0, "Last result");

  // pop standard file choosing box starting at video directory
  sprintf_s(rn.ch, "%s\\results\\%s", cwd, rname);
  rn.C2W();
  (dlg.m_ofn).lpstrFile = rn.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Image Files\0*.bmp;*.jpg\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() != IDOK)
    return;

  // show main image
  sel.Set((dlg.m_ofn).lpstrFile);
  fio.Save(sel.ch, res, 1);
  d.ShowGrid(res, 0, 0, 0, "Saved as %s", fio.Name());
}


/////////////////////////////////////////////////////////////////////////////
//                          Deployment Parameters                          //
/////////////////////////////////////////////////////////////////////////////

// Parameters for warping and camera pose

void CMensEtDoc::OnManusCameraparams()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.body).cps); 
}


// Parameters for interpreting range sensor on robot

void CMensEtDoc::OnManusRangeparams()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.body).rps); 
}


// Parameters for interpreting robot gripper width

void CMensEtDoc::OnManusWidthparams()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.body).wps); 
}


// Parameters describing translation and rotation of robot base

void CMensEtDoc::OnManusDriveparams()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.body).dps); 
}


// Parameters for robot vertical stage control

void CMensEtDoc::OnManusLiftparams()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.body).lps); 
}


// Parameters for control of robot hand motion

void CMensEtDoc::OnManusGripparams()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.body).gps); 
}


// Save current configuration data to a file

void CMensEtDoc::OnEnvironmentSavegeom()
{
  jhcString sel, cfg; 
  CFileDialog dlg(FALSE);

  sprintf_s(cfg.ch, "%s\\%s", cdir, (mc.body).CfgName());
  cfg.C2W();
  (dlg.m_ofn).lpstrFile = cfg.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Configuration Files\0*.cfg\0Text Files\0*.txt\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() == IDOK)
  {
    sel.Set((dlg.m_ofn).lpstrFile);
    (mc.body).SaveVals(sel.ch);    
  }
}


// Load configuration data from a file

void CMensEtDoc::OnEnvironmentLoadgeom()
{
  jhcString sel, cfg;
  CFileDialog dlg(TRUE);

  sprintf_s(cfg.ch, "%s\\%s", cdir, (mc.body).CfgName());
  cfg.C2W();
  (dlg.m_ofn).lpstrFile = cfg.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Configuration Files\0*.cfg\0Text Files\0*.txt\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() == IDOK)
  {
    sel.Set((dlg.m_ofn).lpstrFile);
    (mc.body).Defaults(sel.ch);  
  }
}


/////////////////////////////////////////////////////////////////////////////
//                         Application Parameters                          //
/////////////////////////////////////////////////////////////////////////////

// parameters for overall interaction

int CMensEtDoc::interact_params (const char *fname)
{
  jhcParam *ps = &ips;
  int ok;

  ps->SetTag("mens_opt", 0);
  ps->NextSpec4( &cam,        0, "Camera available");
  ps->NextSpec4( &tid,        0, "Target robot");
  ps->NextSpec4( &(mc.spin),  0, "Speech (none, local, web)");  
  ps->NextSpec4( &(mc.amode), 2, "Attn (none, any, front, only)");
  ps->NextSpec4( &(mc.tts),   0, "Vocalize output");
  ps->Skip();

  ps->NextSpec4( &(mc.vol),   1, "Load baseline volition");
  ps->NextSpec4( &(mc.acc),   0, "Skills (none, load, update)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


// Configure for voice, body, camera, etc.

void CMensEtDoc::OnDemoOptions()
{
	jhcPickVals dlg;
  int tid0 = tid;

  dlg.EditParams(ips);    
  if (tid != tid0)
    (mc.body).LoadCfg(cdir, tid);
}


// Control what basic info gets printed to console window

void CMensEtDoc::OnDemoBasicmsgs()
{
	jhcPickVals dlg;

  dlg.EditParams(mc.mps);    
}


// Adjust thought rate, attention mode, etc.

void CMensEtDoc::OnDemoTiming()
{
	jhcPickVals dlg;

  dlg.EditParams(mc.tps);    
}


// Port numbers for remote brain connection

void CMensEtDoc::OnParametersRemoteparams()
{
	jhcPickVals dlg;

  dlg.EditParams(tais.tps);    
}


// Parameters for nearby object event generation

void CMensEtDoc::OnMotionDistance()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.act).mps);    
}


// Parameters for interpreting translation commands

void CMensEtDoc::OnMotionTranslation()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.act).tps);   
}


// Parameters for interpreting rotation commands

void CMensEtDoc::OnMotionRotation()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.act).rps);   
}


// Parameters for interpreting gripper height commands

void CMensEtDoc::OnMotionLift()
{
	jhcPickVals dlg;

  dlg.EditParams((mc.act).lps);   
}


// save parameters to file

void CMensEtDoc::OnParametersSavedefaults() 
{
  jhcString sel, init(ifile);
  CFileDialog dlg(FALSE, NULL, init.Txt());

  (dlg.m_ofn).lpstrFilter = _T("Initialization Files\0*.ini\0Text Files\0*.txt\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() == IDOK)
  {
    sel.Set((dlg.m_ofn).lpstrFile);    
    ips.SaveVals(sel.ch);
    tais.SaveVals(sel.ch);
    mc.SaveVals(sel.ch);               // change to relevant object
    strcpy_s(ifile, sel.ch);
  }
}


// load parameters from file

void CMensEtDoc::OnParametersLoaddefaults() 
{
  jhcString sel, init(ifile); 
  CFileDialog dlg(TRUE, NULL, init.Txt());
  int tid0 = tid;

  (dlg.m_ofn).lpstrFilter = _T("Initialization Files\0*.ini\0Text Files\0*.txt\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() == IDOK)
  {
    sel.Set((dlg.m_ofn).lpstrFile);
    interact_params(sel.ch);
    tais.Defaults(sel.ch);
    mc.Defaults(sel.ch);               // change to relevant object
    strcpy_s(ifile, sel.ch);
    if (tid != tid0)
      (mc.body).LoadCfg(cdir, tid);
  }
}


/////////////////////////////////////////////////////////////////////////////
//                               Full Demos                                //
/////////////////////////////////////////////////////////////////////////////

// Use remote brain to interpret commands and advice

void CMensEtDoc::OnDemoRemote()
{
Tell("Temporarily disabled");
return;
/*
  HWND me = GetForegroundWindow();
  char in[200];

  // possibly check for video
  if ((cam > 0) && (ChkStream() > 0))
    (mc.body).BindVideo(&v);
  else
    (mc.body).BindVideo(NULL);

  // reset all required components
  system("cls");
  jprintf_open();
  if (tais.Init(tid, 1) <= 0)
    return;
  if (mic > 0)
    if (mc.VoiceInit() <= 0)                     // just speech        
      return;
  chat.Reset(mic);

  // announce start and input mode
  if (mic > 0)
  {
    d.Clear(1, "Voice input to REMOTE system ...");
    jprintf("\nReady for speech (hit ESC to exit) ...\n\n");
    SetForegroundWindow(GetConsoleWindow());
  }
  else
  {
    d.Clear(1, "Text input to REMOTE system ...");
    jprintf("\nReady for typing (hit ESC to exit) ...\n\n");
  }

  // keep taking sentences until ESC
  try
  {
    while (chat.Interact() >= 0)
    {
      // get inputs and compute response
      if (mic > 0)
      {
        mc.Listen();                                 // just speech
        tais.Accept(mc.NewInput(), chat.Done());
      }
      else 
        tais.Accept(chat.Get(in), chat.Done());
      if (tais.Respond(mc.io) <= 0)
        break;

      // show interaction
      if ((mc.body).NewFrame())
        d.ShowGrid((mc.body).View(), 0, 0, 0, "Robot view");
      chat.Post(tais.NewInput(), 1);
      chat.Post(tais.NewOutput());
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  tais.Shutdown();
  mc.Done();
  jprintf("\n:::::::::::::::::::::::::::::::::::::\n");
  jprintf("Done.\n\n");
  jprintf_close();

  // window configuration
  d.StatusText("Stopped."); 
  chat.Mute();
  SetForegroundWindow(me);
*/
}


// Read successive inputs from a text file

void CMensEtDoc::OnDemoFilelocal()
{
  jhcString sel, test;
  CFileDialog dlg(TRUE);
  HWND me = GetForegroundWindow();
  FILE *f;
  char in[200] = "";

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
    (mc.body).BindVideo(&v);
  else
    (mc.body).BindVideo(NULL);

  // reset all required components
  system("cls");
  jprintf_open();
  if (mc.Reset(tid) <= 0)
    return;
  chat.Reset(0, "log");
  if (next_line(in, 200, f))
    chat.Inject(in);

  // announce start and input mode
  d.Clear(1, "File input (ESC to quit) ...");
  d.ResetGrid(0, 640, 360);
  d.StringGrid(0, 0, ">>> NO IMAGES <<<");

  // keep taking sentences until ESC
  try
  {
    while (chat.Interact() >= 0)
    {
      // check for keystroke input 
      if (mc.Accept(chat.Get(in), chat.Done()))
        if (next_line(in, 200, f))
          chat.Inject(in);

      // compute response
      if (mc.Respond() <= 0)
        break;

      // show interaction
      if ((mc.body).NewFrame())
        d.ShowGrid((mc.body).View(), 0, 0, 0, "Robot view");
      (mc.stat).Memory(&d, 0, 1);
      chat.Post(mc.NewInput(), 1);
      chat.Post(mc.NewOutput());
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  mc.Done();
  jprintf_close();
  fclose(f);

  // window configuration
  d.StatusText("Stopped."); 
  chat.Mute();
  SetForegroundWindow(me);
}


//= Gets cleaned up next line from file (removes whitespace and comment from end).
// returns true if txt string filled with something new

bool CMensEtDoc::next_line (char *txt, int ssz, FILE *f) const
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


// Send commands and provide advice using local processing
// *** STANDARD DEMO ***

void CMensEtDoc::OnDemoInteract()
{
  HWND me = GetForegroundWindow();
  char in[200];

  // possibly check for video 
  if ((cam > 0) && (ChkStream() > 0))
    (mc.body).BindVideo(&v);
  else
    (mc.body).BindVideo(NULL);

  // reset all required components
  system("cls");
  jprintf_open();
  if (mc.Reset(tid) <= 0)
    return;
  chat.Reset(0, "log");

  // announce start and input mode
  if ((mc.spin) > 0)
    d.Clear(1, "Voice input (ESC to quit) ...");
  else
    d.Clear(1, "Text input (ESC to quit) ...");
  d.ResetGrid(0, 640, 360);
  d.StringGrid(0, 0, ">>> NO IMAGES <<<");
  SetForegroundWindow(chat);

  // keep taking sentences until ESC
#ifndef _DEBUG
  try
#endif
  {
    while (chat.Interact() >= 0)
    {
      // get inputs and compute response
      mc.Accept(chat.Get(in), chat.Done());
      if (mc.Respond() <= 0)
        break;

      // show interaction
      if ((mc.body).NewFrame())
        d.ShowGrid((mc.body).View(), 0, 0, 0, "Robot view");
      (mc.stat).Memory(&d, 0, 1);
      chat.Post(mc.NewInput(), 1);
      chat.Post(mc.NewOutput());
    }
  }
#ifndef _DEBUG
  catch (...){Tell("Unexpected exit!");}
#endif

  // cleanup
  mc.Done();
  jprintf_close();

  // window configuration
  d.StatusText("Stopped."); 
  chat.Mute();
  SetForegroundWindow(me);
}


/////////////////////////////////////////////////////////////////////////////
//                             Vision Parameters                           //
/////////////////////////////////////////////////////////////////////////////

// Color interpretation for floor and object finding

void CMensEtDoc::OnParametersFloorparams()
{
	jhcPickVals dlg;

  dlg.EditParams(ss->cps);    
}


// Sampling areas for floor color

void CMensEtDoc::OnParametersPatcharea()
{
	jhcPickVals dlg;

  dlg.EditParams(ss->fps);    
}


// Parameters constraining valid object detections

void CMensEtDoc::OnParametersShapeparams()
{
	jhcPickVals dlg;

  dlg.EditParams(ss->sps);    
}


// Parameters for cleaning up object masks

void CMensEtDoc::OnParametersCleanparams()
{
	jhcPickVals dlg;

  dlg.EditParams(ss->mps);    
}


// Parameters controlling extraction of color properties

void CMensEtDoc::OnParametersExtractparams()
{
	jhcPickVals dlg;

  dlg.EditParams(pp->cps);    
}


// Hue thresholds for canonical colors

void CMensEtDoc::OnParametersQuantparams()
{
	jhcPickVals dlg;

  dlg.EditParams(pp->hps);    
}


// Parameters governing which colors to report

void CMensEtDoc::OnParametersPickparams()
{
	jhcPickVals dlg;

  dlg.EditParams(pp->nps);    
}


// Parameters for analyzing stripedness of object

void CMensEtDoc::OnParametersStripedparams()
{
	jhcPickVals dlg;

  dlg.EditParams(pp->sps);    
}


// Parameters for categorizing size and width

void CMensEtDoc::OnParametersSizeparams()
{
	jhcPickVals dlg;

  dlg.EditParams(pp->zps);    
}


/////////////////////////////////////////////////////////////////////////////
//                            Image Preprocessing                          //
/////////////////////////////////////////////////////////////////////////////

// Show geometrically rectified video

void CMensEtDoc::OnCameraparamsDewarp()
{
  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // process video frames
  d.Clear(1, "Dewarping ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      if ((mc.body).UpdateImg() <= 0)
        break;

      d.ShowGrid((mc.body).View(), 0, 0, 0, "Dewarped");
      d.ShowGrid((mc.body).Raw(),  0, 1, 0, "Raw camera"); 
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  res.Clone((mc.body).View());
  sprintf_s(rname, "%s_dewarp.bmp", v.FrameName());
}


// Show pre-processing enhancements

void CMensEtDoc::OnProcessingCleanup()
{
  jhcImg boost0;
  const jhcImg *src;

  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // initialize processing and images
  ss->SetSize(v.XDim(), v.YDim());
  ss->Reset();
  boost0.SetSize(ss->XDim(), ss->YDim(), 3);

  // process video frames
  d.Clear(1, "Preprocessing ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get image 
      if ((mc.body).UpdateImg() <= 0)
        break;
      src = (mc.body).View();

      // do basic processing
      ss->Analyze(*src);
      MaxColor(boost0, *src, 5.0);

      // show results
      d.ShowGrid(ss->est,   0, 0, 0, "Enhanced and smoothed"); 
      d.ShowGrid(ss->boost, 1, 0, 0, "Stable color"); 

      d.ShowGrid(src,       0, 1, 0, "Dewarped");
      d.ShowGrid(boost0,    1, 1, 0, "Original color"); 
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  res.Clone(ss->est);
  sprintf_s(rname, "%s_clean.bmp", v.FrameName());
}


/////////////////////////////////////////////////////////////////////////////
//                               Floor Finding                             //
/////////////////////////////////////////////////////////////////////////////

// Show color difference channels

void CMensEtDoc::OnVisionColordiffs()
{
  const jhcImg *src;

  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // set up for analysis
  ss->SetSize(v.XDim(), v.YDim());
  ss->Reset();

  // process video frames
  d.Clear(1, "Color channels ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get image 
      if ((mc.body).UpdateImg() <= 0)
        break;
      src = (mc.body).View();

      // do basic processing
      ss->Analyze(*src);

      // show results
      d.ShowGrid(ss->wk,  0, 0, 2, "White-black"); 
      d.ShowGrid(ss->est, 1, 0, 0, "Enhanced and smoothed"); 

      d.ShowGrid(ss->rg,  0, 1, 0, "Red-green");
      d.ShowGrid(ss->yb,  1, 1, 0, "Yellow-blue"); 
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  FalseClone(res, ss->wk);
  sprintf_s(rname, "%s_intensity.bmp", v.FrameName());
}


// Show regions not matching surface in color channels

void CMensEtDoc::OnVisionSimilarregions()
{
  jhcImg src2, mono3, wkd, rgd, ybd;
  const jhcImg *src;

  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // set up for analysis
  ss->SetSize(v.XDim(), v.YDim());
  ss->Reset();
  mono3.SetSize(ss->wk, 3);
  wkd.SetSize(ss->wk);
  rgd.SetSize(wkd);
  ybd.SetSize(wkd);

  // process video frames
  d.Clear(1, "Color match ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get image 
      if ((mc.body).UpdateImg() <= 0)
        break;
      src = (mc.body).View();

      // do basic processing
      ss->Analyze(*src);

      // make some pretty pictures
      src2.Clone(ss->est);
      RectEmpty(src2, ss->p1, 3, 255, 0, 255);
      RectEmpty(src2, ss->p2, 3, 255, 0, 255);
      Emphasize(mono3, ss->wk, ss->floor, 128, 0, 80, 0);
      Complement(wkd, ss->wk3);
      Complement(rgd, ss->rg3);
      Complement(ybd, ss->yb3);

      // show results
      d.ShowGrid(wkd,  0, 0, 2, "WK differences");
      d.ShowGrid(src2, 1, 0, 0, "Clean with patches");

      d.GraphGrid(ss->fhist[0], 2, 0, 0, 0, "RG values");  
      d.GraphMark(ss->flims[0], 2);
      d.GraphMark(ss->flims[1], 1);
      d.GraphBelow(ss->fhist[1], 0, 0, "YB values");
      d.GraphMark(ss->flims[2], 4);
      d.GraphMark(ss->flims[3], 3);
      d.GraphBelow(ss->fhist[2], 0, 0, "WK values");
      d.GraphMark(ss->flims[4], 0);
      d.GraphMark(ss->flims[5], 6);

      d.ShowGrid(rgd, 0, 1, 2, "RG differences");
      d.ShowGrid(ybd, 1, 1, 2, "YB differences");
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  FalseClone(res, ss->vsm);
  sprintf_s(rname, "%s_similar.bmp", v.FrameName());
}


// Find likely floor region

void CMensEtDoc::OnProcessingGroundplane()
{
  jhcImg src2, mono3;
  const jhcImg *src;

  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // set up for analysis
  ss->SetSize(v.XDim(), v.YDim());
  ss->Reset();
  mono3.SetSize(ss->wk, 3);

  // process video frames
  d.Clear(1, "Floor region ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get image 
      if ((mc.body).UpdateImg() <= 0)
        break;
      src = (mc.body).View();

      // do basic processing
      ss->Analyze(*src);

      // make some pretty pictures
      src2.Clone(ss->est);
      RectEmpty(src2, ss->p1, 3, 255, 0, 255);
      RectEmpty(src2, ss->p2, 3, 255, 0, 255);
      Emphasize(mono3, ss->wk, ss->floor, 128, 0, 80, 0);

      // show results
      d.ShowGrid(mono3, 0, 0, 0, "Likely floor");
      d.ShowGrid(src2,  1, 0, 0, "Clean with patches");

      d.GraphGrid(ss->fhist[0], 2, 0, 0, 0, "RG values");  
      d.GraphMark(ss->flims[0], 2);
      d.GraphMark(ss->flims[1], 1);
      d.GraphBelow(ss->fhist[1], 0, 0, "YB values");
      d.GraphMark(ss->flims[2], 4);
      d.GraphMark(ss->flims[3], 3);
      d.GraphBelow(ss->fhist[2], 0, 0, "WK values");
      d.GraphMark(ss->flims[4], 0);
      d.GraphMark(ss->flims[5], 6);

      d.ShowGrid(ss->floor, 0, 1, 2, "Floor region");
      d.ShowGrid(ss->vsm,   1, 1, 2, "Similar to patches");
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  res.Clone(mono3);
  sprintf_s(rname, "%s_floor.bmp", v.FrameName());
}


/////////////////////////////////////////////////////////////////////////////
//                              Object Finding                             //
/////////////////////////////////////////////////////////////////////////////

// Find convex exceptions to ground plane

void CMensEtDoc::OnVisionObjects()
{
  jhcImg hint, line, matte, trace;
  const jhcImg *src;

  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // set up for analysis
  ss->SetSize(v.XDim(), v.YDim());
  ss->Reset();
  hint.SetSize(ss->wk);
  line.SetSize(hint);
  matte.SetSize(ss->wk, 3);
  trace.SetSize(matte);

  // process video frames
  d.Clear(1, "Objects ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get image 
      if ((mc.body).UpdateImg() <= 0)
        break;
      src = (mc.body).View();

      // do basic processing
      ss->Analyze(*src);

      // picture with holes red and bays green
      hint.CopyArr(ss->floor);
      UnderGate(hint, hint, ss->holes, 128, 200);       
      UnderGate(hint, hint, ss->bays,  128, 128);       

      // fat outlines around object in input image
      OverGateRGB(matte, ss->est, ss->tmp, 128, 0, 0, 255);
      line.FillArr(0);
      Outline(line, ss->tmp, 128, 255);
      BoxThresh(line, line, 5, 20);
      RectEmpty(ss->est, ss->p1, 3, 255, 0, 255);
      UnderGateRGB(trace, ss->est, line, 128, 0, 255, 0);

      // component image
      Scramble(line, ss->occ);

      // show results
      d.ShowGrid(hint,  0, 0, 2, "%d holes + %d bays", (ss->hblob).CountOver(), (ss->bblob).CountOver());
      d.ShowGrid(trace, 1, 0, 0, "Hole outlines");

      d.ShowGrid(line,  0, 1, 2, "Hole components");
      d.ShowGrid(matte, 1, 1, 0, "Hole objects");
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  res.Clone(matte);
  sprintf_s(rname, "%s_matte.bmp", v.FrameName());
//  FalseClone(res, hint);
//  sprintf_s(rname, "%s_seeds.bmp", v.FrameName());
}


// Grow object seeds into stacks of components

void CMensEtDoc::OnVisionStackgrow()
{
  jhcBlob blob(100);
  jhcImg proto, mask, cc, p2, s2, gate, pass, both, g2;
  const jhcImg *src = NULL;
  int i, n, a, run = 1;

  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // set up for analysis
  ss->SetSize(v.XDim(), v.YDim());
  ss->Reset();
  mask.SetSize(ss->wk);
  proto.SetSize(mask);
  p2.SetSize(mask);
  s2.SetSize(mask, 3);
  cc.SetSize(mask, 2);
  gate.SetSize(mask);
  both.SetSize(mask);
  pass.SetSize(mask);
  g2.SetSize(mask);

  // process video frames
  d.Clear(1, "Growing ...");
  v.Rewind(FASTVID);
  try
  {
    while ((run > 0) && !d.LoopHit(v.StepTime()))
    {
      // get image 
      if ((mc.body).UpdateImg() <= 0)
        break;
      src = (mc.body).View();

      // do basic processing
      ss->Analyze(*src);

      // possibly nothing to show
      if ((ss->bblob).CountOver() <= 0)
      {
        both.FillArr(0);
        d.ShowGrid(both,    0, 0, 2, "No bays");
        d.ShowGrid(ss->est, 1, 0, 0, "Clean");
        continue;
      }

      // mark top section of each blob and try to extend
       n = (ss->bblob).Active();
      for (i = 1; i < n; i++)
        if ((ss->bblob).GetStatus(i) > 0)
        {
          // get top part for color matching
          a = (ss->bblob).BlobArea(i);
          (ss->bblob).HighestPels(mask, ss->bcc, i, ROUND(0.3 * a));
          HistOver8(ss->ohist[0], ss->rg, mask, 127, 100); 
          HistOver8(ss->ohist[1], ss->yb, mask, 127, 100); 
          HistOver8(ss->ohist[2], ss->wk, mask, 127, 100); 

          // find similar color
          ss->color_desc(ss->olims, ss->ohist);
          ss->same_color(pass, ss->olims, NULL);
          Threshold(gate, pass, ss->pick);

          // find parts that are not known to be floor
          UnderGate(gate, gate, ss->floor2, 128);
          BoxThresh(gate, gate, 9, 128);
          CComps4(cc, gate, ss->omin, 128);
          ss->ok_regions(g2, blob, cc);

          // should keep (red) expansion blob that touches seed 

          // picture of seed over expansion
          (ss->bblob).MarkBlob(both, ss->bcc, i, 128);  // seed blob
          UnderGate(both, both, gate, 128, 50);         // bad blobs
          UnderGate(both, both, g2, 128, 255);          // good blobs

          // picture of color sampling
          Threshold(p2, ss->bays, 128, 128);
          RectEmpty(p2, mask, 3, 215);
          s2.CopyArr(ss->est);
          RectEmpty(s2, mask, 3, 255, 0, 255);
          mask.MaxRoi();

          // show results
          d.ShowGrid(both, 0, 0, 2, "Reasonable extensions");
          d.ShowGrid(s2,   1, 0, 0, "Clean");

          d.ShowGrid(pass, 0, 1, 2, "Similar color");
          d.ShowGrid(p2,   1, 1, 2, "Component %d: area = %d", i, a);
          
          d.GraphGrid(ss->ohist[0], 2, 0, 0, 0, "RG values");  
          d.GraphMark(ss->olims[0], 2);
          d.GraphMark(ss->olims[1], 1);
          d.GraphBelow(ss->ohist[1], 0, 0, "YB values");
          d.GraphMark(ss->olims[2], 4);
          d.GraphMark(ss->olims[3], 3);
          d.GraphBelow(ss->ohist[2], 0, 0, "WK values");
          d.GraphMark(ss->olims[4], 0);
          d.GraphMark(ss->olims[5], 6);

          // wait for user
          if (Ask("Continue?") <= 0)
          {
            run = 0;
            break;
          }
        }
    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  res.Clone(*src);
  sprintf_s(rname, "%s_grown.bmp", v.FrameName());
}


// Show analysis of color and stripedness

void CMensEtDoc::OnVisionFeatures()
{
  jhcImg bin, hcomp, vcomp, trace, gate;
  jhcArr chist(90);
  char sterm[3][40] = {"-> small", "", "-> BIG"}, wterm[3][40] = {"-> narrow", "", "-> WIDE"};
  char cname[80], cname2[80], tex[80];
  const jhcImg *src;
  int focus, st, sz, wc, mid = 320, h0 = 150;

  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // set up for analysis
  ss->SetSize(v.XDim(), v.YDim());
  ss->Reset();
  bin.SetSize(ss->wk);
  hcomp.SetSize(bin);
  vcomp.SetSize(bin);
  trace.SetSize(bin);
  gate.SetSize(ss->wk, 3);
  pp->SetSize(ss->wk);

  // process video frames
  d.Clear(1, "Features ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get image 
      if ((mc.body).UpdateImg() <= 0)
        break;
      src = (mc.body).View();

      // do basic processing
      ss->Analyze(*src);

      // pick one hole blob as foregound object
      if ((focus = ss->CloseAbove(mid, h0)) <= 0)
      {
        gate.FillRGB(0, 0, 255);
        trace.FillArr(0);
        *cname = '\0';
        *cname2 = '\0';
        *tex = '\0';
      }
      else
      {
        // get selected patch mask
        ss->PadMask(bin, focus, 1);    

        // analyze color
        pp->FindColors(bin, ss->est);
        pp->QuantColor(chist);
        pp->MainColors(cname);
        pp->AltColors(cname2);

        // analyze texture
        st = pp->Striped(bin, ss->wk);
        strcpy_s(tex, ((st > 0) ? "STRIPED" : "bland"));

        // analyze size and width
        sz = pp->SizeClass(ss->AreaPels(focus), ss->BotScale(focus));
        wc = pp->WidthClass(ss->WidthX(focus), ss->HeightY(focus));

        // show color region
        OverGateRGB(gate, ss->est, bin, 128, 0, 0, 255); 
        Matte(gate, gate, 0, 0, 255);

        // show long edges
        Scramble(hcomp, pp->hcc);
        Scramble(vcomp, pp->vcc);
        MaxFcn(trace, hcomp, vcomp);
        Matte(trace, trace);
        ss->Contour(trace, trace, focus, 1);
      }

      // show results
      d.ShowGrid(gate,  0, 0, 0, "Pixel colors  ->  %s", cname);
      d.ShowGrid(trace, 0, 1, 2, "Long edges  ->  %s", tex);

      d.ClearRange(1, 0, 1, 1);
      if (focus > 0)
      {
        // color bins
        d.GraphGrid(pp->hhist, 1, 0, 0, 0, "Hue histogram");
        d.GraphMark(pp->clim[0], 1, 0.2);
        d.GraphMark(pp->clim[1], 6, 0.2);
        d.GraphMark(pp->clim[2], 3, 0.2);
        d.GraphMark(pp->clim[3], 2, 0.2);
        d.GraphMark(pp->clim[4], 4, 0.2);
        d.GraphMark(pp->clim[5], 5, 0.2);

        // quantized bins and resulting names
        d.GraphBelow(chist, 0, 0, "ROYGBV-KXW");
        d.StringBelow("main: %s", cname);
        d.StringBelow("other: %s", cname2);

        // texture stats and decision
        d.StringGrid(1, 1, "");
        d.StringBelow("Horizontal = %d", pp->nh);
        d.StringBelow("Vertical = %d", pp->nv);
        d.StringBelow("Fraction = %4.2f", pp->ftex);
        d.StringBelow("");
        d.StringBelow(tex);
        d.StringBelow("");
        d.StringBelow("Size: %4.2f\"   %s", pp->dim, sterm[sz]);
        d.StringBelow("Width: %4.2fx   %s", pp->wrel, wterm[wc]);        
      }

    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  res.Clone(trace);
  sprintf_s(rname, "%s_tex.bmp", v.FrameName());
}


// Show region edges for objects

void CMensEtDoc::OnVisionBoundary()
{
  jhcImg ej, ej_wk, ej_rg, ej_yb, ej2, ej3, ej4;
  const jhcImg *src = NULL;

  // make sure video exists
  if (ChkStream() <= 0)
    return;
  (mc.body).BindVideo(&v);
  (mc.body).Reset(0, NULL, 0);
  (mc.body).SetSize(v.XDim(), v.YDim());

  // set up for analysis
  ss->SetSize(v.XDim(), v.YDim());
  ss->Reset();
  ej.SetSize(ss->wk);
  ej_wk.SetSize(ej);
  ej_rg.SetSize(ej);
  ej_yb.SetSize(ej);
  ej2.SetSize(ej);
  ej3.SetSize(ej);
  ej4.SetSize(ej);

  // process video frames
  d.Clear(1, "Boundaries ...");
  v.Rewind(FASTVID);
  try
  {
    while (!d.LoopHit(v.StepTime()))
    {
      // get image 
      if ((mc.body).UpdateImg() <= 0)
        break; 
      src = (mc.body).View();

      // do basic processing
      ss->Analyze(*src);

      // generate pretty pictures
/*
BoxAvg(ej, ss->wk, 9);
SobelEdge(ej_wk, ej, 4.0);
BoxAvg(ej, ss->rg, 9);
SobelEdge(ej_rg, ej, 8.0);
BoxAvg(ej, ss->yb, 9);
SobelEdge(ej_yb, ej, 8.0);
*/
      SobelEdge(ej_wk, ss->wk, 4.0);
SobelEdge(ej_rg, ss->rg, 8.0);
SobelEdge(ej_yb, ss->yb, 8.0);

//ClipSum(ej2, ej_rg, ej_yb);
//ClipSum(ej2, ej2, ej);

//ClipSum(ej2, ej_rg, ej_yb, 0.5);
//Blend(ej2, ej2, ej, 0.66666);

//Threshold(ej, ej, 50);
//Threshold(ej_rg, ej_rg, 50);
//Threshold(ej_yb, ej_yb, 50);

MaxFcn(ej3, ej_rg, ej_yb);
MaxFcn(ej3, ej3, ej_wk);

BoxAvg(ej, ej3, 3);

Threshold(ej2, ej, 50);

BoxAvg(ej4, ej2, 9);
Threshold(ej4, ej4, 80);

      // show results
      d.ShowGrid(ss->est, 0, 0, 0, "Clean input");
//      d.ShowGrid(ej,      1, 0, 2, "Smoothed");
      d.ShowGrid(ej_wk,   1, 0, 2, "WK edges");

//      d.ShowGrid(ej2,     0, 1, 2, "Thresholded");
//      d.ShowGrid(ej4,     1, 1, 2, "Thinned");


      d.ShowGrid(ej_rg,  0, 1, 2, "RG edges");
//      d.ShowGrid(ej2,    1, 1, 2, "Sum edges");
      d.ShowGrid(ej_yb,  1, 1, 2, "YB edges");
//      d.ShowGrid(ej3,    1, 1, 2, "Max edges");

//      d.ShowGrid(ss->wk, 1, 1, 0, "WK");
//      d.ShowGrid(ej2,  0, 1, 2, "Color diff edges");
//      d.ShowGrid(ej3,  1, 1, 2, "RGB edges");

    }
  }
  catch (...){Tell("Unexpected exit!");}

  // cleanup
  v.Prefetch(0);
  d.StatusText("Stopped."); 
  res.Clone(*src);
  sprintf_s(rname, "%s_bounds.bmp", v.FrameName());
}


/////////////////////////////////////////////////////////////////////////////
//                                Reflexes                                 //
/////////////////////////////////////////////////////////////////////////////

// Connect to robot and establish default configuration

void CMensEtDoc::OnReflexesInitpose()
{
  int rc;

  (mc.body).BindVideo(NULL);
  rc = (mc.body).Reset(1, "config", tid);
  Tell((rc > 0) ? "Done" : "FAILED");
}


// Open the gripper wide

void CMensEtDoc::OnReflexesOpen()
{
  UL32 t = 0;
  int n, rc;

  // run motion sequence
  fsm->Reset();
  try
  {
    while (!_kbhit())
    {
      (mc.body).Update();
      n = fsm->FullOpen();
      if ((rc = fsm->Status(n)) != 1)
        break;
      (mc.body).Issue();
      t = jms_wait(t, 50);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  Tell((rc > 0) ? "Done" : "FAILED");
}


// Close the gripper to hold an object

void CMensEtDoc::OnReflexesClose()
{
  UL32 t = 0;
  int n, rc;

  // run motion sequence
  fsm->Reset();
  try
  {
    while (!_kbhit())
    {
      (mc.body).Update();
      n = fsm->GoodGrip();
      if ((rc = fsm->Status(n)) != 1)
        break;
      (mc.body).Issue();
      t = jms_wait(t, 50);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  Tell((rc > 0) ? "Done" : "FAILED");
}


// Move close to object based on distance sensor

void CMensEtDoc::OnReflexesCozyup()
{
  UL32 t = 0;
  int n, rc;

  // run motion sequence
  fsm->Reset();
  try
  {
    while (!_kbhit())
    {
      (mc.body).Update();
      n = fsm->Standoff(2.5);
      if ((rc = fsm->Status(n)) != 1)
        break;
      (mc.body).Issue();
      t = jms_wait(t, 50);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  Tell((rc > 0) ? "Done" : "FAILED");
}


// Take object into jaws of gripper using distance sensor

void CMensEtDoc::OnReflexesEngulf()
{
  UL32 t = 0;
  int n, rc;

  // run motion sequence
  fsm->Reset();
  try
  {
    while (!_kbhit())
    {
      (mc.body).Update();
      n = fsm->Standoff(0.0);
      if ((rc = fsm->Status(n)) != 1)
        break;
      (mc.body).Issue();
      t = jms_wait(t, 50);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  Tell((rc > 0) ? "Done" : "FAILED");
}


// Grab object in front and prepare to move

void CMensEtDoc::OnReflexesAcquire()
{
  UL32 t = 0;
  int n, rc;

  // run motion sequence
  fsm->Reset();
  try
  {
    while (!_kbhit())
    {
      (mc.body).Update();
      n = fsm->Acquire(0);
      if ((rc = fsm->Status(n)) != 1)
        break;
      (mc.body).Issue();
      t = jms_wait(t, 50);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  Tell((rc > 0) ? "Done" : "FAILED");
}


// Gently place held object onto surface

void CMensEtDoc::OnReflexesDeposit()
{
  UL32 t = 0;
  int n, rc;

  // run motion sequence
  fsm->Reset();
  try
  {
    while (!_kbhit())
    {
      (mc.body).Update();
      n = fsm->Deposit();
      if ((rc = fsm->Status(n)) != 1)
        break;
      (mc.body).Issue();
      t = jms_wait(t, 50);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  Tell((rc > 0) ? "Done" : "FAILED");
}


// Place held object on top of obstacle in front

void CMensEtDoc::OnReflexesStack()
{
  UL32 t = 0;
  int n, rc;

  // run motion sequence
  fsm->Reset();
  try
  {
    while (!_kbhit())
    {
      (mc.body).Update();
      n = fsm->AddTop();
      if ((rc = fsm->Status(n)) != 1)
        break;
      (mc.body).Issue();
      t = jms_wait(t, 50);
    }
  }
  catch (...){Tell("Unexpected exit!");}
  Tell((rc > 0) ? "Done" : "FAILED");
}


/////////////////////////////////////////////////////////////////////////////
//                           Grammar Construction                          //
/////////////////////////////////////////////////////////////////////////////

// Get preliminary grammar terms from operators and rules

void CMensEtDoc::OnUtilitiesExtvocab()
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
  if ((n = (mc.net).HarvestLex(sel.ch)) > 0)
    Tell("Extracted %d terms to: %s0.sgm", n, sel.ch + skip);
}


// Refine grammar terms for consistent morphology

void CMensEtDoc::OnUtilitiesTestvocab()
{
  jhcString sel, test;
  CFileDialog dlg(TRUE);
  int err;

  // select file to read 
  sprintf_s(test.ch, "%s\\language\\lex_open.sgm", cwd);
  test.C2W();
  (dlg.m_ofn).lpstrFile = test.Txt();
  (dlg.m_ofn).lpstrFilter = _T("Grammar Files\0*.sgm\0All Files (*.*)\0*.*\0");
  if (dlg.DoModal() != IDOK)
    return;

  // try generating derived terms and do basic inversion testing
  sel.Set((dlg.m_ofn).lpstrFile);    
  if ((err = ((mc.net).mf).LexDeriv(sel.ch)) < 0)
    return;
  if (err > 0)
  {
    Tell("Adjust original =[XXX-morph] section to fix %d problems", err);
    return;
  }

  // try regenerating base words from derived terms
  if ((err = ((mc.net).mf).LexBase("derived.sgm", 1, sel.ch)) < 0)
    return;
  if (err > 0)
    Tell("Adjust original =[XXX-morph] section to fix %d problems", err);
  else
    Tell("Looks good but examine \"derived.sgm\" then \"base_words.txt\"\n\nAdjust original =[XXX-morph] section to fix any problems");
}



/////////////////////////////////////////////////////////////////////////////
//                           Input Conversion                              //
/////////////////////////////////////////////////////////////////////////////

// Check for correct semantic networks from standard input forms

void CMensEtDoc::OnUtilitiesTestgraphizer()
{
  char test[5][80] = {"RULE", "NOTE", "DO", "ANTE", "PUNT"};
  char fname[200], in[200] = "";
  char line[200], line2[200], result[80] = "";
  FILE *f, *f2;
  int i, ok;

  // go through a series of test inputs
  system("cls");
  for (i = 0; i < 5; i++)
  {
    // get input and output file names
    sprintf_s(fname, "test/%s_forms.tst", test[i]);
    sprintf_s(mc.cfile, "%s.cvt", test[i]);
    if (fopen_s(&f, fname, "r") != 0)
    {
      Complain("Could not open file: %s", fname);
      break;
    } 

    // reset all required components
    (mc.body).BindVideo(NULL);
    mc.Reset(0);

    // go through all file inputs and save conversions to cfile
    while (chat.Interact() >= 0)
    {
      // process next line (if any)
      if (!next_line(in, 200, f))
        break;
      mc.Accept(in);

      // compute response
      if (mc.Respond() <= 0)
        break;

      // show interaction
      chat.Post(mc.NewInput(), 1);
      chat.Post(mc.NewOutput());
    }

    // open golden file and current conversion result
    mc.Done();
    fclose(f);
    sprintf_s(fname, "test/%s_forms.cvt", test[i]);
    if (fopen_s(&f, fname, "r") != 0)
      break;
    if (fopen_s(&f2, mc.cfile, "r") != 0)
      break;

    // compare two files line by line
    ok = 0;
    while (1)
    {
      // get next lines (else see if simultaneous end)
      if (fgets(line, 200, f) == NULL)
      {
        if (fgets(line2, 200, f2) == NULL)
          ok = 1;
        break;
      }

      // make sure lines are identical
      if (fgets(line2, 200, f2) == NULL)
        break;
      if (strcmp(line, line2) != 0)
        break;
    }

    // cleanup and mark if any problems
    fclose(f);
    fclose(f2);
    if (ok <= 0)
    {
      strcat_s(result, " ");
      strcat_s(result, test[i]);
    }
  }

  // report summary
  *(mc.cfile) = '\0';
  if (*result != '\0')
    Tell("Anomalies with: %s", result);
  else
    Tell("All forms correct");
}


/////////////////////////////////////////////////////////////////////////////
//                                Testing                                  //
/////////////////////////////////////////////////////////////////////////////

// Test some sub-functionality

void CMensEtDoc::OnUtilitiesTest()
{ 

}

