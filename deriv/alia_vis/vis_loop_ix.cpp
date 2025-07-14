// vis_loop.cpp : simple test rig for vision-enabled ALIA reasoner DLL
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Etaoin Systems
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

#include "Data/jhcImg.h"               // common video
#include "Data/jhcImgIO.h"
#include "Interface/jms_x.h"           

#include "API/alia_vis.h"              // common audio


//= Main entry point for command line program.
// NOTE: needs ffind_ocv and freco_nkr built with installed version of OpenCV 

int main (int argc, char *argv[])
{
  char fname[80], scene[80] = "environ/blocks_t512";
  jhcImgIO jio;
  jhcImg col, rng, view, map;

  // load input color and depth images
  sprintf_s(fname, "%s.bmp", scene);
  if (jio.LoadResize(col, fname) <= 0)
    printf("Could not load color image: %s !\n", fname);
  sprintf_s(fname, "%s_z.ras", scene);
  if (jio.LoadResize(rng, fname) <= 0)
    printf("Could not load depth image: %s !\n", fname);

  // set static sensor pose (for "blocks_t512.bmp")
  alia_cx = alia_rx =   0.3f;
  alia_cy = alia_ry =   9.2f;
  alia_cz = alia_rz =  53.6f;
  alia_cp = alia_rp =  90.0f;
  alia_ct = alia_rt = -51.2f;
  alia_cr = alia_rr =   0.0f;  

  // start ALIA (clears image variables)
  alia_reset(NULL, "Harry", "vis_loop");

  // output images
  view.SetSize(640, 480, 3);
  map.SetSize(alia_wmap(), alia_hmap(), 3);

  // bind input and output pixel buffers
  alia_col  = col.PxlSrc();
  alia_rng  = rng.PxlSrc();
  alia_view = view.PxlDest();
  alia_map  = map.PxlDest();
  alia_vfmt = 1;
  alia_mfmt = 1;  

  while (1)
  {
    // mark input images as newly received
    alia_cfmt = 1;
    alia_rfmt = 1;

    // perform heavy duty processing
    if (alia_think() <= 0)
      break;

    // set pacing
    jms_sleep(20);
  }

  // cleanup
  alia_done(0);
  return 0;
}
