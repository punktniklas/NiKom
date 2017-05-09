TOPDIR = ..
include $(TOPDIR)/common.mk

OBJS    = ServerComm.o Terminal.o NiKFuncs.o NiKFuncs2.o NiKFuncs3.o NiKFuncs4.o \
          NiKFuncs5.o NiKEditor.o NiKRexx.o NiKFiles.o NiKVersion.o Brev.o \
          OrgMeet.o FidoMeet.o NewUser.o CharacterSets.o KOM.o ConfCommon.o \
          ConfHeaderExtensions.o Languages.o InfoFiles.o Fifo.o \
	  Cmd_KOM.o Cmd_Users.o Cmd_Conf.o Cmd_Misc.o compat.o
CONOBJS = NiKomCon.o ConsoleIO.o NiKConTrans.o NiKConOffline.o
SEROBJS = NiKomSer.o SerialIO.o NiKTransfer.o NiKOffline.o

all:	NiKomCon NiKomSer

clean:
	-$(RM) *.o
	-$(RM_RF) Debug NoDebug

NiKomCon: Debug/$(CPU)/NiKomCon NoDebug/$(CPU)/NiKomCon

Debug/$(CPU)/NiKomCon: $(OBJS) $(CONOBJS)
	$(MKDIR_P) Debug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $^ $(UTILLIB)

NoDebug/$(CPU)/NiKomCon: $(OBJS) $(CONOBJS)
	$(MKDIR_P) NoDebug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $^ $(UTILLIB) -s

NiKomSer: Debug/$(CPU)/NiKomSer NoDebug/$(CPU)/NiKomSer

Debug/$(CPU)/NiKomSer: $(OBJS) $(SEROBJS)
	$(MKDIR_P) Debug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $^ $(UTILLIB)

NoDebug/$(CPU)/NiKomSer: $(OBJS) $(SEROBJS)
	$(MKDIR_P) NoDebug/$(CPU)
	$(LD) $(LDFLAGS) -o $@ $^ $(UTILLIB) -s

NiKomCon.o: NiKomCon.c ../Include/NiKomStr.h

ConsoleIO.o: ConsoleIO.c ../Include/NiKomStr.h

NiKomSer.o: NiKomSer.c ../Include/NiKomStr.h

SerialIO.o: SerialIO.c ../Include/NiKomStr.h

ServerComm.o: ServerComm.c ../Include/NiKomStr.h

Terminal.o: Terminal.c ../Include/NiKomStr.h

NiKFuncs.o: NiKFuncs.c ../Include/NiKomStr.h

NiKFuncs2.o: NiKFuncs2.c ../Include/NiKomStr.h

NiKFuncs3.o: NiKFuncs3.c ../Include/NiKomStr.h

NiKFuncs4.o: NiKFuncs4.c ../Include/NiKomStr.h

NiKFuncs5.o: NiKFuncs5.c ../Include/NiKomStr.h

NiKEditor.o: NiKEditor.c ../Include/NiKomStr.h

NiKRexx.o: NiKRexx.c ../Include/NiKomStr.h

NiKFiles.o: NiKFiles.c ../Include/NiKomStr.h

NiKTransfer.o: NiKTransfer.c ../Include/NiKomStr.h

NiKConTrans.o: NiKConTrans.c

NiKVersion.o: NiKVersion.c ../Include/VersionStrings.h

NiKOffline.o: NiKOffline.c ../Include/NiKomStr.h

NiKConOffline.o: NiKConOffline.c

Brev.o: Brev.c ../Include/NiKomStr.h

OrgMeet.o: OrgMeet.c ../Include/NiKomStr.h

FidoMeet.o: FidoMeet.c ../Include/NiKomStr.h ../Include/NiKomLib.h

NewUser.o: NewUser.c ../Include/NiKomStr.h ../Include/NiKomLib.h NewUser.h

CharacterSets.o: CharacterSets.c ../Include/NiKomStr.h ../Include/NiKomLib.h CharacterSets.h

KOM.o: KOM.c ../Include/NiKomStr.h ../Include/NiKomLib.h KOM.h

ConfCommon.o: ConfCommon.c ../Include/NiKomStr.h ../Include/NiKomLib.h ConfCommon.h

ConfHeaderExtensions.o: ConfHeaderExtensions.c ../Include/NiKomStr.h ConfHeaderExtensions.h

Languages.o: Languages.c ../Include/NiKomStr.h Languages.h

InfoFiles.o: InfoFiles.c ../Include/NiKomStr.h InfoFiles.h

Fifo.o: Fifo.c ../Include/NiKomStr.h Fifo.h

Cmd_Kom.o: Cmd_KOM.c ../Include/NiKomStr.h ../Include/NiKomLib.h Cmd_KOM.h

Cmd_Users.o: Cmd_Users.c ../Include/NiKomStr.h ../Include/NiKomLib.h Cmd_Users.h

Cmd_Conf.o: Cmd_Conf.c ../Include/NiKomStr.h ../Include/NiKomLib.h Cmd_Conf.h

Cmd_Misc.o: Cmd_Misc.c ../Include/NiKomStr.h ../Include/NiKomLib.h Cmd_Misc.h

.PHONY: NiKomCon NiKomSer
