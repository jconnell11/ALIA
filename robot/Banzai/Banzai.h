// Banzai.h : main header file for the Banzai application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CBanzaiApp:
// See Banzai.cpp for the implementation of this class
//

class CBanzaiApp : public CWinApp
{
public:
	CBanzaiApp();

  // JHC override
  void AddToRecentFileList(LPCTSTR lpszPathName);

  // JHC retrieval
  LPCTSTR GetLastFile () const;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBanzaiApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL


// Implementation
	//{{AFX_MSG(CBanzaiApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

