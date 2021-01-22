// BanzaiView.h : interface of the CBanzaiView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_BanzaiVIEW_H__CDC4C296_7AE6_4E2A_AAC7_1A5DBF9F8BF9__INCLUDED_)
#define AFX_BanzaiVIEW_H__CDC4C296_7AE6_4E2A_AAC7_1A5DBF9F8BF9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CBanzaiView : public CView
{
protected: // create from serialization only
	CBanzaiView();
	DECLARE_DYNCREATE(CBanzaiView)

// Attributes
public:
	CBanzaiDoc* GetDocument();
  void OnInitialUpdate ();      // JHC: added to show first video frame

// Operations
public:


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBanzaiView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBanzaiView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CBanzaiView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in BanzaiView.cpp
inline CBanzaiDoc* CBanzaiView::GetDocument()
   { return (CBanzaiDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BanzaiVIEW_H__CDC4C296_7AE6_4E2A_AAC7_1A5DBF9F8BF9__INCLUDED_)
