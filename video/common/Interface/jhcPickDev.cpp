// jhcPickDev.cpp : show a list of available framegrabbers
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

#include "Interface\jhcPickDev.h"

// order of includes is important
#include "Interface\jhcMessage.h"
#include "Interface\jhcString.h"
#include <VfW.h>


/* CPPDOC_BEGIN_EXCLUDE */

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* CPPDOC_END_EXCLUDE */


/////////////////////////////////////////////////////////////////////////////
// jhcPickDev dialog

//= Standard constructor invokes base class constructor with layout.

jhcPickDev::jhcPickDev(CWnd* pParent /*=NULL*/)
	: CDialog(jhcPickDev::IDD, pParent)
{
	//{{AFX_DATA_INIT(jhcPickDev)
	//}}AFX_DATA_INIT
}


//= Populate the device list with names.
// the ComboBox is not really created until the DoModal call
// therefore have to use this function to stuff values in

BOOL jhcPickDev::OnInitDialog ()
{
  jhcString driver;
  int i;

  // call method for base type first and ignore its return value
  CDialog::OnInitDialog();
  
  // generate list of driver names and stuff them into combobox
  for (i = 0; i < 10; i++)
    if (!capGetDriverDescription(i, driver.Txt(), 80, NULL, 0))
      break;
    else
      m_devlist.InsertString(i, driver.Txt());
  if (i == 0)
    Complain("No VFW capture devices found");

  // as a default select the first item
  m_devlist.SetCurSel(0);
  return TRUE;
}


//= Record selected device number.
// for some reason the ComboBox is only operational during DoModal
// so the selected line is explictly saved here

void jhcPickDev::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(jhcPickDev)
  DDX_Control(pDX, IDC_JHC_COMBO3, m_devlist);
  //}}AFX_DATA_MAP

  sel = m_devlist.GetCurSel();
}


BEGIN_MESSAGE_MAP(jhcPickDev, CDialog)
  //{{AFX_MSG_MAP(jhcPickDev)
    // NOTE: the ClassWizard will add message map macros here
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// jhcPickDev message handlers


