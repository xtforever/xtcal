#ifndef XBORDERLESS_JH
#define XBORDERLESS_JH

#include <X11/Intrinsic.h>

/* window will stay on top of others */
void make_stay_above(Widget top);

/* remove decoration a.k.a window borders */
void make_borderless_window(Widget top );

/* move window with Mouse-Button 1 pressed,
   kill window with Mouse-Button 2 */
void add_winmove_translations(Widget w);

#endif
