// jhcChatHist.h : GUI element for showing turns in a conversation
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#include "stdafx.h"

#include "Interface/jhcString.h"       // common video

#include "Acoustic/jhcChatHist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CChatHist

BEGIN_MESSAGE_MAP(CChatHist, CListBox)
	//{{AFX_MSG_MAP(CChatHist)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//= Constructor sets default display parameters.
// values should only be changed when the list is empty (e.g. at beginning)

CChatHist::CChatHist ()
{
  // geometry
	sz     = 16;   
  indent = 50;   
  fill   = 0.6;
  hpad   = 10;   
  vpad   = 5;    
  skip2  = 1;   
  edge   = 10; 
  round  = 20;   
  turns  = 20;
}


//= Add an item to the display list.
// if rt > 0 then it will be shifted to the right and have a blue background

void CChatHist::AddTurn (const char *utterance, int rt)
{
  jhcString guts;
  int id;

  // check for real message then possibly purge an old entry
  if ((utterance == NULL) || (*utterance == '\0'))
    return;
  while (GetCount() >= turns)
    DeleteString(0);

  // always capitalize first letter then convert to wide (if needed)
  strcpy_s(guts.ch, utterance);
  (guts.ch)[0] = (char) toupper((guts.ch)[0]);
  guts.C2W();

  // add to the bottom of the list and make sure it is visible
  id = InsertString(-1, guts.Txt());
  SetItemData(id, rt);
  SetCaretIndex(id, 1);
}


//= Get rid of all old text strings.

void CChatHist::Clear ()
{
  ResetContent();
}


/////////////////////////////////////////////////////////////////////////////
// CChatHist message handlers

//= Adjust stored item height to handle line wrap for long strings.

void CChatHist::MeasureItem (LPMEASUREITEMSTRUCT lpMIS)
{
	CPaintDC dc(this);
  HDC hdc = dc;
  HFONT font;
	CRect r;
	CString txt;
	int id = lpMIS->itemID;

  // for click in bad places
  if (id < 0)
   return;

  // adjust nominal display region for indenting and side margins
	GetItemRect(id, r);
  r.DeflateRect(indent + hpad + edge, 0, hpad + edge, 0);

  // switch to font which will be used to display
  font = CreateFont(abs(sz), 0, 0, 0, ((sz > 0) ? FW_REGULAR : FW_BOLD), 
                    FALSE, FALSE, FALSE, ANSI_CHARSET, 
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, 
                    FF_DONTCARE, _T("jhcChatHist"));
  SelectObject(hdc, font);

  // compute actual rectangle height if multiple lines
	GetText(id, txt);
  dc.DrawText(txt, -1, r, DT_WORDBREAK | DT_CALCRECT);
	lpMIS->itemHeight = r.Height() + 2 * (vpad + skip2);
}


//= Draw the text associated with an item inside a round edged box.
// if rt was > 1 then shift to the right and draw as white on blue
// otherwise draw as back on light gray

void CChatHist::DrawItem (LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pdc = CDC::FromHandle(lpDIS->hDC);
  CRect r0, r2, r = lpDIS->rcItem;
  COLORREF tcol, bcol;
  POINT corner = {round, round};
  HFONT font;
	CString txt;
  int mid, shrink, ln, id = lpDIS->itemID;

  // check for click in bad places else get basic info
  if (id < 0)
   return;
	GetItemRect(id, r0);
	GetText(id, txt);

  // see if special gray separator line is needed
  if (txt == "---")
  {
    mid = r0.bottom + ROUND(0.25 * (r0.top - r0.bottom));
    CPen pen(PS_SOLID, 1, (COLORREF) RGB(200, 200, 200));
    pdc->SelectObject(&pen);
    pdc->MoveTo(r0.left, mid);
    pdc->LineTo(r0.right, mid);
    return;
  }
  
  // switch to font which will be used to display
  font = CreateFont(abs(sz), 0, 0, 0, ((sz > 0) ? FW_REGULAR : FW_BOLD), 
                    FALSE, FALSE, FALSE, ANSI_CHARSET, 
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, 
                    FF_DONTCARE, _T("jhcChatHist"));
  SelectObject(lpDIS->hDC, font);

  // adjust display region for indenting and side margins then find text box
  r0.DeflateRect(indent + hpad + edge, 0, hpad + edge, 0);
  pdc->DrawText(txt, -1, r0, DT_WORDBREAK | DT_CALCRECT);

  // compute minimum rectangle width needed and sideways shrinking
  ln = r0.Height() / sz;
  if (ln > 1)
  {
    // try shrinking width if multiple (possibly partial) lines
    r2 = r0;
    r0.left += ROUND(r0.Width() * fill / (double) ln);
    pdc->DrawText(txt, -1, r0, DT_WORDBREAK | DT_CALCRECT);
    if ((r0.Height() / sz) > ln)
      r0 = r2;
  }
  shrink = r.Width() - (r0.Width() + 2 * hpad + edge);

  // set colors and indent position of rectangle
  if (lpDIS->itemData == 0)
  {
    // left side box with black on gray
    r.DeflateRect(edge, skip2, shrink, skip2);
    tcol = (COLORREF) RGB(0, 0, 0);
    bcol = (COLORREF) RGB(220, 220, 220);
  }
  else
  {
    // right side box with white on blue
    r.DeflateRect(shrink, skip2, edge, skip2);
    tcol = (COLORREF) RGB(255, 255, 255);
    bcol = (COLORREF) RGB(0, 0, 255);
  }

  // draw colored background region
	CBrush brush(bcol);
  pdc->SelectObject(&brush);
  pdc->SelectObject(GetStockObject(WHITE_PEN));
  pdc->RoundRect(&r, corner);

  // draw text inside rectangle
  r.DeflateRect(hpad, vpad, hpad, vpad);
  pdc->SetTextColor(tcol);
	pdc->SetBkColor(bcol);
	pdc->DrawText(txt, -1, &r, DT_WORDBREAK);
}

