BUILD := build

# an arbitrary file produced by the kamek build
KAMEK_BUILD := $(BUILD)/pal_riiv.xml

KAMEK_EXT := cpp c S x yaml template py
DEPENDS := $(wildcard $(foreach ext,$(KAMEK_EXT),*.$(ext)))

riiv:

all: $(KAMEK_BUILD) riiv rb2_ntsc rb2_pal

PYTHON ?= python

PYTHON_VERSION := $(shell $(PYTHON) --version 2>&1)
ifeq (,$(findstring Python 3.,$(PYTHON_VERSION)))
PYTHON := python3
endif

export PATH := $(PATH):$(DEVKITPPC)/bin
$(KAMEK_BUILD):
	@$(PYTHON) kamek.py project.yaml

riiv: $(BUILD)/riiv.xml
$(BUILD)/riiv.xml: riiv.template $(KAMEK_BUILD)
	@echo "Creating XML $(notdir $@)..."
	@cat $< > $@
	@echo '<patch id="test_ntsc">' >> $@
	@cat $(BUILD)/ntsc_riiv.xml >> $@
	@echo '</patch>' >> $@
	@echo '<patch id="test_pal">' >> $@
	@cat $(BUILD)/pal_riiv.xml >> $@
	@echo '</patch>' >> $@
	@echo '</wiidisc>' >> $@

rb2_ntsc: $(BUILD)/rb2_ntsc.xml
$(BUILD)/rb2_ntsc.xml: riiv_ntsc.template $(KAMEK_BUILD)
	@cat $< > $@
	@echo '<patch id="kamek_ntsc">' >> $@
	@cat $(BUILD)/ntsc_riiv.xml >> $@
	@echo '</patch>' >> $@
	@echo '</wiidisc>' >> $@
	
rb2_pal: $(BUILD)/rb2_pal.xml
$(BUILD)/rb2_pal.xml: riiv_pal.template $(KAMEK_BUILD)
	@cat $< > $@
	@echo '<patch id="kamek_pal">' >> $@
	@cat build/pal_riiv.xml >> $@
	@echo '</patch>' >> $@
	@echo '</wiidisc>' >> $@

clean:
	@echo clean kamek ...
	@rm -rf $(BUILD)

.PHONY: all riiv rb2_ntsc rb2_pal
.DELETE_ON_ERROR:
.SECONDARY:
