// MensEtView.cpp : implementation of the CMensEtView class
//

#include "stdafx.h"
#include "MensEt.h"

#include "MensEtDoc.h"
#include "MensEtView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMensEtView

IMPLEMENT_DYNCREATE(CMensEtView, CView)

BEGIN_MESSAGE_MAP(CMensEtView, CView)
	//{{AFX_MSG_MAP(CMensEtView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMensEtView construction/destruction

CMensEtView::CMensEtView()
{
	// TODO: add construction code here

}

CMensEtView::~CMensEtView()
{
}

BOOL CMensEtView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}


// JHC: override so windows are not cleared

void CMensEtView::OnInitialUpdate ()
{
//	CMensEtDoc* pDoc = GetDocument();

//  pDoc->ShowFirst();  
}


/////////////////////////////////////////////////////////////////////////////
// CMensEtView drawing

void CMensEtView::OnDraw(CDC* pDC)
{
	CMensEtDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CMensEtView diagnostics

#ifdef _DEBUG
void CMensEtView::AssertValid() const
{
	CView::AssertValid();
}

void CMensEtView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMensEtDoc* CMensEtView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMensEtDoc)));
	return (CMensEtDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMensEtView message handlers
