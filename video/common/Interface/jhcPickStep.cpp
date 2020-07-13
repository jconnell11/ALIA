// jhcPickStep.cpp : let user pick start and stop frame of video
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

#include "Interface/jhcPickStep.h"


/* CPPDOC_BEGIN_EXCLUDE */

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* CPPDOC_END_EXCLUDE */


/////////////////////////////////////////////////////////////////////////////
// jhcPickStep dialog

//= Standard constructor invokes base class constructor with layout.

jhcPickStep::jhcPickStep(CWnd* pParent /*=NULL*/)
  : CDialog(jhcPickStep::IDD, pParent)
{
  //{{AFX_DATA_INIT(jhcPickStep)
  m_pause  = 0;
  m_rate   = 0.0;
  m_step   = 0;
  m_start  = 0;
  m_stop   = 0;
  m_where  = 0;
  m_final  = 0;
	m_pstart = 0;
	//}}AFX_DATA_INIT
}


void jhcPickStep::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(jhcPickStep)
  DDX_Text(pDX, IDC_JHC_PAUSE,  m_pause);
  DDX_Text(pDX, IDC_JHC_RATE,   m_rate);
  DDX_Text(pDX, IDC_JHC_STEP,   m_step);
  DDX_Text(pDX, IDC_JHC_START,  m_start);
  DDX_Text(pDX, IDC_JHC_STOP,   m_stop);
  DDX_Text(pDX, IDC_JHC_WHERE,  m_where);
  DDX_Text(pDX, IDC_JHC_FINAL,  m_final);
	DDX_Text(pDX, IDC_JHC_PSTART, m_pstart);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(jhcPickStep, CDialog)
  //{{AFX_MSG_MAP(jhcPickStep)
  ON_BN_CLICKED(IDC_JHC_DEFAULT, OnDefault)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// jhcPickStep message handlers

//= Run dialog and update values, return 0 if cancel chosen.

int jhcPickStep::EditStep (jhcParam& vps, double freq)
{
  int ans = 1;

  f = ((freq <= 0.0) ? 1.0 : freq);
  p = &vps;
  PostAll();
  if (DoModal() != IDOK)
    ans = 0;
  else
    ExtractAll();
  p = NULL;
  return ans;
}


//= Copies values out of the parameter list into dialog box variables.

void jhcPickStep::PostAll ()
{
  m_pause  = p->Lval(0);
  m_pstart = p->Lval(1);
  m_start  = p->Lval(2);
  m_stop   = p->Lval(3);
  m_where  = p->Lval(4);
  m_final  = p->Lval(5);
  m_step   = p->Lval(6);
  m_rate   = f / p->Fval(7);
  if (m_final < 0)
    m_final = -m_final;
}


//= Pull values off menu and inserts into prespecified positions.

void jhcPickStep::ExtractAll ()
{
  p->Lset(0, m_pause);
  p->Lset(1, m_pstart);
  p->Lset(2, m_start);
  p->Lset(3, m_stop);

  p->Lset(6, m_step);
  if (m_rate > 0.0)
    p->Fset(7, f / m_rate);
  else
    p->Fset(7, 0.001);
}


//= Copy defaults from parameter list into dialog box.

void jhcPickStep::OnDefault ()
{
  m_pause  = p->Ldef(0);
  m_pstart = p->Ldef(1);
  m_start  = p->Ldef(2);
  m_stop   = p->Ldef(3);

  m_step   = p->Ldef(6);
  m_rate   = f / p->Fdef(7);
  UpdateData(FALSE);
}


