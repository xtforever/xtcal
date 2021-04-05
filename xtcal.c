/*

 ./xtcal -OffsetX 0 -OffsetY 0 -WinX 0 -WinY 0 -WinWidth 500 -WinHeight 500 -geometry 500x500+2000+1000

 
                           y : 0 .. 768 
                           x : 1980 .. 3004
			   
 --------------------      -------------
 | (0,0)            |      | 1024x768   |
 |                  |      |            |
 |                  |      |            |
 |    (1980,1080)   |      ------------- 
 --------------------

                           OffSetX: 1980
			   OffSetY: 0
			   WinX: 1980
			   WinY: 0
			   WinWidth: 1024
			   WinHeight: 768

144 144 2148 1214          
880 144 2906 1119   -->    touch(2906,1119)  == screen( 880+OffSetX, 144+OffSetY)
880 624 2891 1669   
144 624 2145 1669

			   

*/

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>              /* Obtain O_* constant definitions */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xmu/Editres.h>
#include <X11/Vendor.h>
#include <X11/Xaw/XawInit.h>

#include "xtcw/Calib.h"
#include "xfullscreen.h"
#include "xborderless.h"

#ifdef DEBUG
#define pr_mx(a,b,c) dpr_mx(a,b,c)
#define print_matrix(a,b,c,d) dprint_matrix(a,b,c,d)
#define trace1(a,b...) fprintf(stderr,a, ## b )
#else
#define trace1(a,b...) do {} while(0)
#define pr_mx(a,b,c) do {} while(0)
#define print_matrix(a,b,c,d) do {} while(0)
#endif

int x_width, x_height;

Widget TopLevel;


char* fallback_resources[] = {
    "*calib.foreground: White",
    "*calib.highlight:  Red",
    "*calib.lineWidth: 10",
    "*calib.background: Lightblue",
    NULL
};

static XrmOptionDescRec options[] = {
    { "-OffsetX",	"*offsetX",	XrmoptionSepArg, NULL },
    { "-OffsetY",	"*offsetY",	XrmoptionSepArg, NULL },
    { "-WinX",          "*winX", 	XrmoptionSepArg, NULL },
    { "-WinY",          "*winY", 	XrmoptionSepArg, NULL },
    { "-WinWidth",      "*winWidth", 	XrmoptionSepArg, NULL },
    { "-WinHeight",     "*winHeight", 	XrmoptionSepArg, NULL }
};



struct CALIB_CONF_st {
    int offsetX, offsetY, winX, winY, winWidth,winHeight;
};

typedef struct CALIB_CONF_st CALIB_CONF_t;

CALIB_CONF_t CALIB_CONF;

#define FLD(n)  XtOffsetOf(CALIB_CONF_t,n)
static XtResource CALIB_CONF_RES [] = {
  { "offsetX", "OffsetX", XtRInt, sizeof(int),
   FLD(offsetX), XtRImmediate, 0
  },
  { "offsetY", "OffsetY", XtRInt, sizeof(int),
   FLD(offsetY), XtRImmediate, 0
  },
  { "winX", "WinX", XtRInt, sizeof(int),
   FLD(winX), XtRImmediate, 0
  },
  { "winY", "WinY", XtRInt, sizeof(int),
   FLD(winY), XtRImmediate, 0
  },
  { "winWidth", "WinWidth", XtRInt, sizeof(int),
   FLD(winWidth), XtRImmediate, 0
  },
  { "winHeight", "WinHeight", XtRInt, sizeof(int),
   FLD(winHeight), XtRImmediate, 0
  }
};
#undef FLD



void quit_gui( Widget w, void *u, void *c )
{
    XtAppSetExitFlag( XtWidgetToApplicationContext(w) );
}

static void wm_quit ( Widget w, XEvent *event, String *params,
		   Cardinal *num_params )
{
    XtAppSetExitFlag( XtWidgetToApplicationContext(w) );
}


/* --------------------------------------------------------------------------------------------------------------------

                        IMPLEMENTATION

  -------------------------------------------------------------------------------------------------------------------- */

void dpr_mx(void *B, int rows, int cols)
{
    double *b = B;
    int i,j;

    for( i=0;i<rows;i++) {
        for(j=0;j<cols;j++)
            printf("%c%5.2f", j==cols-1 ? '|' : ' ', *b++);
        printf("\n");
    }

    for(i=0;i<cols;i++)
        printf("------");
    printf("\n\n");
}

/* gauss solver:
   B   : matrix [ eq ] [eq+1]
   res : vector [ eq ]
*/
void gauss1(void *b, void *res, int eq )
{
#define MX(a,y,x) (((double*)a)[ (x) + (y) * (n+1)])
    int i,n,k,j;
    double t,temp;
    double *B = b;
    double *a = res;

    // n=-0.5 + sqrt(0.25+m_len(b));
    n=eq;
    pr_mx( B,n,n+1);

    for (i=0;i<n;i++)
        for (k=i+1;k<n;k++)
            if ( MX(B,i,i) < MX( B,k,i) )
                for (j=0;j<=n;j++)
                {
                    temp=MX(B,i,j);
                    MX(B,i,j) = MX(B,k,j);
                    MX(B,k,j) = temp;
                }

    pr_mx(B,3,4);

    /* gauss elimination */
    for (i=0;i<n-1;i++) {

        for (k=i+1;k<n;k++)
            {
                t=MX(B,k,i)/MX(B,i,i);

                /* make the elements below the pivot elements
                   equal to zero or elimnate the variables
                */
                for (j=0;j<=n;j++)
                    MX(B,k,j)=MX(B,k,j) - t * MX(B,i,j);

                pr_mx(B,3,4);
            }


    }


    for (i=n-1;i>=0;i--)        // back-substitution
        {
            a[i]=MX(B,i,n); /* make the variable to be calculated equal
                             to the rhs of the last equation */
            for (j=0;j<n;j++)
                if (j!=i) /* then subtract all the lhs values except
                             the coefficient of the
                             variable whose value is being calculated */
                    a[i]=a[i]-MX(B,i,j)*a[j];
            a[i]=a[i]/MX(B,i,i); /* now finally divide the rhs by the coefficient
                                  of the variable to be calculated */
        }

    pr_mx(a,1,3);
    #undef MX
}



void dprint_matrix(char *name,
                  void *B, int rows, int cols)
{
    double *b = B;
    int i,j;

    printf("------------- %s ----------------------------\n", name );

    for( i=0;i<rows;i++) {
        for(j=0;j<cols;j++)
            printf("%c%5.2f", ' ', *b++);
        printf("\n");
    }

    for(i=0;i<cols;i++)
        printf("------");
    printf("\n\n");
}


struct pt {
    int xp,yp, /* disp */
        xt,yt; /* touch */

};


/* calculate the 3x3 Coordinate Transformation Matrix
   points: array[cnt] of (int screen_x, screen_y, touch_x, touch_y)
   cnt - number of points (4-tuple)
   width   - desktop/screen width in number of pixels
   height  - desktop/screen height in number of pixels

   
   CTM       Pointer      Screen    
   |a b c|   | px |       | sx |  
   |d e f| * | py |   =   | sy | 
   |0 0 1|   |  1 |       |  1 |
   

   
     a * px0 + b * py0 + c = sx0
     a * px1 + b * py1 + c = sx1
     a * px2 + b * py2 + c = sx2
     a * px3 + b * py3 + c = sx3

     px0 py0 1   a    sx0
     px1 py1 1   b    sx1
     px2 py2 1   c    sx2
     px3 py3 1        sx3

    // d * px + e * py + f = sy
    

*/
void touch_cal(struct pt *points, int cnt, int width, int height)
{
    struct pt *p;
    /* normieren */
    double xy[ 4 * cnt ];
    int i; int t;
    for( i=0; i<cnt; i++) {
        p=points+i;
        t=i*4;
        xy[0+t] = p->xt * 1.0 / width;
        xy[1+t] = p->yt * 1.0 / height;
        xy[2+t] = p->xp * 1.0 / width;
        xy[3+t] = p->yp * 1.0 / height;
    }

    print_matrix( "XY", xy, cnt, 4 );

    double ata[9] = { 0 }, atx[3] = { 0 }, aty[3] = { 0 };

    for( i=0; i<cnt; i++ ) {
        double *xy1 = xy + 4 * i;
        ata[0] += xy1[0] * xy1[0]; /* sigma xt^2 */
        ata[1] += xy1[0] * xy1[1];  /* sigma xt*yt */
        ata[2] += xy1[0];          /* sigma xt */
        ata[4] += xy1[1] * xy1[1]; /* sigma yt^2 */
        ata[5] += xy1[1];          /* sigma yt */

        atx[0] += xy1[0]*xy1[2]; /* sigma xt*xp */
        atx[1] += xy1[1]*xy1[2]; /* sigma yt*xp */
        atx[2] += xy1[2];        /* sigma xp */

        aty[0] += xy1[0]*xy1[3]; /* sigma xt*yp */
        aty[1] += xy1[1]*xy1[3]; /* sigma yt*yp */
        aty[2] += xy1[3];        /* sigma yp */
    }

    ata[3] = ata[1];
    ata[6] = ata[2];
    ata[7] = ata[5];
    ata[8] = cnt;


    print_matrix( "ata", ata, 3,3 );
    print_matrix( "atx", atx, 3,1 );
    print_matrix( "aty", aty, 3,1 );

    // abc
    double bgauss1[ 4 * 3 ], bgauss2[ 4 * 3 ];
    for(i=0;i<3;i++) {
        double *b = bgauss1 + i*4;
        double *a = ata     + i*3;
        double *x = atx + i;
        b[0] = a[0]; b[1] = a[1]; b[2] = a[2]; b[3] = x[0];
    }
    // def
    for(i=0;i<3;i++) {
        double *b = bgauss2 + i*4;
        double *a = ata     + i*3;
        double *x = aty + i;
        b[0] = a[0]; b[1] = a[1]; b[2] = a[2]; b[3] = x[0];
    }

    print_matrix( "ata\\atx", bgauss1, 3,4 );
    print_matrix( "ata\\aty", bgauss2, 3,4 );

    double abc[3], def[3];

    gauss1(bgauss1, abc, 3 );
    gauss1(bgauss2, def, 3 );

    print_matrix( "abc", abc, 3,1 );
    print_matrix( "def", def, 3,1 );

    for(i=0;i<3;i++)
        printf("%f ", abc[i] );
    for(i=0;i<3;i++)
        printf("%f ", def[i] );
    printf( "0 0 1\n");
}

// c : array of 4 points (x,y,x1,y1)
void dump_cal_data( cal_point *c )
{
#ifdef DEBUG    
    puts("calibration data\n  WinX   WinY\t TouchX TouchY");
    for(int i=0;i<4;i++) {
        printf("%6d %6d \t %6d %6d\n", c->x0, c->y0, c->x1, c->y1 );
        c++;
    }
#endif
}

void process_cal_data( Widget w, void *u, void *p )
{
    cal_point *c = p;    

    trace1("offsetX: %d\n", CALIB_CONF.offsetX );
    trace1("offsetY: %d\n", CALIB_CONF.offsetY );
    trace1("width: %d\n", CALIB_CONF.winWidth );
    trace1("height: %d\n", CALIB_CONF.winHeight );
    dump_cal_data(c);
    
    /* translate window coordinates to target-screen coordinates */
    for(int i=0;i < 4; i++ ) {
	c[i].x0 += CALIB_CONF.offsetX;
	c[i].y0 += CALIB_CONF.offsetY;	
    }

    trace1("translated screen/touch coordinates");
    dump_cal_data(c);

    /* calculate the CTM and exit */ 
    touch_cal( p,4, CALIB_CONF.winWidth, CALIB_CONF.winHeight ); 
    XtAppSetExitFlag( XtWidgetToApplicationContext(w) );
}



/******************************************************************************
**  Private Functions
******************************************************************************/

/* if the window manager closes the window, tell the
   window manager to send a message to xt. xt will
   call the wm_quit callback.
*/
void grab_window_quit(Widget top)
{
    XtAppContext app = XtWidgetToApplicationContext(top);
    /* if the user closes the window
       the Window Manager will call our quit() function
    */
    static XtActionsRec actions[] = {
        {"quit",	wm_quit}
    };
    XtAppAddActions
	(app, actions, XtNumber(actions));
    XtOverrideTranslations
	(TopLevel, XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));

    /* http://www.lemoda.net/c/xlib-wmclose/ */
    static Atom wm_delete_window;
    wm_delete_window = XInternAtom(XtDisplay(top), "WM_DELETE_WINDOW",
				   False);
    (void) XSetWMProtocols (XtDisplay(top), XtWindow(top),
                            &wm_delete_window, 1);
}






/******************************************************************************
*   MAIN function
******************************************************************************/
int main ( int argc, char **argv )
{


    XtAppContext app;
    Widget appShell,w;
    XtSetLanguageProc (NULL, NULL, NULL);
    XawInitializeWidgetSet();
    /*  -- Intialize Toolkit creating the application shell
     */
    appShell = XtOpenApplication (&app, argv[0],
				  options, XtNumber(options),
				  &argc, argv,
				  fallback_resources,
				  sessionShellWidgetClass,
				  NULL, 0
				  );
    TopLevel = appShell;
    w = XtVaCreateManagedWidget( "calib", calibWidgetClass, TopLevel,
                             NULL );
    XtAddCallback(w, XtNcallback, process_cal_data, NULL );

    /* enable Editres support */
    XtAddEventHandler(appShell, (EventMask) 0, True, _XEditResCheckMessages, NULL);
    /* grab window close event */
    XtAddCallback( appShell, XtNdieCallback, quit_gui, NULL );


    /*  -- Realize the widget tree and enter the main application loop
     */
    XtRealizeWidget ( appShell );
    grab_window_quit( appShell );
    make_borderless_window(appShell);

    /*  -- Get application resources and widget ptrs
     */
    XtGetApplicationResources(	appShell, (XtPointer)&CALIB_CONF,
				CALIB_CONF_RES,
				XtNumber(CALIB_CONF_RES),
				(ArgList)0, 0 );

    

    
/*    if( xfullscreen(appShell, &x_width, &x_height) ) */
        {

            Dimension xw,xh;
            XtVaGetValues(appShell, "height", &xh, "width", &xw, NULL );
            x_width = xw; x_height = xh;
        }


    XtAppMainLoop ( app ); /* use XtAppSetExitFlag */
    XtDestroyWidget(appShell);
    return EXIT_SUCCESS;
}
