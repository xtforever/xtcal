wb_config=/etc/X11/wbuild
wb_widgets=/usr/local/include/wbuild_widgets
WBUILD=/usr/local/bin/wbuild
WB=/usr/local/bin/wb.sh

CFLAGS +=-Wall
CPPFLAGS+=-I. -D_GNU_SOURCE
WFLAGS= -d doc -c src -i xtcw --include-prefix=xtcw $(wb_config)/tex.w $(wb_config)/nroff.w
LOADLIBES=-lX11 -lXaw -lXxf86vm -lXft -lXt -lXext -lXmu

ifeq ($(debug),1)
CFLAGS+=-g -O0 -DDEBUG
else
CFLAGS+=-O
endif


%.h %c: %.widget
	wb_search=$(wb_widgets) $(WB) $^ $(WBUILD) $(WFLAGS)

all: xtcal

xtcw/Calib.h src/Calib.c: Calib.widget
	wb_search=$(wb_widgets) $(WB) $^ $(WBUILD) $(WFLAGS)

xtcal: src/Calib.o xborderless.o xfullscreen.o

clean:
	-${RM} -r xtcw src doc *.o xtcal
