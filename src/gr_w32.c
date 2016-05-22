#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "gr.h"

typedef int (*gr_setup_t)(struct gr_ctx *, int, char **);
typedef int (*gr_frame_t)(struct gr_ctx *);

struct gr_w32_ctx {
  struct gr_ctx;
  HMODULE  dll;
  int    (*setup)(struct gr_ctx *, int, char **);
  int    (*frame)(struct gr_ctx *);
  WNDCLASS wc;
  HWND     hw;
  HDC      dc;
  u32      winsz;
  u32      qhead;
  u32      qdata[256][2];
  u32      qtail;
};

static void gr_w32_wintext(struct gr_ctx *gr, const char *text)
{
  HWND hw = ((struct gr_w32_ctx *)gr)->hw;
  _ReadWriteBarrier();
  SetWindowTextA(hw, text);
}

static void gr_w32_winsize(struct gr_ctx *gr, u32 x, u32 y)
{
  HWND hw = ((struct gr_w32_ctx *)gr)->hw;
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

  gr_w32_winsize(gr, xres, yres);
  gr_w32_wintext(gr, title);
}

static int gr_w32_addevent(struct gr_ctx *gr, u32 ev, u32 ep)
{
  struct gr_w32_ctx *wgr = (struct gr_w32_ctx *)gr;
  u32 head = wgr->qhead;
  u32 tail = wgr->qtail;
  u32 ntail = (tail+1) & 255;

  _ReadWriteBarrier();
  if (ntail == head) return 0;
  wgr->qdata[tail][0] = ev;
  wgr->qdata[tail][1] = ep;
  _ReadWriteBarrier();
  wgr->qtail = ntail;
  return 1;
}

static u32 gr_w32_event(struct gr_ctx *gr, u32 *ep)
{
  struct gr_w32_ctx *w32 = (struct gr_w32_ctx *)gr;
  u32 head = w32->qhead;
  u32 tail = w32->qtail;
  u32 ev;

  _ReadWriteBarrier();
  if (head == tail) return 0;
  ev  = w32->qdata[head][0];
  *ep = w32->qdata[head][1];
  _ReadWriteBarrier();
  w32->qhead = (head+1) & 255;
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
  static struct gr_w32_ctx *wgr;
  u32 ev=0, ep=0;

  switch (msg) {
  case WM_NCCREATE:    wgr = ((CREATESTRUCT *)lp)->lpCreateParams;
                       return TRUE;
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
                       gr_w32_addevent((struct gr_ctx *)wgr, GR_EV_KEYDOWN, ep);
                       gr_w32_addevent((struct gr_ctx *)wgr, GR_EV_KEYUP, ep);
                       return 0;
  case WM_MOUSEMOVE:   ev = GR_EV_MOUSE;
                       ep = (u32)lp;
                       goto event;
  case WM_SIZE:       _ReadWriteBarrier(); wgr->winsz = (u32)lp; return 0;
  case WM_ERASEBKGND: return 1;
  case WM_CLOSE:      PostQuitMessage(0); return 0;
  }
  return DefWindowProc(hw, msg, wp, lp);
mup:
  ev = GR_EV_KEYUP; goto event;
mdown:
  ev = GR_EV_KEYDOWN;
event:
  gr_w32_addevent((struct gr_ctx *)wgr, ev, ep);
  return 0;
}

static DWORD WINAPI gr_w32_winloop(void *arg)
{
  struct gr_w32_ctx *wgr = arg;
  MSG msg;

  wgr->wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
  wgr->wc.lpfnWndProc = gr_w32_winproc;
  wgr->wc.cbWndExtra = (int)sizeof(struct gr_w32_ctx *);
  wgr->wc.hInstance = GetModuleHandle(0);
  wgr->wc.hIcon = LoadIcon(0, IDI_APPLICATION);
  wgr->wc.hCursor = LoadCursor(0, IDC_ARROW);
  wgr->wc.lpszClassName = "gr";
  RegisterClass(&wgr->wc);

  wgr->hw = CreateWindow("gr", "gr", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                         CW_USEDEFAULT, CW_USEDEFAULT, 320, 200,
                         0, 0, wgr->wc.hInstance, wgr);
  _ReadWriteBarrier();
  wgr->dc = GetDC(wgr->hw);

  while (GetMessage(&msg, 0, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  gr_w32_addevent((struct gr_ctx *)wgr, GR_EV_WINSHUT, 0);
  return 0;
}

static void gr_w32_getpaths(const char *dllname, char *dllfile, char *tmpfile)
{
  DWORD n = GetModuleFileNameA(GetModuleHandle(0), dllfile, MAX_PATH);
  dllfile[MAX_PATH-1] = 0;
  while (n > 1 && dllfile[n] != '\\') --n;
  dllfile[n] = 0;
  GetTempFileNameA(dllfile, "gr", 0, tmpfile);
  dllfile[n++] = '\\';
  while (*dllname && n < MAX_PATH-1) dllfile[n++] = *dllname++;
  dllfile[n] = 0;
}

static void gr_w32_reload(struct gr_w32_ctx *wgr, const char *dllfile,
                          const char *tmpfile)
{
  static FILETIME savedtime;
  WIN32_FIND_DATA fd;
  HANDLE h;

  if (GetFileAttributes("pdb.lock") != INVALID_FILE_ATTRIBUTES) return;
  if ((h = FindFirstFile(dllfile, &fd)) == INVALID_HANDLE_VALUE) return;

  if (CompareFileTime(&fd.ftLastWriteTime, &savedtime) > 0) {
    printf("reloading: %s\n", dllfile);
    savedtime = fd.ftLastWriteTime;
    if (wgr->dll)
      FreeLibrary(wgr->dll);
    CopyFile(dllfile, tmpfile, 0);
    wgr->dll = LoadLibrary(tmpfile);
    wgr->setup = (gr_setup_t)GetProcAddress(wgr->dll, "gr_setup");
    wgr->frame = (gr_frame_t)GetProcAddress(wgr->dll, "gr_frame");
  }
  FindClose(h);
}

int main(int argc, char **argv)
{
  static BITMAPINFO bi = { sizeof(BITMAPINFOHEADER), 0, 0, 1, 32 };
  static struct gr_w32_ctx wgr = {
    .config = gr_w32_config,
    .event = gr_w32_event,
    .winsize = gr_w32_winsize
  };
  LARGE_INTEGER tbase, tlast, tnow;
  double invfreq;
  HWND hw;
  HDC dc;
  u32 xwin, ywin;
  int ret = 0;
  char dllfile[MAX_PATH];
  char tmpfile[MAX_PATH];

  if (argc < 2) {
    puts("usage: dllfile");
    return 1;
  }

  gr_w32_getpaths(argv[1], dllfile, tmpfile);
  gr_w32_reload(&wgr, dllfile, tmpfile);

  CreateThread(0, 0, gr_w32_winloop, &wgr, 0, 0);

  QueryPerformanceFrequency(&tnow);
  invfreq = 1.0 / (double)tnow.QuadPart;

  do {
    _mm_pause();
    dc = wgr.dc;
    _ReadWriteBarrier();
  } while (!dc);

  hw = wgr.hw;
  _ReadWriteBarrier();

  if ((ret = wgr.setup((struct gr_ctx *)&wgr, argc, argv)) != 0)
    return ret;

  bi.bmiHeader.biWidth = (LONG)wgr.xres;
  bi.bmiHeader.biHeight = -(LONG)wgr.yres;

  QueryPerformanceCounter(&tbase);
  tlast.QuadPart = 0;
  for (;;) {
    QueryPerformanceCounter(&tnow);
    tnow.QuadPart -= tbase.QuadPart;
    wgr.time = invfreq * tnow.QuadPart;
    wgr.dt = invfreq * (tnow.QuadPart - tlast.QuadPart);
    tlast = tnow;

    xwin = wgr.winsz;
    _ReadWriteBarrier();
    ywin = xwin >> 16;
    xwin &= 0xFFFF;

    wgr.xwin = xwin;
    wgr.ywin = ywin;

    gr_w32_reload(&wgr, dllfile, tmpfile);

    if ((ret = wgr.frame((struct gr_ctx *)&wgr)) != 0)
      return ret;

    StretchDIBits(dc, 0, 0, xwin, ywin, 0, 0, wgr.xres, wgr.yres, wgr.fb, &bi,
                  DIB_RGB_COLORS, SRCCOPY);
    ValidateRect(hw, 0);
  }
}
