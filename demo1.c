#include <stdio.h>
#define GR_IMPL
#include "gr.h"

int gr_setup(struct gr_ctx *gr, int argc, char **argv)
{
  puts("demo1");
  gr->config(gr, 1024u*1024u*32u, 640, 480, "demo");
  return 0;
}

static void draw(struct gr_ctx *gr)
{
  u32 x, y, xres=gr->xres, yres=gr->yres, *p=gr->fb, t=(int)(gr->time*1000.0);
  for (y=0; y<yres; ++y) {
    u32 z = y*y + t;
    for (x=0; x<xres; ++x)
      p[x] = x*x + z;
    p += xres;
  }
}

int gr_frame(struct gr_ctx *gr)
{
  u32 ev, ep;

  while ((ev = gr->event(gr, &ep)) != 0) {
    if (ev == GR_EV_WINSHUT || (ev == GR_EV_KEYDOWN && ep == GR_KEY_ESC))
      return GR_EXIT;
  }

  draw(gr);

  return 0;
}
