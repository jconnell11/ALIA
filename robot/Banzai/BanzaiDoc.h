// BanzaiDoc.h : top level GUI framework does something
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#if !defined(AFX_BanzaiDOC_H__88A3963D_AA79_44B8_ADD0_FFB56CC99566__INCLUDED_)
#define AFX_BanzaiDOC_H__88A3963D_AA79_44B8_ADD0_FFB56CC99566__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// JHC: definitions of added member variables
#include "Data/jhcImg.h"               // common video
#include "Interface/jhcConsole.h"
#include "Interface/jhcDisplay.h"
#include "Processing/jhcTools.h"
#include "Video/jhcExpVSrc.h"

#include "Acoustic/jhcChatBox.h"       // common audio

#include "RWI/jhcEliCoord.h"           // class encapsulating base functionality


class CBanzaiDoc : public CDocument, public jhcTools
{
protected: // create from serialization only
	CBanzaiDoc();
	DECLARE_DYNCREATE(CBanzaiDoc)

// JHC: useful to add these directly to window
private:
  int cripple;
  double ver;
  jhcDisplay d;
  jhcExpVSrc v;
  jhcImg res;
  char rname[200], cwd[200], ifile[200];


public:
  int cmd_line;


// Attributes
public:
  jhcEliCoord ec;      // class encapsulating base functionality
  jhcConsole prt;      // place for jprintf output
  jhcChatBox chat;     // place for text interaction

  jhcEliBody *eb;

  // joint swing testing
  jhcParam jps;
  double acc, slope, start, chg, rate, fchk, gap;
  int jnum;

  // overall configuration choices
  jhcParam ips;
  int rob, cam, fsave;


// JHC: helper functions
public:
  void RunDemo ();
  int LockAfter (int mon, int yr, int smon =0, int syr =0);
  int LockedFcn ();
  void ShowFirst ();   
  int ChkStream (int dual =1);    

  void set_tilt (const char *fname);
  int swing_params (const char *fname =NULL);
  int interact_params (const char *fname =NULL);
  bool next_line (char *txt, int ssz, FILE *f) const;


// Operations
public:


// JHC: manually added OnOpenDocument
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBanzaiDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBanzaiDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:


// Generated message map functions
protected:
	//{{AFX_MSG(CBanzaiDoc)
	afx_msg void OnFileCamera();
	afx_msg void OnFileCameraadjust();
	afx_msg void OnFileOpenexplicit();
	afx_msg void OnParametersVideocontrol();
	afx_msg void OnParametersImagesize();
	afx_msg void OnTestPlayvideo();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileOpenvideo();
	afx_msg void OnParametersSavedefaults();
	afx_msg void OnParametersLoaddefaults();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnFileKinectsensor();
  afx_msg void OnFileKinecthires();
  afx_msg void OnFileSavesource();
  afx_msg void OnUtilitiesPlaydepth();
  afx_msg void OnUtilitiesPlayboth();
  afx_msg void OnAnimationIdle();
  afx_msg void OnAnimationNeutral();
  afx_msg void OnArmGotopose();
  afx_msg void OnUtilitiesTest();
  afx_msg void OnArmSwingjoint();
  afx_msg void OnArmSwingparams();
  afx_msg void OnArmLimp();
  afx_msg void OnArmHandforce();
  afx_msg void OnForceDraghand();
  afx_msg void OnForceDragrobot();
  afx_msg void OnDemoDemooptions();
  afx_msg void OnDemoTextfile();
  afx_msg void OnDemoInteractive();
  afx_msg void OnParametersMovecmd();
  afx_msg void OnParametersTurncmd();
  afx_msg void OnParametersBaseprogress();
  afx_msg void OnParametersBaseramp();
  afx_msg void OnDemoResetrobot();
  afx_msg void OnParametersLiftcmd();
  afx_msg void OnParametersLiftramp();
  afx_msg void OnParametersGrabcmd();
  afx_msg void OnParametersGrabramp();
  afx_msg void OnParametersArmhome();
  afx_msg void OnGroundingHandcmd();
  afx_msg void OnGroundingWristcmd();
  afx_msg void OnGroundingNeckcmd();
  afx_msg void OnProfilingArmramp();
  afx_msg void OnRampNeckramp();
  afx_msg void OnDepthPersonmap();
  afx_msg void OnDepthTrackhead();
  afx_msg void OnPeopleSpeaking();
  afx_msg void OnAttentionEnrollphoto();
  afx_msg void OnAttentionEnrolllive();
  afx_msg void OnPeopleSocialevents();
  afx_msg void OnPeopleSocialmove();
  afx_msg void OnUtilitiesExtractwords();
  afx_msg void OnUtilitiesChkgrammar();
  afx_msg void OnEnvironFloormap();
  afx_msg void OnNavigationUpdating();
  afx_msg void OnEnvironIntegrated();
  afx_msg void OnNavCamcalib();
  afx_msg void OnNavGuidance();
  afx_msg void OnEnvironLocalpaths();
  afx_msg void OnEnvironDistances();
  afx_msg void OnEnvironGoto();
  afx_msg void OnPeopleVisibility();
  afx_msg void OnDemoAttn();
  afx_msg void OnObjectsTrackobjs();
  afx_msg void OnObjectsSurfacemap();
  afx_msg void OnObjectsDetect();
  afx_msg void OnObjectsShape();
  afx_msg void OnObjectsTrack();
  afx_msg void OnObjectsFilter();
  afx_msg void OnSurfacePicktable();
  afx_msg void OnObjectsPlanefit();
  afx_msg void OnNavDepthfov();
  afx_msg void OnObjectsSurfacezoom();
  afx_msg void OnDemoStaticpose();
  afx_msg void OnSurfaceHybridseg();
  afx_msg void OnObjectsColorparams();
  afx_msg void OnSurfaceColorobjs();
  afx_msg void OnUtilitiesValuerules();
  afx_msg void OnVisualDist();
  afx_msg void OnVisualDimsbins();
  afx_msg void OnVisualColorfinding();
  afx_msg void OnVisualHuethresholds();
  afx_msg void OnVisualPrimarycolors();
  afx_msg void OnVisualObjcomparison();
  afx_msg void OnPeopleChest();
  afx_msg void OnPeopleHead();
  afx_msg void OnPeopleShoulder();
  afx_msg void OnRoomHeadtrack();
  afx_msg void OnUtilitiesBatteryfull();
  afx_msg void OnObjectsShapebins();
  afx_msg void OnRoomPersonmap();
  afx_msg void OnMoodActivitylevel();
  afx_msg void OnMoodEnergylevel();
  afx_msg void OnMoodInteractionlevel();
  afx_msg void OnRoomRobotdims();
  afx_msg void OnSocialSaccades();
  afx_msg void OnMotionCalibwrist();
  afx_msg void OnMotionCalibarm();
  afx_msg void OnManipulationHandeye();
  afx_msg void OnManipulationGotovia();
  afx_msg void OnManipulationAdjustz();
  afx_msg void OnManipulationMoveobj();
  afx_msg void OnObjectsGrasppoint();
  afx_msg void OnObjectsTrajectoryctrl();
  afx_msg void OnObjectsMotiondone();
  afx_msg void OnDetectionGazesurface();
  afx_msg void OnSocialSurffind();
  afx_msg void OnTableSurface();
  afx_msg void OnObjectsSurfheight();
  afx_msg void OnObjectsSurflocation();
  afx_msg void OnMotionInvkinematics();
  afx_msg void OnObjectsWorkspace();
  afx_msg void OnManipulationDeposit();
  afx_msg void OnObjectsEmptyspot();
  afx_msg void OnGrabBodyshift();
  afx_msg void OnRoomForkcalib();
  afx_msg void OnUtilitiesWeedgrammar();
  afx_msg void OnDemoConsolemsgs();
  afx_msg void OnDemoKerneldebug();
  afx_msg void OnGrabSurfevents();
  afx_msg void OnGrabSurftracking();
  afx_msg void OnMoodLtmmatch();
  afx_msg void OnDemoCyclerate();
  afx_msg void OnGrabSurfmotion();
  afx_msg void OnMoodValencelevel();
  afx_msg void OnMoodControllevel();
  afx_msg void OnMoodSurenesslevel();
  afx_msg void OnMoodAdjustmix();
  afx_msg void OnMoodPreferencemix();
  afx_msg void OnMoodNagtiming();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BanzaiDOC_H__88A3963D_AA79_44B8_ADD0_FFB56CC99566__INCLUDED_)
