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

INIT_ADDR := 0x80900400

include $(LAUNCHER_DIR)/common.mk
