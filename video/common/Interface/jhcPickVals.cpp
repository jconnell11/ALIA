// jhcPickVals.cpp : let user edit some selected parameters
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2014 IBM Corporation
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

#include "Interface/jhcString.h"

#include "Interface/jhcPickVals.h"


/* CPPDOC_BEGIN_EXCLUDE */

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* CPPDOC_END_EXCLUDE */


/////////////////////////////////////////////////////////////////////////////
// jhcPickVals dialog

//= Standard constructor invokes base class constructor with layout.

jhcPickVals::jhcPickVals(CWnd* pParent /*=NULL*/)
  : CDialog(jhcPickVals::IDD, pParent)
{
  //{{AFX_DATA_INIT(jhcPickVals)
  m_farg1 = 0.0;
  m_farg2 = 0.0;
  m_farg3 = 0.0;
  m_farg4 = 0.0;
  m_farg5 = 0.0;
  m_farg6 = 0.0;
  m_farg7 = 0.0;
  m_farg8 = 0.0;
  m_farg1txt = _T("");
  m_farg2txt = _T("");
  m_farg3txt = _T("");
  m_farg4txt = _T("");
  m_farg5txt = _T("");
  m_farg6txt = _T("");
  m_farg7txt = _T("");
  m_farg8txt = _T("");
    // NOTE: the ClassWizard will add member initialization here
  //}}AFX_DATA_INIT

  Clear();
}


void jhcPickVals::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(jhcPickVals)
  DDX_Text(pDX, IDC_JHC_ARG1, m_farg1);
  DDX_Text(pDX, IDC_JHC_ARG2, m_farg2);
  DDX_Text(pDX, IDC_JHC_ARG3, m_farg3);
  DDX_Text(pDX, IDC_JHC_ARG4, m_farg4);
  DDX_Text(pDX, IDC_JHC_ARG5, m_farg5);
  DDX_Text(pDX, IDC_JHC_ARG6, m_farg6);
  DDX_Text(pDX, IDC_JHC_ARG7, m_farg7);
  DDX_Text(pDX, IDC_JHC_ARG8, m_farg8);
  DDX_Text(pDX, IDC_JHC_TXT1, m_farg1txt);
  DDX_Text(pDX, IDC_JHC_TXT2, m_farg2txt);
  DDX_Text(pDX, IDC_JHC_TXT3, m_farg3txt);
  DDX_Text(pDX, IDC_JHC_TXT4, m_farg4txt);
  DDX_Text(pDX, IDC_JHC_TXT5, m_farg5txt);
  DDX_Text(pDX, IDC_JHC_TXT6, m_farg6txt);
  DDX_Text(pDX, IDC_JHC_TXT7, m_farg7txt);
  DDX_Text(pDX, IDC_JHC_TXT8, m_farg8txt);
    // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(jhcPickVals, CDialog)
  //{{AFX_MSG_MAP(jhcPickVals)
  ON_BN_CLICKED(IDC_JHC_ARGDEFAULT, OnArgdefault)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// jhcPickVals message handlers

//= Override initialization in order to set correct title.

BOOL jhcPickVals::OnInitDialog ()
{
  const char *name;
  jhcString msg;

  if (p != NULL)
  {
    name = p->GetTitle();
    if ((name != NULL) && (*name != '\0'))
    {
      strcat_s(full, name);
      msg.Set(full);
      SetWindowText(msg.Txt());
    }
  }
  return CDialog::OnInitDialog();
}


//= Pops up dialog box, returns 1 if OK but 0 if cancel.
// Posts values from parameter list and lets user edit them.
// Can prefix title string with given header if desired.

int jhcPickVals::EditParams (jhcParam& src, const char *title0)
{
  int ans = 1;

  // set up start of menu title
  *full = '\0';
  if ((title0 != NULL) && (*title0 != '\0'))
    sprintf_s(full, "%s ", title0);

  // set up parameter values and strings
  p = &src;
  PostAll();
  
  // let user edit values as needed
  if (DoModal() != IDOK)
    ans = 0;
  else
    ExtractAll();

  // clean up
  Clear();
  return ans;
}


//= No parameter array currently bound.

void jhcPickVals::Clear ()
{
  p = NULL;
}


//= Function called by the "Defaults" button.

void jhcPickVals::OnArgdefault() 
{
  DefaultAll();
  UpdateData(FALSE);
}


//= Copies values out of the parameter list into dialog box variables.

void jhcPickVals::PostAll ()
{
  PostVal(&m_farg1, &m_farg1txt, 0);
  PostVal(&m_farg2, &m_farg2txt, 1);
  PostVal(&m_farg3, &m_farg3txt, 2);
  PostVal(&m_farg4, &m_farg4txt, 3);
  PostVal(&m_farg5, &m_farg5txt, 4);
  PostVal(&m_farg6, &m_farg6txt, 5);

  PostVal(&m_farg7, &m_farg7txt, 6);
  PostVal(&m_farg8, &m_farg8txt, 7);
}


//= Translate an item to be an integer entry in dialog box.
// if a float, multiply by 100 to retain some precision
// NOTE: not used anymore (6/12)

void jhcPickVals::PostVal (int *var, CString *txt, int i)
{   
  // read specified entry from array given   
  if ((i < 0) || (i >= p->Size()))
    return;
  *var = 0;
  *txt = p->Text(i);

  // determine integer or float type by checking if pointer bound
  if (p->Ltype(i))
    *var = p->Lval(i);
  else if (p->Ftype(i))
  {
    *var = (int)(p->Fval(i) * 100.0 + ((p->Fval(i) >= 0) ? 0.5 : -0.5));
    *txt += " (x 100)";
  }
  else
    *txt = "";
}


//= Translate an item to be a floating point entry in dialog box.

void jhcPickVals::PostVal (double *var, CString *txt, int i)
{   
  // read specified entry from array given   
  if ((i < 0) || (i >= p->Size()))
    return;
  *var = 0.0;
  *txt = p->Text(i);

  // determine integer or float type by checking if pointer bound
  if (p->Ltype(i))
    *var = (double)(p->Lval(i));
  else if (p->Ftype(i))
    *var = p->Fval(i);
  else
    *txt = "";
}


//= Pull values off menu and inserts into prespecified positions.

void jhcPickVals::ExtractAll ()
{
  ExtractVal(0, m_farg1);
  ExtractVal(1, m_farg2);
  ExtractVal(2, m_farg3);
  ExtractVal(3, m_farg4);
  ExtractVal(4, m_farg5);
  ExtractVal(5, m_farg6);

  ExtractVal(6, m_farg7);
  ExtractVal(7, m_farg8);
}


//= Push an integer answer into some entry.
// correct for x100 multiplication of floats
// NOTE: not used anymore (6/12)

void jhcPickVals::ExtractVal (int i, int val)
{
  if ((i < 0) || (i >= p->Size()))
    return;

  if (p->Ltype(i))
    p->Lset(i, val);
  else if (p->Ftype(i))
    p->Fset(i, (double)(val / 100.0));
}


//= Push a floating point answer into some entry.

void jhcPickVals::ExtractVal (int i, double val)
{
  if ((i < 0) || (i >= p->Size()))
    return;

  if (p->Ltype(i))
    p->Lset(i, (int) val);
  else if (p->Ftype(i))
    p->Fset(i, val);
}


//= Copy defaults from parameter list into dialog box.

void jhcPickVals::DefaultAll ()
{
  DefaultVal(&m_farg1, 0);
  DefaultVal(&m_farg2, 1);
  DefaultVal(&m_farg3, 2);
  DefaultVal(&m_farg4, 3);
  DefaultVal(&m_farg5, 4);
  DefaultVal(&m_farg6, 5);

  DefaultVal(&m_farg7, 6);
  DefaultVal(&m_farg8, 7);
}


//= Loads a default value into an integer field of dialog box.
// if a float, multiplies by 100 to retain some precision
// NOTE: not used anymore (6/12)

void jhcPickVals::DefaultVal (int *var, int i)
{
  if ((i < 0) || (i >= p->Size()))
    return;

  if (p->Ltype(i))
    *var = p->Ldef(i);
  else if (p->Ftype(i))
    *var = (int)(p->Fdef(i) * 100.0 + 0.5);
}


//= Loads a default value into an floating point field of dialog box.

void jhcPickVals::DefaultVal (double *var, int i)
{
  if ((i < 0) || (i >= p->Size()))
    return;

  if (p->Ltype(i))
    *var = (double) p->Ldef(i);
  else if (p->Ftype(i))
    *var = p->Fdef(i);
}




