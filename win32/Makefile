CC = gcc -m32
CX = gcc -m32
RC = windres


OBJDIR = .obj

RCFLAGS = -J rc -O coff
CRTLIBS = -lmsvcrt -lwininet -lkernel32 -luser32 -lshell32 -lgdi32 -lole32 -lcomctl32 -lcomdlg32
CRTFUNC = -Dstrdup=_strdup -D__mingw_realloc=realloc -D__mingw_free=free

LLIB = $(CRTLIBS) -lopengl32
LINC =
LCXFLAGS =
LCFLAGS = -I..\core -mno-stack-arg-probe -Wall -march=i486 -DLIB_MAKE $(CRTFUNC)

LOBJ = $(OBJDIR)\core\core.o $(OBJDIR)\core\gif\gifstd.o $(OBJDIR)\core\ogl\oglstd.o $(OBJDIR)\lib.o
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

DbgJoined: LCFLAGS += -DLIB_NONE -D__always_inline__=
DbgJoined: ECFLAGS += -DLIB_NONE -D__always_inline__=
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

ldebug: LCFLAGS += -g -D__always_inline__=
ldebug: lbuild $(LOBJ)

edebug: ECFLAGS += -g -D__always_inline__=
edebug: ebuild $(EOBJ)


clean:
	if exist $(LOUT) del /F /Q $(LOUT)
	if exist $(EOUT) del /F /Q $(EOUT)
	if exist $(OBJDIR) rmdir /S /Q $(OBJDIR)

lbuild:
$(OBJDIR)\core\core.o: ..\core\core.c
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	if not exist $(OBJDIR)\core mkdir $(OBJDIR)\core
	$(CC) $(LCFLAGS) $(LINC) -c ..\core\core.c -o $(OBJDIR)\core\core.o
$(OBJDIR)\core\gif\gifstd.o: ..\core\gif\gifstd.c
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	if not exist $(OBJDIR)\core mkdir $(OBJDIR)\core
	if not exist $(OBJDIR)\core\gif mkdir $(OBJDIR)\core\gif
	$(CC) $(LCFLAGS) $(LINC) -c ..\core\gif\gifstd.c -o $(OBJDIR)\core\gif\gifstd.o
$(OBJDIR)\core\ogl\oglstd.o: ..\core\ogl\oglstd.c
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	if not exist $(OBJDIR)\core mkdir $(OBJDIR)\core
	if not exist $(OBJDIR)\core\ogl mkdir $(OBJDIR)\core\ogl
	$(CC) $(LCFLAGS) $(LINC) -c ..\core\ogl\oglstd.c -o $(OBJDIR)\core\ogl\oglstd.o
$(OBJDIR)\lib.o: lib.c
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(LCFLAGS) $(LINC) -c lib.c -o $(OBJDIR)\lib.o

ebuild:
$(OBJDIR)\rsrc\run.o: rsrc\run.rc
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	if not exist $(OBJDIR)\rsrc mkdir $(OBJDIR)\rsrc
	$(RC) $(RCFLAGS) $(EINC) -i rsrc\run.rc -o $(OBJDIR)\rsrc\run.o
$(OBJDIR)\exec\exec.o: ..\exec\exec.c
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	if not exist $(OBJDIR)\exec mkdir $(OBJDIR)\exec
	$(CC) $(ECFLAGS) $(EINC) -c ..\exec\exec.c -o $(OBJDIR)\exec\exec.o
$(OBJDIR)\run.o: run.c
	if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(ECFLAGS) $(EINC) -c run.c -o $(OBJDIR)\run.o

jlink: $(LOBJ) $(EOBJ)
	$(CX) $(ECXFLAGS) $(LOBJ) $(EOBJ) -o $(EOUT) $(LLIB)

llink: LCXFLAGS += -shared
llink: $(LOBJ)
	$(CX) $(LCXFLAGS) $(LOBJ) -o $(LOUT) $(LLIB)

elink: $(EOBJ)
	$(CX) $(ECXFLAGS) $(EOBJ) -o $(EOUT) $(ELIB)
