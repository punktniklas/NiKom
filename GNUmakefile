TOPDIR = .

include common.mk

SUBDIRS = Catalogs UtilLib NiKomLib InitNiKom PreNode Server Nodes \
	Tools/CryptPasswords Tools/SetNodeState Tools/NiKomFido

all: subdirs

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean: subdirs
	-$(RM) Include/nikom_pragmas.h

.PHONY: subdirs $(SUBDIRS)
