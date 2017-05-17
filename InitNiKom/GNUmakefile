TOPDIR = ..
include $(TOPDIR)/common.mk

FLAGS = NMINC STRMERGE IDIR=/Include

all: InitNiKom

clean:
	-$(RM_RF) *.o InitNiKom

InitNiKom: InitNiKom.o
	$(LD) $(CFLAGS) -o $@ $^ -s

InitNiKom.o: InitNiKom.c ../Include/NiKomStr.h ../Include/NiKomLib.h
