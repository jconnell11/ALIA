// MensEt.h : main header file for the MensEt application
//

#if !defined(AFX_MensEt_H__868A49A6_D146_40D0_B59B_70791E7E61F7__INCLUDED_)
#define AFX_MensEt_H__868A49A6_D146_40D0_B59B_70791E7E61F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMensEtApp:
// See MensEt.cpp for the implementation of this class
//

class CMensEtApp : public CWinApp
{
public:
	CMensEtApp();

  // JHC override
  void AddToRecentFileList(LPCTSTR lpszPathName);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMensEtApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL


// Implementation
	//{{AFX_MSG(CMensEtApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MensEt_H__868A49A6_D146_40D0_B59B_70791E7E61F7__INCLUDED_)
