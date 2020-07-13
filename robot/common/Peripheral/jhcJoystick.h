// jhcJoystick.h : reads potentiometers and buttons on joystick controller
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2014 IBM Corporation
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

#ifndef _JHCJOYSTICK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCJOYSTICK_
/* CPPDOC_END_EXCLUDE */


#define DIRECTINPUT_VERSION 0x0800

// Build.Settings.Link.General must have libraries dxguid.lib and dinput8.lib 
//#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "dinput8.lib")


#include "jhcGlobal.h"

#include <windows.h>
#include <initguid.h>
#include <dinput.h>
#include <dinputd.h>


//= Reads potentiometers and buttons on joystick controller.
// Logitech F310 mapping onto pots and buttons:
// <pre>
//            +-----+                                 +-----+                      
//            | p2+ |                                 | p2- |             
//           +-------+                               +-------+           
//         +-|  b4   |----+                     +----|  b5   |-+         
//       /   +-------+      \                 /      +-------+   \     
//     /                     +--------------+                     \    
//    +                                                            +    
//    |      +---+         +----+        +----+        /--\        |   
//    |    / |b15| \       | b6 |        | b7 |       | b3 |       |   
//    |   +--+   +--+      +----+        +----+   /--\ \--/ /--\   | 
//    |   |b14   b12|                            | b2 |    | b1 |  |    
//    |   +--+   +--+       __                    \--/ /--\ \--/   | 
//    |    \ |b13| /       (  )                       | b0 |       |   
//    |      +---+          ~~  *                      \--/        |   
//    |                                                            |    
//    +                p1-                       p4-               +    
//     \               __                        __               /    
//      \            /    \                    /    \            /   
//       \      p0- |  b8  | p0+          p3- |  b9  | p3+      /      
//        \          \ __ /                    \ __ /          /     
//         \                                                  /        
//          \          p1+                       p4+         /         
//           \______________________________________________/          
//  
// </pre>


class jhcJoystick
{
// PRIVATE MEMBER VARIABLES
private:
  LPDIRECTINPUT8 dio;
  LPDIRECTINPUTDEVICE8 dev;


// PUBLIC MEMBER VARIABLES
public:


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcJoystick (int n =0);
  ~jhcJoystick ();

  // main functions
  int Connection ();
  int GetState (int& buts, double *pots =NULL, int np =0);
  void TestLoop ();


// PRIVATE MEMBER FUNCTIONS
private:
  int bind_dev (int n);
  void release_dev ();

};


#endif  // once




