CC = gcc
CX = gcc


OBJDIR = .obj

LLIB = -lXrender -lX11 -lGL -lm -lcurl -lssl -lcrypto
LINC = -I/usr/include/gtkglext-1.0 -I/usr/lib/gtkglext-1.0/include
LCXFLAGS = `pkg-config gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0 --libs` -pthread -fno-stack-protector -nostartfiles -Wl,--gc-sections
LCFLAGS = `pkg-config gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0 --cflags` -I../core -Wall -ffunction-sections -fdata-sections

LOBJ = $(OBJDIR)/core/core.o $(OBJDIR)/core/gif/gifstd.o $(OBJDIR)/core/ogl/oglstd.o $(OBJDIR)/lib.o
LOUT = ../libengine.so


ELIB = -lengine -lm -lcurl -lssl -lcrypto
EINC =
ECXFLAGS = `pkg-config gtk+-2.0 --libs` -L.. -pthread -fno-stack-protector -nostartfiles -Wl,--gc-sections
ECFLAGS = `pkg-config gtk+-2.0 --cflags` -I../core -Wall -ffunction-sections -fdata-sections

EOBJ = $(OBJDIR)/exec/exec.o $(OBJDIR)/run.o
EOUT = ../linux-gtk


RelJoined: lrelease
RelJoined: erelease
RelJoined: LCXFLAGS += -s -flto -Wl,--build-id=none
RelJoined: jlink
RelJoined: estrip

DbgJoined: ldebug
DbgJoined: edebug
DbgJoined: jlink

RelLibOnly: LCFLAGS += -fPIC
RelLibOnly: lrelease
RelLibOnly: LCXFLAGS += -s -Wl,--build-id=none
RelLibOnly: llink
RelLibOnly: lstrip

DbgLibOnly: LCFLAGS += -fPIC
DbgLibOnly: ldebug
DbgLibOnly: llink

RelSplit: RelLibOnly
RelSplit: erelease
RelSplit: ECXFLAGS += -s -Wl,--build-id=none
RelSplit: elink
RelSplit: estrip

DbgSplit: DbgLibOnly
DbgSplit: edebug
DbgSplit: elink


cleanRelJoined: clean
cleanDbgJoined: clean

cleanRelSplit: clean
cleanDbgSplit: clean

cleanRelLibOnly: clean
cleanDbgLibOnly: clean


lrelease: LCFLAGS += -fexpensive-optimizations -O2 -fvisibility=hidden -fno-unwind-tables -fno-asynchronous-unwind-tables
lrelease: lbuild $(LOBJ)

erelease: ECFLAGS += -fexpensive-optimizations -O2 -fvisibility=hidden -fno-unwind-tables -fno-asynchronous-unwind-tables
erelease: ebuild $(EOBJ)

ldebug: LCFLAGS += -g
ldebug: lbuild $(LOBJ)

edebug: ECFLAGS += -g
edebug: ebuild $(EOBJ)


clean:
	rm -f $(LOUT) $(EOUT)
	rm -rf $(OBJDIR)

lstrip: llink
	strip -R .eh_frame -R .gnu.version -R .comment $(LOUT)
	if type upx >/dev/null 2>&1; then upx -qq $(LOUT); fi

estrip: elink
	strip -R .eh_frame -R .gnu.version -R .comment $(EOUT)
	if type upx >/dev/null 2>&1; then upx -qq $(EOUT); fi

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
$(OBJDIR)/exec/exec.o: ../exec/exec.c
	mkdir -p $(OBJDIR)/exec
	$(CC) $(ECFLAGS) $(EINC) -c ../exec/exec.c -o $(OBJDIR)/exec/exec.o
$(OBJDIR)/run.o: run.c
	mkdir -p $(OBJDIR)
	$(CC) $(ECFLAGS) $(EINC) -c run.c -o $(OBJDIR)/run.o

jlink: ELIB = $(LLIB)
jlink: EOBJ += $(LOBJ)
jlink: ECXFLAGS = $(LCXFLAGS)
jlink: $(LOBJ) $(EOBJ)
jlink: elink $(EOBJ)

llink: LCXFLAGS += -shared
llink: $(LOBJ)
	$(CX) $(LOBJ) $(LCXFLAGS) -o $(LOUT) $(LLIB)

elink: $(EOBJ)
	$(CX) $(EOBJ) $(ECXFLAGS) -o $(EOUT) $(ELIB)
