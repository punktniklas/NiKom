# Compiler tool chain.
ifeq ($(OS),AmigaOS)
CROSSPREFIX =
else
CROSSPREFIX = m68k-amigaos-
endif
CC = $(CROSSPREFIX)gcc
LD = $(CROSSPREFIX)gcc
AR = $(CROSSPREFIX)ar
ARFLAGS = -r
STRIP = $(CROSSPREFIX)strip
LINKSTRIPFLAG = -s

AMIGAOPT = -noixemul -m$(CPU) -msmall-code
LIBCODE = -funsigned-char
LIBCODELD = -nostartfiles -nostdlib
LIBCODELIBS = -lnix -lnix20
CROSSOPT = -Os $(AMIGAOPT) -fomit-frame-pointer
CROSSLD = -noixemul
CROSSDEF = -D__AMIGADATE__=\"$(date +%-d.%-m.%Y)\"
CROSSDEF += -DCLIB_NIKOM_PROTOS_H
WARNINGS = -Wall -W -Wformat-nonliteral -Wbad-function-cast -Wno-unused -Wno-char-subscripts -Wno-sign-compare
SDKINCLUDE =
