// man_drv.h : simple semantic network interpretation for Manus robot control
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _MAN_DRV_DLL_
#define _MAN_DRV_DLL_


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef MAN_DRV_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub

#ifndef MAN_DRV_EXPORTS
  #ifdef _DEBUG
    #pragma comment(lib, "man_drv_d.lib")
  #else
    #pragma comment(lib, "man_drv.lib")
  #endif
#endif


// functions required by jhcAliaDLL class

extern "C" DEXP void pool_bind (void *body);
extern "C" DEXP void pool_reset (class jhcAliaNote *attn);
extern "C" DEXP void pool_volunteer ();
extern "C" DEXP int pool_start (const class jhcAliaDesc *desc, int bid);
extern "C" DEXP int pool_status (const class jhcAliaDesc *desc, int bid);
extern "C" DEXP int pool_stop (const class jhcAliaDesc *desc, int bid);


///////////////////////////////////////////////////////////////////////////

#endif  // once