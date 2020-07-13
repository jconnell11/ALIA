// jhcDisplay.h : some display routines specific to Windows
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2020 IBM Corporation
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

#ifndef _JHCDISPLAY_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDISPLAY_
/* CPPDOC_END_EXCLUDE */

// order of includes is important
#include "jhcGlobal.h"

#include "StdAfx.h"                          
#include <VfW.h>

#include "Data/jhcArr.h"
#include "Data/jhcImg.h"


///////////////////////////////////////////////////////////////////////////

//= Some display routines specific to Windows.
// Beware dangling pointers: constructor takes reference to Document or View
// Retains internal pointer to this object (so don't delete it externally)
 
class jhcDisplay
{
// PRIVATE MEMBER VARIABLES
private:
  // window creation and resizing
  CWnd *win;
  UL32 bgcol;
  LONG_PTR style;
  WINDOWPLACEMENT place; 
  int full;

  // graphics and drawing
  HDRAWDIB hdd;
  LPBITMAPINFOHEADER ctab_hdrs[5];
  RGBQUAD *ctables[4];

  // image conversion and placement
  DWORD tdisp;
  jhcImg tmp;
  int imgx, imgy, imgw, imgh, gcnt;


// PUBLIC MEMBER VARIABLES
public:
  // could package into jhcParam structures for easier modification
  int offx;    /** Left border on screen (in pixels).            */
  int offy;    /** Top border on screen (in pixels).             */
  int bdx;     /** Horizontal space between columns (in pixels). */
  int bdy;     /** Vertical space between rows (in pixels).      */

  int cw;      /** Width of display items in columns.        */
  int rh;      /** Height of display items in rows.          */
  int row;     /** Number of rows for automatic placement.   */
  int n;       /** Next item number for automatic placement. */

  int gwid;    /** Width to make graphs (in pixels).                  */
  int ght;     /** Height to make graphs (in pixels).                 */
  int square;  /** Whether images should be shown with square pixels. */

  int mx;      /** Last mouse X from any mouse function.      */
  int my;      /** Last mouse Y from any mouse function.      */
  int mbut;    /** Last mouse button from any mouse function. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and destruction
  ~jhcDisplay ();
  jhcDisplay ();
  jhcDisplay (CWnd *w);
  jhcDisplay (CDocument *d);
  jhcDisplay (CDC *ctx);
  void BindTo (CWnd *w, UL32 bg =0x00FFFFFF);
  void BindTo(CDocument *d, UL32 bg =0x00FFFFFF);
  void BindTo (CDC *ctx, UL32 bg =0x00FFFFFF);
  void Background (int r =255, int g =255, int b =255);
  int Valid () const {if (win == NULL) return 0; return 1;};  /** Whether ready to display images. */

  // full screen 
  int ScreenDims (int& w, int& h) const;
  void FullSize (jhcImg& dest, int f =1);
  int FullScreen (int doit =2, int w =0, int h =0);
  int Scrape (jhcImg& dest, int left =0, int top =0);

  // meta-display functions
  void StatusText (const char *msg, ...);
  void Pace (int ms, int grid =0);

  // layout and erasing
  void Clear (int flush =1, const char *status =NULL);
  int ClearGrid (int i, int j, int txt =0);
  int ClearRange (int i0, int j0, int i1, int j1);
  int ClearNext ();
  int GridX (int i, int wdef =0);
  int GridY (int j, int hdef =0);
  void SetGrid (const jhcImg& ref);
  void ResetGrid (int across =3, int w =0, int h =0);
  void Skip (int i =1) {n += i;}    /** Advance over one or more display positions on screen. */
  void Backup (int i =1) {n -= i;}  /** Backup one or more display positions on screen.       */

  // image positioning
  int NextI () const {return(n % row);}  /** Next panel horizontal index. */
  int NextJ () const {return(n / row);}  /** Next panel vertical index.   */
  int AdjX (int n =1) const;
  int AdjY () const;
  int BelowX () const;
  int BelowY (int n =1) const;

  // image rendering
  int Show (const jhcImg& src, int x =0, int y =0, 
            int mode =0, const char *title =NULL, ...);
  int ShowGrid (const jhcImg& src, int i =0, int j =0, 
                int mode =0, const char *title =NULL, ...);
  int ShowGrid (const jhcImg *src, int i =0, int j =0, 
                int mode =0, const char *title =NULL, ...);
  int ShowNext (const jhcImg& src, int mode =0, const char *title =NULL, ...);
  int ShowAdj (const jhcImg& src, int mode =0, const char *title =NULL, ...);
  int ShowBelow (const jhcImg& src, int mode =0, const char *title =NULL, ...);
  
  // graph rendering
  int Graph (const jhcArr& h, int x =0, int y =0, int maxval =0, 
             int col =0, const char *title =NULL, ...);
  int GraphV (const jhcArr& h, int x =0, int y =0, int maxval =0, 
              int col =0, const char *title =NULL, ...);
  int GraphGrid (const jhcArr& h, int i =0, int j =0, int maxval =0, 
                 int col =0, const char *title =NULL, ...);
  int GraphGrid (const jhcArr *h, int i =0, int j =0, int maxval =0, 
                 int col =0, const char *title =NULL, ...);
  int GraphGridV (const jhcArr& h, int i =0, int j =0, int maxval =0, 
                  int col =0, const char *title =NULL, ...);
  int GraphGridV (const jhcArr *h, int i =0, int j =0, int maxval =0, 
                  int col =0, const char *title =NULL, ...);
  int GraphNext (const jhcArr& h, int maxval =0, 
                 int col =0, const char *title =NULL, ...);
  int GraphNext (const jhcArr *h, int maxval =0, 
                 int col =0, const char *title =NULL, ...);
  int GraphAdj (const jhcArr &h, int maxval =0, 
                int col =0, const char *title =NULL, ...);
  int GraphAdjV (const jhcArr &h, int maxval =0, 
                 int col =0, const char *title =NULL, ...);
  int GraphBelow (const jhcArr &h, int maxval =0, 
                  int col =0, const char *title =NULL, ...);
  int GraphOver (const jhcArr& h, int maxval =0, int col =0)
    {return Graph(h, imgx, imgy, maxval, -col);}               /** Draws another trace on last graph. */
  int GraphOver (const jhcArr *h, int maxval =0, int col =0) 
    {return GraphOver(*h, maxval, col);}                       /** Like other GraphOver but de-references pointer. */
  int GraphOverV (const jhcArr& h, int maxval, int col =0)
    {return GraphV(h, imgx, imgy, maxval, -col);}              /** Draws another trace on last vertical graph. */
  int GraphMark (double bin, int col =1, double ht =1.0);
  int GraphMarkV (double bin, int col =1, double ht =1.0);
  int GraphVal (int lvl, int maxval, int col =0);
  int GraphValV (int lvl, int maxval, int col =0);
  
  // on-screen text
  int String (int x, int y, const char *msg, ...);
  int StringGrid (int i, int j, const char *msg, ...);
  int StringBelow (const char *msg, ...);
  int StringNext (const char *msg, ...);
  int StringAdj (const char *msg, ...);
  int StringOff (int dx, int dy, const char *msg, ...);

  // mouse and key functions
  void FlushMsg ();
  int AnyHit ();
  int WaitHit (int ms);
  int PaceOrHit (int ms, int grid =3);    // grid default was 0
  int LoopHit (int ms, int grid =3, int strict =0, char *msg =NULL);

  // mouse clicks
  int ClickAny (int *x =NULL, int *y =NULL);
  int ClickR (int *x =NULL, int *y =NULL);
  int ClickL (int *x =NULL, int *y =NULL);
  int ClickWait (int *x =NULL, int *y =NULL);
  int ClickRel (int *x, int *y, 
                int lx =-1, int ly =-1, int cw =-1, int ch =-1); 
  int DownWait (int *x =NULL, int *y =NULL, int rel =0);

  // mouse tracking
  int MousePos (int *x, int *y, int block =1);
  int MouseRel (int *x, int *y, 
                int lx =-1, int ly =-1, int cw =-1, int ch =-1); 
  int MouseRel (int *x, int *y, int specs[]);
  int MouseRel0 (int& x, int& y);
  int MouseRel2 (int& x, int& y);
  int MouseExit (int code);
  int MouseNotL (int code);

  // mouse selection
  int MouseBox (int *x, int *y, int *w, int *h, 
                int lx =-1, int ly =-1, int cw =-1, int ch =-1);                  
  int MouseBox (int region[], 
                int lx =-1, int ly =-1, int cw =-1, int ch =-1); 
  int MouseBox (int region[], int specs[]); 


// PRIVATE MEMBER FUNCTIONS
private:
  void init_vals ();

  // low level graphics
  UL32 color_n (int col) const;
  int clear_rect (int x, int y, int w, int h, int wht =0);
  int label (int x, int y, int w, const char *msg);
  int frame (int x, int y, int w, int h);

  // coordinate conversion
  int img_coords (int *xi, int *yi, int xs, int ys,  
                  int lx =-1, int ly =-1, int cw =-1, int ch =-1) const; 
  void scr_coords (int *xs, int *ys, int xi, int yi, 
                   int lx =-1, int ly =-1, int ch =-1) const; 

  // color map generators
  void tables ();
  void equalize (RGBQUAD *ctable, const jhcImg& src);
  void linear (RGBQUAD *ctable);
  void bands (RGBQUAD *ctable);
  void pseudo (RGBQUAD *ctable);

  // low-level mouse functions
  void handle_others (MSG *msg);
  int peek_no_menu (MSG *msg);

};


#endif





