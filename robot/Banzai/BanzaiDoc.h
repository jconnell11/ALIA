// BanzaiDoc.h : top level GUI framework does something
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

#include "jhcEliCoord.h"               // class encapsulating base functionality


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
  afx_msg void OnInterestVividview();
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
  afx_msg void OnParametersBatterylevel();
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
  afx_msg void OnParametersWatching();
  afx_msg void OnParametersOrienting();
  afx_msg void OnParametersTargettime();
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
  afx_msg void OnNavFovlimits();
  afx_msg void OnEnvironGoto();
  afx_msg void OnNavConfidence();
  afx_msg void OnPeopleVisibility();
  afx_msg void OnDemoAttn();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BanzaiDOC_H__88A3963D_AA79_44B8_ADD0_FFB56CC99566__INCLUDED_)
