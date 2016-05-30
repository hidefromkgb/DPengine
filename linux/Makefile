CC = gcc
CX = g++


OBJDIR = .obj

LLIB = -lXrender -lX11 -lGL
LINC = -I/usr/include/gtkglext-1.0 -I/usr/lib/gtkglext-1.0/include
LCXFLAGS = `pkg-config gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0 --libs` -pthread
LCFLAGS = `pkg-config gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0 --cflags` -I../core -Wall -fPIC -fvisibility=hidden

LOBJ = $(OBJDIR)/core/core.o $(OBJDIR)/core/gif/common.o $(OBJDIR)/core/gif/gifstd.o $(OBJDIR)/core/ogl/common.o $(OBJDIR)/core/ogl/oglstd.o $(OBJDIR)/lib.o
LOUT = ../libengine.so


ELIB = -lengine
EINC =
ECXFLAGS = `pkg-config gtk+-2.0 --libs` -L..
ECFLAGS = `pkg-config gtk+-2.0 --cflags` -I../core -Wall

EOBJ = $(OBJDIR)/exec/exec.o $(OBJDIR)/run.o
EOUT = ../linux-gtk


RelJoined: lrelease
RelJoined: erelease
RelJoined: LCXFLAGS += -s
RelJoined: jlink

DbgJoined: ldebug
DbgJoined: edebug
DbgJoined: jlink

RelLibOnly: lrelease
RelLibOnly: LCXFLAGS += -s
RelLibOnly: llink

DbgLibOnly: ldebug
DbgLibOnly: llink

RelSplit: RelLibOnly
RelSplit: erelease
RelSplit: ECXFLAGS += -s
RelSplit: elink

DbgSplit: DbgLibOnly
DbgSplit: edebug
DbgSplit: elink


cleanRelJoined: clean
cleanDbgJoined: clean

cleanRelSplit: clean
cleanDbgSplit: clean

cleanRelLibOnly: clean
cleanDbgLibOnly: clean


lrelease: LCFLAGS += -fexpensive-optimizations -O2
lrelease: lbuild $(LOBJ)

erelease: ECFLAGS += -fexpensive-optimizations -O2
erelease: ebuild $(EOBJ)

ldebug: LCFLAGS += -g
ldebug: lbuild $(LOBJ)

edebug: ECFLAGS += -g
edebug: ebuild $(EOBJ)


clean:
	rm -f $(LOUT) $(EOUT)
	rm -rf $(OBJDIR)

lbuild:
	mkdir -p $(OBJDIR)/core/gif
	mkdir -p $(OBJDIR)/core/ogl
$(OBJDIR)/core/core.o: ../core/core.c
	$(CC) $(LCFLAGS) $(LINC) -c ../core/core.c -o $(OBJDIR)/core/core.o
$(OBJDIR)/core/gif/common.o: ../core/gif/common.c
	$(CC) $(LCFLAGS) $(LINC) -c ../core/gif/common.c -o $(OBJDIR)/core/gif/common.o
$(OBJDIR)/core/gif/gifstd.o: ../core/gif/gifstd.c
	$(CC) $(LCFLAGS) $(LINC) -c ../core/gif/gifstd.c -o $(OBJDIR)/core/gif/gifstd.o
$(OBJDIR)/core/ogl/common.o: ../core/ogl/common.c
	$(CC) $(LCFLAGS) $(LINC) -c ../core/ogl/common.c -o $(OBJDIR)/core/ogl/common.o
$(OBJDIR)/core/ogl/oglstd.o: ../core/ogl/oglstd.c
	$(CC) $(LCFLAGS) $(LINC) -c ../core/ogl/oglstd.c -o $(OBJDIR)/core/ogl/oglstd.o
$(OBJDIR)/lib.o: lib.c
	$(CC) $(LCFLAGS) $(LINC) -c lib.c -o $(OBJDIR)/lib.o

ebuild:
	mkdir -p $(OBJDIR)/exec
$(OBJDIR)/exec/exec.o: ../exec/exec.c
	$(CC) $(ECFLAGS) $(EINC) -c ../exec/exec.c -o $(OBJDIR)/exec/exec.o
$(OBJDIR)/run.o: run.c
	$(CC) $(ECFLAGS) $(EINC) -c run.c -o $(OBJDIR)/run.o

jlink:
	$(CX) $(LCXFLAGS) $(LOBJ) $(EOBJ) -o $(EOUT) $(LLIB)
	if type upx &> /dev/null; then upx -qq $(EOUT); fi

llink: LCXFLAGS += -shared
llink:
	$(CX) $(LCXFLAGS) $(LOBJ) -o $(LOUT) $(LLIB)

elink:
	$(CX) $(ECXFLAGS) $(EOBJ) -o $(EOUT) $(ELIB)
	if type upx &> /dev/null; then upx -qq $(EOUT) $(LOUT); fi
