#include <stdio.h>
#define GR_IMPL
#include "gr.h"

int gr_setup(struct gr_ctx *gr, int argc, char **argv)
{
  puts("demo1");
  getchar();
  gr->config(gr, 1024*1024*32, 640, 480, "demo");
  return 0;
}

int gr_frame(struct gr_ctx *gr)
{
  u32 ev, ep;

  while ((ev = gr->event(gr, &ep)) != 0) {
    if (ev == GR_EV_WINSHUT || (ev == GR_EV_KEYDOWN && ep == GR_KEY_ESC))
      return GR_EXIT;
  }
  return 0;
}
