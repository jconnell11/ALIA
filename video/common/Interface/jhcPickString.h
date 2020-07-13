// jhcPickString.h : let user type in some text string
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2011 IBM Corporation
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
// use the commands in the "edit" menu to "copy" dialog IDC_JHCNAME, then 
// "paste" it into the Dialog folder on the ResourceView tab of current project. 

#if !defined(AFX_JHCPICKSTRING_H__A2753E01_3F81_11D3_9645_006094EB2F79__INCLUDED_)
/* CPPDOC_BEGIN_EXCLUDE */
#define AFX_JHCPICKSTRING_H__A2753E01_3F81_11D3_9645_006094EB2F79__INCLUDED_
/* CPPDOC_END_EXCLUDE */

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "jhcGlobal.h"

#include "stdafx.h"             // <====
#include "resource.h"


///////////////////////////////////////////////////////////////////////////

//= Let user type in some text string.

class jhcPickString : public CDialog
{
// Construction
public:
  jhcPickString(CWnd* pParent = NULL);   // standard constructor

  // main functions
  int EditString (char *string, int nodef, char *prompt, int ssz);
  int EditString (char *string, int *cbox, 
                  int nostr, char *strtxt, 
                  int nobox, char *boxtxt,
                  int ssz);

  // convenience
  template <size_t ssz>
    int EditString (char (&string)[ssz], int nodef =0, char *prompt =NULL)
      {return EditString(string, nodef, prompt, ssz);}
  template <size_t ssz>
    int EditString (char (&string)[ssz], int *cbox, 
                    int nostr =0, char *strtxt =NULL, 
                    int nobox =0, char *boxtxt =NULL)
      {return EditString(string, cbox, nostr, strtxt, nobox, boxtxt, ssz);}


private:
// Dialog Data
  //{{AFX_DATA(jhcPickString)
  enum { IDD = IDD_JHCNAME };
  CString  m_name;
  CString  m_tprompt;
  CString  m_bprompt;
  BOOL  m_check;
  //}}AFX_DATA


// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(jhcPickString)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:

  // Generated message map functions
  //{{AFX_MSG(jhcPickString)
    // NOTE: the ClassWizard will add member functions here
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JHCPICKSTRING_H__A2753E01_3F81_11D3_9645_006094EB2F79__INCLUDED_)
