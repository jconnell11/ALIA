// grab_sample.cpp : open a video source and grab one frame

#include <stdio.h>
#include <conio.h>

#include "Data/jhcImg.h"
#include "Data/jhcImgIO.h"
#include "Interface/jms_x.h"

#include "vid_ocv3.h"


// takes name of stream to open else picks first camera

int main (int argc, const char *argv[])
{
  jhcImgIO jio;
  jhcImg samp;
  UL32 t0;
  double rate;
  int ok, w, h, nf, i;

  // try opening source
  if (argc > 1)
  {
    printf("Opening %s ...\n", argv[1]);
    ok = vid_ocv3_open(argv[1]);
  }
  else
  {
    printf("Opening camera 0 ...\n");
    ok = vid_ocv3_cam(0);
  }

  // tell result and determine frame size
  if (ok <= 0)
  {
    printf("  FAILED!\n");
    _getch();
    return 0;
  }
  w = vid_ocv3_w();
  h = vid_ocv3_h();
  nf = vid_ocv3_nf();
  rate = vid_ocv3_fps();
  printf("  Images are (%d %d) x %d at %3.1f fps\n", w, h, nf, rate);

  // get an image and save it
  samp.SetSize(w, h, nf);
  if ((ok = vid_ocv3_get(samp.PxlDest())) <= 0)
    printf("Could not grab image!\n");
  else if (jio.Save("sample.bmp", samp) <= 0)
    printf("Problem saving image!\n");
  else
    printf("Saved image as sample.bmp\n");

  // check frame rate
  if (ok > 0)
  {
    printf("\nGrabbing 1000 frames for timing ...\n");
    jtimer_clr();
    t0 = jms_now();
    for (i = 0; i < 1000; i++)
    {
      jtimer(1, "get");
      vid_ocv3_get(samp.PxlDest());
      jtimer_x(1);
    }
    printf("Actual rate = %3.1f fps\n", 1000000.0 / jms_diff(jms_now(), t0));
    jtimer_rpt();
  }

  // let user see messages
  _getch();
  return 1;
}