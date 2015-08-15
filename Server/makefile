CPU   = 68020
FLAGS = NMINC STRMERGE CPU=$(CPU) DEBUG=FULL IDIR=/Include
OBJS  = NiKServer.o NiKHost.o NiKHost2.o NiKHost3.o NiKSerVersion.o NodeComm.o \
	Config.o

NIKVERSION  = `/GetNiKomVersion.rexx Version /NiKVersion.txt`
NIKREVISION = `/GetNiKomVersion.rexx ServerRevision /NiKVersion.txt`

all: NiKServer

setup:
  makedir Debug Debug/$(CPU)
  makedir NoDebug NoDebug/$(CPU)

clean:
  delete *.o
  delete Debug/$(CPU)/* NoDebug/$(CPU)/*

NiKServer: $(OBJS)
  slink FROM lib:c.o $(OBJS) \
        TO Debug/$(CPU)/NiKServer \
        LIB lib:sc.lib+lib:amiga.lib+lib:rexxglue.o \
        NOALVS NOICONS
  slink FROM Debug/$(CPU)/NiKServer TO NoDebug/$(CPU)/NiKServer NODEBUG NOICONS


NiKServer.o: NiKServer.c /Include/NiKomStr.h
  sc $(FLAGS) NiKServer.c

NiKHost.o: NiKHost.c /Include/NiKomStr.h
  sc $(FLAGS) NiKHost.c

NiKHost2.o: NiKHost2.c /Include/NiKomStr.h
  sc $(FLAGS) NiKHost2.c

NiKHost3.o: NiKHost3.c /Include/NiKomStr.h
  sc $(FLAGS) NiKHost3.c

NiKSerVersion.o: NiKSerVersion.c NiKServer.o NiKHost.o NiKHost2.o NiKHost3.o NodeComm.o /NiKVersion.txt
  sc $(FLAGS) DEFINE=NIKSERVERVERSION=$(NIKVERSION) DEFINE=NIKSERVERREVISION=$(NIKREVISION) NiKSerVersion.c

NodeComm.o: NodeComm.c /Include/NiKomStr.h
  sc $(FLAGS) NodeComm.c

Config.o: Config.c /Include/NiKomStr.h
  sc $(FLAGS) Config.c
