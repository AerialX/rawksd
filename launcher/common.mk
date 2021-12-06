COMMON_DIR := $(LAUNCHER_DIR)/..
PORTLIBS := $(PORTLIBS_PATH)/ppc
LIBDIRS := $(PORTLIBS)
BUILD ?= build
TEXTURES ?= textures

# common deps
DOLLZ3 := $(LAUNCHER_DIR)/dollz3/dollz3.exe
DOL2ELF := $(LAUNCHER_DIR)/dollz3/dol2elf.exe
LIB_XML2 := $(LAUNCHER_DIR)/lib/libxml2/libxml2.a
LIB_XMLXX := $(LAUNCHER_DIR)/lib/libxml++/libxml++.a
LIB_FILE := $(COMMON_DIR)/filemodule/libfile/libfile_wii.a
LIB_MEGA := $(COMMON_DIR)/megamodule/libmega/libmega_wii.a
MODULES += filemodule dipmodule

LIBS :=	$(LIB_XMLXX) $(LIB_XML2) -lpng -lz -lfat -lwiiuse -lbte -lasnd -logc -lvorbisidec -logg -lfreetype -lbz2 $(LIB_FILE) $(LIBS)
INCLUDES := $(SOURCE_DIR)/$(BUILD) $(SOURCE_DIR)/include $(LAUNCHER_DIR)/include $(LAUNCHER_DIR)/lib/libwiigui $(LAUNCHER_DIR)/lib \
						$(COMMON_DIR)/filemodule/include \
						$(LAUNCHER_DIR)/lib/libxml2/include $(LAUNCHER_DIR)/lib/libxml++ $(LAUNCHER_DIR)/lib/libxml++/libxml++

CFLAGS		=	-fdata-sections -ffunction-sections -g -O2 -Wall -Wno-deprecated-declarations -Wno-multistatement-macros $(MACHDEP) $(INCLUDE)
CXXFLAGS	=	-Xassembler -aln=$@.lst $(CFLAGS)
LDFLAGS		=	$(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(notdir $@).map,--section-start,.init=$(INIT_ADDR) -T $(LAUNCHER_DIR)/lib/rvl.ld

export DEBUGGER ?=
export RETURN_TO_MENU ?=
export FWRITE_PATCH ?=

ifdef $(DEBUGGER)
	MODULES += megamodule
	CFLAGS += -DDEBUGGER=1
endif
ifdef $(RETURN_TO_MENU)
	CFLAGS += -DRETURN_TO_MENU=1
endif
ifdef $(FWRITE_PATCH)
	CFLAGS += -DFWRITE_PATCH=1
endif

RES_EXTS += ttf png ogg pcm dat tpl xml
BIN_EXTS += $(RES_EXTS)

HOSTMAKE := env -u AS -u CC -u CXX -u LD -u AR -u OBJCOPY -u STRIP -u NM -u RANLIB $(MAKE)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGETDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(SOURCEDIRS),$(CURDIR)/$(dir)) \
					$(foreach dir,$(TEXTURES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

$(DOLLZ3):
	@$(HOSTMAKE) --no-print-directory -C $(LAUNCHER_DIR)/dollz3 dollz3.exe
$(DOL2ELF):
	@$(HOSTMAKE) --no-print-directory -C $(LAUNCHER_DIR)/dollz3 dol2elf.exe

BINFILES += $(foreach dir,$(SOURCES),$(notdir $(wildcard $(foreach ext,$(BIN_EXTS),$(dir)/*.$(ext)))))
BINFILES += $(MODULES:=.elf)

export CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c))) \
				$(filter %.c,$(SOURCEFILES))
export CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp))) \
				$(filter %.cpp,$(SOURCEFILES))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
SCFFILES	:=	$(foreach dir,$(TEXTURES),$(notdir $(wildcard $(dir)/*.scf)))
TPLFILES	:=	$(SCFFILES:.scf=.tpl)

export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(addsuffix .o,$(TPLFILES)) \
					$(sFILES:.s=.o) $(SFILES:.S=.o) \
					$(BINFILES:=.o) \

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(LIBOGC_INC) -I$(PORTLIBS)/include/freetype2

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export PATH := $(PATH):$(DEVKITPPC)/bin:$(DEVKITPRO)/tools/bin

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

.PHONY: $(BUILD) clean run z

all: $(BUILD)
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol $(OUTPUT).z.dol boot.elf

run: $(BUILD)
	@wiiload $(OUTPUT).dol

boot.elf: $(BUILD) $(DOLLZ3) $(DOL2ELF)
	@echo compressing ... $@
	@$(DOLLZ3) $(OUTPUT).dol $(OUTPUT).z.dol -m
	@$(DOL2ELF) $(OUTPUT).z.dol boot.elf

z: boot.elf

else

OFILES_RES := $(filter $(foreach ext,$(RES_EXTS),%.$(ext).o),$(OFILES))

$(LIB_XML2):
	@$(HOSTMAKE) --no-print-directory -C $(LAUNCHER_DIR)/lib/libxml2
$(LIB_XMLXX):
	@$(HOSTMAKE) --no-print-directory -C $(LAUNCHER_DIR)/lib/libxml++
$(LIB_FILE):
	@$(HOSTMAKE) --no-print-directory -C $(COMMON_DIR)/filemodule/libfile -f Makefile.wii
$(LIB_MEGA):
	@$(HOSTMAKE) --no-print-directory -C $(COMMON_DIR)/megamodule/libmega -f Makefile.wii

# serialize module builds...
build-modules:
	@for mod in $(MODULES); do $(HOSTMAKE) --no-print-directory -C $(COMMON_DIR)/$$mod || exit 1; done

define moduledep =
$(COMMON_DIR)/$(1)/bin/$(1).elf: build-modules
	@true

$(1).elf.o: $(COMMON_DIR)/$(1)/bin/$(1).elf
	@$$(bin2o)
endef
$(foreach mod,$(MODULES),$(eval $(call moduledep,$(mod))))

define binrules =
%.$(1).o: %.$(1)
	@echo $$(notdir $$<)
	@$$(bin2o)
endef
$(foreach ext,$(BIN_EXTS),$(eval $(call binrules,$(ext))))

DEPENDS := $(OFILES:.o=.d)
-include $(DEPENDS)
$(CFILES:.c=.o) $(CPPFILES:.cpp=.o): $(OFILES_RES)

all: $(OUTPUT).dol
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES) $(filter %.a,$(LIBS))

.PHONY: $(LIB_MEGA) $(LIB_FILE) build-modules

endif

.DELETE_ON_ERROR:
.SECONDARY:
