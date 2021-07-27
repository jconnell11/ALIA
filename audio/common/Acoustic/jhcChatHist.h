// jhcChatHist.h : GUI element for showing turns in a conversation
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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

#if !defined(AFX_CHATHIST_H__BF8E1EC2_833B_11D2_8BE7_A813DF000000__INCLUDED_)
#define AFX_CHATHIST_H__BF8E1EC2_833B_11D2_8BE7_A813DF000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CChatHist

//= GUI element for showing turns in a conversation.
// Adapted from MultiLineListBox example by Scott Milne on CodeGuru
// Actual class is "CChatHist" to keep it Microsoft flavored
// To use this first add a ListBox control using the Dialog Editor, then make
// a member variable for it, and finally change it from CListBox to CChatHist.
// NOTE: Have to change the following Properties: 
//   Notify = False, Owner Draw = Variable, Has Strings = True

class CChatHist : public CListBox
{
// Attributes
public:
	int sz;        /** Font height in pixels (negative for bold). */
  int indent;    /** Minimum indent of box from some side.      */
  double fill;   /** Min last line length if multiple lines.    */
  int hpad;      /** Side margin of text inside box.            */
  int vpad;      /** Top and bottom margin of text inside box.  */
  int skip2;     /** Half the space between successive boxes.   */
  int edge;      /** Inset of text messages from window edges.  */
  int round;     /** Equivalent circle diameter of box corner.  */
  int turns;     /** How many text strings to show in display.  */


// Operations
public:
  CChatHist ();
	void AddTurn (const char *utterance, int rt =0);
  void Clear ();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChatHist)
	//}}AFX_VIRTUAL

	virtual void MeasureItem (LPMEASUREITEMSTRUCT lpMIS);
	virtual void DrawItem (LPDRAWITEMSTRUCT lpDIS);


// Implementation
public:

	// Generated message map functions
protected:
	//{{AFX_MSG(CChatHist)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHATHIST_H__BF8E1EC2_833B_11D2_8BE7_A813DF000000__INCLUDED_)
