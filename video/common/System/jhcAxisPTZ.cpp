// jhcAxisPTZ.cpp : controls Axis pan, tilt, zoom network cameras 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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
#include <string.h>
#include <process.h>

#include "Data/jhcParam.h"

#include "System/jhcAxisPTZ.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcAxisPTZ::jhcAxisPTZ ()
{
  *name = '\0';    // can use something like "_mid"
  Defaults();
}


//= Read connection parameters from some file.

int jhcAxisPTZ::Defaults (const char *fname)
{
  char tag[80];
  int ok = 1;

  // camera access values
  sprintf_s(tag, "axis%s_ip", name);
  ok &= hps.LoadText(ip, fname, tag,   "9.2.182.3");
  sprintf_s(tag, "axis%s_upwd", name);
  ok &= hps.LoadText(upwd, fname, tag, "root:c0gnitive");

  // home position
  ok &= home_params(fname);
  return ok;
}


//= Parameters used for defining reset state of camera.

int jhcAxisPTZ::home_params (const char *fname)
{
  char tag[80];
  jhcParam *ps = &hps;
  int ok;

  sprintf_s(tag, "axis%s_home", name);
  ps->SetTag(tag, 0);
  ps->NextSpecF( &p0,     -88.0, "Initial pan");     // was -90
  ps->NextSpecF( &t0,     -11.0, "Initial tilt");    // was -10
  ps->NextSpec4( &z0,    1500,   "Initial zoom");    // was 1000
  ps->Skip();
  ps->NextSpec4( &brite,   60,   "Brightness");
  ps->NextSpec4( &back,     1,   "Backlight compensation");  

  ps->NextSpec4( &iris,     1,   "Auto iris");  
  ps->NextSpec4( &focus,    1,   "Auto focus");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Save current connection parameters to some file.

int jhcAxisPTZ::SaveVals (const char *fname) const
{
  char tag[80];
  int ok = 1;

  // camera access values
  sprintf_s(tag, "axis%s_ip", name);
  ok &= hps.SaveText(fname, tag, ip);
  sprintf_s(tag, "axis%s_upwd", name);
  ok &= hps.SaveText(fname, tag, upwd);

  // home position
  ok &= hps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                        Home Position and Mode                         //
///////////////////////////////////////////////////////////////////////////

//= Move camera to standard position with standard imaging mode.

int jhcAxisPTZ::Reset () const
{
  int ok = 1;

  ok &= SetMode(brite, back, iris, focus);
  ok &= SetPTZ(p0, t0, z0);
  return ok;
}


//= Request certain automatic camera controls (0 for manual).

int jhcAxisPTZ::SetMode (int brite, int back, int iris, int focus) const
{
  char fcn[1000];
  int b = ROUND(brite * 100.0); 

  sprintf_s(fcn, "curl -u %s -d \"brightness=%d&autoiris=%s&backlight=%s&autofocus=%s\" http://%s/axis-cgi/com/ptz.cgi", 
            upwd, 
            __max(1, __min(b, 9999)),
            ((back > 0) ? "on" : "off"), ((iris > 0) ? "on" : "off"), ((focus > 0) ? "on" : "off"), 
            ip);

  if (system(fcn) != 0)
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Set pan and tilt angles of camera and (optionally) zoom number.

int jhcAxisPTZ::SetPTZ (double pan, double tilt, int zoom) const
{
  char fcn[1000];

  if (zoom > 0)
    sprintf_s(fcn, "curl -u %s -d \"pan=%f&tilt=%f&zoom=%d\" http://%s/axis-cgi/com/ptz.cgi", 
              upwd, pan, tilt, zoom, ip);
  else
    sprintf_s(fcn, "curl -u %s -d \"pan=%f&tilt=%f\" http://%s/axis-cgi/com/ptz.cgi", 
              upwd, pan, tilt, ip);
  if (system(fcn) != 0)
    return 0;
  return 1;
}


//= Change pan and tilt angles by a certain amount.

int jhcAxisPTZ::Shift (double dp, double dt) const
{
  char fcn[1000];

  sprintf_s(fcn, "curl -u %s -d \"rpan=%f&rtilt=%f\" http://%s/axis-cgi/com/ptz.cgi", 
            upwd, dp, dt, ip);
  if (system(fcn) != 0)
    return 0;
  return 1;
}


//= Zoom the camera by some factor at the current pointing angle.

int jhcAxisPTZ::Zoom (double sc) const
{
  char fcn[1000];

  sprintf_s(fcn, "curl -u %s -d \"areazoom=640,360,%d\" http://%s/axis-cgi/com/ptz.cgi", 
            upwd, ROUND(100.0 * sc), ip);
  if (system(fcn) != 0)
    return 0;
  return 1;
}


//= Aim the camera so that current coordinates (x y) are in the center of the image.
// can also scale the image by some factor

int jhcAxisPTZ::Center (int x, int y, double sc) const
{
  char fcn[1000];

  if (sc == 1.0)
    sprintf_s(fcn, "curl -u %s -d \"center=%d,%d\" http://%s/axis-cgi/com/ptz.cgi", 
              upwd, x, y, ip);
  else
    sprintf_s(fcn, "curl -u %s -d \"areazoom=%d,%d,%d\" http://%s/axis-cgi/com/ptz.cgi", 
              upwd, x, y, ROUND(100.0 * sc), ip);
  if (system(fcn) != 0)
    return 0;
  return 1;
}


//= Read out the current pan and tilt angles as well as the zoom number.

int jhcAxisPTZ::GetPTZ (double& pan, double& tilt, int& zoom) const
{
  char fcn[1000], line[80] = "curl.txt";
  FILE *in;
  int ok = 0;

  // get response from camera
  sprintf_s(fcn, "curl -u %s -d \"query=position\" http://%s/axis-cgi/com/ptz.cgi > %s", 
            upwd, ip, line);
  if (system(fcn) != 0)
    return 0;

  // parse response to recover values
  if (fopen_s(&in, line, "r") != 0)
    return 0;
  while (fgets(line, 80, in) != NULL)
  {
    // look for lines beginning with given tags
    if (strncmp(line, "pan=", 4) == 0)
      ok += sscanf_s(line + 4, "%lf", &pan);
    else if (strncmp(line, "tilt=", 5) == 0)
      ok += sscanf_s(line + 5, "%lf", &tilt);
    else if (strncmp(line, "zoom=", 5) == 0)
      ok += sscanf_s(line + 5, "%d", &zoom);
  }
  fclose(in);  
  return((ok == 3) ? 1 : 0);
}

