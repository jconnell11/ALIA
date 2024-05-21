// BanzaiView.cpp : implementation of the CBanzaiView class
//

#include "stdafx.h"
#include "Banzai.h"

#include "BanzaiDoc.h"
#include "BanzaiView.h"

#ifdef _DEBUG
  #define new DEBUG_NEW
  #undef THIS_FILE
  static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBanzaiView

IMPLEMENT_DYNCREATE(CBanzaiView, CView)

BEGIN_MESSAGE_MAP(CBanzaiView, CView)
	//{{AFX_MSG_MAP(CBanzaiView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBanzaiView construction/destruction

CBanzaiView::CBanzaiView()
{
	// TODO: add construction code here

}

CBanzaiView::~CBanzaiView()
{
}

BOOL CBanzaiView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}


// JHC: override so windows are not cleared

void CBanzaiView::OnInitialUpdate ()
{
//	CBanzaiDoc* pDoc = GetDocument();

//  pDoc->ShowFirst();  
}


/////////////////////////////////////////////////////////////////////////////
// CBanzaiView drawing

void CBanzaiView::OnDraw(CDC* pDC)
{
	CBanzaiDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CBanzaiView diagnostics

#ifdef _DEBUG
void CBanzaiView::AssertValid() const
{
	CView::AssertValid();
}

void CBanzaiView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CBanzaiDoc* CBanzaiView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBanzaiDoc)));
	return (CBanzaiDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBanzaiView message handlers

