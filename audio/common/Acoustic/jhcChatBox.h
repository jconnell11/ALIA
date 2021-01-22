// jhcChatBox.h : dialog for text entry and conversation history
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

// To get the graphical layout: "open" file common.rc in development environment,
// use the commands in the "edit" menu to "copy" dialog IDC_JHCCHATBOX, then 
// "paste" it into the Dialog folder on the ResourceView tab of current project. 

#if !defined(AFX_JHCCHATBOX_H__A2753E01_3F81_11D3_9645_006094EB2F79__INCLUDED_)
/* CPPDOC_BEGIN_EXCLUDE */
#define AFX_JHCCHATBOX_H__A2753E01_3F81_11D3_9645_006094EB2F79__INCLUDED_
/* CPPDOC_END_EXCLUDE */

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "jhcGlobal.h"

#include "stdafx.h"                      // <====
#include "resource.h"
#include "afxwin.h"

#include "Acoustic/jhcChatHist.h"        // custom control

///////////////////////////////////////////////////////////////////////////

//= Dialog for text entry and conversation history.

class jhcChatBox : public CDialog
{
// Construction
public:
  jhcChatBox(CWnd* pParent = NULL);            // standard constructor
  ~jhcChatBox ();                              // cleanup destructor
  void Launch (int x, int y);                  // modeless placement

// Main functions
public:
  void Reset (int disable =0, const char *dir =NULL);           
  void Mute (int gray =1); 
  int Interact ();                             // message pump
  char *Get (char *input, int ssz);  
  template <size_t ssz>
    char *Get (char (&input)[ssz])             // for convenience
      {return Get(input, ssz);}
  int Done () const {return quit;}
  const char *Post (const char *output, int user =0);
  void Inject (const char *input);


// helper functions
private:
  void grab_text ();


// Dialog Data 
private:
  double scene;                                // separator gap (sec)
  FILE *log;                                   // log file (if any)
  char entry[200];                             // last unseen user input
  UL32 last;                                   // time of last post
  int disable;                                 // ignore user input
  int quit;                                    // escape key seen

  //{{AFX_DATA(jhcChatBox)
  enum { IDD = IDD_JHCCHATBOX };
  //}}AFX_DATA


// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(jhcChatBox)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL
  void OnOK() {}
  void OnCancel() {}


// Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(jhcChatBox)
    // NOTE: the ClassWizard will add member functions here
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

public:
  CChatHist m_hist;
  CEdit m_input;

  afx_msg void OnBnClickedQuit();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JHCCHATBOX_H__A2753E01_3F81_11D3_9645_006094EB2F79__INCLUDED_)
