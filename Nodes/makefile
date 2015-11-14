CPU     = 68020
FLAGS   = NMINC STRMERGE CPU=$(CPU) DEBUG=FULL GST=Include:all.gst \
          IDIR=/Include IDIR=/ExtInclude IDIR=/UtilLib
OBJS    = ServerComm.o Terminal.o NiKFuncs.o NiKFuncs2.o NiKFuncs3.o NiKFuncs4.o \
          NiKFuncs5.o NiKEditor.o NiKRexx.o NiKFiles.o NiKVersion.o Brev.o \
          OrgMeet.o FidoMeet.o NewUser.o CharacterSets.o _tzset.o \
	  Cmd_Users.o
CONOBJS = NiKomCon.o ConsoleIO.o NiKConTrans.o NiKConOffline.o
SEROBJS = NiKomSer.o SerialIO.o NiKTransfer.o NiKOffline.o

all:	NiKomCon NiKomSer

Debug:
  -makedir Debug

Debug/$(CPU): Debug
  -makedir Debug/$(CPU)

NoDebug:
  -makedir NoDebug

NoDebug/$(CPU): NoDebug
  -makedir NoDebug/$(CPU)

clean:
  -delete \#?.o
  -delete Debug NoDebug all

NiKomCon: Debug/$(CPU) NoDebug/$(CPU) $(OBJS) $(CONOBJS)
  slink FROM lib:c.o $(CONOBJS) $(OBJS) \
        TO Debug/$(CPU)/NiKomCon \
        LIB lib:sc.lib+lib:amiga.lib+/UtilLib/Debug/$(CPU)/nikomutils.lib \
        NOICONS
  slink Debug/$(CPU)/NiKomCon TO NoDebug/$(CPU)/NiKomCon NODEBUG NOICONS

NiKomSer: Debug/$(CPU) NoDebug/$(CPU) $(OBJS) $(SEROBJS)
  slink FROM lib:c.o $(SEROBJS) $(OBJS) \
        TO Debug/$(CPU)/NiKomSer \
        LIB lib:sc.lib+lib:amiga.lib+/UtilLib/Debug/$(CPU)/nikomutils.lib \
        NOICONS VERBOSE
  slink Debug/$(CPU)/NiKomSer TO NoDebug/$(CPU)/NiKomSer NODEBUG NOICONS

NiKomCon.o: NiKomCon.c /Include/NiKomStr.h
  sc $(FLAGS) NiKomCon.c OBJNAME=NiKomCon.o

ConsoleIO.o: ConsoleIO.c /Include/NiKomStr.h
  sc $(FLAGS) ConsoleIO.c OBJNAME=ConsoleIO.o

NiKomSer.o: NiKomSer.c /Include/NiKomStr.h
  sc $(FLAGS) NiKomSer.c OBJNAME=NiKomSer.o

SerialIO.o: SerialIO.c /Include/NiKomStr.h
  sc $(FLAGS) SerialIO.c OBJNAME=SerialIO.o

ServerComm.o: ServerComm.c /Include/NiKomStr.h
  sc $(FLAGS) ServerComm.c OBJNAME=ServerComm.o

Terminal.o: Terminal.c /Include/NiKomStr.h
  sc $(FLAGS) Terminal.c OBJNAME=Terminal.o

NiKFuncs.o: NiKFuncs.c /Include/NiKomStr.h
  sc $(FLAGS) NiKFuncs.c OBJNAME=NiKFuncs.o

NiKFuncs2.o: NiKFuncs2.c /Include/NiKomStr.h
  sc $(FLAGS) NiKFuncs2.c OBJNAME=NiKFuncs2.o

NiKFuncs3.o: NiKFuncs3.c /Include/NiKomStr.h
  sc $(FLAGS) NiKFuncs3.c OBJNAME=NiKFuncs3.o

NiKFuncs4.o: NiKFuncs4.c /Include/NiKomStr.h
  sc $(FLAGS) NiKFuncs4.c OBJNAME=NiKFuncs4.o

NiKFuncs5.o: NiKFuncs5.c /Include/NiKomStr.h
  sc $(FLAGS) NiKFuncs5.c OBJNAME=NiKFuncs5.o

NiKEditor.o: NiKEditor.c /Include/NiKomStr.h
  sc $(FLAGS) NiKEditor.c OBJNAME=NiKEditor.o

NiKRexx.o: NiKRexx.c /Include/NiKomStr.h
  sc $(FLAGS) NiKRexx.c OBJNAME=NiKRexx.o

NiKFiles.o: NiKFiles.c /Include/NiKomStr.h
  sc $(FLAGS) NiKFiles.c OBJNAME=NiKFiles.o

NiKTransfer.o: NiKTransfer.c /Include/NiKomStr.h
  sc $(FLAGS) NOSTKCHK NiKTransfer.c OBJNAME=NiKTransfer.o

NiKConTrans.o: NiKConTrans.c
  sc $(FLAGS) NiKConTrans.c OBJNAME=NiKConTrans.o

NiKVersion.o: NiKVersion.c /Include/VersionStrings.h
  sc $(FLAGS) NiKVersion.c

NiKOffline.o: NiKOffline.c /Include/NiKomStr.h
  sc $(FLAGS) NiKOffline.c OBJNAME=NiKOffline.o

NiKConOffline.o: NiKConOffline.c
  sc $(FLAGS) NiKConOffline.c OBJNAME=NiKConOffline.o

Brev.o: Brev.c /Include/NiKomStr.h
  sc $(FLAGS) Brev.c OBJNAME=Brev.o

OrgMeet.o: OrgMeet.c /Include/NiKomStr.h
  sc $(FLAGS) OrgMeet.c OBJNAME=OrgMeet.o

FidoMeet.o: FidoMeet.c /Include/NiKomStr.h /Include/NiKomLib.h
  sc $(FLAGS) FidoMeet.c OBJNAME=FidoMeet.o

NewUser.o: NewUser.c /Include/NiKomStr.h /Include/NiKomLib.h NewUser.h
  sc $(FLAGS) NewUser.c OBJNAME=NewUser.o

CharacterSets.o: CharacterSets.c /Include/NiKomStr.h /Include/NiKomLib.h CharacterSets.h
  sc $(FLAGS) CharacterSets.c OBJNAME=CharacterSets.o

Cmd_Users.o: Cmd_Users.c /Include/NiKomStr.h /Include/NiKomLib.h Cmd_Users.h
  sc $(FLAGS) Cmd_Users.c OBJNAME=Cmd_Users.o

_tzset.o: sc:source/_tzset.c
  sc $(FLAGS) PARM=BOTH DEFINE=LESS_GETENV=1 OBJNAME=_tzset.o sc:source/_tzset.c
