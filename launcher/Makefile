.SUFFIXES:
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

export LAUNCHER_DIR ?= $(CURDIR)
export SOURCE_DIR ?= $(CURDIR)

all:

include $(DEVKITPPC)/wii_rules

TARGET		:=	launcher
SOURCES		:=	data source lib/libwiigui lib/libwiigui/libwiigui \
				data/images data/fonts data/sounds
INIT_ADDR := 0x80a00000

LIBS :=	$(LIB_MEGA)

include $(LAUNCHER_DIR)/common.mk

ifneq ($(BUILD),$(notdir $(CURDIR)))

riiv.zip: boot.elf
	@echo packaging ... $@
	@cp $< hbc/apps/riivolution/boot.elf
	@cd hbc/apps; zip -D riiv.zip riivolution/*
	@mv hbc/apps/riiv.zip .

z: riiv.zip

clean: clean-riiv
clean-riiv:
	@rm -f riiv.zip hbc/apps/riivolution/boot.elf

.PHONY: clean-riiv

endif
