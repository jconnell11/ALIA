// freco_test.cpp : simple test of face recognition DLL on images
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

#include "Data/jhcImg.h"               // common video
#include "Data/jhcImgIO.h"
#include "Interface/jms_x.h"

#include "Face/freco_nkr.h"


///////////////////////////////////////////////////////////////////////////

//= Main entry point for command line program.
// argument 0 = CPU, 1 = GPU if available (default)

int main (int argc, char *argv[])
{
  jhcImgIO jio;
  jhcImg s1, s17, s47, s69;
  double f1[256], f17[256], f47[256], f69[256]; 
  FILE *out;
  UL32 t0;
  int rc, gpu = ((argc > 1) ? atoi(argv[1]) : 1), i = 0;

  // announce program 
  printf("\n");
  printf("freco_test version %4.2f -- jconnell@alum.mit.edu\n", 3.00);
  printf("Computes DNN features for images in \"samples\" then compares them\n\n");

  // configure DNN
  rc = freco_setup(NULL, gpu);
  if (rc <= 0)
    return printf("DNN initialization FAILED!\n");
  freco_start();

  // load test images
  if (jio.LoadResize(s1,  "samples/image_0001.bmp") > 0)
    i++;
  if (jio.LoadResize(s17, "samples/image_0017.bmp") > 0)
    i++;
  if (jio.LoadResize(s47, "samples/image_0047.bmp") > 0)
    i++;
  if (jio.LoadResize(s69, "samples/image_0069.bmp") > 0)
    i++;
  if (i < 4)
    return printf("Only read %d of 4 test images!\n", i);

  // get feature vectors given face bounding boxes
  printf("\nComputing 100 feature vectors (takes a while) ... ");
  fflush(stdout);
  t0 = jms_now();
  for (i = 0; i < 25; i++)
  {
    freco_vect( f1,  s1.PxlSrc(),  s1.XDim(),  s1.YDim(), 458, 756,  74, 422);   // 0001
    freco_vect(f17, s17.PxlSrc(), s17.XDim(), s17.YDim(), 317, 584,  97, 408);   // 0017
    freco_vect(f47, s47.PxlSrc(), s47.XDim(), s47.YDim(), 316, 590, 113, 415);   // 0047
    freco_vect(f69, s69.PxlSrc(), s69.XDim(), s69.YDim(), 316, 565, 109, 428);   // 0069
  }
  printf("%3.1f ms avg %s\n\n", 10.0 * jms_elapsed(t0), ((rc >= 2) ? "- GPU" : ""));

  // dump feature vector for image 0001
  printf("Writing feature vector values to: test_0001.vals ...\n");
  fopen_s(&out, "test_0001.vals", "w");
  for (i = 0; i < 256; i++)
  {
    if (i == 0)
      printf("  %10.6f [  2.542674]\n", f1[i]);
    else if (i == 1)
      printf("  %10.6f [ 18.435728]\n", f1[i]);
    else if (i == 2)
      printf("  %10.6f [ 17.857012]\n", f1[i]);
    else if (i == 3)
      printf("  %10.6f [-12.279771]\n", f1[i]);
    fprintf(out, "%10.6f\n", f1[i]);
  }
  fclose(out);

  // report image-to-image matching vs expected values
  printf("\nCross-matching:\n");
  printf("  0001 0017 -> %8.6f [0.929553]\n", 1.0 - freco_dist( f1, f17));
  printf("  0001 0047 -> %8.6f [0.629113]\n", 1.0 - freco_dist( f1, f47));
  printf("  0001 0069 -> %8.6f [0.616572]\n", 1.0 - freco_dist( f1, f69));
  printf("\n");
  printf("  0017 0047 -> %8.6f [0.625981]\n", 1.0 - freco_dist(f17, f47));
  printf("  0017 0069 -> %8.6f [0.648003]\n", 1.0 - freco_dist(f17, f69));
  printf("\n");
  printf("  0047 0069 -> %8.6f [0.666409]\n", 1.0 - freco_dist(f47, f69));
  printf("\n");

  // cleanup
  freco_done();
  freco_cleanup();
  return 0;
}