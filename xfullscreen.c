#include "xfullscreen.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <locale.h>

#include <X11/StringDefs.h>
#include <X11/extensions/xf86vmode.h>

static XF86VidModeModeInfo **videomodes;
static int  	    	     num_videomodes;

static int x11_fullscreen_supported(Display *display)
{
    int majorversion, minorversion;
    int eventbase, errorbase;

    if (!XF86VidModeQueryVersion(display, &majorversion, &minorversion))
    {
	return 0;

    }
    if (!XF86VidModeQueryExtension(display, &eventbase, &errorbase))
    {
	return 0;
    }

    if (XF86VidModeGetAllModeLines(display, DefaultScreen(display), &num_videomodes, &videomodes))
    {
    	if (num_videomodes >= 2) return 1;
    }

    return 0;
}

static void x11_fullscreen_switchmode(Display *display, int *w, int *h)
{
    int i, mode;

    for(i = 1, mode = 0; i < num_videomodes; i++)
    {
    	if ((videomodes[i]->hdisplay >= *w) &&
	    (videomodes[i]->vdisplay >= *h) &&
	    (videomodes[i]->hdisplay < videomodes[mode]->hdisplay) &&
	    (videomodes[i]->vdisplay < videomodes[mode]->vdisplay))
	{
	    mode = i;
	}
    }

    *w = videomodes[mode]->hdisplay;
    *h = videomodes[mode]->vdisplay;

    XF86VidModeSwitchToMode(display, DefaultScreen(display), videomodes[mode]);
    XF86VidModeSetViewPort(display, DefaultScreen(display), 0, 0);
}

int xfullscreen( Widget main, int *ret_width, int *ret_height )
{
  int ret;
  Display *display = XtDisplay(main);
  Screen *screen = XtScreen(main);
  int width = WidthOfScreen (screen);
  int height = HeightOfScreen (screen);
  Window window = XtWindow(main);
  typedef struct {
                unsigned long   flags;
                unsigned long   functions;
                unsigned long   decorations;
                long            inputMode;
                unsigned long   status;
  } Hints;

  Hints   hints;
  Atom    property;

  ret = x11_fullscreen_supported(display);
  if(ret==0) { fprintf(stderr,"fullscreen not supported\n"); return 1; }

  hints.flags = 2;        // Specify that we're changing the window decorations.
  hints.decorations = 0;  // 0 (false) means that window decorations should go bye-bye.
  property = XInternAtom(display,"_MOTIF_WM_HINTS",True);
  XChangeProperty(display,window,property,property,32,PropModeReplace,(unsigned char *)&hints,5);

  x11_fullscreen_switchmode(display, &width, &height );

  if( ret_width )   *ret_width = width;
  if( ret_height )  *ret_height = height;

  XMoveResizeWindow(display,window,0,0,width,height);
  XMapRaised(display,window);
  // XGrabPointer(display,window,True,0,GrabModeAsync,GrabModeAsync,window,0L,CurrentTime);
  // XGrabKeyboard(display,window,False,GrabModeAsync,GrabModeAsync,CurrentTime);
  return 0;
}
