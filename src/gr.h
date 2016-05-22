/* gr.h
 * Written in 2016 by Mårten Hansson <hmrten@gmail.com>
 * License: Public domain */

#include <stddef.h>
#include <stdint.h>

typedef int8_t    i8;
typedef uint8_t   u8;
typedef int16_t  i16;
typedef uint16_t u16;
typedef int32_t  i32;
typedef uint32_t u32;
typedef int64_t  i64;
typedef uint64_t u64;

enum {
  /* return codes */
  GR_EXIT=1,
  /* events */
  GR_EV_WINSHUT = 1, GR_EV_KEYDOWN, GR_EV_KEYUP, GR_EV_KEYCHAR, GR_EV_MOUSE,
  /* keycodes ('0'..'9' and 'a'..'z' as lowercase ascii) */
  GR_KEY_BACK=8, GR_KEY_TAB=9, GR_KEY_RET=13, GR_KEY_ESC=27, GR_KEY_SPACE=32,
  GR_KEY_CTRL=128, GR_KEY_SHIFT, GR_KEY_ALT, GR_KEY_UP, GR_KEY_DOWN,
  GR_KEY_LEFT, GR_KEY_RIGHT, GR_KEY_INS, GR_KEY_DEL, GR_KEY_HOME, GR_KEY_END,
  GR_KEY_PGUP, GR_KEY_PGDN, GR_KEY_F1, GR_KEY_F2, GR_KEY_F3, GR_KEY_F4,
  GR_KEY_F5, GR_KEY_F6, GR_KEY_F7, GR_KEY_F8, GR_KEY_F9, GR_KEY_F10,
  GR_KEY_F11, GR_KEY_F12, GR_KEY_M1, GR_KEY_M2, GR_KEY_M3, GR_KEY_M4, GR_KEY_M5
};

struct gr_ctx {
  void  *mem;
  void  *fb;
  u32    xres;
  u32    yres;
  u32    xwin;
  u32    ywin;
  double time;
  double dt;
  void (*config)(struct gr_ctx *, size_t, u32, u32, const char *);
  u32  (*event)(struct gr_ctx *, u32 *);
  void (*winsize)(struct gr_ctx *, u32, u32);
};

int gr_setup(struct gr_ctx *, int, char **);
int gr_frame(struct gr_ctx *);
