TOPDIR = ..
include $(TOPDIR)/common.mk

CFLAGS += -DNIKOMLIB=1 $(LIBCODE)
LDFLAGS += $(LIBCODELD)
LIBS = $(UTILNLIB) $(LIBCODELIBS)

OBJS    = LibInit.o UserLibInit.o Matrix.o Rexx.o Lock.o ReadFidoText.o Echo.o \
          NodeTypes.o Conf.o NiKVersion.o Terminal.o ServerComm.o Hash.o \
          UnreadTexts.o Crypt.o FCrypt.o ConferenceTexts.o Util.o

all:	nikom_pragmas.h # FIXME - not working yet: nikom.library

clean:
	-$(RM) *.o nikom_pragmas.h func_table.h
	-$(RM_RF) Debug NoDebug

nikom_pragmas.h: nikom.fd
	$(FD2PRAGMA) -i $^ -s 7
	mv NiKom_lib.h $@
	$(CP) $@ $(PARENT)/Include/
	$(FD2PRAGMA) -i $^ --clib $(PARENT)/Include/nikom_protos.h -s 47
	$(MKDIR_P) $(PARENT)/Include/inline
	sed -e 's/"NIKOMSTR_H"/NIKOMSTR_H/' < NiKom.h > $(PARENT)/Include/inline/NiKom.h
	$(RM) NiKom.h

func_table.h: nikom.fd
	sed -nre 's/^([A-Z][^(]*)\(.*/  (myPFL)LIB\1,/p' < $< > $@

nikom.library: Debug/$(CPU)/nikom.library NoDebug/$(CPU)/nikom.library

Debug/$(CPU)/nikom.library: $(OBJS) nikom.fd
	$(MKDIR_P) Debug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

NoDebug/$(CPU)/nikom.library: $(OBJS) nikom.fd
	$(MKDIR_P) NoDebug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) -s

LibInit.o:      LibInit.c       nikom.fd

UserLibInit.o:  UserLibInit.c   nikom.fd CharTabs.h

Matrix.o:       Matrix.c        nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

Rexx.o:         Rexx.c          nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

Lock.o:         Lock.c          nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

ReadFidoText.o: ReadFidoText.c  nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

Echo.o:	        Echo.c          nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

NodeTypes.o:    NodeTypes.c     nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

Conf.o:         Conf.c          nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

NiKVersion.o:   NiKVersion.c    $(TOPDIR)/Include/VersionStrings.h

Terminal.o:     Terminal.c      nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

ServerComm.o:   ServerComm.c    nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

Hash.o:         Hash.c          nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

UserGroups.o:   UserGroups.c    nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h

UnreadTexts.o:  UnreadTexts.c nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h \
                  UnreadTexts.h Util.h

ConferenceTexts.o: ConferenceTexts.c nikom.fd $(TOPDIR)/Include/NiKomLib.h \
                         $(TOPDIR)/Include/NiKomStr.h ConferenceTexts.h UnreadTexts.h

Crypt.o:        Crypt.c nikom.fd $(TOPDIR)/Include/NiKomLib.h $(TOPDIR)/Include/NiKomStr.h FCrypt.h

FCrypt.o:       FCrypt.c Des.h

Util.o:         Util.c Util.h

.PHONY: nikom.library
