// alia_gnd.h : grounding kernel jhcAliaGnd as DLL for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
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

#ifndef _ALIAGND_DLL_
#define _ALIAGND_DLL_


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef ALIAGND_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub

#ifndef ALIAGND_EXPORTS
  #pragma comment(lib, "alia_gnd.lib")
#endif


// functions required by jhcAliaDLL class

extern "C" DEXP const char *gnd_name ();
extern "C" DEXP void gnd_platform (void *soma);
extern "C" DEXP void gnd_reset (class jhcAliaNote& attn);
extern "C" DEXP void gnd_volunteer ();
extern "C" DEXP int gnd_start (const class jhcAliaDesc& desc, int bid);
extern "C" DEXP int gnd_status (const class jhcAliaDesc& desc, int bid);
extern "C" DEXP int gnd_stop (const class jhcAliaDesc& desc, int bid);


///////////////////////////////////////////////////////////////////////////

#endif  // once
