// ffind_test.cpp : simple test of face finder DLL on an image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 IBM Corporation
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
#include <string.h>

#include "Data/jhcImg.h"
#include "Data/jhcImgIO.h"
#include "Data/jhcRoi.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcLabel.h"

#include "Face/ffind_ocv.h"


///////////////////////////////////////////////////////////////////////////

//= Explanation of what function does.

int usage ()
{
  printf("Usage:\n");
  printf("  ffind_test image.jpg\n");
  printf("where:\n");
  printf("  image.jpg = image to analyze\n");
  printf("\n");
  return 1;
}


//= Main entry point for command line program.

int main (int argc, char *argv[])
{
  jhcImg img;
  jhcImgIO jio;
  jhcDraw dr;
  jhcLabel lab;
  jhcRoi box;
  char fname[80] = "[no faces]";
  char *end;
  int i, n, x, y, w, h;

  // announce program
  printf("\n");
  printf("ffind_test version %4.2f -- jconnell@alum.mit.edu\n", 1.35);
  printf("Finds faces in image X and puts boxes around them in image X_faces\n\n");

  // parse arguments
  if ((argc <= 1) || ((argc > 1) && (strcmp(argv[1], "?") == 0)))
    return usage();
  if (jio.LoadResize(img, argv[1]) <= 0)
    return printf("Could not read image: %s !", argv[1]);
  printf("Read (%d %d) image: %s\n", img.XDim(), img.YDim(), argv[1]);

  // find faces in image
  ffind_setup(NULL);
  ffind_start();
  n = ffind_run(img.PxlDest(), img.XDim(), img.YDim(), img.Fields());

  // report results
  for (i = 0; i < n; i++)
  {
    // mark faces
    ffind_box(x, y, w, h, i);
    box.SetRoi(x, y, w, h);
    dr.RectEmpty(img, box, 3, -2);
    lab.LabelBox(img, box, i + 1, -16, -2);
  }

  // save modified image
  if (n > 0)
  {
    strcpy_s(fname, argv[1]);
    if ((end = strrchr(fname, '.')) != NULL)
      *end = '\0';
    strcat_s(fname, "_faces.bmp");
    jio.Save(fname, img);
  }

  // cleanup
  printf("%d face%s found -> %s\n", n, ((n == 1) ? "" : "s"), 
         ((n <= 0) ? "[no face image]" : fname));
  ffind_done();
  ffind_cleanup();  
  return 0;
}
