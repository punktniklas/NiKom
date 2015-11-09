FLAGS = NMINC STRMERGE IDIR=/Include

all: InitNiKom

clean:
        delete \#?.o InitNiKom

InitNiKom: InitNiKom.o
        slink FROM lib:c.o InitNiKom.o TO InitNiKom LIB lib:sc.lib lib:amiga.lib lib:rexxglue.o NOALVS NOICONS

InitNiKom.o: InitNiKom.c /Include/NiKomstr.h /Include/NiKomLib.h
        sc $(FLAGS) InitNiKom.c
