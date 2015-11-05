CPU=68020
FLAGS = NMINC STRMERGE CPU=$(CPU) DEBUG=FULL IDIR=/Include IDIR=/UtilLib \
        IDIR=/Nodes
OBJS = Ser.o SerialIO.o Terminal.o ServerComm.o Modem.o Rexx.o Stuff.o \
       NiKVersion.o NewUser.o CharacterSets.o

all: PreNode

setup:
  makedir Debug Debug/$(CPU)
  makedir NoDebug NoDebug/$(CPU)

clean:
  delete \#?.o
  delete Debug/$(CPU)/\#? NoDebug/$(CPU)/\#?

PreNode: $(OBJS)
  slink FROM lib:c.o $(OBJS) TO Debug/$(CPU)/PreNode \
        LIB lib:sc.lib lib:amiga.lib /UtilLib/Debug/$(CPU)/nikomutils.lib \
        NOALVS
  slink Debug/$(CPU)/PreNode TO NoDebug/$(CPU)/PreNode NODEBUG NOICONS

Ser.o: Ser.c /Include/NiKomStr.h
  sc $(FLAGS) Ser.c

Modem.o: Modem.c /Include/NiKomStr.h
  sc $(FLAGS) Modem.c

Rexx.o: Rexx.c /Include/NiKomStr.h
  sc $(FLAGS) Rexx.c

Stuff.o: Stuff.c /Include/NiKomStr.h
  sc $(FLAGS) Stuff.c

NiKVersion.o: NiKVersion.c /Include/VersionStrings.h
  sc $(FLAGS) NiKVersion.c

SerialIO.o: /Nodes/SerialIO.c /Include/NiKomStr.h
  sc $(FLAGS) OBJNAME=SerialIO.o /Nodes/SerialIO.c

Terminal.o: /Nodes/Terminal.c /Include/NiKomStr.h
  sc $(FLAGS) OBJNAME=Terminal.o /Nodes/Terminal.c

ServerComm.o: /Nodes/ServerComm.c /Include/NiKomStr.h
  sc $(FLAGS) OBJNAME=ServerComm.o /Nodes/ServerComm.c

NewUser.o: /Nodes/NewUser.c /Include/NiKomStr.h
  sc $(FLAGS) OBJNAME=NewUser.o /Nodes/NewUser.c

CharacterSets.o: /Nodes/CharacterSets.c /Include/NiKomStr.h
  sc $(FLAGS) OBJNAME=CharacterSets.o /Nodes/CharacterSets.c

