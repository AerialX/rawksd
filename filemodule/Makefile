#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data  
INCLUDES	:=  include ../libios/include libfat/include
SCRIPTDIR	:=  scripts
BIN		:=  bin

STRIPIOS	:=  ../stripios/stripios.exe

LIBS		:=  ../libfat/libfat.a ../../libios/libios.a
LIBDIRS		:=

#---------------------------------------------------------------------------------
# the prefix on the compiler executables
#---------------------------------------------------------------------------------
PREFIX		:= $(DEVKITARM)/bin/arm-none-eabi-
CC			:= $(PREFIX)gcc
CXX			:= $(PREFIX)g++
AR			:= $(PREFIX)ar
OBJCOPY		:= $(PREFIX)objcopy
LD			:= $(PREFIX)g++
AS			:= $(PREFIX)g++

#---------------------------------------------------------------------------------
# linker script
#---------------------------------------------------------------------------------
LINKSCRIPT		:= $(ROOT)/$(SCRIPTDIR)/link.ld
SPECS			:= $(ROOT)/$(SCRIPTDIR)/nostart.specs	

ifeq ($(BUILDING),$(emptystring))

export ROOT	:= $(CURDIR)


all:
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@[ -d $(BIN) ] || mkdir -p $(BIN)
	@$(MAKE) --no-print-directory -C ../stripios
	@$(MAKE) --no-print-directory -C ../libios
	@$(MAKE) --no-print-directory -C libfile -f Makefile.ios
	@$(MAKE) --no-print-directory -C libfat
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile BUILDING=all
clean:
	@echo clean ...
	@$(MAKE) -C libfile clean
	@$(MAKE) -C libfat clean
	@rm -fr $(BUILD) $(BIN)
else

TARGET := $(notdir $(ROOT))
STRIPIOS := $(ROOT)/$(STRIPIOS)
#----------------------------------------------------
# MS Visual Studio Style Fix:
#----------------------------------------------------
STYLEFIX	= 2>&1 | sed -e 's/\([a-zA-Z\.]\+\):\([0-9]\+\):\([0-9]\+:\)\?\(.\+\)/\1(\2):\4/' -e 's/undefined/error: undefined/'

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
OUTPUT		:=  $(ROOT)/$(BIN)/$(TARGET)
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(ROOT)/$(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(ROOT)/$(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(ROOT)/$(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(ROOT)/$(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(ROOT)/$(dir)/*.*)))

OFILES		:=	$(addsuffix _bin.o,$(BINFILES)) \
					$(CPPFILES:.cpp=_cpp.o) $(CFILES:.c=_c.o) \
					$(sFILES:.s=_s.o) $(SFILES:.S=_S.o)
					
DEPENDS		:= $(OFILES:.o=.d)

VPATH		=  $(foreach dir,$(SOURCES),$(ROOT)/$(dir))


#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
INCLUDE	:=			$(foreach dir,$(INCLUDES),-I$(ROOT)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(ROOT)/$(BUILD) 

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) 

ARCH	=	-mcpu=arm926ej-s -mtune=arm926ej-s -mthumb -mthumb-interwork -mbig-endian

CFLAGS	=	-g $(ARCH) $(INCLUDE) -fno-strict-aliasing -Wall -O2 -fomit-frame-pointer -ffast-math -fverbose-asm \
			-Wpointer-arith -Winline -Wundef -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti


AFLAGS	=	-g $(ARCH) -x assembler-with-cpp

LDFLAGS	= -g $(ARCH) -specs=$(SPECS) -T$(LINKSCRIPT) $(LIBPATHS) $(LIBS) -Wl,-wrap,mktime -Wl,--gc-sections -Wl,-static -Wl,-Map,$(TARGET).map -nostartfiles

$(OUTPUT).elf: $(TARGET).elf
	@echo stripping $(notdir $@)
	@$(STRIPIOS) $< $@
	
%.elf: $(OFILES) $(LIBS)
	@echo linking $(notdir $@)
	@$(LD) -g -o $@ $(OFILES) $(LDFLAGS) $(STYLEFIX)
	
%_cpp.o : %.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MF $*_cpp.d $(CFLAGS) -c $< -o$@ $(STYLEFIX)

%_c.o : %.c
	@echo $(notdir $<)
	@$(CC) -MMD -MF $*_c.d $(CFLAGS)  -c $< -o$@ $(STYLEFIX)

%_s.o : %.s
	@echo $(notdir $<)
	@$(AS) -MMD -MF $*_s.d $(AFLAGS) -c $< -o$@ $(STYLEFIX)

%_bin.o : %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

endif

.DELETE_ON_ERROR:
.SECONDARY:
