MAC_VER = $(shell sw_vers -productVersion | cut -d. -f-2 | tr -d .)
MAC_OLD = $(shell [ $(MAC_VER) -le 107 ] && echo true)

MAC_MIN = 10.6.0

CC = clang -march=core2 -mstackrealign -fno-stack-protector -fvisibility=hidden -fconstant-cfstrings -mmacosx-version-min=$(MAC_MIN)
ifeq ($(MAC_OLD), true)
CC += -no-integrated-as -DMAC_OLD
endif
CX = $(CC) -Wl,-macosx_version_min -Wl,$(MAC_MIN) -lcurl -lcrypto -framework AppKit -headerpad_max_install_names

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

ERES = $(EXEPATH)/../Info.plist $(EXEPATH)/../Resources/main.icns
EOBJ = $(OBJDIR)/exec/exec.o $(OBJDIR)/run.o
EOUT = $(EXEPATH)/DPE


RelJoined32: CC += -m32
RelJoined32: lrelease
RelJoined32: erelease
RelJoined32: jlink
RelJoined32: estrip

DbgJoined32: CC += -m32
DbgJoined32: ldebug
DbgJoined32: edebug
DbgJoined32: jlink

RelLibOnly32: CC += -m32
RelLibOnly32: lrelease
RelLibOnly32: llink
RelLibOnly32: lstrip

DbgLibOnly32: CC += -m32
DbgLibOnly32: ldebug
DbgLibOnly32: llink

RelSplit32: RelLibOnly32
RelSplit32: erelease
RelSplit32: elink
RelSplit32: estrip

DbgSplit32: DbgLibOnly32
DbgSplit32: edebug
DbgSplit32: elink


RelJoined64: CC += -m64
RelJoined64: lrelease
RelJoined64: erelease
RelJoined64: jlink
RelJoined64: estrip

DbgJoined64: CC += -m64
DbgJoined64: ldebug
DbgJoined64: edebug
DbgJoined64: jlink

RelLibOnly64: CC += -m64
RelLibOnly64: lrelease
RelLibOnly64: llink
RelLibOnly64: lstrip

DbgLibOnly64: CC += -m64
DbgLibOnly64: ldebug
DbgLibOnly64: llink

RelSplit64: RelLibOnly64
RelSplit64: erelease
RelSplit64: elink
RelSplit64: estrip

DbgSplit64: DbgLibOnly64
DbgSplit64: edebug
DbgSplit64: elink


cleanRelJoined32: clean
cleanDbgJoined32: clean
cleanRelJoined64: clean
cleanDbgJoined64: clean

cleanRelSplit32: clean
cleanDbgSplit32: clean
cleanRelSplit64: clean
cleanDbgSplit64: clean

cleanRelLibOnly32: clean
cleanDbgLibOnly32: clean
cleanRelLibOnly64: clean
cleanDbgLibOnly64: clean


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

lstrip: llink
	if type strip >/dev/null 2>&1; then strip $(LOUT); fi

estrip: elink
	if type strip >/dev/null 2>&1; then strip $(EOUT); fi

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
$(EXEPATH)/../Resources/main.icns: rsrc/main.icns
	mkdir -p $(EXEPATH)/../Resources
	cp rsrc/main.icns $(EXEPATH)/../Resources
$(EXEPATH)/../Info.plist: rsrc/Info.plist
	mkdir -p $(EXEPATH)
	cp rsrc/Info.plist $(EXEPATH)/..
$(OBJDIR)/exec/exec.o: ../exec/exec.c
	mkdir -p $(OBJDIR)/exec
	$(CC) $(ECFLAGS) $(EINC) -c ../exec/exec.c -o $(OBJDIR)/exec/exec.o
$(OBJDIR)/run.o: run.c
	mkdir -p $(OBJDIR)
	$(CC) $(ECFLAGS) $(EINC) -c run.c -o $(OBJDIR)/run.o

jlink: ELIB = $(LLIB)
jlink: EOBJ += $(LOBJ)
jlink: $(LOBJ) $(EOBJ) $(ERES)
jlink: elink $(EOBJ) $(ERES)

llink: LCXFLAGS += -shared
llink: $(LOBJ)
	$(CX) $(LCXFLAGS) $(LOBJ) -o $(LOUT) $(LLIB)

# the last line is a dirty hack that forces libcurl to use current_version=7.0.0
# and compatibility_version=6.0.0 so that the lib was still usable on MacOS 10.6
# (and in case the corresponding import was not found, nothing happens to EOUT)
elink: $(EOBJ) $(ERES)
	$(CX) $(ECXFLAGS) $(EOBJ) -o $(EOUT) $(ELIB)
	strings -o -t x - $(EOUT) | grep -m1 "/usr/lib/libcurl.4.dylib" | sed "s/ .*/: 0000 0700 0000 0600/" | xxd -s -8 -r - $(EOUT)
