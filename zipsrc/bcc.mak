# Borland C makefile for the ZIP Infocom interpreter

.autodepend
CC = bcc
CFLAGS = -ml -d -DMSDOS=1 -D__STDC__=1
LD = bcc
LDFLAGS = -ml
LIBS =

INC = ztypes.h
OBJS = zip.obj control.obj extern.obj fileio.obj input.obj interpre.obj \
        math.obj memory.obj object.obj operand.obj osdepend.obj \
        property.obj screen.obj text.obj variable.obj getopt.obj bccio.obj

zip.exe: $(OBJS)
        $(LD) $(LDFLAGS) @&&!
-e$&
$(OBJS)
$(LIBS)
!

.c.obj:
        $(CC) $(CFLAGS) -c {$. }
