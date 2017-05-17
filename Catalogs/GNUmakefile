TOPDIR = ..
include $(TOPDIR)/common.mk

DEFAULTS = $(TOPDIR)/Include/CatalogDefaults.h

all: svenska/NiKom.catalog $(DEFAULTS)

clean:
	-$(RM_RF) svenska
	-$(RM) $(DEFAULTS)

%/NiKom.catalog: NiKom.cd NiKom_%.ct
	$(MKDIR_P) $(dir $@)
	$(FLEXCAT) $^ CATALOG $@

$(DEFAULTS): NiKom.cd
	$(FLEXCAT) $^ $@=$(FLEXCATSD)CatComp_h.sd
