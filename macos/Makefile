MAC_VER = $(shell sw_vers -productVersion | cut -d. -f-2 | tr -d .)
MAC_OLD = $(shell [ $(MAC_VER) -le 107 ] && echo true)

CC = clang -m32 -mstackrealign -fno-stack-protector -fvisibility=hidden -fconstant-cfstrings
ifeq ($(MAC_OLD), true)
CC += -no-integrated-as -DMAC_OLD
endif
CX = $(CC) -lcurl -lcrypto -framework AppKit

OBJDIR = .obj
EXEBASE = ../macos.app
EXEPATH = $(EXEBASE)/Contents/MacOS

LLIB = -framework OpenGL
LINC =
LCXFLAGS =
LCFLAGS = -I../core -Wall -fPIC

LOBJ = $(OBJDIR)/core/core.o $(OBJDIR)/core/gif/gifstd.o $(OBJDIR)/core/ogl/oglstd.o $(OBJDIR)/lib.o
LOUT = ../libengine.dylib


ELIB = -lengine
EINC =
ECXFLAGS = -L..
ECFLAGS = -I../core -Wall

EOBJ = $(OBJDIR)/exec/exec.o $(OBJDIR)/run.o
EOUT = $(EXEPATH)/DPE


RelJoined: lrelease
RelJoined: erelease
RelJoined: jlink
RelJoined: strip

DbgJoined: ldebug
DbgJoined: edebug
DbgJoined: jlink

RelLibOnly: lrelease
RelLibOnly: llink
RelLibOnly: strip

DbgLibOnly: ldebug
DbgLibOnly: llink

RelSplit: RelLibOnly
RelSplit: erelease
RelSplit: elink
RelSplit: strip

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
	rm -rf $(OBJDIR) $(EXEBASE)

strip:
	if type strip >/dev/null 2>&1 && [ -f $(EOUT) ]; then strip -x $(EOUT); fi
	if type strip >/dev/null 2>&1 && [ -f $(LOUT) ]; then strip -x $(LOUT); fi

lbuild:
$(OBJDIR)/core/core.o: ../core/core.c
	mkdir -p $(OBJDIR)/core
	$(CC) $(LCFLAGS) $(LINC) -c ../core/core.c -o $(OBJDIR)/core/core.o
$(OBJDIR)/core/gif/gifstd.o: ../core/gif/gifstd.c
	mkdir -p $(OBJDIR)/core/gif
	$(CC) $(LCFLAGS) $(LINC) -c ../core/gif/gifstd.c -o $(OBJDIR)/core/gif/gifstd.o
$(OBJDIR)/core/ogl/oglstd.o: ../core/ogl/oglstd.c
	mkdir -p $(OBJDIR)/core/ogl
	$(CC) $(LCFLAGS) $(LINC) -c ../core/ogl/oglstd.c -o $(OBJDIR)/core/ogl/oglstd.o
$(OBJDIR)/lib.o: lib.c
	mkdir -p $(OBJDIR)
	$(CC) $(LCFLAGS) $(LINC) -c lib.c -o $(OBJDIR)/lib.o

ebuild:
	mkdir -p $(EXEPATH) $(EXEPATH)/../Resources
	cp rsrc/main.icns $(EXEPATH)/../Resources
	cp rsrc/Info.plist $(EXEPATH)/..
$(OBJDIR)/exec/exec.o: ../exec/exec.c
	mkdir -p $(OBJDIR)/exec
	$(CC) $(ECFLAGS) $(EINC) -c ../exec/exec.c -o $(OBJDIR)/exec/exec.o
$(OBJDIR)/run.o: run.c
	mkdir -p $(OBJDIR)
	$(CC) $(ECFLAGS) $(EINC) -c run.c -o $(OBJDIR)/run.o

jlink: $(LOBJ) $(EOBJ) $(EXEPATH)/../Info.plist $(EXEPATH)/../Resources/main.icns
	$(CX) $(ECXFLAGS) $(LOBJ) $(EOBJ) -o $(EOUT) $(LLIB)

llink: LCXFLAGS += -shared
llink: $(LOBJ)
	$(CX) $(LCXFLAGS) $(LOBJ) -o $(LOUT) $(LLIB)

elink: $(EOBJ) $(EXEPATH)/../Info.plist $(EXEPATH)/../Resources/main.icns
	$(CX) $(ECXFLAGS) $(EOBJ) -o $(EOUT) $(ELIB)
