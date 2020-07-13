// jhcPickList.cpp : show a list of values from a jhcList derived object
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

#include "Interface\jhcPickList.h"

#include "Interface\jhcMessage.h"
/* CPPDOC_BEGIN_EXCLUDE */

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* CPPDOC_END_EXCLUDE */


///////////////////////////////////////////////////////////////////////////
// jhcPickList dialog

//= Standard constructor invokes base class constructor with layout.

jhcPickList::jhcPickList(CWnd* pParent /*=NULL*/)
	: CDialog(jhcPickList::IDD, pParent)
{
	//{{AFX_DATA_INIT(jhcPickList)
	//}}AFX_DATA_INIT

  obj = NULL;
  hilite = -1;
  sel = -1;
  ans = NULL;
}


//= Populate the list with appropriate strings.
// the ComboBox is not really created until the DoModal call
// therefore have to use this function to stuff values in
// strings are automatically sorted in the ComboBox
// starting position

BOOL jhcPickList::OnInitDialog ()
{
  const char *item;
  int v, j, i = 0, h = ((hilite >= 0) ? hilite : obj->recent);
  WCHAR w[200] = L"";
  
  // call method for base type first and ignore its return value
  CDialog::OnInitDialog();
  
  // object function returns series of string entry then NULL at end
  // function should also give a unique numeric value for each selection
  // typically this value is the item's position in the original list
  if (obj != NULL)
    while ((item = obj->NextItem(i)) != NULL)
    {
      // stuff current string into sorted ComboBox and find position
#ifndef UNICODE
      m_itemlist.AddString(item);
      j = m_itemlist.FindString(-1, item);
#else
      MultiByteToWideChar(CP_ACP, 0, item, -1, w, 200);
      m_itemlist.AddString((LPCTSTR) w);
      j = m_itemlist.FindString(-1, (LPCTSTR) w);
#endif

      // mark entry with numeric value (e.g. position) and 
      // possibly highlight it if value (e.g. position) matches key
      v = obj->LastVal();
      m_itemlist.SetItemData(j, v);
      if (v == h)
        m_itemlist.SetCurSel(j);
      i++;
    }
     
  // as a default select item in the middle of list
  if ((h < 0) || (h >= i))
    m_itemlist.SetCurSel(i / 2);
  return TRUE;
}


//= Record selected text string number.
// for some reason the ComboBox is only operational during DoModal
// so the selected line is explictly saved here

void jhcPickList::DoDataExchange(CDataExchange* pDX)
{
  int i;

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(jhcPickList)
	DDX_Control(pDX, IDC_JHC_COMBO2, m_itemlist);
	//}}AFX_DATA_MAP

  // look up value associated with string selected (if any)
  if ((i = m_itemlist.GetCurSel()) != CB_ERR)
    sel = (int) m_itemlist.GetItemData(i);
  if (ans != NULL)
    m_itemlist.GetWindowText((LPTSTR) ans, 250);
}


BEGIN_MESSAGE_MAP(jhcPickList, CDialog)
	//{{AFX_MSG_MAP(jhcPickList)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// jhcPickList message handlers


//= Simple user call, returns value associated with selection.
// -1 = not in list, -2 = cancelled, "ans" bound to typed entry
// if "add" is non-zero, will attempt to add new things to list

int jhcPickList::ChooseItem (jhcList *src, int add, char *dest)
{
  char tmp[250] = "";

  obj = src;
  if (dest != NULL)
    ans = dest;
  else
    ans = tmp;
  if (DoModal() != IDOK)
    return -2;
  if ((sel < 0) && (add != 0))
    sel = src->AddItem(ans);
  obj->recent = sel;
  ans = NULL;
  return sel;
}


