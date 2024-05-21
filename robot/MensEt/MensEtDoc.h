// MensEtDoc.h : top level GUI framework does something
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#if !defined(AFX_MensEtDOC_H__88A3963D_AA79_44B8_ADD0_FFB56CC99566__INCLUDED_)
#define AFX_MensEtDOC_H__88A3963D_AA79_44B8_ADD0_FFB56CC99566__INCLUDED_

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

#include "Body/jhcTaisRemote.h"        // common robot

#include "RWI/jhcManusCoord.h"         // class encapsulating base functionality


class CMensEtDoc : public CDocument, private jhcTools
{
protected: // create from serialization only
	CMensEtDoc();
	DECLARE_DYNCREATE(CMensEtDoc)

// JHC: useful to add these directly to window
private:
  int cripple;
  double ver;
  jhcDisplay d;
  jhcExpVSrc v;
  jhcImg res, now, dnow;
  char rname[200], ifile[200], cwd[200], cdir[200];

  // overall configuration choices
  jhcParam ips;
  int tid, cam;


public:
  int cmd_line;


// Attributes
public:
  jhcManusCoord mc;    // class encapsulating base functionality
  jhcTaisRemote tais;  // linkage to remotely hosted brain
  jhcConsole prt;      // place for jprintf output
  jhcChatBox chat;     // place for text interaction

  class jhcInteractFSM *fsm;  // motion sequences
  class jhcStackSeg *ss;      // vision routines
  class jhcPatchProps *pp;    // feature analysis


// JHC: helper functions
public:
  void RunDemo ();
  int LockAfter (int mon, int yr, int smon =0, int syr =0);
  int LockedFcn ();
  void ShowFirst ();   
  int ChkStream (int dw =0, int dh =0);    

private:
  int interact_params (const char *fname);
  bool next_line (char *txt, int ssz, FILE *f) const;


// Operations
public:


// JHC: manually added OnOpenDocument
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMensEtDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL


// Implementation
public:
	virtual ~CMensEtDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CMensEtDoc)
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
  afx_msg void OnEnvironmentLoadgeom();
  afx_msg void OnEnvironmentSavegeom();
  afx_msg void OnUtilitiesTest();
  afx_msg void OnManusDriveparams();
  afx_msg void OnManusRangeparams();
  afx_msg void OnManusLiftparams();
  afx_msg void OnManusGripparams();
  afx_msg void OnDemoOptions();
  afx_msg void OnDemoInteract();
  afx_msg void OnDemoRemote();
  afx_msg void OnParametersRemoteparams();
  afx_msg void OnManusCameraparams();
  afx_msg void OnCameraparamsDewarp();
  afx_msg void OnProcessingGroundplane();
  afx_msg void OnProcessingCleanup();
  afx_msg void OnParametersPatcharea();
  afx_msg void OnParametersFloorparams();
  afx_msg void OnVisionObjects();
  afx_msg void OnVisionBoundary();
  afx_msg void OnReflexesCozyup();
  afx_msg void OnReflexesEngulf();
  afx_msg void OnReflexesAcquire();
  afx_msg void OnReflexesDeposit();
  afx_msg void OnReflexesStack();
  afx_msg void OnReflexesOpen();
  afx_msg void OnReflexesClose();
  afx_msg void OnManusWidthparams();
  afx_msg void OnVisionStackgrow();
  afx_msg void OnParametersShapeparams();
  afx_msg void OnVisionColordiffs();
  afx_msg void OnVisionSimilarregions();
  afx_msg void OnFileSaveinput();
  afx_msg void OnParametersCleanparams();
  afx_msg void OnVisionFeatures();
  afx_msg void OnParametersQuantparams();
  afx_msg void OnParametersExtractparams();
  afx_msg void OnParametersPickparams();
  afx_msg void OnParametersStripedparams();
  afx_msg void OnDemoFilelocal();
  afx_msg void OnParametersSizeparams();
  afx_msg void OnMotionDistance();
  afx_msg void OnMotionTranslation();
  afx_msg void OnMotionRotation();
  afx_msg void OnMotionLift();
  afx_msg void OnReflexesInitpose();
  afx_msg void OnDemoTiming();
  afx_msg void OnUtilitiesExtvocab();
  afx_msg void OnUtilitiesTestvocab();
  afx_msg void OnUtilitiesTestgraphizer();
  afx_msg void OnDemoBasicmsgs();
  afx_msg void OnDemoCyclerate();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MensEtDOC_H__88A3963D_AA79_44B8_ADD0_FFB56CC99566__INCLUDED_)
