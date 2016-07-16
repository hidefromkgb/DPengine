CC = gcc
CX = gcc


OBJDIR = .obj
EXEPATH = ../macos.app/Contents/MacOS

LLIB = -framework AppKit -framework OpenGL
LINC =
LCXFLAGS =
LCFLAGS = -I../core -Wall -fPIC -fvisibility=hidden

LOBJ = $(OBJDIR)/core/core.o $(OBJDIR)/core/gif/common.o $(OBJDIR)/core/gif/gifstd.o $(OBJDIR)/core/ogl/common.o $(OBJDIR)/core/ogl/oglstd.o $(OBJDIR)/lib.o
LOUT = ../libengine.dylib


ELIB = -lengine -framework AppKit
EINC =
ECXFLAGS = -L..
ECFLAGS = -I../core -Wall

EOBJ = $(OBJDIR)/exec/exec.o $(OBJDIR)/run.o
EOUT = $(EXEPATH)/DPE


RelJoined: lrelease
RelJoined: erelease
RelJoined: jlink

DbgJoined: ldebug
DbgJoined: edebug
DbgJoined: jlink

RelLibOnly: lrelease
RelLibOnly: llink

DbgLibOnly: ldebug
DbgLibOnly: llink

RelSplit: RelLibOnly
RelSplit: erelease
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


lrelease: LCFLAGS += -O2
lrelease: lbuild $(LOBJ)

erelease: ECFLAGS += -O2
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
	if ! [ -h $(EXEPATH)/Ponies ]; then ln -s ../../../Ponies $(EXEPATH)/Ponies; fi

llink: LCXFLAGS += -shared
llink:
	$(CX) $(LCXFLAGS) $(LOBJ) -o $(LOUT) $(LLIB)

elink:
	$(CX) $(ECXFLAGS) $(EOBJ) -o $(EOUT) $(ELIB)
