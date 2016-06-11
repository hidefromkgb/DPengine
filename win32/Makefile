CC = gcc
CX = gcc
RC = windres


OBJDIR = .obj

RCFLAGS = -J rc -O coff
CRTLIBS = -lmsvcrt -lkernel32 -luser32 -lshell32 -lgdi32 -lcomctl32
CRTFUNC = -Dstrdup=_strdup -Dwcsdup=_wcsdup -Dvsnprintf=_vsnprintf

LLIB = $(CRTLIBS) -lopengl32 -lwinmm
LINC =
LCXFLAGS =
LCFLAGS = -I..\core -mno-stack-arg-probe -Wall -march=i486 -DLIB_MAKE $(CRTFUNC)

LOBJ = $(OBJDIR)\core\core.o $(OBJDIR)\core\gif\common.o $(OBJDIR)\core\gif\gifstd.o $(OBJDIR)\core\ogl\common.o $(OBJDIR)\core\ogl\oglstd.o $(OBJDIR)\lib.o
LOUT = ..\engine.dll


ELIB = $(CRTLIBS) -lengine
EINC =
ECXFLAGS = -L.. -Wl,--subsystem,windows -nostdlib -e_WinMain@16
ECFLAGS = -I..\core -mno-stack-arg-probe -Wall $(CRTFUNC)

EOBJ = $(OBJDIR)\rsrc\run.o $(OBJDIR)\exec\exec.o $(OBJDIR)\run.o
EOUT = ..\win32.exe


RelJoined: LCFLAGS += -DLIB_NONE
RelJoined: ECFLAGS += -DLIB_NONE
RelJoined: lrelease
RelJoined: erelease
RelJoined: ECXFLAGS += -s
RelJoined: jlink

DbgJoined: LCFLAGS += -DLIB_NONE
DbgJoined: ECFLAGS += -DLIB_NONE
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
	if exist $(LOUT) del /F /Q $(LOUT)
	if exist $(EOUT) del /F /Q $(EOUT)
	if exist $(OBJDIR) rmdir /S /Q $(OBJDIR)

lbuild:
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	if not exist $(OBJDIR)\core mkdir $(OBJDIR)\core
	if not exist $(OBJDIR)\core\gif mkdir $(OBJDIR)\core\gif
	if not exist $(OBJDIR)\core\ogl mkdir $(OBJDIR)\core\ogl
$(OBJDIR)\core\core.o: ..\core\core.c
	$(CC) $(LCFLAGS) $(LINC) -c ..\core\core.c -o $(OBJDIR)\core\core.o
$(OBJDIR)\core\gif\common.o: ..\core\gif\common.c
	$(CC) $(LCFLAGS) $(LINC) -c ..\core\gif\common.c -o $(OBJDIR)\core\gif\common.o
$(OBJDIR)\core\gif\gifstd.o: ..\core\gif\gifstd.c
	$(CC) $(LCFLAGS) $(LINC) -c ..\core\gif\gifstd.c -o $(OBJDIR)\core\gif\gifstd.o
$(OBJDIR)\core\ogl\common.o: ..\core\ogl\common.c
	$(CC) $(LCFLAGS) $(LINC) -c ..\core\ogl\common.c -o $(OBJDIR)\core\ogl\common.o
$(OBJDIR)\core\ogl\oglstd.o: ..\core\ogl\oglstd.c
	$(CC) $(LCFLAGS) $(LINC) -c ..\core\ogl\oglstd.c -o $(OBJDIR)\core\ogl\oglstd.o
$(OBJDIR)\lib.o: lib.c
	$(CC) $(LCFLAGS) $(LINC) -c lib.c -o $(OBJDIR)\lib.o

ebuild:
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	if not exist $(OBJDIR)\rsrc mkdir $(OBJDIR)\rsrc
	if not exist $(OBJDIR)\exec mkdir $(OBJDIR)\exec
$(OBJDIR)\rsrc\run.o: rsrc\run.rc
	$(RC) $(RCFLAGS) $(EINC) -i rsrc\run.rc -o $(OBJDIR)\rsrc\run.o
$(OBJDIR)\exec\exec.o: ..\exec\exec.c
	$(CC) $(ECFLAGS) $(EINC) -c ..\exec\exec.c -o $(OBJDIR)\exec\exec.o
$(OBJDIR)\run.o: run.c
	$(CC) $(ECFLAGS) $(EINC) -c run.c -o $(OBJDIR)\run.o

jlink:
	$(CX) $(ECXFLAGS) $(LOBJ) $(EOBJ) -o $(EOUT) $(LLIB)

llink: LCXFLAGS += -shared
llink:
	$(CX) $(LCXFLAGS) $(LOBJ) -o $(LOUT) $(LLIB)

elink:
	$(CX) $(ECXFLAGS) $(EOBJ) -o $(EOUT) $(ELIB)
