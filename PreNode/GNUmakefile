TOPDIR = ..
include $(TOPDIR)/common.mk

CFLAGS += -I../Nodes
OBJS = Ser.o SerialIO.o Terminal.o ServerComm.o Modem.o Rexx.o Stuff.o \
       NiKVersion.o NewUser.o CharacterSets.o InfoFiles.o Languages.o compat.o

all: PreNode

clean:
	-$(RM) *.o
	-$(RM_RF) Debug NoDebug

PreNode: Debug/$(CPU)/PreNode NoDebug/$(CPU)/PreNode

Debug/$(CPU)/PreNode: $(OBJS)
	$(MKDIR_P) Debug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $^ $(UTILLIB)

NoDebug/$(CPU)/PreNode: $(OBJS)
	$(MKDIR_P) NoDebug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $^ $(UTILLIB) -s

Ser.o: Ser.c ../Include/NiKomStr.h

Modem.o: Modem.c ../Include/NiKomStr.h

Rexx.o: Rexx.c ../Include/NiKomStr.h

Stuff.o: Stuff.c ../Include/NiKomStr.h

NiKVersion.o: NiKVersion.c ../Include/VersionStrings.h

SerialIO.o: ../Nodes/SerialIO.c ../Include/NiKomStr.h
	$(CC) $(CFLAGS) -o $@ -c $<

Terminal.o: ../Nodes/Terminal.c ../Include/NiKomStr.h
	$(CC) $(CFLAGS) -o $@ -c $<

ServerComm.o: ../Nodes/ServerComm.c ../Include/NiKomStr.h
	$(CC) $(CFLAGS) -o $@ -c $<

NewUser.o: ../Nodes/NewUser.c ../Include/NiKomStr.h
	$(CC) $(CFLAGS) -o $@ -c $<

CharacterSets.o: ../Nodes/CharacterSets.c ../Include/NiKomStr.h
	$(CC) $(CFLAGS) -o $@ -c $<

InfoFiles.o: ../Nodes/InfoFiles.c ../Include/NiKomStr.h
	$(CC) $(CFLAGS) -o $@ -c $<

Languages.o: ../Nodes/Languages.c ../Include/NiKomStr.h
	$(CC) $(CFLAGS) -o $@ -c $<

compat.o: ../Nodes/compat.c ../Include/NiKomStr.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: PreNode
