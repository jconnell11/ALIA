// jhcPickString.cpp : pop a dialog box for entering a string
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

#include "Interface/jhcString.h"

#include "Interface/jhcPickString.h"


/* CPPDOC_BEGIN_EXCLUDE */

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* CPPDOC_END_EXCLUDE */


/////////////////////////////////////////////////////////////////////////////
// jhcPickString dialog

//= Standard constructor invokes base class constructor with layout.

jhcPickString::jhcPickString(CWnd* pParent /*=NULL*/)
	: CDialog(jhcPickString::IDD, pParent)
{
	//{{AFX_DATA_INIT(jhcPickString)
	m_name = _T("");
	m_tprompt = _T("");
	m_bprompt = _T("");
	m_check = FALSE;
	//}}AFX_DATA_INIT
}


void jhcPickString::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(jhcPickString)
	DDX_Text(pDX, IDC_JHC_NAME1, m_name);
	DDX_Text(pDX, IDC_JHC_TPROMPT1, m_tprompt);
	DDX_Text(pDX, IDC_JHC_BPROMPT1, m_bprompt);
	DDX_Check(pDX, IDC_JHC_CHECK1, m_check);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(jhcPickString, CDialog)
	//{{AFX_MSG_MAP(jhcPickString)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// jhcPickString message handlers

//= Update given string (if any) to match user's choice.
// set "nodef" to 1 to suppress display of string's current value
// can optional supply a prompting string

int jhcPickString::EditString (char *string, int nodef, char *prompt, int ssz)
{
  jhcString guts;
  int n;

  if (string == NULL)
    return -1;

  // set up member variables
  if (nodef == 0)
    m_name = string;
  if (prompt != NULL)
    m_tprompt = prompt;

  // pop dialog box and interact with user
  if (DoModal() != IDOK)
    return 0;

  // extract the text part as an old-fashioned string
  n = m_name.GetLength() + 1;
  guts.Set(m_name.GetBuffer(n));
  strcpy_s(string, ssz, guts.ch);
  m_name.ReleaseBuffer();
  return 1;
}


//= Same as other EditString but shows and reads boolean from check box.

int jhcPickString::EditString (char *string, int *cbox, 
                               int nostr, char *strtxt, 
                               int nobox, char *boxtxt,
                               int ssz)
{
  jhcString guts;
  int n;

  if ((string == NULL) || (cbox == NULL))
    return -1;

  // set up member variables
  if (nostr == 0)
    m_name = string;
  if (nobox == 0)
    m_check = (BOOL)(*cbox);
  if (strtxt!= NULL)
    m_tprompt = strtxt;
  if (boxtxt != NULL)
    m_bprompt = boxtxt;

  // pop dialog box then copy out values
  if (DoModal() != IDOK)
    return 0;
  if (m_check)
    *cbox = 1;
  else
    *cbox = 0;

  // extract the text part as an old-fashioned string
  n = m_name.GetLength() + 1;
  guts.Set(m_name.GetBuffer(n));
  strcpy_s(string, ssz, guts.ch);
  m_name.ReleaseBuffer();
  return 1;
}
