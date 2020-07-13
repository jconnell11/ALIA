// MensEt.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MensEt.h"

#include "MensEtFrm.h"
#include "MensEtDoc.h"
#include "MensEtView.h"

#include "afxadv.h"   // JHC: for CRecentFileList

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMensEtApp

BEGIN_MESSAGE_MAP(CMensEtApp, CWinApp)
	//{{AFX_MSG_MAP(CMensEtApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMensEtApp construction

CMensEtApp::CMensEtApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMensEtApp object

CMensEtApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMensEtApp initialization

BOOL CMensEtApp::InitInstance()
{
	AfxEnableControlContainer();

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMensEtDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CMensEtView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so get attached document
	POSITION pos = pDocTemplate->GetFirstDocPosition();
	CMensEtDoc *doc = (CMensEtDoc *)(pDocTemplate->GetNextDoc(pos));

	// JHC: changed to maximize window on start up
	m_pMainWnd->ShowWindow(SW_SHOWNORMAL);         // fixed size
//	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);      // whole screen
	if (doc->cmd_line > 0)
		m_pMainWnd->ShowWindow(SW_SHOWMINIMIZED);    // may want to remove this
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles(TRUE);

	// JHC: possibly run demo at startup
	if (doc != NULL)
		doc->RunDemo();

	return TRUE;
}


// JHC: override to store strings which are not necessarily files

 void CMensEtApp::AddToRecentFileList(LPCTSTR lpszPathName)
{
	ASSERT_VALID(this);
	ENSURE_ARG(lpszPathName != NULL);
	ASSERT(AfxIsValidString(lpszPathName));
	
	if (m_pRecentFileList != NULL)
		m_pRecentFileList->Add(lpszPathName);
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CMensEtApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMensEtApp message handlers

