CPU=68020
FLAGS = NMINC STRMERGE CPU=$(CPU) DEBUG=FULL IDIR=/Include
OBJS = Ser.o /Nodes/SerialIO.o /Nodes/Terminal.o /Nodes/ServerComm.o Modem.o Rexx.o Stuff.o Version.o

NIKVERSION   = `/GetNiKomVersion.rexx Version /NiKVersion.txt`
NIKREVISION  = `/GetNiKomVersion.rexx PreNodeRevision /NiKVersion.txt`

all: PreNode

setup:
  makedir Debug Debug/$(CPU)
  makedir NoDebug NoDebug/$(CPU)

clean:
  delete *.o
  delete Debug/$(CPU)/* NoDebug/$(CPU)/*

PreNode: $(OBJS)
  slink FROM lib:c.o $(OBJS) TO Debug/$(CPU)/PreNode LIB lib:sc.lib lib:amiga.lib lib:rexxglue.o NOALVS
  slink Debug/$(CPU)/PreNode TO NoDebug/$(CPU)/PreNode NODEBUG NOICONS

Ser.o: Ser.c /Include/NiKomstr.h
  sc $(FLAGS) Ser.c

Modem.o: Modem.c /Include/NiKomstr.h
  sc $(FLAGS) Modem.c

Rexx.o: Rexx.c /Include/NiKomstr.h
  sc $(FLAGS) Rexx.c

Stuff.o: Stuff.c /Include/NiKomstr.h
  sc $(FLAGS) Stuff.c

Version.o: Version.c /NiKVersion.txt
  sc $(FLAGS) DEFINE=NIKVERSION=$(NIKVERSION) DEFINE=NIKREVISION=$(NIKREVISION) Version.c
