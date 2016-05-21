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
};

int gr_setup(struct gr_ctx *, int, char **);
int gr_frame(struct gr_ctx *);

#ifdef GR_IMPL
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static struct {
  WNDCLASS wc;
  HWND     hw;
  HDC      dc;
  u32      winsz;
  u32      qhead;
  u32      qdata[256][2];
  u32      qtail;
} gr_w32;

static void gr_w32_wintext(struct gr_ctx *gr, const char *text)
{
  HWND hw = gr_w32.hw;
  _ReadWriteBarrier();
  SetWindowTextA(hw, text);
}

static void gr_w32_winsize(struct gr_ctx *gr, u32 x, u32 y)
{
  HWND hw = gr_w32.hw;
  RECT r = {0, 0, (LONG)x, (LONG)y};
  _ReadWriteBarrier();
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
  SetWindowPos(hw, HWND_TOP, 0, 0, r.right-r.left, r.bottom-r.top,
               SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
}

static void gr_w32_config(struct gr_ctx *gr, size_t memsz, u32 xres, u32 yres,
                         const char *title)
{
  size_t fbsz = xres*yres*4;

  gr->fb = VirtualAlloc(0, fbsz, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
  gr->mem = VirtualAlloc(0, memsz, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
  gr->xres = xres;
  gr->yres = yres;
}

static int gr_w32_addevent(struct gr_ctx *gr, u32 ev, u32 ep)
{
  u32 head = gr_w32.qhead;
  u32 tail = gr_w32.qtail;
  u32 ntail = (tail+1) & 255;

  _ReadWriteBarrier();
  if (ntail == head) return 0;
  gr_w32.qdata[tail][0] = ev;
  gr_w32.qdata[tail][1] = ep;
  _ReadWriteBarrier();
  gr_w32.qtail = ntail;
  return 1;
}

static u32 gr_w32_event(struct gr_ctx *gr, u32 *ep)
{
  u32 head = gr_w32.qhead;
  u32 tail = gr_w32.qtail;
  u32 ev;

  _ReadWriteBarrier();
  if (head == tail) return 0;
  ev  = gr_w32.qdata[head][0];
  *ep = gr_w32.qdata[head][1];
  _ReadWriteBarrier();
  gr_w32.qhead = (head+1) & 255;

  return ev;
}

static LRESULT CALLBACK gr_w32_winproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
  static const u8 vkmap[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, '\b', '\t', 0, 0, 0, '\r', 0, 0, GR_KEY_SHIFT,
    GR_KEY_CTRL, GR_KEY_ALT, 0, 0, 0, 0, 0, 0, 0, 0, GR_KEY_ESC, 0, 0, 0, 0,
    ' ', GR_KEY_PGUP, GR_KEY_PGDN, GR_KEY_END, GR_KEY_HOME, GR_KEY_LEFT,
    GR_KEY_UP, GR_KEY_RIGHT, GR_KEY_DOWN, 0, 0, 0, 0, GR_KEY_INS, GR_KEY_DEL,
    0, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0, 0, 0, 0, 0, 0, 0,
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, GR_KEY_F1, GR_KEY_F2, GR_KEY_F3,
    GR_KEY_F4, GR_KEY_F5, GR_KEY_F6, GR_KEY_F7, GR_KEY_F8, GR_KEY_F9,
    GR_KEY_F10, GR_KEY_F11, GR_KEY_F12
  };
  u32 ev=0, ep=0;

  switch (msg) {
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN:  if ((lp&(1<<29)) && !(lp&(1<<31)) && wp == VK_F4) {
                         PostQuitMessage(0);
                         return 0;
                       }
  case WM_KEYUP:
  case WM_KEYDOWN:     if (!(lp&(1<<30)) == !!(lp&(1<<31))) return 0;
                       ev = (lp&(1<<31)) ? GR_EV_KEYUP : GR_EV_KEYDOWN;
                       if ((ep = vkmap[wp & 255])) goto event;
                       return 0;
  case WM_CHAR:        ev = GR_EV_KEYCHAR; ep = (u32)wp; goto event;
  case WM_LBUTTONUP:   ep = GR_KEY_M1; goto mup;
  case WM_MBUTTONUP:   ep = GR_KEY_M2; goto mup;
  case WM_RBUTTONUP:   ep = GR_KEY_M3; goto mup;
  case WM_LBUTTONDOWN: ep = GR_KEY_M1; goto mdown;
  case WM_MBUTTONDOWN: ep = GR_KEY_M2; goto mdown;
  case WM_RBUTTONDOWN: ep = GR_KEY_M3; goto mdown;
  case WM_MOUSEWHEEL:  ep = ((int)wp>>16) / WHEEL_DELTA > 0 ? GR_KEY_M4 : GR_KEY_M5;
                       gr_w32_addevent(0, GR_EV_KEYDOWN, ep);
                       gr_w32_addevent(0, GR_EV_KEYUP, ep);
                       return 0;
  case WM_MOUSEMOVE:   ev = GR_EV_MOUSE;
                       ep = (u32)lp;
                       goto event;
  case WM_SIZE:       _ReadWriteBarrier(); gr_w32.winsz = (u32)lp; return 0;
  case WM_ERASEBKGND: return 1;
  case WM_CLOSE:      PostQuitMessage(0); return 0;
  }
  return DefWindowProc(hw, msg, wp, lp);
mup:
  ev = GR_EV_KEYUP; goto event;
mdown:
  ev = GR_EV_KEYDOWN;
event:
  gr_w32_addevent(0, ev, ep);
  return 0;
}

static DWORD WINAPI gr_w32_winloop(void *arg)
{
  struct gr_ctx *gr = arg;
  RECT r = { 0, 0, (LONG)gr->xres, (LONG)gr->yres };
  MSG msg;

  gr_w32.wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
  gr_w32.wc.lpfnWndProc = gr_w32_winproc;
  gr_w32.wc.cbWndExtra = (int)sizeof(struct gr_ctx *);
  gr_w32.wc.hInstance = GetModuleHandle(0);
  gr_w32.wc.hIcon = LoadIcon(0, IDI_APPLICATION);
  gr_w32.wc.hCursor = LoadCursor(0, IDC_ARROW);
  gr_w32.wc.lpszClassName = "gr";
  RegisterClass(&gr_w32.wc);

  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
  r.right -= r.left, r.bottom -= r.top;
  gr_w32.hw = CreateWindow("gr", "gr", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, r.right, r.bottom,
                           0, 0, gr_w32.wc.hInstance, gr);
  _ReadWriteBarrier();
  gr_w32.dc = GetDC(gr_w32.hw);

  while (GetMessage(&msg, 0, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  gr_w32_addevent(gr, GR_EV_WINSHUT, 0);
  return 0;
}

int main(int argc, char **argv)
{
  static BITMAPINFO bi = { sizeof(BITMAPINFOHEADER), 0, 0, 1, 32 };
  static struct gr_ctx gr = {
    .config = gr_w32_config,
    .event = gr_w32_event,
  };
  LARGE_INTEGER tbase, tlast, tnow;
  double invfreq;
  HWND hw;
  HDC dc;
  u32 xwin, ywin;
  int ret;

  if (gr_setup(&gr, argc, argv) != 0)
    return 1;

  bi.bmiHeader.biWidth = (LONG)gr.xres;
  bi.bmiHeader.biHeight = -(LONG)gr.yres;

  CreateThread(0, 0, gr_w32_winloop, &gr, 0, 0);

  QueryPerformanceFrequency(&tnow);
  invfreq = 1.0 / (double)tnow.QuadPart;

  do {
    _mm_pause();
    dc = gr_w32.dc;
    _ReadWriteBarrier();
  } while (!dc);

  hw = gr_w32.hw;
  _ReadWriteBarrier();

  QueryPerformanceCounter(&tbase);
  tlast.QuadPart = 0;
  for (;;) {
    QueryPerformanceCounter(&tnow);
    tnow.QuadPart -= tbase.QuadPart;
    gr.time = invfreq * tnow.QuadPart;
    gr.dt = invfreq * (tnow.QuadPart - tlast.QuadPart);
    tlast = tnow;

    xwin = gr_w32.winsz;
    _ReadWriteBarrier();
    ywin = xwin >> 16;
    xwin &= 0xFFFF;

    gr.xwin = xwin;
    gr.ywin = ywin;

    if ((ret = gr_frame(&gr)) != 0)
      return ret;

    StretchDIBits(dc, 0, 0, xwin, ywin, 0, 0, gr.xres, gr.yres, gr.fb, &bi,
                  DIB_RGB_COLORS, SRCCOPY);
    ValidateRect(hw, 0);
  }
}
#endif
