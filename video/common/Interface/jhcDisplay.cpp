// jhcDisplay.cpp : code to interface to Windows graphics
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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

// Build.Settings.Link.General must have library winmm.lib and vfw32.lib 
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "vfw32.lib")


#include <math.h>
#include <stdarg.h>

#include "Interface/jhcMessage.h"
#include "Interface/jhcString.h"
#include "Interface/jms_x.h"

#include "Interface/jhcDisplay.h"


//////////////////////////////////////////////////////////////////////////////
//                      Creator and destructor                              //
//////////////////////////////////////////////////////////////////////////////

//= Clean up things.

jhcDisplay::~jhcDisplay ()
{
  int i;

  // flush the output queue 
  if (hdd != NULL)
  {
    DrawDibEnd(hdd);
    DrawDibClose(hdd);
  }

  // free allocated color tables and headers
  for (i = 4; i >= 0; i--)
    delete [] ((char *) ctab_hdrs[i]);
}


//= Build basic structure -- not valid for use until BindTo called.

jhcDisplay::jhcDisplay ()
{
  init_vals();
}


//= Create based on some window (or CView).

jhcDisplay::jhcDisplay (CWnd *w)
{
  init_vals();
  BindTo(w);
}


//= Create based on some document.

jhcDisplay::jhcDisplay (CDocument *d)
{
  init_vals();
  BindTo(d);
}


//= Create based on some display context.

jhcDisplay::jhcDisplay (CDC *ctx)
{
  init_vals();
  BindTo(ctx);
}


//= Set up defaults and build color tables.

void jhcDisplay::init_vals ()
{
  win = NULL;
  hdd = NULL;
  full = 0;
  tables();

  offx = 20;
  offy = 35;
  bdx  = 20;
  bdy  = 35;

  imgx = offx;
  imgy = offy;
  imgw = 0;
  imgh = 0;
  gcnt = 0;
  gmax = 0;

  cw   = 0;
  rh   = 0;
  row  = 3;
  n    = 0;

  mx   = 0;
  my   = 0;
  mbut = 0;

  gwid = 200;
  ght  = 100;
  square = 1;

  tdisp = 0;
  bgcol = 0x00FFFFFF;
} 


//= Bind self to a certain (possibly different) window after creation.
// can provide alternate background color for drawing functions

void jhcDisplay::BindTo (CWnd *w, UL32 bg)
{
  // remember main window
  if (w == NULL)
    return;
  win = w;
  bgcol = bg;
  full = 0;

  // initialize drawing routines 
  if (hdd != NULL)
  {
    DrawDibEnd(hdd);
    DrawDibClose(hdd);
  }
  hdd = DrawDibOpen();
}


//= Same as other BindTo but takes first window under document.

void jhcDisplay::BindTo (CDocument *d, UL32 bg)
{
  POSITION viewpos = d->GetFirstViewPosition();

  BindTo(d->GetNextView(viewpos), bg);
}


//= Same as other BindTo but takes gets window associated with CDC.

void jhcDisplay::BindTo (CDC *ctx, UL32 bg)
{
  BindTo(ctx->GetWindow(), bg);
}


//= Set the standard background color for drawing operations.
// also changes color window is filled with when un-minimized
// NOTE: does not immediately repaint window with new color

void jhcDisplay::Background (int r, int g, int b)
{
  HBRUSH old, brush = CreateSolidBrush(RGB(r, g, b));

  bgcol = ((b & 0xFF) << 16) | ((g & 0xFF) << 8) | (r & 0xFF);
  old = (HBRUSH) SetClassLongPtr(win->m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR) brush);
  DeleteObject(old);
}


//////////////////////////////////////////////////////////////////////////////
//                         Full Screen Functions                            //
//////////////////////////////////////////////////////////////////////////////

//= Determine full pixel extent of current display (not application window).
// better than GetSystemMetrics(SM_CXSCREEN) for multiple monitor computers

int jhcDisplay::ScreenDims (int& w, int& h) const
{
  HWND hwnd = GetActiveWindow();
  MONITORINFO mi;
  RECT *r;

  // get monitor information
  mi.cbSize = sizeof(MONITORINFO);
  if (!GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
    return 0;
  r = &(mi.rcMonitor);

  // parse out the useful bits (generally: left = top = 0)
  w = r->right - r->left;
  h = r->bottom - r->top;
  return 1;
}


//= Set image to be the same dimensions as the full display.

void jhcDisplay::FullSize (jhcImg& dest, int f)
{
  int w, h;

  ScreenDims(w, h);
  dest.SetSize(w, h, f);
}


//= Expand current window to completely cover screen (or shrink back to normal).
// suppresses margins, menu bar, status bar, and possibly cursor
// can provide specific desired width and height but corner always at NW = (0 0)
// doit: 0 = normal window with cursor, 1 = full screen with cursor, 2 = no cursor

int jhcDisplay::FullScreen (int doit, int w, int h)
{
  HWND hwnd = GetActiveWindow();
  CFrameWnd *frame;
  CWnd *status;
  int dx, dy, fw = w, fh = h;

  // see if already in correct state
  if (((doit > 0) && (full > 0)) || ((doit <= 0) && (full <= 0)))
    return 1;

  // get main frame window and status bar at bottom
  if (win == NULL)
    return 0;
  if ((frame = win->GetParentFrame()) == NULL)
    return 0;
  if ((status = frame->GetMessageBar()) == NULL)
    return 0;

  if (doit <= 0)
  {
    // restore top menu bar and bottom status bar
    frame->SetMenuBarState(AFX_MBS_VISIBLE);
    status->ShowWindow(SW_SHOW);

    // restore frame window style, size, and position
    SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(hwnd, &place);
    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    if (full > 1)
      ShowCursor(true);
    full = 0;
    return 1;
  }

  // save old window style and position
  style = GetWindowLongPtr(hwnd, GWL_STYLE);
  place.length = sizeof(WINDOWPLACEMENT);
  if (!GetWindowPlacement(hwnd, &place))
    return 0;

  // hide menu and status bars, background is black
  frame->SetMenuBarState(AFX_MBS_HIDDEN);
  status->ShowWindow(SW_HIDE);

  // determine desired full screen size
  if (w <= 0)
    ScreenDims(fw, fh);
  dx = GetSystemMetrics(SM_CXEDGE);
  dy = GetSystemMetrics(SM_CYEDGE);

  // expand window to target size (no borders)
  SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
  SetWindowPos(hwnd, HWND_TOPMOST, -dx, -dy, fw + 2 * dx, fh + 2 * dy, SWP_FRAMECHANGED);
  full = 1;

  // possibly suppress cursor
  if (doit > 1)
  {
    ShowCursor(false);
    full = 2;
  }
  return 1;
}


//= Extract some portion of screen as drawn into an image.
// size determined by passed image, upper left corner at (cx cy)

int jhcDisplay::Scrape (jhcImg& dest, int left, int top)
{
  LPBITMAPINFOHEADER hdr = ctab_hdrs[4];
  jhcString tag("DISPLAY");
  RECT rect;
  HDC scrn_dc, copy_dc;
  HBITMAP trash_bmap, dest_bmap;
  int cx, cy, w = dest.XDim(), h = dest.YDim();

  if (!dest.Valid(3))
    return Fatal("Bad image to jhcDisplay::Scrape");

  // get display context for whole screen then make a copy and a new bitmap
  scrn_dc   = CreateDC(tag.Txt(), NULL, NULL, NULL); 
  copy_dc   = CreateCompatibleDC(scrn_dc); 
  dest_bmap = CreateCompatibleBitmap(scrn_dc, w, h);

  // swap in destination bitmap and copy to it
  trash_bmap = (HBITMAP) SelectObject(copy_dc, dest_bmap);
  GetWindowRect(win->m_hWnd, &rect);
  cx = rect.left + left + 2;            // beveled edges
  cy = rect.top + top + 2;
  BitBlt(copy_dc, 0, 0, w, h, scrn_dc, cx, cy, SRCCOPY); 

  // swap out the filled bitmap and convert it to a DIB
  dest_bmap = (HBITMAP) SelectObject(copy_dc, trash_bmap); 
  hdr->biWidth = w;
  hdr->biHeight = h;
  GetDIBits(scrn_dc, dest_bmap, 0, h, dest.PxlDest(), (LPBITMAPINFO) hdr, DIB_RGB_COLORS);

  // clean up
  if (dest_bmap != 0)
    DeleteObject(dest_bmap);
  if (copy_dc != 0)
    DeleteDC(copy_dc); 
  if (scrn_dc != 0)
    DeleteDC(scrn_dc); 
  return 1;
}


//////////////////////////////////////////////////////////////////////////////
//                         Meta-Display Functions                           //
//////////////////////////////////////////////////////////////////////////////

//= Copy string to status bar of application.

void jhcDisplay::StatusText (const char *msg, ...)
{
  va_list args;
  jhcString val;
  CFrameWnd *frame;

  va_start(args, msg); 
  vsprintf_s(val.ch, msg, args);
  val.C2W();
  if (win == NULL)
    return;
  frame = win->GetParentFrame();
  if (frame != NULL)
    frame->SetMessageText(val.Txt());
}


//= Optionally wait if displaying faster than desired rate.
// can play 1 frame out of 5 at original rate (i.e. jerky)
// call StepTime member of video source to get correct time
// can also reset image autoplacement if grid is non-zero

void jhcDisplay::Pace (int ms, int grid)
{
  DWORD tnext, tprev = tdisp;

  // possibly reset screen
  if (grid > 0)
    ResetGrid(grid, 0, 0);

  // get current time and remember
  tdisp = jms_now();
  if ((tprev == 0) || (ms <= 0))
    return;

  // if not close enough to next frame time then sleep a while
  // have to test tnext and tdisp explicitly since DWORD
  tnext = tprev + (DWORD) ms;
  if (tnext > tdisp)
  {
    jms_sleep(tnext - tdisp); 
    tdisp = jms_now();
  }
}


//////////////////////////////////////////////////////////////////////////////
//                           Layout and Erasing                             //
//////////////////////////////////////////////////////////////////////////////

//= Wipe everything off of window.
// can also flush any mouse clicks 
// and set status line at bottom of screen

void jhcDisplay::Clear (int flush, const char *status)
{
  if (win != NULL)
    win->RedrawWindow();
  n = 0;
  if (flush > 0)
    FlushMsg();
  if (status != NULL)
    StatusText(status);
  ResetGrid();
}


//= Set image spacing properly for given size of image.

void jhcDisplay::SetGrid (const jhcImg& ref)
{
  ResetGrid(3, ref.XDim(), ref.YDim());
}


//= Draw next image at upper left point of "grid" of like-sized images.
// optionally set the number of images to draw in a row before going down
// get set characteristic dimensions (w, h) for all objects to set spacing
// if some dimension given as zero, takes dimension of first thing shown 

void jhcDisplay::ResetGrid (int across, int w, int h)
{
  imgx = offx;
  imgy = offy;
  imgw = 0;
  imgh = 0;
  gcnt = 0;
  gmax = 0;
  n = 0;
  cw = 0;
  rh = 0;
  if (across > 0)
    row = across;
  if (w > 0)
    cw = w;
  if (h > 0)
    rh = h;
}


//= Clear space usually occupied by an image (but not its label).
// useful for images which are only displayed on some cycles

int jhcDisplay::ClearGrid (int i, int j, int txt)
{
  int hfont = 22;
  int x = GridX(i), y = GridY(j);

  if (txt > 0)
    return clear_rect(x, y - hfont, cw, rh);
  return clear_rect(x, y, cw, rh);
}


//= Clear several panels on screen including their label areas.

int jhcDisplay::ClearRange (int i0, int j0, int i1, int j1)
{
  int hfont = 22;
  int x0 = GridX(i0), y0 = GridY(j0) - hfont;
  int x1 = GridX(i1) + cw, y1 = GridY(j1) + rh; 

  if ((x0 >= x1) || (y0 >= y1))
    return 0;
  return clear_rect(x0, y0, x1 - x0, y1 - y0);
}


//= Clear space occupied by next image -- does NOT advance position.
// useful for clearing a region to write variable length text in

int jhcDisplay::ClearNext ()
{
  return ClearGrid(NextI(), NextJ());
}


//= Translate X integer panel coordinate into pixel displacement.
// can set spacing using wdef if currently unknown

int jhcDisplay::GridX (int i, int wdef)
{
  if ((wdef > 0) && (cw <= 0))
    cw = wdef;
  return(offx + i * (cw + bdx));
}


//= Translate Y integer panel coordinate into pixel displacement.
// can set spacing using hdef if currently unknown

int jhcDisplay::GridY (int j, int hdef)
{
  if ((hdef > 0) && (rh <= 0))
    rh = hdef;
  return(offy + j * (rh + bdy));
}


//= Spaces images and graphs tightly in rows.
// determine next image X corner based solely on last image

int jhcDisplay::AdjX (int n) const
{
  return(imgx + n * (imgw + bdx));
}


//= Spaces images and graphs tightly in rows.
// determine next image Y corner based solely on last image

int jhcDisplay::AdjY () const
{
  return imgy;
}


//= Spaces images and graphs tightly in columns.
// determine next image X corner based solely on last image

int jhcDisplay::BelowX () const
{
  return imgx;
}


//= Spaces images and graphs tightly in columns.
// determine next image Y corner based solely on last image

int jhcDisplay::BelowY (int n) const
{
  return(imgy + n * (imgh + bdy));
}


//= Get a corner position for displaying the next item relative to grid.
// use negative i for adjacent, negative j for below 

void jhcDisplay::ScreenPos (int& x, int& y, int i, int j)
{
  if (i < 0)
  {
    x = AdjX();
    y = AdjY();
  }
  else if (j < 0)
  {
    x = BelowX();
    y = BelowY();
  }
  else
  {
    x = GridX(i);
    y = GridY(j);
  }
}


//////////////////////////////////////////////////////////////////////////////
//                             Render Images                                //
//////////////////////////////////////////////////////////////////////////////

//= Show the image on a window given upper left corner (in pixels).
// coerces 2 and 4 byte images to monochrome using value saturation
// modes for 8 bit images: 0 = grayscale, 1 = histogram equalized
//   2 = 16 color bands, 3 = smooth pseudocolor
// can take a format string and arguments to make title bar over image

int jhcDisplay::Show (const jhcImg& src, int x, int y, int mode, const char *title, ...)
{
  va_list args;
  char msg[200];
  const jhcImg *s = &tmp;
  int w = src.XDim(), h = src.YDim(), f = src.Fields(), hsc = h;
  LPBITMAPINFOHEADER hdr;
  HDC hdc;

  // check for various problems 
  if (hdd == NULL)
    return -2;
  if (!src.Valid() || (f > 4))
    return Fatal("Bad image to jhcDisplay::Show");

  // handle 16 and 32 bit images as monochrome
  if ((f == 2) || (f == 4))
    tmp.Sat8(src);
  else
    s = &src;

  // if image is invalid, then clear space belonging to it
  if (!s->Valid() || (s->status <= 0))  // was == 0
  {
    clear_rect(x, y, __max(cw, s->XDim()), __max(rh, s->YDim()));
    label(x, y, __max(cw, s->XDim()), "");
    return 0;
  }

  // figure out which color table (if any) to use 
  if (s->Fields() != 1)
    hdr = ctab_hdrs[4];
  else 
    hdr = ctab_hdrs[max(0, min(mode, 3))];
  hdr->biWidth = w;
  hdr->biHeight = h;
  if (mode == 1)
    equalize(ctables[1], *s);
  if (square == 0)
    hsc = ROUND(h / s->Ratio());

  // draw image and clean up
  hdc = GetDC(win->m_hWnd);
  DrawDibDraw(hdd, hdc, x, y, w, hsc, hdr, (LPVOID) s->PxlSrc(),
              0, 0, w, h, DDF_BACKGROUNDPAL);
  ReleaseDC(win->m_hWnd, hdc);
  
  // draw thin black border around whole image and label across top
  frame(x, y, w, h);
  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
    label(x, y, __max(cw, s->XDim()), msg);
  }

  // record size and position for subsequent operations
  imgx = x;
  imgy = y;
  imgw = w;
  imgh = h;
  gcnt = w;
  return 1;
}


//= Show the image as one of N all having the same size.
// the position is controlled by i and j, the offset and margins default
// same choices for display modes as Show function

int jhcDisplay::ShowGrid (const jhcImg& src, int i, int j, 
                          int mode, const char *title, ...)
{
  va_list args;
  char msg[200] = "";

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return Show(src, GridX(i, src.XDim()), GridY(j, src.YDim()), mode, msg);
}


//= Standard display method but taking a pointer to an image.

int jhcDisplay::ShowGrid (const jhcImg *src, int i, int j, 
                          int mode, const char *title, ...)
{
  va_list args;
  char msg[200] = "";

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return Show(*src, GridX(i, src->XDim()), GridY(j, src->YDim()), mode, msg);
}


//= Show next image in scan order relative to the proceeding ones.
// puts "row" across then moves down (can mix graphs and images)
// size of cell in table controlled by first item displayed 

int jhcDisplay::ShowNext (const jhcImg& src, int mode, const char *title, ...)
{
  va_list args;
  char msg[200] = "";
  int i = NextI(), j = NextJ();

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  n++;
  return Show(src, GridX(i, src.XDim()), GridY(j, src.YDim()), mode, msg);
}


//= Show image right next to last (ignores row width rw).

int jhcDisplay::ShowAdj (const jhcImg& src, int mode, const char *title, ...)
{
  va_list args;
  char msg[200] = "";
  int x = AdjX(), y = AdjY();

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return Show(src, x, y, mode, msg);
}


//= Show image right below last one (ignores column height ch).

int jhcDisplay::ShowBelow (const jhcImg& src, int mode, const char *title, ...)
{
  va_list args;
  char msg[200] = "";
  int x = BelowX(), y = BelowY();

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return Show(src, x, y, mode, msg);
}


//////////////////////////////////////////////////////////////////////////////
//                             Render Graphs                                //
//////////////////////////////////////////////////////////////////////////////

//= Create an empty graph with just a title bar.
// can subsequently use GraphOver and GraphVal (use col = 8 for black)

int jhcDisplay::Graph0 (int x, int y, const char *title, ...)
{
  va_list args;
  char msg[200];

  // make border around plot and label with given string
  clear_rect(x, y, gwid, ght, 1);
  frame(x, y, gwid, ght);
  if (title != NULL) 
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
    label(x, y, __max(cw, gwid), msg);
  }

  // record size and position for subsequent operations
  imgx = x;
  imgy = y;
  imgw = gwid;
  imgh = ght;
  return 1;
}


//= Takes an array of values and draws it on the screen along x axis.
// negative maxval makes graph symmetric around zero
// change size by setting gwid and ght defaults
// to overdraw previous graph, use a negative color value
// to graph along y axis set vert positive

int jhcDisplay::Graph (const jhcArr& h, int x, int y, int maxval, 
                       int col, const char *title, ...)
{
  va_list args;
  char msg[200];
  int i, px, py, ysw = y + ght, n = h.Size();
  double vsc = 1.0, hsc = gwid / (double)(n - 1);
  int bot = __min(0, h.MinVal(1)), top = __max(0, h.MaxVal(1));
  CDC *cdc = win->GetDC();
  CPen new_pen(PS_SOLID, 1, color_n(col));
  CPen *old_pen;

  if (cdc == NULL)
    return -1;

  // if image is invalid, then clear space belonging to it
  if (h.status <= 0)
  {
    clear_rect(x, y, __max(cw, gwid), __max(rh, ght));
    label(x, y, __max(cw, gwid), "");
    return 0;
  }

  // check if symmetric or positive-only limit on graph
  if (maxval != 0)
    top = abs(maxval);
  if (maxval < 0)
    bot = maxval;
  
  // figure out vertical scaling factor and line endpoints
  if (top > bot)
    vsc = ght / (double)(top - bot);
  LPPOINT graph = new POINT[n];
  for (i = 0; i < n; i++)
  {
    px = ROUND(hsc * i);
    py = ROUND(vsc * (h.RollRef(i) - bot));      // for scrolling
    px = __max(0, __min(px, gwid));
    py = __max(0, __min(py, ght));
    graph[i].x = x + px;
    graph[i].y = ysw - py;
  }

  // clear space if not an overlay, then set trace color and draw graph 
  if (col >= 0)
    clear_rect(x, y, gwid, ght, 1);
  old_pen = cdc->GetCurrentPen();
  cdc->SelectObject(&new_pen);
  cdc->Polyline(graph, n);
  cdc->SelectObject(old_pen);

  // make border around plot and label with given string
  frame(x, y, gwid, ght);
  if ((title != NULL) && (col >= 0))
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
    label(x, y, __max(cw, gwid), msg);
  }

  // clean up
  delete [] graph;
  win->ReleaseDC(cdc);

  // record size and position for subsequent operations
  imgx = x;
  imgy = y;
  imgw = gwid;
  imgh = ght;
  gcnt = n;
  gmax = ((maxval == 0) ? top : maxval);
  return 1;
}


//= Takes an array of values and draws it on the screen along y axis.
// negative maxval makes graph symmetric around zero
// change size by setting gwid and ght defaults
// to overdraw previous graph, use a negative color value
// to graph along y axis set vert positive

int jhcDisplay::GraphV (const jhcArr& h, int x, int y, int maxval, 
                        int col, const char *title, ...)
{
  va_list args;
  char msg[200];
  int i, px, py, ysw = y + ght, n = h.Size();
  double hsc = 1.0, vsc = ght / (double)(n - 1);
  int lf = __min(0, h.MinVal(1)), rt = __max(0, h.MaxVal(1));
  CDC *cdc = win->GetDC();
  CPen new_pen(PS_SOLID, 1, color_n(col));
  CPen *old_pen;

  if (cdc == NULL)
    return -1;

  // if image is invalid, then clear space belonging to it
  if (h.status <= 0)
  {
    clear_rect(x, y, __max(cw, gwid), __max(rh, ght));
    label(x, y, __max(cw, gwid), "");
    return 0;
  }

  // check if symmetric or positive-only limit on graph
  if (maxval != 0)
    rt = abs(maxval);
  if (maxval < 0)
    lf = maxval;

  // figure out horizontal scaling factor and line endpoints
  if (rt > lf)
    hsc = gwid / (double)(rt - lf);
  LPPOINT graph = new POINT[n];
  for (i = 0; i < n; i++)
  {
    px = ROUND(hsc * (h.RollRef(i) - lf));       // for scrolling
    py = ROUND(vsc * i);
    px = __max(0, __min(px, gwid));
    py = __max(0, __min(py, ght));
    graph[i].x = x + px;
    graph[i].y = ysw - py; 
  }

  // clear space if not an overlay, then set trace color and draw graph 
  if (col >= 0)
    clear_rect(x, y, gwid, ght, 1);
  old_pen = cdc->GetCurrentPen();
  cdc->SelectObject(&new_pen);
  cdc->Polyline(graph, n);
  cdc->SelectObject(old_pen);

  // make border around plot and label with given string
  frame(x, y, gwid, ght);
  if ((title != NULL) && (col >= 0))
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
    label(x, y, __max(cw, gwid), msg);
  }

  // clean up
  delete [] graph;
  win->ReleaseDC(cdc);

  // record size and position for subsequent operations
  imgx = x;
  imgy = y;
  imgw = gwid;
  imgh = ght;
  gcnt = n;
  gmax = ((maxval == 0) ? rt : maxval);
  return 1;
}


//= Show the graph as one of N all having the same size.
// the position is controlled by i and j, the offset and margins default
// change size by setting gwid and ght defaults

int jhcDisplay::GraphGrid (const jhcArr& h, int i, int j, int maxval, 
                           int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return Graph(h, GridX(i, gwid), GridY(j, ght), maxval, col, msg);
}


//= Standard graphing method but taking a pointer to an array.

int jhcDisplay::GraphGrid (const jhcArr *h, int i, int j, int maxval, 
                           int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return Graph(*h, GridX(i, gwid), GridY(j, ght), maxval, col, msg);
}


//= Show the graph along y axis as one of N all having the same size.
// the position is controlled by i and j, the offset and margins default
// change size by setting gwid and ght defaults

int jhcDisplay::GraphGridV (const jhcArr& h, int i, int j, int maxval, 
                            int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return GraphV(h, GridX(i, gwid), GridY(j, ght), maxval, col, msg);
}


//= Standard vertical graphing method but taking a pointer to an array.

int jhcDisplay::GraphGridV (const jhcArr *h, int i, int j, int maxval, 
                            int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return GraphV(*h, GridX(i, gwid), GridY(j, ght), maxval, col, msg);
}


//= Show next item in scan order relative to the proceeding ones.
// puts "row" across then moves down (can mix graphs and images)
// size of cell in table controlled by first item displayed 

int jhcDisplay::GraphNext (const jhcArr& h, int maxval, 
                           int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";
  int i = NextI(), j = NextJ();

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  n++;
  return Graph(h, GridX(i, gwid), GridY(j, ght), maxval, col, msg);
}


//= De-references pointer then calls other function GraphNext.

int jhcDisplay::GraphNext (const jhcArr *h, int maxval, 
                           int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return GraphNext(*h, maxval, col, msg);
}


//= Show graph right next to last (ignores row width rw).

int jhcDisplay::GraphAdj (const jhcArr& h, int maxval, 
                          int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";
  int x = AdjX(), y = AdjY();

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return Graph(h, x, y, maxval, col, msg);
}


//= Show graph along y axis right next to last (ignores row width rw).

int jhcDisplay::GraphAdjV (const jhcArr& h, int maxval, 
                           int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";
  int x = AdjX(), y = AdjY();

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return GraphV(h, x, y, maxval, col, msg);
}


//= Show graph right below last one (ignores column height ch).

int jhcDisplay::GraphBelow (const jhcArr& h, int maxval, 
                            int col, const char *title, ...)
{
  va_list args;
  char msg[200] = "";
  int x = BelowX(), y = BelowY();

  if (title != NULL)
  {
    va_start(args, title); 
    vsprintf_s(msg, title, args);
  }
  return Graph(h, x, y, maxval, col, msg);
}


//= Draw a vertical line at corresponding index over last graph drawn.
// if ht < 0.0 then makes symmetric around middle
// should also work with images

int jhcDisplay::GraphMark (double bin, int col, double ht, int dash)
{
  CPen new_pen(((dash > 0) ? PS_DOT : PS_SOLID), 1, color_n(col));
  CPen *old_pen;
  CDC *cdc;
  int x, y0, y1, dy = 0;

  if ((gcnt <= 0) || (bin >= gcnt) || (bin < 0))
    return -1;

  // select new colored pen 
  cdc = win->GetDC();
  if (cdc == NULL)
    return -1;
  old_pen = cdc->GetCurrentPen();
  cdc->SelectObject(&new_pen);

  // find endpoints (possibly centered)
  x  = imgx + (int)(bin * imgw / (double)(gcnt - 1));
  y0 = imgy + imgh - 1;
  y1 = y0 - (int)(fabs(ht) * imgh);
  if (ht < 0.0)
    dy = (int)(0.5 * (1.0 + ht) * imgh);
 
  // draw line
  cdc->MoveTo(x, y0 - dy);
  cdc->LineTo(x, y1 - dy);

  // clean up
  cdc->SelectObject(old_pen);
  win->ReleaseDC(cdc);
  return 1;
} 


//= Draw a horizontal line at corresponding index over last graph drawn.
// if ht < 0.0 then makes symmetric around middle
// should also work with images

int jhcDisplay::GraphMarkV (double bin, int col, double ht, int dash)
{
  CPen new_pen(((dash > 0) ? PS_DOT : PS_SOLID), 1, color_n(col));
  CPen *old_pen;
  CDC *cdc;
  int y, x0, x1, dx = 0;

  if ((gcnt <= 0) || (bin >= gcnt) || (bin < 0))
    return -1;

  // select new colored pen 
  cdc = win->GetDC();
  if (cdc == NULL)
    return -1;
  old_pen = cdc->GetCurrentPen();
  cdc->SelectObject(&new_pen);

  // find endpoints (possibly centered)
  y  = imgy + imgh - (int)(bin * imgh / (double)(gcnt - 1));
  x0 = imgx;
  x1 = imgx + (int)(fabs(ht) * imgw);
  if (ht < 0.0)
    dx = ROUND(0.5 * (1.0 + ht) * imgw);

  // draw line
  cdc->MoveTo(x0 + dx, y);
  cdc->LineTo(x1 + dx, y);

  // clean up
  cdc->SelectObject(old_pen);
  win->ReleaseDC(cdc);
  return 1;
} 


//= Draw a horizontal line at some value given range(s) of last graph.
// negative maxval signals a graph that is symmetric around zero

int jhcDisplay::GraphVal (int lvl, int maxval, int col, int dash)
{
  CPen new_pen(((dash > 0) ? PS_DOT : PS_SOLID), 1, color_n(col));
  CPen *old_pen;
  CDC *cdc;
  double ht;
  int y, top = ((maxval == 0) ? gmax : maxval), bot = ((top < 0) ? top : 0);

  // determine height to draw line
  if ((top == 0) || (lvl > abs(top)) || (lvl < bot))
    return 0;
  ht = ght * (lvl - bot) / (double)(abs(top) - bot);

  // select new colored pen 
  cdc = win->GetDC();
  if (cdc == NULL)
    return -1;
  old_pen = cdc->GetCurrentPen();
  cdc->SelectObject(&new_pen);

  // draw line
  y = imgy + imgh - (int) ht;
  cdc->MoveTo(imgx, y);
  cdc->LineTo(imgx + imgw, y);

  // clean up
  cdc->SelectObject(old_pen);
  win->ReleaseDC(cdc);
  return 1;
}


//= Draw a vertical line at some value given range(s) of last graph.
// negative maxval signals a graph that is symmetric around zero

int jhcDisplay::GraphValV (int lvl, int maxval, int col, int dash)
{
  CPen new_pen(((dash > 0) ? PS_DOT : PS_SOLID), 1, color_n(col));
  CPen *old_pen;
  CDC *cdc;
  double ht;
  int x, top = ((maxval == 0) ? gmax : maxval), bot = ((top < 0) ? top : 0);

  // determine height to draw line
  if ((top == 0) || (lvl > abs(top)) || (lvl < bot))
    return 0; 
  ht = gwid * (lvl - bot) / (double)(abs(top) - bot);

  // select new colored pen 
  cdc = win->GetDC();
  if (cdc == NULL)
    return -1;
  old_pen = cdc->GetCurrentPen();
  cdc->SelectObject(&new_pen);

  // draw line
  x = imgx + (int) ht;
  cdc->MoveTo(x, imgy + imgh - 1);
  cdc->LineTo(x, imgy - 1);

  // clean up
  cdc->SelectObject(old_pen);
  win->ReleaseDC(cdc);
  return 1;
}


//////////////////////////////////////////////////////////////////////////////
//                    Render Other Assorted Entities                        //
//////////////////////////////////////////////////////////////////////////////

//= Write a formatted string on the window.
// consider using Label if you want old text cleared

int jhcDisplay::String (int x, int y, const char *msg, ...)
{
  CDC *cdc = win->GetDC();
  va_list args;
  jhcString val;

  if (cdc == NULL)
    return -1;
  va_start(args, msg); 
  vsprintf_s(val.ch, msg, args);
  cdc->SetBkMode(TRANSPARENT);
  cdc->TextOut(x, y, val.Txt(), val.Len());
  win->ReleaseDC(cdc);
  return 1;
}


//= Display on indexed panel whose size has previously been set.
// crops to column width using Label function

int jhcDisplay::StringGrid (int i, int j, const char *msg, ...)
{
  va_list args;
  char val[200];

  va_start(args, msg); 
  vsprintf_s(val, msg, args);
  imgx = GridX(i);
  imgy = GridY(j);
  imgh = 0;
  return label(imgx, imgy, cw, val);
}


//= Displays string just below last thing shown (possibly another string).

int jhcDisplay::StringBelow (const char *msg, ...)
{
  va_list args;
  char val[200];

  va_start(args, msg); 
  vsprintf_s(val, msg, args);
  if (imgh > 0)
    imgy += (imgh + 20);
  imgy += 20;
  imgh = 0;
  return label(imgx, imgy, cw, val);
}


//= Display string at upper left of next open panel.
// crops to column width using Label function

int jhcDisplay::StringNext (const char *msg, ...)
{
  va_list args;
  char val[200];
  int i = NextI(), j = NextJ();

  n++;
  va_start(args, msg); 
  vsprintf_s(val, msg, args);
  return label(GridX(i), GridY(j), cw, val);
}


//= Display string at upper left of next open position.
// crops to column width using Label function

int jhcDisplay::StringAdj (const char *msg, ...)
{
  va_list args;
  char val[200];
  int x = AdjX(), y = AdjY();

  // put arguments into string
  va_start(args, msg); 
  vsprintf_s(val, msg, args);

  // set up for next output using approximate size of most strings
  imgx = x;
  imgy = y;
  imgw = 200;     
  imgh = 20;
  return label(x, y, cw, val);
}


//= Display string displaced left and DOWN within current open panel.
// does NOT advance panel pointer after writing

int jhcDisplay::StringOff (int dx, int dy, const char *msg, ...)
{
  va_list args;
  char val[200];
  int x = GridX(NextI()) + dx, y = GridY(NextJ()) + dy;

  va_start(args, msg); 
  vsprintf_s(val, msg, args);
  return label(x, y, cw, val);
}


//////////////////////////////////////////////////////////////////////////////
//                      Drawing Support Functions                           //
//////////////////////////////////////////////////////////////////////////////

//= Method for choosing color to draw with based on a single number.
//  0 = black,  1 = red,  2 = green, 3 = yellow, 4 = blue, 
//  5 = purple, 6 = aqua, 7 = white, 8 = black again, etc.

UL32 jhcDisplay::color_n (int col) const
{
  int n = col;
  UL32 ans = 0;

  if (n < 0)
    n = -n;
  if ((n & 0x04) != 0)
    ans |= 0x00FF0000;   // blue
  if ((n & 0x02) != 0)
    ans |= 0x0000FF00;   // green
  if ((n & 0x01) != 0)
    ans |= 0x000000FF;   // red
  return ans;
}


//= Set a rectangle of the screen to the standard background color.
// automatically expands the rectangle by 1 pixel in each direction
// fills with background color unless white forced with "wht" non-zero

int jhcDisplay::clear_rect (int x, int y, int w, int h, int wht)
{
  CDC *cdc = win->GetDC();
  CBrush fill_brush((COLORREF)((wht != 0) ? 0x00FFFFFF : bgcol));
  RECT area;

  if (cdc == NULL)
    return -1;
  if ((w <= 0) || (h <= 0))
    return 0;
  area.left   = x - 1;
  area.right  = x + w + 1;
  area.top    = y - 1;
  area.bottom = y + h + 1;
  cdc->FillRect(&area, &fill_brush);
  win->ReleaseDC(cdc);
  return 1;
}


//= Clear a box and write a title string above object.

int jhcDisplay::label (int x, int y, int w, const char *msg)
{
  RECT rect;
  CDC *cdc = win->GetDC();
  jhcString lab(msg);
  int hfont = 22, tab = 10;

  if (cdc == NULL)
    return -1;

  // clear any previous string (if non-zero width)
  clear_rect(x, y - hfont, w, hfont - 2);

  // write new message, possibly with tabs or constrained to some size
  cdc->SetBkMode(TRANSPARENT);
  if (strchr(msg, '\t') != NULL)
    cdc->TabbedTextOut(x, y - hfont, lab.Txt(), lab.Len(), 1, &tab, x);  // does not trim to size!
  else if (w <= 0)
    cdc->TextOut(x, y, lab.Txt(), lab.Len());
  else
  {
    rect.left   = x;
    rect.top    = y - hfont;
    rect.right  = x + w;
    rect.bottom = y;
    cdc->ExtTextOut(x, y - hfont, ETO_CLIPPED, &rect, lab.Txt(), lab.Len(), NULL);
  }

  // clean up
  win->ReleaseDC(cdc);
  return 1;
}
  

//= Draw a thin black rectangle around a region.

int jhcDisplay::frame (int x, int y, int w, int h)
{
  CDC *cdc = win->GetDC();
  CBrush new_brush((COLORREF) 0);
  RECT area;

  if (cdc == NULL)
    return -1;

  area.left   = x - 1;
  area.right  = x + w + 1;
  area.top    = y - 1;
  area.bottom = y + h + 1;
  cdc->FrameRect(&area, &new_brush);
  win->ReleaseDC(cdc);
  return 1;
}


//////////////////////////////////////////////////////////////////////////////
//                      Low-Level Mouse Functions                           //
//////////////////////////////////////////////////////////////////////////////

//= Gets rid of any pending mouse clicks or typed characters.
// resets pacing timer so first frame not delayed

void jhcDisplay::FlushMsg ()
{
  MSG msg;

  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    handle_others(&msg);
  tdisp = 0;
}


//= Let system take care of non-mouse and keyboard messages.

void jhcDisplay::handle_others (MSG *msg)
{
  if (msg == NULL)
    return;
  if ((msg->message != WM_LBUTTONDOWN) && 
      (msg->message != WM_LBUTTONUP)   && 
      (msg->message != WM_MBUTTONDOWN) && 
      (msg->message != WM_MBUTTONUP)   && 
      (msg->message != WM_RBUTTONDOWN) && 
      (msg->message != WM_RBUTTONUP)   && 
      (msg->message != WM_KEYDOWN)     &&
      (msg->message != WM_KEYUP))
  {
    TranslateMessage(msg);
    DispatchMessage(msg);
  }
}


//= Get top message in queue into supplied structure.
// returns 0 for no msg, 1 for valid message
// returns -1 if some main menu item is selected and
//   leaves message posted so main window can handle it

int jhcDisplay::peek_no_menu (MSG *msg)
{
  if (msg == NULL)
    return 0;
  if (!PeekMessage(msg, NULL, 0, 0, PM_NOREMOVE))
    return 0;
  if (msg->message == WM_SYSCOMMAND)
    if (msg->wParam != SC_RESTORE)       // for shortcuts with "?"
      return -1;
  if (msg->message == WM_NCLBUTTONDOWN) 
    return -1;
  if (!GetMessage(msg, NULL, 0, 0))
    return -1;                           // WM_QUIT read
  return 1;
}


//////////////////////////////////////////////////////////////////////////////
//                 Mouse and Key Interface Functions                        //
//////////////////////////////////////////////////////////////////////////////

//= Immediately return non-zero if some mouse button or some keyboard key hit.
// -1 = left mouse, -2 = middle mouse, -3 = right mouse, positive = key
// -4 = some main menu command selected (leaves message in queue) or app closed
// Note: mouse codes negated relative to AnyClick

int jhcDisplay::AnyHit ()
{
  int some, ans = 0;
  MSG msg;

  while ((some = peek_no_menu(&msg)) != 0)
    if (some < 0)  
      return -4;
    else if (msg.message == WM_LBUTTONDOWN)   // was up
    {
      mx = LOWORD(msg.lParam);
      my = HIWORD(msg.lParam);
      mbut = 1;
      ans = -1;
    }
    else if (msg.message == WM_MBUTTONDOWN)   // was up
    {
      mx = LOWORD(msg.lParam);
      my = HIWORD(msg.lParam);
      mbut = 2;
      ans = -2;
    }
    else if (msg.message == WM_RBUTTONDOWN)   // was up
    {
      mx = LOWORD(msg.lParam);
      my = HIWORD(msg.lParam);
      mbut = 3;
      ans = -3;
    }
    else if (msg.message == WM_KEYDOWN)
      ans = (int) msg.wParam;
    else 
      handle_others(&msg);
  return ans;
}


//= Like "AnyHit" but will wait up to "ms" milliseconds for activity.
// waits in 100 ms chunks, returns as soon as a hit detected
// if wait is negative, then hangs until some input

int jhcDisplay::WaitHit (int ms)
{
  int hit;
  int left = ms, chunk = 100;

  while ((hit = AnyHit()) == 0)
    if (ms < 0)
      jms_sleep(chunk);
    else
    {
      if (left <= 0)
        break;
      jms_sleep(__min(left, chunk));
      left -= chunk;
    }
  return hit;
}


//= Like "AnyHit" but will wait up to "ms" milliseconds since last call.
// waits in 100 ms chunks, returns as soon as a hit detected
// sets member variable "tdisp" to time fcn last called

int jhcDisplay::PaceOrHit (int ms, int grid)
{
  int hit, left, early = 2, chunk = 100;
  DWORD tnext, tprev = tdisp;

  // get current time and remember then possibly reset screen
  // assumes only one major loop, so this is last time fcn was called
  tdisp = jms_now();
  if (grid > 0)
    ResetGrid(grid, 0, 0);

  // return immediately if already hit 
  // or if this is first call or if interval already elapsed
  if ((hit = AnyHit()) != 0)
    return hit;
  if ((tprev == 0) || (ms <= 0))
    return 0;
  tnext = tprev + (DWORD)(ms - early);
  if (tnext <= tdisp)
    return 0;

  // otherwise sample mouse and keyboard at a regular interval
  left = jms_diff(tnext, tdisp);
  while (left > 0)
  {
    jms_sleep(__min(left, chunk));
    if ((hit = AnyHit()) != 0)
      return hit;
    left = jms_diff(tnext, jms_now());
  }
  return 0;
}


//= Return 0 to continue loop, non-zero if some key or click sensed.
//   always waits the specified time before returning with ok
// aborts immediately if menu chosen or right click or ESC key  
//   if other click or key and not strict, then asks user
//   give message "" to stop without asking for confirmation
// can optionally reset the display position (i.e. grid non-zero)

int jhcDisplay::LoopHit (int ms, int grid, int strict, char *msg)
{
  int hit;

  // analyze for no hit or guaranteed abort cases
  hit = PaceOrHit(ms, grid);
  if ((hit == 0) || (hit == -4))
    return hit;
  if ((strict > 0) && (hit != -3) && (hit != 27))
    return hit;

  // ask user about stopping then reset display interval timer
  if (msg == NULL) 
  {
    if (!Ask("Stop function?"))
      hit = 0;
  }
  else if (*msg != '\0') 
  { 
    if (!Ask(msg))
      hit = 0;
  }
  tdisp = jms_now();
  return hit;
}


//////////////////////////////////////////////////////////////////////////////
//                              Mouse Clicks                                //
//////////////////////////////////////////////////////////////////////////////

//= Returns 1 if the left mouse button was clicked recently.
// can return coordinates wrt client window if variables supplied
// returns -1 if some main menu command selected (leaves posted) or app closed

int jhcDisplay::ClickL (int *x, int *y)
{
  int some, ans = 0;
  MSG msg;

  while ((some = peek_no_menu(&msg)) != 0)
    if (some < 0) 
      return -1;
    else if (msg.message == WM_LBUTTONUP)
    {
      ans = 1;
      mx = LOWORD(msg.lParam);
      my = HIWORD(msg.lParam);
      mbut = 1;
      if (x != NULL)
        *x = mx;
      if (y != NULL)
        *y = my;
    }
    else
      handle_others(&msg);
  return ans;
}


//= Returns 1 if the right mouse button was clicked recently.
// can return coordinates wrt client window if variables supplied
// returns -1 if some main menu command selected (leaves posted) or app closed

int jhcDisplay::ClickR (int *x, int *y)
{
  int some, ans = 0;
  MSG msg;

  while ((some = peek_no_menu(&msg)) != 0)
    if (some < 0)
      return -1;
    else if (msg.message == WM_RBUTTONUP)
    {
      ans = 1;
      mx = LOWORD(msg.lParam);
      my = HIWORD(msg.lParam);
      mbut = 3;
      if (x != NULL)
        *x = mx;
      if (y != NULL)
        *y = my;
    }
    else
      handle_others(&msg);
  return ans;
}


//= Checks if some mouse button has been clicked (takes latest value).
// returns 0 if no clicks, 1 for left, 2 for middle, 3 for right
// special return of -1 if main menu selection made (leaves msg posted) or app closed
// can return coordinates in client window if variables supplied

int jhcDisplay::ClickAny (int *x, int *y)
{
  int some, ans = 0;
  MSG msg;

  while ((some = peek_no_menu(&msg)) != 0)
    if (some < 0) 
      return -1;
    else if ((msg.message == WM_LBUTTONUP) ||
             (msg.message == WM_MBUTTONUP) ||
             (msg.message == WM_RBUTTONUP))
    {
      if (msg.message == WM_LBUTTONUP)
        ans = 1;
      else if (msg.message == WM_MBUTTONUP)
        ans = 2;
      else
        ans = 3;
      mx = LOWORD(msg.lParam);
      my = HIWORD(msg.lParam);
      mbut = ans;
      if (x != NULL)
        *x = mx;
      if (y != NULL)
        *y = my;
    }
    else
      handle_others(&msg);
  return ans;
}


//= Waits until user clicks some button (takes latest).
// returns 1 for left, 2 for middle, 3 for right
// can return coordinates in client window if variables supplied

int jhcDisplay::ClickWait (int *x, int *y)
{
  int ans;

  FlushMsg();
  while ((ans = ClickAny(x, y)) == 0)
    jms_sleep(10);
  return ans;
}


//= Like ClickWait but returns coordinates relative to an image.
// corrects for top-down graphics vs. bottom-up image coords
// returns mouse button number, or -1 if out of bounds

int jhcDisplay::ClickRel (int *x, int *y, int lx, int ly, int cw, int ch)
{
  int x0, y0, ans;

  ans = ClickWait(&x0, &y0);
  if (img_coords(x, y, x0, y0, lx, ly, cw, ch) <= 0)
    return -1;
  return ans;
}


//= Like ClickWait but does not wait for button to be released.
// special return of -1 if main menu item is selected or app closed
// if rel is positive then tries to convert to image coordinates

int jhcDisplay::DownWait (int *x, int *y, int rel)
{
  int some, ans = 0;
  MSG msg;

  FlushMsg();
  while (ans == 0)
    while ((some = peek_no_menu(&msg)) != 0)
      if (some < 0)
        return -1;
      else if ((msg.message == WM_LBUTTONDOWN) ||
               (msg.message == WM_MBUTTONDOWN) ||
               (msg.message == WM_RBUTTONDOWN))
      {
        if (msg.message == WM_LBUTTONDOWN)
          ans = 1;
        else if (msg.message == WM_MBUTTONDOWN)
          ans = 2;
        else
          ans = 3;
        mx = LOWORD(msg.lParam);
        my = HIWORD(msg.lParam);
        if (rel > 0)
          if (img_coords(&mx, &my, mx, my) <= 0)  
            ans = 0;        // clicks outside image don't count
        mbut = ans;           
        if (x != NULL)
          *x = mx;
        if (y != NULL)
          *y = my;
      }
      else
        handle_others(&msg);
  return ans;
}


//////////////////////////////////////////////////////////////////////////////
//                            Mouse tracking                                //
//////////////////////////////////////////////////////////////////////////////

//= Finds current position of the mouse (waiting if needed).
// changed to BLOCK until some mouse event occurs by default
// returns 0 if none, 1 if left button down, 2 for middle, 3 for right
// returns -1 if main menu selected or app closed, -2 if key pressed
// returns latest mouse button for non-blocking mode (i.e. mbut not cleared)

int jhcDisplay::MousePos (int *x, int *y, int block)
{
  MSG msg;
  int some, esc = 0, key = 0, ans = -1;

  // no mouse click messages have been found yet
  while (ans < 0)                                     // restored 6/12 for MouseBox
  {
    // go through all messages currently on queue
    while ((some = peek_no_menu(&msg)) != 0)
    {
      // look for window menu or keyboard action
      if (some < 0)                             
      {
        esc = 1;
        break;
      }
      if (msg.message == WM_KEYDOWN)
      {
        key = 1;
        break;
      }

      // peek at pending mouse messages but process others
      if ((msg.message == WM_MOUSEMOVE)   ||
          (msg.message == WM_LBUTTONDOWN) ||
          (msg.message == WM_MBUTTONDOWN) ||
          (msg.message == WM_RBUTTONDOWN) ||
          (msg.message == WM_LBUTTONUP)   ||
          (msg.message == WM_MBUTTONUP)   ||
          (msg.message == WM_RBUTTONUP))
      {
        // if some mouse-related message found then get button
        if ((msg.wParam & MK_RBUTTON) != 0)
          ans = 3;
        else if ((msg.wParam & MK_MBUTTON) != 0)
          ans = 2;
        else if ((msg.wParam & MK_LBUTTON) != 0)
          ans = 1;
        else
          ans = 0;

        // extract position and load persistent variables
        mx = LOWORD(msg.lParam);
        my = HIWORD(msg.lParam);
        mbut = ans;
      }
      else
        handle_others(&msg);
    }

    // if no mouse message then try again in a while
    if ((esc > 0) || (block <= 0))
      break;
    jms_sleep(10);           
  }

  // bind values
  if (x != NULL)
    *x = mx;
  if (y != NULL)
    *y = my;

  // figure out value to return
  if (esc > 0)
    return -1;
  if (key > 0)
    return -2;
  return mbut;
}


//= Finds position relative to the the last object displayed.
// corrects for top-down graphics vs. bottom-up image coords
// returns 0 if no mouse button pressed, -1 if out of bounds, -2 for exit

int jhcDisplay::MouseRel (int *x, int *y, int lx, int ly, int cw, int ch)
{
  int ans;

  ans = MousePos(x, y);
  if (ans == -1)
    return -2;
  if (img_coords(x, y, *x, *y, lx, ly, cw, ch) <= 0)
    return -1;
  return ans;
} 


//= Same as other MouseRel but takes an array containing image bounds.

int jhcDisplay::MouseRel (int *x, int *y, int specs[])
{
  return MouseRel(x, y, specs[0], specs[1], specs[2], specs[3]);
}


//= Tells mouse position relative to last image but does not block.
// always returns valid x and y values in image (i.e. clips if outside)
// returns: -2 = menu or key, -1 = out of image, 0 = no button, 1-3 = button
// Note: only returns button number if mouse clicked inside image
// Note: make sure grid is not being reset (e.g. by LoopHit)

int jhcDisplay::MouseRel0 (int& x, int& y)
{
  mbut = 0;                              // added default if no messages
  if (MousePos(NULL, NULL, 0) < 0)       // refreshes mx, my, and mbut
    return -2;
  if (img_coords(&x, &y, mx, my, imgx, imgy, imgw, imgh) <= 0)
    return -1;
  return mbut;
} 


//= Convert last click into a position relative to current image.
// does not wait for a new click, just reinterprets old click

int jhcDisplay::MouseRel2 (int& x, int& y)
{
  if (img_coords(&x, &y, mx, my, imgx, imgy, imgw, imgh) <= 0)
    return -1;
  return mbut;
}


//= Given MouseRel0 return code tells whether to exit routine.
// often used with MouseRel0 to find clicks in a live image
// returns 1 if main menu, or R click plus confirmation

int jhcDisplay::MouseExit (int code)
{
  if (code < -1)
    return 1;
  if (code == 3) 
    if (Ask("Stop function?") > 0)
      return 1;
  return 0;
}


//= Tells whether something other than left click received by MouseRel0.
// returns 0 if left click or nothing

int jhcDisplay::MouseNotL (int code)
{
  if ((code != 1) && (code != 0))
    return 1;
  return 0;
}


//= Changes mouse screen coordinates into image coordinates.
// reads then alters variables pointed to, clips if needed
// returns 0 if true coords were outside of image

int jhcDisplay::img_coords (int *xi, int *yi, int xs, int ys, 
                            int lx, int ly, int cw, int ch) const
{
  int ans = 0, xmin = lx, ymin = ly, wid = cw, ht = ch;

  // compute image bounds using defaults if needed
  if (lx < 0)
    xmin = imgx;
  if (ly < 0)
    ymin = imgy;
  if (cw < 0)
    wid = imgw;
  if (ch < 0)
    ht = imgh; 

  // convert X screen coordinate and clip
  *xi = xs - xmin;
  if (*xi < 0)
    *xi = 0;
  else if ((wid > 0) && (*xi > wid))
    *xi = wid - 1;
  else
    ans++;

  // convert Y screen coordinate and clip
  if (ht > 0)
    *yi = (ymin + ht - 1) - ys;
  else
    *yi = ys - ymin;
  if (*yi < 0)
    *yi = 0;
  else if ((ht > 0) && (*yi > ht))
    *yi = ht - 1;
  else
    ans++;

  // see if both dimensions passed containment check
  if (ans < 2)
    return 0;
  return 1;
}


//= Takes x and y in image coordinates and converts to screen coords.

void jhcDisplay::scr_coords (int *xs, int *ys, int xi, int yi, 
                             int lx, int ly, int ch) const
{
  int xmin = lx, ymin = ly, ht = ch;

  // compute image bounds using defaults if needed
  if (lx < 0)
    xmin = imgx;
  if (ly < 0)
    ymin = imgy;
  if (ch < 0)
    ht = imgh; 

  // transform coords depending on whether image height is known
  *xs = xi + xmin;
  if (ht > 0)
    *ys = (ymin + ht - 1) - yi;
  else
    *ys = yi + ymin;
}


//////////////////////////////////////////////////////////////////////////////
//                           Mouse selection                                //
//////////////////////////////////////////////////////////////////////////////

//= Let user draw a single XOR box over region specified.
//   if mouse makes box width or height negative, new box started
//   user can abort by clicking right outside of region (returns 0)
// if region coordinates are negative, uses last image as reference
// clips box to region and returns relative position and size
//   adjusts for top-down screen coords vs. bottom-up image coords
// Note: parameters not altered if aborted by right click

int jhcDisplay::MouseBox (int *x, int *y, int *w, int *h, 
                          int lx, int ly, int cw, int ch)
{
  int msx, msy, fx, fy, rx, ry;
  CDC *cdc = win->GetDC();
  RECT rect, old;
  SIZE bd;
  CBrush style(0x00FFFFFF);

  // loop to allow new boxes to start if needed
  bd.cx = 2;
  bd.cy = 2;
  while (1)
  {
    // wait for first click: left starts box, right aborts
    // only left clicks inside valid area count
    while (1)
    {
      if (DownWait(&msx, &msy) != 1)
      {
        win->ReleaseDC(cdc);
        return 0;              // aborted
      }
      if (img_coords(&rx, &fy, msx, msy, lx, ly, cw, ch) > 0)
        break;
    }

    // set up initial rectangle and copy to old, then draw 
    rect.left   = msx;
    rect.top    = msy;
    rect.right  = msx;
    rect.bottom = msy;
    old.left    = msx;
    old.top     = msy;
    old.right   = msx;
    old.bottom  = msy;
    cdc->DrawDragRect(&rect, bd, NULL, bd, &style, &style);

    // "rubber band" rectangle over image area when button held
    // if negative dimensions, allow user to start new box or abort
    //   else transform coords to image and clip to borders
    while (MousePos(&msx, &msy) == 1)
    {
      if ((msx < rect.left) || (msy < rect.top))
      {
        cdc->DrawDragRect(&rect, bd, NULL, bd, &style, &style);
        rect.right = rect.left;
        break;
      }
      img_coords(&fx, &ry, msx, msy, lx, ly, cw, ch);
      scr_coords(&msx, &msy, fx, ry, lx, ly, ch);
      rect.right  = msx + 1;
      rect.bottom = msy + 1;
      cdc->DrawDragRect(&rect, bd, &old, bd, &style, &style);
      old.right  = msx + 1;
      old.bottom = msy + 1;
    }

    // make sure final rectangle has some nonzero area
    // msx and msy are changed when MousePos has no button
    if ((rect.right > rect.left) && (rect.bottom > rect.top))
      break;
  }
  
  // erase rectangle and copy out coordinates 
  cdc->DrawDragRect(&rect, bd, NULL, bd, &style, &style);
  *x = rx;
  *y = ry;
  *w = fx - rx + 1;
  *h = fy - ry;
  win->ReleaseDC(cdc);
  return 1;
}


//= Like other MouseBox but position and size are returned in an array.

int jhcDisplay::MouseBox (int region[],
                          int lx, int ly, int cw, int ch)
{
  return MouseBox(region, region + 1, region + 2, region + 3,
                  lx, ly, cw, ch);
}


//= Like other MouseBox but region is specified by an array also.

int jhcDisplay::MouseBox (int region[], int specs[])
{
  return MouseBox(region, specs[0], specs[1], specs[2], specs[3]);
}


//////////////////////////////////////////////////////////////////////////////
//                    Private color map generators                          //
//////////////////////////////////////////////////////////////////////////////

//= Private function to create and fill in special color maps.

void jhcDisplay::tables ()
{
  LPBITMAPINFOHEADER hdr;  
  int hsz = sizeof(BITMAPINFOHEADER);
  int csz = 256 * sizeof(RGBQUAD);
  int i;

  // allocate standard color maps with headers
  for (i = 0; i <= 4; i++)
  {
    if (i <= 3)
    {
      hdr = (LPBITMAPINFOHEADER) new char[hsz + csz];
      hdr->biBitCount = 8;
      hdr->biClrUsed  = 256;
      ctables[i] = (RGBQUAD *)((BYTE *) hdr + hsz);
    }
    else
    {
      hdr = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;
      hdr->biBitCount = 24;
      hdr->biClrUsed  = 0;
    }
    hdr->biSize          = hsz;
    hdr->biPlanes        = 1;
    hdr->biCompression   = BI_RGB;
    hdr->biSizeImage     = 0;
    hdr->biXPelsPerMeter = 1000;
    hdr->biYPelsPerMeter = 1000;
    hdr->biClrImportant  = 0;
    ctab_hdrs[i] = hdr;
  }

  // fill in standard color maps (except equalize)
  linear(ctables[0]);
  bands(ctables[2]);
  pseudo(ctables[3]);
}


//= Remap gray values so histogram of values is flatter.
// assumes each line is padded to int word (32 bit) boundary

void jhcDisplay::equalize (RGBQUAD *ctable, const jhcImg& src)
{
  int x, y, i;
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  BYTE v;
  double sc = 255.0 / (rw * rh);
  int cnt[256];
  int below = 0;
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  RGBQUAD *cols = ctable;
    
  // clear then build a histogram within the ROI
  for (i = 0; i < 256; i++)
    cnt[i] = 0;
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      cnt[*s++] += 1;
    s += rsk;
  }

  // build a gray scale mapping table from histogram
  for (i = 0; i < 256; i++)
  {
    below += cnt[i] / 2;
    v = (BYTE)(sc * below);
    cols->rgbRed   = v;
    cols->rgbGreen = v;
    cols->rgbBlue  = v;
    cols++;
    below += (cnt[i] + 1) / 2;
  }
}


//= Set color map for linear scaling of intensities.

void jhcDisplay::linear (RGBQUAD *ctable)
{
  int i;
  BYTE v = 0;
  RGBQUAD *cols = ctable;

  for (i = 0; i < 256; i++)
  {
    cols->rgbRed   = v;
    cols->rgbGreen = v;
    cols->rgbBlue  = v;
    cols++;
    v++;
  }
}


//= Set color map for hue = 0 to 240 (no purple), saturation 1, intensity 0.5.

void jhcDisplay::pseudo (RGBQUAD *ctable)
{
  int i;
  const double third = 255.0 / 2.0, sixth = 255.0 / 4.0;
  const double HalfRad3 = 255.0 * sqrt(3.0) / 4.0;
  const double DegToRad = 3.14159265358979 / 180.0;
  const double step = -240.0 * DegToRad / 256.0;
  double c, s, ang = 240.0 * DegToRad;
  RGBQUAD *cols = ctable;

  // sine and cosine can not be called at beginning of loop?!
  s = sin(ang);
  c = cos(ang);
  for (i = 0; i < 256; i++)
  {
    cols->rgbRed   = (BYTE)(third * c + third);   
    cols->rgbGreen = (BYTE)(HalfRad3 * s - sixth * c + third);  
    cols->rgbBlue  = (BYTE)(-HalfRad3 * s - sixth * c + third); 
    cols++;
    ang += step;
    s = sin(ang);
    c = cos(ang);
  }
}


//= Set up for distinct bands of color (like mini-grok).
//   0 = black
//   1 = dark gray   25 = dark violet  40 = light violet  50 = blue
//  70 = light blue  90 = sky blue    100 = dark green   120 = mud green
// 128 = green      150 = pea green   165 = brown        180 = orange
// 200 = red        215 = magenta     230 = yellow       255 = white

void jhcDisplay::bands (RGBQUAD *ctable)
{
  int i, j;
  RGBQUAD *cols = ctable;
  BYTE vals[16][3] = {{ 70,  70,  70}, {  0,   0, 128},   
                      { 72,  61, 139}, {  0,   0, 255},   
                      { 30, 144, 255}, {135, 206, 250},   
                      { 34, 139,  34}, {107, 142,  35},   
                      { 50, 205,  50}, {154, 205,  50},   
                      {205, 133,  63}, {255, 165,   0},   
                      {255,   0,   0}, {255,   0, 255},   
                      {255, 255,   0}, {255, 255, 255}};  

  // bands for most colors
  for (i = 0; i < 16; i++)
    for (j = 0; j < 16; j++)
    {
      cols->rgbRed   = vals[i][0];
      cols->rgbGreen = vals[i][1];
      cols->rgbBlue  = vals[i][2];
      cols++;
    }

  // zero is special black
  ctable->rgbRed   = 0;
  ctable->rgbGreen = 0;
  ctable->rgbBlue  = 0;
}
