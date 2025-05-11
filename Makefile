
GFXLIB:=ugfx
include $(GFXLIB)/gfx.mk
include $(GFXLIB)/boards/base/HHK/board.mk

SOURCEDIR = src
BUILDDIR = obj
OUTDIR = dist
DEPDIR = .deps
METADIR = meta

AS:=sh4a_nofpueb-elf-gcc
AS_FLAGS:=-gdwarf-5

SDK_DIR?=/sdk

DEPFLAGS=-MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
WARNINGS=-Wall -Wextra -Wno-prio-ctor-dtor -pedantic -Werror -pedantic-errors
INCLUDES=-I$(SDK_DIR)/include $(GFXINC:%=-I%) -I$(SOURCEDIR)
DEFINES=$(GFXDEFS)
FUNCTION_FLAGS=-flto=auto -ffat-lto-objects -fno-builtin -ffunction-sections -fdata-sections -mrelax -gdwarf-5 -Oz
COMMON_FLAGS=$(FUNCTION_FLAGS) $(INCLUDES) $(WARNINGS) $(DEFINES)

CC:=sh4a_nofpueb-elf-gcc
CC_FLAGS=-std=c23 $(COMMON_FLAGS)

CXX:=sh4a_nofpueb-elf-g++
CXX_FLAGS=-std=c++20 $(COMMON_FLAGS)

LD:=sh4a_nofpueb-elf-g++
LD_FLAGS:=$(FUNCTION_FLAGS) -Wl,--gc-sections --specs=custom-start.specs
LIBS:=-L$(SDK_DIR) -lsdk

READELF:=sh4a_nofpueb-elf-readelf
OBJCOPY:=sh4a_nofpueb-elf-objcopy

PYTHON:=python

ADDRESSES := $(shell find $(METADIR) -maxdepth 1 -name 'addresses.*.json')
VERSIONS := $(ADDRESSES:$(METADIR)/addresses.%.json=%)
OVERRIDES := $(foreach json, $(ADDRESSES), $(OUTDIR)/addresses.$(shell $(PYTHON) -c 'import json; print(json.load(open("$(json)"))["safeGuard"])').override)
$(foreach json, $(ADDRESSES), $(eval $(OUTDIR)/addresses.$(shell $(PYTHON) -c 'import json; print(json.load(open("$(json)"))["safeGuard"])').override: $(BUILDDIR)/addresses.$(json:$(METADIR)/addresses.%.json=%).prep))

APP_ELFS := $(VERSIONS:%=$(OUTDIR)/run.%.elf)
APP_BINS := $(APP_ELFS:.elf=.bin)

AS_SOURCES:=$(shell find $(SOURCEDIR) -name '*.S' -not -name '*.template.*')
CC_SOURCES:=$(shell find $(SOURCEDIR) -name '*.c' -not -name '*.template.*') $(GFXSRC)
CXX_SOURCES:=$(shell find $(SOURCEDIR) -name '*.cpp' -not -name '*.template.*')
OBJECTS := $(addprefix $(BUILDDIR)/,$(AS_SOURCES:.S=.o)) \
	$(addprefix $(BUILDDIR)/,$(CC_SOURCES:.c=.o)) \
	$(addprefix $(BUILDDIR)/,$(CXX_SOURCES:.cpp=.o))

NOLTOOBJS := $(foreach obj, $(OBJECTS), $(if $(findstring /nolto/, $(obj)), $(obj)))

DEPFILES := $(OBJECTS:$(BUILDDIR)/%.o=$(DEPDIR)/%.d)

bin: $(APP_BINS) Makefile

hhk: $(APP_ELFS) Makefile

overrides: $(OVERRIDES) Makefile

all: bin hhk overrides
.DEFAULT_GOAL := all
.SECONDARY: # Prevents intermediate files from being deleted

clean:
	rm -rf $(BUILDDIR) $(OUTDIR) $(DEPDIR)


%.bin: %.elf
	@mkdir -p $(dir $@)
	$(OBJCOPY) --output-target=binary --set-section-flags .bss=alloc,load,contents,data $^ $@

$(OUTDIR)/run.%.elf: $(OBJECTS) linker.ld $(BUILDDIR)/addresses.%.o
	@mkdir -p $(dir $@)
	$(LD) -T linker.ld -Wl,-Map $@.map -o $@ $(LD_FLAGS) $(filter-out linker.ld, $^) $(LIBS)


$(OUTDIR)/addresses.%.override:
	@mkdir -p $(dir $@)
	cp $< $@

$(NOLTOOBJS): FUNCTION_FLAGS+=-fno-lto
$(addprefix $(BUILDDIR)/,$(GFXSRC:.c=.o)): WARNINGS=-Wall -Wextra -Wno-error=incompatible-pointer-types

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) -c $< -o $@ $(AS_FLAGS)

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$<)
	+$(CC) -c $< -o $@ $(CC_FLAGS) $(DEPFLAGS)

$(BUILDDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$<)
	+$(CXX) -c $< -o $@ $(CXX_FLAGS) $(DEPFLAGS)

$(BUILDDIR)/addresses.%.prep: $(METADIR)/addresses.%.json convert_json.py
	@mkdir -p $(dir $@)
	$(PYTHON) convert_json.py $< $@

$(BUILDDIR)/addresses.%.o: $(SOURCEDIR)/addresses.template.c $(BUILDDIR)/addresses.%.prep
	$(CC) -c $< -o $@ $(CC_FLAGS) --embed-dir=$(BUILDDIR) -D PREP_FILE=\<$(@:$(BUILDDIR)/addresses.%.o=addresses.%.prep)\>

#clangd gets confused on these flags
compile_commands.json:
	$(MAKE) $(MAKEFLAGS) clean
	bear --output $@.tmp -- sh -c "$(MAKE) $(MAKEFLAGS) --keep-going all || exit 0"
	#sed -i 's/-m4a-nofpu//' compile_commands.json.tmp
	mv compile_commands.json.tmp compile_commands.json

.PHONY: bin hhk overrides all clean compile_commands.json

-include $(DEPFILES)