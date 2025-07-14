// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

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

