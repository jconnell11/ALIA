// jhcPickList.h : select some entry from a jhcList derived object
// 
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2007 IBM Corporation
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
// use the commands in the "edit" menu to "copy" dialog IDC_JHCLISTNEW, then 
// "paste" it into the Dialog folder on the ResourceView tab of current project. 


#if !defined(AFX_JHCPICKLIST_H__5ED18CC0_B92B_11D3_A6D8_942CB4714832__INCLUDED_)
/* CPPDOC_BEGIN_EXCLUDE */
#define AFX_JHCPICKLIST_H__5ED18CC0_B92B_11D3_A6D8_942CB4714832__INCLUDED_
/* CPPDOC_END_EXCLUDE */

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "jhcGlobal.h"
#include "stdafx.h"
#include "resource.h"
#include "Data\jhcList.h"


///////////////////////////////////////////////////////////////////////////

//= Select some entry from a jhcList derived object.

class jhcPickList : public CDialog
{
private:
  jhcList *obj;   // pointer to external object which generates items 
  int hilite;     // initial selection value to highlight
  int sel;        // return value associated with slection in combobox
  char *ans;      // text of item selected (or new string typed by user)

// Construction
public:
  jhcPickList(CWnd* pParent = NULL);   // standard constructor

  int ChooseItem (jhcList *src, int add =0, char *dest =0);

private:
  BOOL OnInitDialog ();

private:
// Dialog Data
  //{{AFX_DATA(jhcPickList)
  enum { IDD = IDD_JHCLISTNEW };
  CComboBox  m_itemlist;
  //}}AFX_DATA


// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(jhcPickList)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:

  // Generated message map functions
  //{{AFX_MSG(jhcPickList)
    // NOTE: the ClassWizard will add member functions here
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JHCPICKLIST_H__5ED18CC0_B92B_11D3_A6D8_942CB4714832__INCLUDED_)
