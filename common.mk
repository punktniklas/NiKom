CPU = 68020

# Utility programs.
CP = cp
RM = rm -f
RM_RF = $(RM) -r
MKDIR_P = mkdir -p
FLEXCAT = flexcat
FLEXCATSD =
FD2PRAGMA = fd2pragma

OS = $(shell uname)

ifeq ($(OS),AmigaOS)
# Native Amiga build.
PARENT =
CP = copy
else
PARENT = ..
endif

# Default compiler family.
CCTYPE = gcc

UTILLIB = -L$(TOPDIR)/UtilLib/Debug/$(CPU) -lnikomutils
UTILNLIB = -L$(TOPDIR)/UtilLib/Debug/$(CPU) -lnikomutils_nlib

include $(TOPDIR)/$(CCTYPE).mk

CROSSFLAGS = $(CROSSOPT) $(CROSSDEF)
INCLUDES = -I$(TOPDIR)/Include -I$(TOPDIR)/UtilLib -I$(TOPDIR)/ExtInclude $(SDKINCLUDE)
CFLAGS = $(CROSSFLAGS) -g $(WARNINGS) $(INCLUDES) -DNiKom_NUMBERS -DNiKom_STRINGS
LDFLAGS = $(CROSSLD)
