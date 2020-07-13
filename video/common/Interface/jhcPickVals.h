// jhcPickVals.h : edit a selection of labelled values
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2004 IBM Corporation
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
// use the commands in the "edit" menu to "copy" dialog IDC_JHCARGS, then 
// "paste" it into the Dialog folder on the ResourceView tab of current project. 

#if !defined(AFX_JHCPICKVALS_H__1D6401A0_2042_11D3_A6D8_829F75309203__INCLUDED_)
/* CPPDOC_BEGIN_EXCLUDE */
#define AFX_JHCPICKVALS_H__1D6401A0_2042_11D3_A6D8_829F75309203__INCLUDED_
/* CPPDOC_END_EXCLUDE */

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "jhcGlobal.h"
#include "stdafx.h"
#include "resource.h"

#include "Data/jhcParam.h"


///////////////////////////////////////////////////////////////////////////

//= Edit a selection of labelled values.
// Note: Originally took 2 byte integer, 4 byte integer, and floating values.
// Note: Originally top 6 entries were integer while bottom 2 were floats

class jhcPickVals : public CDialog
{
private:
  char full[200];
  jhcParam *p;

// Construction
public:
  jhcPickVals(CWnd* pParent = NULL);   // standard constructor

private:
// Dialog Data
  //{{AFX_DATA(jhcPickVals)
  enum { IDD = IDD_JHCARGS };
  double   m_farg1;
  double   m_farg2;
  double   m_farg3;
  double   m_farg4;
  double   m_farg5;
  double   m_farg6;
  double   m_farg7;
  double   m_farg8;
  CString  m_farg1txt;
  CString  m_farg2txt;
  CString  m_farg3txt;
  CString  m_farg4txt;
  CString  m_farg5txt;
  CString  m_farg6txt;
  CString  m_farg7txt;
  CString  m_farg8txt;
    // NOTE: the ClassWizard will add data members here
  //}}AFX_DATA


// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(jhcPickVals)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
public:
  BOOL OnInitDialog ();
  int EditParams (jhcParam& src, const char *title0 =NULL);

private:
  void Clear ();
  void PostAll ();
  void PostVal (int *var, CString *txt,  int i);
  void PostVal (double *var, CString *txt, int i);
  void ExtractAll ();
  void ExtractVal (int i, int val);
  void ExtractVal (int i, double val);
  void DefaultAll ();
  void DefaultVal (int *var, int i);
  void DefaultVal (double *var, int i);

protected:

  // Generated message map functions
  //{{AFX_MSG(jhcPickVals)
  afx_msg void OnArgdefault();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_JHCPICKVALS_H__1D6401A0_2042_11D3_A6D8_829F75309203__INCLUDED_)
