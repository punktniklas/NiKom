TOPDIR = ..
include $(TOPDIR)/common.mk

OBJS  = NiKServer.o NiKHost.o NiKHost2.o NiKHost3.o NiKVersion.o NodeComm.o \
	Config.o Startup.o Shutdown.o

all: NiKServer

clean:
	-$(RM) *.o
	-$(RM_RF) Debug NoDebug

NiKServer: Debug/$(CPU)/NiKServer NoDebug/$(CPU)/NiKServer

Debug/$(CPU)/NiKServer: $(OBJS)
	$(MKDIR_P) Debug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $^ $(UTILLIB)

NoDebug/$(CPU)/NiKServer: $(OBJS)
	$(MKDIR_P) NoDebug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $^ $(UTILLIB) -s

NiKServer.o: NiKServer.c ../Include/NiKomStr.h

Startup.o: Startup.c ../Include/NiKomStr.h

Shutdown.o: Shutdown.c ../Include/NiKomStr.h

NiKHost.o: NiKHost.c ../Include/NiKomStr.h

NiKHost2.o: NiKHost2.c ../Include/NiKomStr.h

NiKHost3.o: NiKHost3.c ../Include/NiKomStr.h

NiKVersion.o: NiKVersion.c ../Include/VersionStrings.h

NodeComm.o: NodeComm.c ../Include/NiKomStr.h

Config.o: Config.c ../Include/NiKomStr.h

.PHONY: NiKServer
