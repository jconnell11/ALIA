// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__EAF9441A_B614_4D7C_9FE7_F05BF5630E62__INCLUDED_)
#define AFX_STDAFX_H__EAF9441A_B614_4D7C_9FE7_F05BF5630E62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WIN32_WINNT
  #define _WIN32_WINNT _WIN32_WINNT_MAXVER       // JHC: upgrade from VS2010
#endif

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// better memory leak detection (define JHC_MEM_LEAK in Preprocessor)
// add include and bin from download at https://kinddragon.github.io/vld/
#ifdef JHC_MEM_LEAK
  #include <vld.h>
#endif

#include <afxwin.h>         	// MFC core and standard components
#include <afxext.h>         	// MFC extensions
#include <afxdisp.h>        	// MFC Automation classes
#include <afxdtctl.h>		      // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>		        // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>		      // MFC socket extensions

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__EAF9441A_B614_4D7C_9FE7_F05BF5630E62__INCLUDED_)
