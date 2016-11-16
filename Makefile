wb_install=../wbuild
WBUILD=../wbuild/wbuild
CFLAGS +=-Wall
CPPFLAGS+=-I. -D_GNU_SOURCE
WFLAGS= -d doc -c src -i xtcw --include-prefix=xtcw $(wb_install)/tex.w $(wb_install)/nroff.w
LOADLIBES=-lX11 -lXaw -lXxf86vm -lXft -lXt -lXext -lXmu

ifeq ($(debug),1)
CFLAGS+=-g -O0 -DDEBUG
else
CFLAGS+=-Os
endif


%.h %c: %.widget
	wb_search=../wb $(wb_install)/wb.sh $^ $(WBUILD) $(WFLAGS)

all: xtcal

xtcw/Calib.h src/Calib.c: Calib.widget
	wb_search=../wb $(wb_install)/wb.sh $^ $(WBUILD) $(WFLAGS)

xtcal: src/Calib.o xborderless.o xfullscreen.o

clean:
	-${RM} -r xtcw src doc *.o xtcal
