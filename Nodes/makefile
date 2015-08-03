CPU     = 68020
FLAGS   = NMINC STRMERGE CPU=$(CPU) DEBUG=FULL GST=Include:all.gst IDIR=/Include
OBJS    = ServerComm.o Terminal.o NiKFuncs.o NiKFuncs2.o NiKFuncs3.o NiKFuncs4.o \
          NiKFuncs5.o NiKEditor.o NiKRexx.o NiKFiles.o NiKVersion.o Brev.o \
          OrgMeet.o FidoMeet.o _tzset.o
CONOBJS = NiKomCon.o ConsoleIO.o NiKConTrans.o NiKConOffline.o
SEROBJS = NiKomSer.o SerialIO.o NiKTransfer.o NiKOffline.o

MENYOBJS    = menyversion.o ServerComm.o Terminal.o MenyFuncs.o NiKFuncs2.o \
              NiKFuncs3.o NiKFuncs4.o NiKFuncs5.o NiKEditor.o NiKRexx.o \
              NiKFiles.o MenyBrev.o MenyOrgMeet.o MenyFidoMeet.o _tzset.o \
              Meny1.o sprakstrings.o noansi.o

NIKVERSION      = `/GetNiKomVersion.rexx Version /NiKVersion.txt`
NIKKOMREVISION  = `/GetNiKomVersion.rexx KOMRevision /NiKVersion.txt`
NIKMENYREVISION = `/GetNiKomVersion.rexx MenyRevision /NiKVersion.txt`


all: NiKomCon NiKomSer MenyCon MenySer

install: all
	copy nodebug/$(CPU)/nikomcon NiKom:c
	copy nodebug/$(CPU)/nikomser NiKom:c
	copy nodebug/$(CPU)/prenode NiKom:c
	copy nodebug/$(CPU)/nikserver NiKom:c
	copy nodebug/$(CPU)/menycon NiKom:c
	copy nodebug/$(CPU)/menyser NiKom:c
	copy nodebug/nikom.library libs:

setup:
  makedir Debug
  makedir Debug/$(CPU)
  makedir NoDebug
  makedir NoDebug/$(CPU)


clean:
  delete *.o
  delete Debug/$(CPU)/*
  delete NoDebug/$(CPU)/*

NiKomCon: $(OBJS) $(CONOBJS)
  slink FROM lib:c.o $(CONOBJS) $(OBJS) \
        TO Debug/$(CPU)/NiKomCon \
        LIB lib:sc.lib+lib:amiga.lib+lib:rexxglue.o \
        NOALVS NOICONS
  slink debug/$(CPU)/NiKomCon TO nodebug/$(CPU)/NiKomCon NODEBUG NOICONS

NiKomSer: $(OBJS) $(SEROBJS)
  slink FROM lib:c.o $(SEROBJS) $(OBJS) \
        TO Debug/$(CPU)/NiKomSer \
        LIB lib:sc.lib+lib:amiga.lib+lib:rexxglue.o \
        NOALVS NOICONS VERBOSE
  slink debug/$(CPU)/NiKomSer TO noDebug/$(CPU)/NiKomSer NODEBUG NOICONS

MenyCon: $(MENYOBJS) $(CONOBJS)
  slink FROM lib:c.o $(CONOBJS) $(MENYOBJS) \
        TO Debug/$(CPU)/MenyCon \
        LIB lib:sc.lib+lib:amiga.lib+lib:rexxglue.o \
        NOALVS NOICONS
  slink Debug/$(CPU)/MenyCon TO NoDebug/$(CPU)/MenyCon NODEBUG NOICONS


MenySer: $(MENYOBJS) $(SEROBJS)
  slink FROM lib:c.o $(SEROBJS) $(MENYOBJS) \
        TO Debug/$(CPU)/MenySer \
        LIB lib:sc.lib+lib:amiga.lib+lib:rexxglue.o \
        NOALVS NOICONS
  slink Debug/$(CPU)/MenySer TO NoDebug/$(CPU)/MenySer NODEBUG NOICONS

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

NiKVersion.o: NiKVersion.c /NiKVersion.txt
  sc $(FLAGS) DEFINE=NIKVERSION=$(NIKVERSION) DEFINE=NIKREVISION=$(NIKKOMREVISION) NiKVersion.c

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

_tzset.o: sc:source/_tzset.c
  sc $(FLAGS) PARM=BOTH DEFINE=LESS_GETENV=1 OBJNAME=_tzset.o sc:source/_tzset.c



MenyFuncs.o: MenyFuncs.c /Include/NiKomStr.h /Include/NiKomLib.h MenyStr.h sprakstrings.h
  sc $(FLAGS) MenyFuncs.c OBJNAME=MenyFuncs.o

MenyBrev.o: MenyBrev.c /Include/NiKomStr.h
  sc $(FLAGS) MenyBrev.c OBJNAME=MenyBrev.o

MenyOrgMeet.o: MenyOrgMeet.c /Include/NiKomStr.h
  sc $(FLAGS) MenyOrgMeet.c OBJNAME=MenyOrgMeet.o

MenyFidoMeet.o: MenyFidoMeet.c /Include/NiKomStr.h /Include/NiKomLib.h
  sc $(FLAGS) MenyFidoMeet.c OBJNAME=MenyFidoMeet.o

Meny1.o: Meny1.c /Include/NiKomStr.h /Include/NiKomLib.h MenyStr.h sprakstrings.h
  sc $(FLAGS) Meny1.c OBJNAME=Meny1.o

sprakstrings.o: sprakstrings.c sprakstrings.h
  SC $(FLAGS) sprakstrings.c OBJNAME=sprakstrings.o

menyversion.o: menyversion.c /NiKVersion.txt
  sc $(FLAGS) DEFINE=NIKVERSION=$(NIKVERSION) DEFINE=NIKREVISION=$(NIKMENYREVISION) MenyVersion.c

noansi.o: noansi.c
  SC $(FLAGS) noansi.c OBJNAME=noansi.o
