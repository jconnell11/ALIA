// jhcPlainFloor.cpp : makes synthetic depth image for an untextured floor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2026 Etaoin Systems
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

#include <math.h>

#include "Interface/jhcMessage.h"      // common video

#include "Environ/jhcPlainFloor.h"

#include "Interface/jtimer.h"
///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcPlainFloor::~jhcPlainFloor ()
{
jtimer_rpt();
}


//= Default constructor initializes certain values.

jhcPlainFloor::jhcPlainFloor ()
{
  // max blobs in image
  boxes.SetSize(1000);

  // region finding 
  sc   =  4.0;               // Colored edge scaling
  msc  =  4.0;               // Monochrome edge scaling
  eth  = 50;                 // Edge threshold
  bd   =  3;                 // Border blanking
  blk  = 20;                 // Dark area threshold
  smin = 30;                 // Min separator area
  rsm  =  3;                 // Region smoothing

  // depth inference 
  amin  = 50;                // Min object area
  claim =  6;                // Blob expansion amt
  bot   = 10;                // Nearness to bottom
  frac  =  0.3;              // Floor blob wrt max
  gap   = 50;                // Max sep to cliff
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Configure system for camera input of some size.
// assumes fx = fy and optical center is middle of image

void jhcPlainFloor::Init (double f, int w, int h)
{
  // configure internal images
  boost.SetSize(w, h, 3);
  comps.SetSize(w, h, 2);
  seeds.SetSize(comps);
  mono.SetSize(w, h, 1);
  tmp.SetSize(mono);
  tmp2.SetSize(mono);
  edge.SetSize(mono);
  sep.SetSize(mono);
  reg.SetSize(mono);
  floor.SetSize(mono);

  // remember focal length and image size
  flen = f;
  iw = w;
  ih = h;
jtimer_clr();
}


//= Creates synthetic depth image registered to color image based on camera pose.
// h is camera height in inches, t is tilt is in degs, assumes no roll 
// d16 pixels are 16 bit offsets from image plane in 0.25mm steps (like Kinect)

int jhcPlainFloor::Range (jhcImg& d16, const jhcImg& rgb, double h, double t)
{
  if (!mono.SameSize(rgb, 3) || !mono.SameSize(d16, 2))
    return Fatal("Bad images to jhcPlainFloor::Range");

jtimer(1, "Range");

  // cache image parameters
  ht = h;
  tilt = t;

  // find non-textured regions and choose some as floor
jtimer(2, "combo_edges");
  combo_edges(edge, mono, rgb);
jtimer_x(2);
jtimer(3, "bland_areas");                       
  bland_areas(reg, edge, mono);
jtimer_x(3);
jtimer(4, "pick_floor");                    
  pick_floor(floor, comps, reg);                      
jtimer_x(4);

  // convert floor and obstacles to frontal depth image
jtimer(5, "ground_d16");
  ground_d16(d16, floor);                  
jtimer_x(5);
jtimer(6, "obst_scan");
  obst_scan(d16, comps, floor);     
jtimer_x(6);

jtimer_x(1);
  return 1;
}


//= Create an version of the input with a green floor region.

int jhcPlainFloor::Grass (jhcImg& dest)
{
  int horiz = (int)(0.5 * (ih - 1) - flen * tan(D2R * tilt)), dev = 80;

  if (!boost.SameFormat(dest))
    return Fatal("Bad images to jhcPlainFloor::Grass");
  Threshold(tmp, floor, 128, dev);
  ClipDiff(tmp2, mono, tmp);
  ClipSum(tmp, mono, tmp);
  MergeRGB(dest, tmp2, tmp, tmp2);
  DrawLine(dest, 0, horiz, iw, horiz, 1, 0, 255, 255);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                     Segmentation and Estimation                       //
///////////////////////////////////////////////////////////////////////////

//= Combine edge maps from individual channels with grayscale edges.
// also fills internal image "boost" 

void jhcPlainFloor::combo_edges (jhcImg& sob, jhcImg& bw, const jhcImg& rgb)
{
  ForceMono(bw, rgb);               
  SobelEdge(sob, bw, msc);
  MaxColor(boost, rgb);
  SobelMaxRGB(sob, boost, sc, 0);
}


//= Find regions between cleaned-up edges.
// also fills internal image "sep" 

void jhcPlainFloor::bland_areas (jhcImg& proto, const jhcImg& sob, const jhcImg& bw)
{
  // add very dark regions to edges
  Threshold(sep, sob, eth);             
  Border(sep, -bd);
  OverGate(sep, sep, bw, blk, 255);

  // clean up to get homogeneous regions
  BoxAvg(tmp, sep, rsm);
  RemSmall(proto, tmp, 0.0, smin, 80);
  BoxThresh(proto, proto, rsm, 80, 0, 255);    
}


//= Choose biggest non-textured region(s) near the bottom to comprise the floor.
// also fills internal imagew "seeds", sets "boxes" data 

void jhcPlainFloor::pick_floor (jhcImg& gnd, jhcImg& cc, const jhcImg& proto)
{
  int big;

  // find components
  CComps4(seeds, proto, amin);
  Expand(cc, seeds, claim);                  

  // keep only areas touching bottom of image
  boxes.FindBBox(cc);
  boxes.YBotThresh(-bot);

  // keep only biggest components to form floor
  big = boxes.Biggest();
  boxes.AreaThresh(ROUND(frac * boxes.PixelCnt(big)));
  boxes.MarkOver(gnd, cc);
}


//= Assign range values to floor pixels based on camera height and tilt.
// same image plane offset for all floor pixels on a particular scan line
//   dz = -ht * flen / (iy * cos(tilt) + flen * sin(tilt)) 
// depth values are in 0.25 mm steps (ht is in inches)
// only considers image rays that are below horizontal

void jhcPlainFloor::ground_d16 (jhcImg& d16, const jhcImg& gnd) const
{
  double nhf = -101.6 * ht * flen, mid = 0.5 * (ih - 1), iy = -mid;
  double trads = D2R * tilt, ct = cos(trads), fst = flen * sin(trads);
  int x, y, flat = (int)(mid - flen * tan(trads)) - 1, ylim = __min(flat, ih);
  US16 dz;
  US16 *d = (US16 *) d16.PxlDest();
  const UC8 *g = gnd.PxlSrc();

  d16.FillArr(0xFF);                             // default = very far (53') 
  for (y = 0; y < ylim; y++, iy += 1.0)            
  {
    dz = (US16)(nhf / (ct * iy + fst));
    for (x = 0; x < iw; x++, g++, d++)           
      if (*g != 0)
        *d = dz;
  }     
}


//= Assume all components at boundary of floor region rise vertically straight up.
// at last valid floor pixel with known (ix iy) and d:
//   wx0 = d0 * ix0 / f
//   wy0 = d0 * (f * ct - iy0 * st) / f
// scan in world up-direction until exit of first component above boundary
// successively increment iy and save new d at corresponding new (ix iy):
//   ix = (wx0 / wy0) * (iy * st - f * ct)
//   d  =     f * wy0 / (iy * st - f * ct)
// substituting:
//   ix = [ix0 / (f * ct - iy0 * st)] * (iy * st - f * ct)
//   d  =  [d0 * (f * ct - iy0 * st)] / (iy * st - f * ct)

void jhcPlainFloor::obst_scan (jhcImg& d16, const jhcImg& cc, const jhcImg& gnd) const
{
  double trads = D2R * tilt, st = sin(trads), fct = flen * cos(trads);
  double mix, df0, xf0, xmid = 0.5 * (iw - 1), ymid = 0.5 * (ih - 1);
  int flat = (int)(ymid - flen * tan(trads)) - 1, ylim = __min(flat, ih - 1);
  int x, y, sx, sy, lab, obj, miss;
  const UC8 *g = gnd.PxlSrc();

  // scan up from bottom for pixels on boundary of floor region
  for (y = 0; y < ylim; y++)
    for (x = 0; x < iw; x++, g++)
      if ((*g != 0) && (*(g + iw) == 0))         // not floor above
      {
        // compute coefficients for new ix and d based on new iy
        mix = fct - (y - ymid) * st;
        xf0 = (x - xmid) / mix;
        df0 = d16.ARef16(x, y) * mix;

        // start generating streak angled upwards from boundary
        obj = 0;
        miss = 0;
        for (sy = y + 1; sy < ih; sy++)
        {
          // check that streak has not left image and is not in floor
          mix = fct - (sy - ymid) * st;
          sx = ROUND(xf0 * mix + xmid);
          if ((sx < 0) || (sx >= iw))          
            break;
          if (gnd.ARef(sx, sy) != 0)
            break;

          // make sure still in original non-floor component
          lab = cc.ARef16(sx, sy);          
          if (obj == 0)                          // not started yet
          {
            if ((lab == 0) && (++miss > gap))    // too high above
              break;
            obj = lab;                           // remember component
          }
          else if (lab != obj)                   // exited component
            break;

          // record depth for vertical face (only if valid object)
          if (lab != 0)
            d16.ASet16(sx, sy, (US16) ROUND(df0 / mix));
          }
        }
}

