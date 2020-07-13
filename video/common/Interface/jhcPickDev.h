// jhcPickDev.h : select VFW device for image capture
// 
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2016 IBM Corporation
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
// use the commands in the "edit" menu to "copy" dialog IDC_JHCDEVICES, then 
// "paste" it into the Dialog folder on the ResourceView tab of current project. 


#if !defined(AFX_JHCPICKDEV_H__8C0B3661_B06B_11D2_9645_002035223D8D__INCLUDED_)
/* CPPDOC_BEGIN_EXCLUDE */
#define AFX_JHCPICKDEV_H__8C0B3661_B06B_11D2_9645_002035223D8D__INCLUDED_
/* CPPDOC_END_EXCLUDE */

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "jhcGlobal.h"
#include "stdafx.h"
#include "resource.h"


///////////////////////////////////////////////////////////////////////////

//= Select VFW device for image capture.

class jhcPickDev : public CDialog
{
// return value from internal combobox
private:
  int sel;

// Construction
public:
  jhcPickDev(CWnd* pParent = NULL);   // standard constructor

private:
  BOOL OnInitDialog ();
  
private:
// Dialog Data
  //{{AFX_DATA(jhcPickDev)
  enum { IDD = IDD_JHCDEVICES };
  CComboBox  m_devlist;
  //}}AFX_DATA


// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(jhcPickDev)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:

  // Generated message map functions
  //{{AFX_MSG(jhcPickDev)
    // NOTE: the ClassWizard will add member functions here
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JHCPICKDEV_H__8C0B3661_B06B_11D2_9645_002035223D8D__INCLUDED_)
