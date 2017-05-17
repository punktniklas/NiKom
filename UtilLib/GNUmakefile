TOPDIR = ..
include $(TOPDIR)/common.mk

NLCFLAGS = -DNIKOMLIB=1 $(LIBCODE)

OBJS     = DiskUtils.o FileAreaUtils.o Logging.o RexxUtils.o ExecUtils.o \
	   ServerMemUtils.o StringUtils.o ConfigUtils.o FidoUtils.o Stack.o
NLIBOBJS = Logging_nlib.o

UTILSLIB = Debug/$(CPU)/libnikomutils.a
UTILSNLIB = Debug/$(CPU)/libnikomutils_nlib.a

all: $(UTILSLIB) $(UTILSNLIB)

clean:
	-$(RM_RF) *.o
	-$(RM_RF) Debug

$(UTILSLIB): $(OBJS)
	-$(MKDIR_P) Debug/$(CPU)
	$(AR) $(ARFLAGS) $@ $^

$(UTILSNLIB): $(NLIBOBJS)
	-$(MKDIR_P) Debug/$(CPU)
	$(AR) $(ARFLAGS) $@ $^

DiskUtils.o: DiskUtils.c DiskUtils.h

FileAreaUtils.o: FileAreaUtils.c FileAreaUtils.h

Logging.o: Logging.c Logging.h

Logging_nlib.o: Logging.c Logging.h
	$(CC) $(CFLAGS) $(NLCFLAGS) -c -o $@ $<

RexxUtils.o: RexxUtils.c RexxUtils.h

ExecUtils.o: ExecUtils.c ExecUtils.h

ServerMemUtils.o: ServerMemUtils.c ServerMemUtils.h

StringUtils.o: StringUtils.c StringUtils.h

ConfigUtils.o: ConfigUtils.c ConfigUtils.h

FidoUtils.o: FidoUtils.c FidoUtils.h

Stack.o: Stack.c Stack.h
