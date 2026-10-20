# Alternative bare-metal PowerPC/XeLL experiment, NOT the .xex build.
.SUFFIXES:
ifeq ($(strip $(DEVKITXENON)),)
$(error DEVKITXENON is not set; install libxenon before building)
endif
include $(DEVKITXENON)/rules
TARGET := xcloud_xell_probe
BUILD := build
SOURCES := source
DATA :=
INCLUDES :=
LIBDIRS :=
LIBS := -lxenon -lm
CFLAGS := -g -O2 -Wall $(MACHDEP) $(INCLUDE)
LDFLAGS := -g $(MACHDEP) -Wl,--gc-sections

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES := $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) -I$(CURDIR)/$(BUILD) -I$(LIBXENON_INC)
export LIBPATHS := -L$(LIBXENON_LIB)
export LD := $(CC)
.PHONY: $(BUILD) clean
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).elf32
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).elf32: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
