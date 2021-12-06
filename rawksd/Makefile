.SUFFIXES:
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

export LAUNCHER_DIR ?= $(CURDIR)/../launcher
export SOURCE_DIR ?= $(CURDIR)

all:

include $(DEVKITPPC)/wii_rules

TARGET		:=	rawksd
SOURCES		:=	data source ../launcher/lib/libwiigui ../launcher/lib/libwiigui/libwiigui \
				data/images data/fonts data/sounds \
				../launcher/data ../launcher/data/images ../launcher/data/sounds
TEXTURES	:=	textures				

# Shared source files from launcher
SOURCEDIRS	:=	../launcher/source
SOURCEFILES	:=	init.cpp haxx.cpp installer.cpp \
				rijndael.c http.cpp sha1.cpp wdvd.cpp \
				riivolution.cpp riivolution_config.cpp launcher.cpp fwrite.cpp debugprint.cpp ssl.cpp

BINFILES += rb2_ntsc.xml rb2_pal.xml

INIT_ADDR := 0x80900400

include $(LAUNCHER_DIR)/common.mk

ifneq ($(BUILD),$(notdir $(CURDIR)))

rawk.zip: boot.elf
	@echo packaging ... $@
	@cp $< hbc/apps/rawksd/boot.elf
	@cd hbc/apps; zip -D $@ rawksd/*
	@mv hbc/apps/$@ $@

z: rawk.zip

clean: clean-kamek clean-rawk

clean-kamek:
	@$(HOSTMAKE) --no-print-directory -C kamek clean

clean-rawk:
	@rm -f rawk.zip hbc/apps/rawksd/boot.elf

.PHONY: clean-kamek clean-rawk

else

KAMEK := $(SOURCE_DIR)/kamek
KAMEK_EXT := cpp c S x yaml template py
KAMEK_DEPENDS := $(wildcard $(foreach ext,$(KAMEK_EXT),$(KAMEK)/*.$(ext)))

$(KAMEK)/build/rb2_pal.xml: $(KAMEK_DEPENDS)
	@$(HOSTMAKE) -C $(KAMEK) rb2_ntsc rb2_pal
$(KAMEK)/build/rb2_ntsc.xml: $(KAMEK)/build/rb2_pal.xml

rb2_ntsc.xml.o: $(KAMEK)/build/rb2_ntsc.xml
	@$(bin2o)

rb2_pal.xml.o: $(KAMEK)/build/rb2_pal.xml
	@$(bin2o)

endif
