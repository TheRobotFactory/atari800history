#
# Note: x11-nas and xview-nas versions are linked with -lsocket and
#       -lnsl which is required for the Sun versions. Remove them
#       if they cause a problem on your system.
#

CC		= gcc
CPPFLAGS	= $(OTHER)
CFLAGS		= -c -O6 -DGNU_C
LD		= gcc
LDFLAGS		=
LDLIBS		= -lm

PREFIX		= /usr/local
BIN_PATH	= ${PREFIX}/bin
LIB_PATH	= ${PREFIX}/lib
MAN_PATH	= ${PREFIX}/man

default :
	@echo "To build the Atari 800 Emulator, type:"
	@echo "make <version>"
	@echo ""
	@echo "where <version> is one of"
	@echo "  basic"
	@echo "  freebsd-x11 freebsd-x11-shm"
	@echo "  freebsd-xview freebsd-xview-shm"
	@echo "  linux-svgalib linux-svgalib-nas"
	@echo "  linux-x11-nas linux-x11-nas-shm"
	@echo "  linux-xview-nas linux-xview-nas-shm"
	@echo "  x11 x11-shm x11-nas x11-nas-shm"
	@echo "  xview xview-shm xview-nas xview-nas-shm"
	@echo "  motif"
	@echo "  hp9000-ansic-x11"
	@echo "  curses sunos-curses linux-ncurses freebsd-ncurses"
	@echo ""
	@echo "To reconfigure options, type: make config"
	@echo "To clean directory, type: make clean"
	@echo "To install the Emulator, type:"
	@echo ""
	@echo "make install-svgalib"
	@echo "make install"

basic :
	make atari800 \
		CPPFLAGS="-DBASIC" \
		LDLIBS="-lm" \
		OBJ="atari_basic.o"

linux-svgalib :
	make atari800 \
		LDLIBS="-lvgagl -lvga -lm" \
		OBJ="atari_svgalib.o"

linux-svgalib-nas :
	make atari800 \
		OTHER="-DNAS" \
		LDFLAGS="-L/usr/X11/lib" \
		LDLIBS="-lvgagl -lvga -lm -laudio -lXau" \
		OBJ="atari_svgalib.o nas.o"

linux-x11-nas :
	make atari800 \
		CPPFLAGS="-I/usr/openwin/include -DNAS" \
		LDFLAGS="-L/usr/X11/lib -L/usr/openwin/lib" \
		LDLIBS="-laudio -lXau -lX11 -lm" \
		OBJ="atari_x11.o nas.o"

linux-x11-nas-shm :
	make atari800 \
		CPPFLAGS="-I/usr/openwin/include -DNAS -DSHM" \
		LDFLAGS="-L/usr/X11/lib -L/usr/openwin/lib" \
		LDLIBS="-laudio -lXau -lX11 -lXext -lm" \
		OBJ="atari_x11.o nas.o"

linux-xview-nas :
	make atari800 \
		OTHER="-I/usr/openwin/include -DXVIEW -DNAS" \
		LDFLAGS="-L/usr/openwin/lib -L/usr/X11/lib" \
		LDLIBS="-lxview -lolgx -laudio -lXau -lX11 -lm" \
		OBJ="atari_x11.o nas.o"

linux-xview-nas-shm :
	make atari800 \
		OTHER="-I/usr/openwin/include -DXVIEW -DNAS -DSHM" \
		LDFLAGS="-L/usr/openwin/lib -L/usr/X11/lib" \
		LDLIBS="-lxview -lolgx -laudio -lXau -lX11 -lXext -lm" \
		OBJ="atari_x11.o nas.o"

x11 :
	make atari800 \
		CPPFLAGS="-I/usr/openwin/include" \
		LDFLAGS="-L/usr/X11/lib -L/usr/openwin/lib" \
		LDLIBS="-lX11 -lm" \
		OBJ="atari_x11.o"

x11-shm :
	make atari800 \
		CPPFLAGS="-I/usr/openwin/include -DSHM" \
		LDFLAGS="-L/usr/X11/lib -L/usr/openwin/lib" \
		LDLIBS="-lX11 -lXext -lm" \
		OBJ="atari_x11.o"

x11-nas :
	@echo "******************************************************"
	@echo "* Remove -lsocket and -lnsl if they produce an error *"
	@echo "******************************************************"
	make atari800 \
		CPPFLAGS="-I/usr/openwin/include -DNAS" \
		LDFLAGS="-L/usr/X11/lib -L/usr/openwin/lib" \
		LDLIBS="-laudio -lXau -lX11 -lsocket -lnsl -lm" \
		OBJ="atari_x11.o nas.o"

x11-nas-shm :
	@echo "******************************************************"
	@echo "* Remove -lsocket and -lnsl if they produce an error *"
	@echo "******************************************************"
	make atari800 \
		CPPFLAGS="-I/usr/openwin/include -DNAS -DSHM" \
		LDFLAGS="-L/usr/X11/lib -L/usr/openwin/lib" \
		LDLIBS="-laudio -lXau -lX11 -lXext -lsocket -lnsl -lm" \
		OBJ="atari_x11.o nas.o"

hp9000-ansic-x11 :
	make atari800 \
		CC="cc" \
		CPPFLAGS="-D_POSIX_SOURCE" \
		CFLAGS="-c -O -Aa -I/usr/include/X11R5" \
		LD="cc" \
		LDFLAGS="-L/usr/lib/X11R5" \
		LDLIBS="-lX11 -lm" \
		OBJ="atari_x11.o"

xview :
	make atari800 \
		OTHER="-I/usr/openwin/include -DXVIEW" \
		LDFLAGS="-L/usr/openwin/lib -L/usr/X11/lib" \
		LDLIBS="-lxview -lolgx -lX11 -lm" \
		OBJ="atari_x11.o"

xview-shm :
	make atari800 \
		OTHER="-I/usr/openwin/include -DXVIEW -DSHM" \
		LDFLAGS="-L/usr/openwin/lib -L/usr/X11/lib" \
		LDLIBS="-lxview -lolgx -lX11 -lXext -lm" \
		OBJ="atari_x11.o"

xview-nas :
	@echo "******************************************************"
	@echo "* Remove -lsocket and -lnsl if they produce an error *"
	@echo "******************************************************"
	make atari800 \
		OTHER="-I/usr/openwin/include -DXVIEW -DNAS" \
		LDFLAGS="-L/usr/openwin/lib -L/usr/X11/lib" \
		LDLIBS="-lxview -lolgx -laudio -lXau -lX11 -lsocket -lnsl -lm" \
		OBJ="atari_x11.o nas.o"

xview-nas-shm :
	@echo "******************************************************"
	@echo "* Remove -lsocket and -lnsl if they produce an error *"
	@echo "******************************************************"
	make atari800 \
		OTHER="-I/usr/openwin/include -DXVIEW -DNAS -DSHM" \
		LDFLAGS="-L/usr/openwin/lib -L/usr/X11/lib" \
		LDLIBS="-lxview -lolgx -laudio -lXau -lX11 -lXext -lsocket -lnsl -lm" \
		OBJ="atari_x11.o nas.o"

motif :
	make atari800 \
		OTHER="-I/opt/IXImd12s/include -DMOTIF" \
		LDFLAGS="-L/opt/IXImd12s/lib" \
		LDLIBS="-lXm -lXt -lX11 -lgen -lsocket -lm" \
		OBJ="atari_x11.o"

curses :
	make atari800 \
		CPPFLAGS="-DCURSES" \
		LDLIBS="-lcurses -lm" \
		OBJ="atari_curses.o"

sunos-curses :
	make atari800 \
		CPPFLAGS="-I/usr/5include -DCURSES" \
		LDLIBS="-lcurses -lm" \
		LDFLAGS="-L/usr/5lib" \
		OBJ="atari_curses.o"

linux-ncurses :
	make atari800 \
		CPPFLAGS="-I/usr/include/ncurses -DCURSES -DNCURSES" \
		LDLIBS="-lncurses -lm" \
		OBJ="atari_curses.o"

freebsd-ncurses :
	make atari800 \
		CPPFLAGS="-I/usr/include/ncurses -DCURSES -DNCURSES" \
		LDLIBS="-lncurses -lm" \
		OBJ="atari_curses.o"

freebsd-x11 :
	make atari800 \
		CPPFLAGS="-I/usr/X11R6/include" \
		LDFLAGS="-L/usr/X11R6/lib" \
		LDLIBS="-lX11 -lm" \
		OBJ="atari_x11.o"

freebsd-x11-shm :
	make atari800 \
		CPPFLAGS="-I/usr/X11R6/include -DSHM" \
		LDFLAGS="-L/usr/X11R6/lib" \
		LDLIBS="-lX11 -lXext -lm" \
		OBJ="atari_x11.o"

freebsd-xview : 
	make atari800 \
		OTHER="-I/usr/X11R6/include -DXVIEW" \
		LDFLAGS="-L/usr/X11R6/lib" \
		LDLIBS="-lxview -lolgx -lX11 -lm" \
		OBJ="atari_x11.o"

freebsd-xview-shm :
	make atari800 \
		OTHER="-I/usr/X11R6/include -DXVIEW -DSHM" \
		LDFLAGS="-L/usr/X11R6/lib" \
		LDLIBS="-lxview -lolgx -lX11 -lXext -lm" \
		OBJ="atari_x11.o"

#
# ======================================================
# You should not need to modify anything below this here
# ======================================================
#

INCLUDES	=	Makefile \
			config.h \
			cpu.h \
			atari.h \
			colours.h \
			antic.h \
			gtia.h \
			pokey.h \
			pia.h

config config.h	:	configure
	configure

configure	:	configure.o
	$(LD) $(LDFLAGS) configure.o $(LDLIBS) -o configure

configure.o	:	configure.c
	$(CC) $(CPPFLAGS) $(CFLAGS) configure.c

OBJECTS	=	atari.o \
		ffp.o \
		cpu.o \
		monitor.o \
		atari_sio.o \
		devices.o \
		antic.o \
		gtia.o \
		pokey.o \
		pia.o \
		supercart.o

atari800	:	$(OBJECTS) $(OBJ)
	$(LD) $(LDFLAGS) $(OBJECTS) $(OBJ) $(LDLIBS) -o atari800

atari.o		:	atari.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) atari.c

ffp.o		:	ffp.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) ffp.c

cpu.o		:	cpu.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) cpu.c

monitor.o	:	monitor.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) monitor.c

atari_sio.o	:	atari_sio.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) atari_sio.c

devices.o	:	devices.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) devices.c

antic.o		:	antic.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) antic.c

gtia.o		:	gtia.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) gtia.c

pokey.o		:	pokey.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) pokey.c

pia.o		:	pia.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) pia.c

supercart.o	:	supercart.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) supercart.c

atari_x11.o	:	atari_x11.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) atari_x11.c

atari_svgalib.o	:	atari_svgalib.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) atari_svgalib.c

atari_curses.o	:	atari_curses.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) atari_curses.c

atari_amiga.o	:	atari_amiga.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) atari_amiga.c

nas.o		:	nas.c $(INCLUDES)
	$(CC) $(CPPFLAGS) $(CFLAGS) nas.c

clean	:
	rm -f configure
	rm -f config.h
	rm -f core
	rm -f *.o

install-svgalib : install
	chown root.root ${BIN_PATH}/atari800
	chmod 4755 ${BIN_PATH}/atari800

install :
	cp atari800 ${BIN_PATH}/atari800
	cp atari800.man ${MAN_PATH}/man1/atari800.1
