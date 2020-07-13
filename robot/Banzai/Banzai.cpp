// Banzai.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Banzai.h"

#include "BanzaiFrm.h"
#include "BanzaiDoc.h"
#include "BanzaiView.h"

#include "afxadv.h"   // JHC: for CRecentFileList

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBanzaiApp

BEGIN_MESSAGE_MAP(CBanzaiApp, CWinApp)
	//{{AFX_MSG_MAP(CBanzaiApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBanzaiApp construction

CBanzaiApp::CBanzaiApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CBanzaiApp object

CBanzaiApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CBanzaiApp initialization

BOOL CBanzaiApp::InitInstance()
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
		RUNTIME_CLASS(CBanzaiDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CBanzaiView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so get attached document
	POSITION pos = pDocTemplate->GetFirstDocPosition();
	CBanzaiDoc *doc = (CBanzaiDoc *)(pDocTemplate->GetNextDoc(pos));

	// JHC: changed to maximize window on start up (vs. SW_SHOW)
//	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);  // <== remove for fixed size
//	if (doc->cmd_line > 0)                     // <== remove to show always
//		m_pMainWnd->ShowWindow(SW_SHOWMINIMIZED);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles(TRUE);

	// JHC: possibly run demo at startup
	if (doc != NULL)
		doc->RunDemo();

	return TRUE;
}


// JHC: override to store strings which are not necessarily files

 void CBanzaiApp::AddToRecentFileList(LPCTSTR lpszPathName)
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
void CBanzaiApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CBanzaiApp message handlers

