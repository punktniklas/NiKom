TOPDIR = ../..
include $(TOPDIR)/common.mk

CFLAGS += -I../../NiKomLib
OBJS = CryptPasswords.o FCrypt.o

all: CryptPasswords

clean:
	-$(RM) *.o
	-$(RM) CryptPasswords

CryptPasswords: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LINKSTRIPFLAG)

CryptPasswords.o: CryptPasswords.c ../../NiKomLib/FCrypt.h

FCrypt.o: ../../NiKomLib/FCrypt.c
	$(CC) $(CFLAGS) -o $@ -c $<
