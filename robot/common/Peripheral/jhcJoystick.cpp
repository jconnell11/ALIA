// jhcJoystick.cpp : reads potentiometers and buttons on joystick controller
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

#include <stdio.h>
#include <conio.h>

#include "jhcJoystick.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.
// can automatically connect to some joystick (-1 = none)

jhcJoystick::jhcJoystick (int n)
{
  dio = NULL;
  dev = NULL;
  bind_dev(n);
}


//= Default destructor does necessary cleanup.

jhcJoystick::~jhcJoystick ()
{
  release_dev();
}


//= Connect class to some particular joystick (0 = default).
// returns 1 if succeeded, 0 or negative for failure

int jhcJoystick::bind_dev (int n)
{
  IDirectInputJoyConfig8 *jcfg;  
  DIJOYCONFIG jprefs = {0};   

  // undo any previous joystick association then get a DirectInput connection
  release_dev();
  if (FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
                                IID_IDirectInput8, (void **) &dio, NULL)))
    return -4;

  // check for the particular numbered joystick then link to control point
  if (FAILED(dio->QueryInterface(IID_IDirectInputJoyConfig8, (void **) &jcfg)))
    return -3;
  jprefs.dwSize = sizeof(jprefs); 
  if (FAILED(jcfg->GetConfig(n, &jprefs, DIJC_GUIDINSTANCE)))
  {
    jcfg->Release();
    return -2;
  }
  jcfg->Release();
  if (FAILED(dio->CreateDevice(jprefs.guidInstance, &dev, NULL)))
    return -1;

  // configure device as a "simple joystick" (passing DIJOYSTATE2 structures)
  if (FAILED(dev->SetDataFormat(&c_dfDIJoystick2)))
    return 0;
  return 1;
}


//= Disconnect from current joystick (if any) to allow re-use.

void jhcJoystick::release_dev ()
{
  if (dev != NULL)
    dev->Release();
  if (dio != NULL)
    dio->Release();
  dev = NULL;
  dio = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Tells whether successfully connected.

int jhcJoystick::Connection ()
{
  if (dev == NULL)
    return 0;
  return 1;
}


//= Read the current values of all controls on the joystick.
// pots: 0 = X, 1 = Y, 2 = Z, 3 = Rx, 4 = Ry, 5 = Rz, 6 = U, 7 = V
// X, Y, and Z are floating point from -1.0 to +1.0
// buts is a bit vector of numbered buttons (bit 0 = first button)
// direction pads sets bits 15-12 = FLRB (like bits 3-0 = YXBA)

int jhcJoystick::GetState (int& buts, double *pots, int np)
{
  DIJOYSTATE2 js;   
  double all[8];
  int i, d, n = __min(np, 8);

  // make sure there is a joystick then update and read its state
  if (dev == NULL) 
    return -1;
  if (FAILED(dev->Poll()))
    if (FAILED(dev->Acquire()))
      return -2;
  if (FAILED(dev->GetDeviceState(sizeof(DIJOYSTATE2), &js)))
    return 0;

  // copy basic potentiometer values into variables
  if (pots != NULL)
  {
    all[0] = (js.lX  - 32768) / 32768.0;
    all[1] = (js.lY  - 32768) / 32768.0;
    all[2] = (js.lZ  - 32768) / 32768.0;
    all[3] = (js.lRx - 32768) / 32768.0;
    all[4] = (js.lRy - 32768) / 32768.0;
    all[5] = (js.lRz - 32768) / 32768.0;
    all[6] = (js.rglSlider[0] - 32768) / 32768.0;
    all[7] = (js.rglSlider[1] - 32768) / 32768.0;
    for (i = 0; i < n; i++)
      pots[i] = all[i];
  }

  // load up to 12 buttons
  buts = 0;
  for (i = 11; i >= 0; i--)
  {
    buts <<= 1;
    if ((js.rgbButtons[i] & 0x80) != 0) 
      buts |= 0x01;
  } 

  // set top 4 buttons to D-hat control (if any)
  if ((d = js.rgdwPOV[0]) >= 0)
  {
    d /= 100;
    if (d == 0)
      buts |= 0x8000;
    else if (d ==  45)
      buts |= 0xA000;
    else if (d ==  90)
      buts |= 0x2000;
    else if (d == 135)
      buts |= 0x3000;
    else if (d == 180)
      buts |= 0x1000;
    else if (d == 225)
      buts |= 0x5000;
    else if (d == 270)
      buts |= 0x4000;
    else if (d == 315)
      buts |= 0xC000;
  }
  return 1;
}


//= Simple test of joystick functionality.

void jhcJoystick::TestLoop ()
{
  int b, ok = 1;
  double p[8];

  printf("Reading current joystick values (hit any key to exit) ...\n");
  while (!_kbhit())
  {
    if ((ok = GetState(b, p, 8)) <= 0)
      break;
    printf("X %5.2f Y %5.2f Z %5.2f Rx %5.2f Ry %5.2f Rz %5.2f U %5.2f V %5.2f B 0x%04X\r", 
           p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], b);
    Sleep(100);
  }

  if (ok <= 0)
    printf("Failed with code %d\n", ok);
  printf("\nDone.\n");
}
