ifneq "$(wildcard config.mk)" "config.mk"
$(error Error: could not find file `config.mk`. Please make a copy of `config.mk.template`, rename the copy to `config.mk`, and adjust the copy as necessary before attempting to build again.)
endif

# Get all of the locations for dependencies
# Modify dependencies.mk to set the locations/commands for each tool
include config.mk

# Local prefixes
INCLUDE_PREFIX := ./include/
SOURCE_PREFIX := ./source/
OBJ_PREFIX := ./obj/
BIN_PREFIX := ./bin/
DEBUG_PREFIX := $(BIN_PREFIX)/Debug/
RELEASE_PREFIX := $(BIN_PREFIX)/Release/

DOLPHIN_PREFIX := Dolphin/
DOLPHIN_OBJ_PREFIX := $(OBJ_PREFIX)/$(DOLPHIN_PREFIX)


OUTPUT_PREFIX := $(DEBUG_PREFIX)

DEFINES :=

DOLPHIN_DEFINES := -DDOLPHIN

debug: DEFINES += -DDEBUG

# Add more target prefixes here
release: OUTPUT_PREFIX := $(RELEASE_PREFIX)

ifneq ($(COMPLETE_PREREQUISITES), true) #1
COMPLETE_PREREQUISITES := false
endif #1

ifeq ($(COMPLETE_PREREQUISITES), true) #1
AUTO_GENERATE_FLAG := -MM

ifeq ($(SYSTEM_INCLUDE_PREREQUISITE), true) #2
AUTO_GENERATE_FLAG := -M
endif #2

endif #1

INCLUDE := -nodefaults -i $(INCLUDE_PREFIX) -I- -i $(PATH_TO_GAME) -i $(PATH_TO_RFL) -i $(PATH_TO_RVL) -I- -i $(PATH_TO_TRK) -I- -i $(PATH_TO_RUNTIME) -I- -i $(PATH_TO_MSL_C) -I- -i $(PATH_TO_MSL_CPP) -I- -i $(PATH_TO_JSYS) -I- -i $(PATH_TO_NW4R) -I- -i $(BUSSUN_INCLUDE)

# Do not warn over consistent redeclarations, empty declarations
WARNFLAGS := -w all -pragma "warning off (10122)" -pragma "warning off (10216)"

CXXFLAGS := -c -Cpp_exceptions off -proc gekko -fp hard -ipa file -inline auto,level=2 -O4,s -rtti off -sdata 0 -sdata2 0 -align powerpc -enum int $(INCLUDE)

CFLAGS := -lang c99 $(CXXFLAGS) $(WARNFLAGS)

#ASMFLAGS := -c -proc gecko

O_FILES := net.o packets.o multimodel.o transmission.o packetProcessor.o multiplayer.o debug.o netActor.o alignment.o StarPieceSync.o

O_FILES := $(foreach obj, $(O_FILES), $(OBJ_PREFIX)/$(obj))

DOLPHIN_DEPENDENT_O_FILES := beacon.o

DOLPHIN_INSTANCE_O_FILES := $(foreach obj, $(DOLPHIN_DEPENDENT_O_FILES), $(DOLPHIN_OBJ_PREFIX)/$(obj))

NONDOLPHIN_INSTANCE_O_FILES := $(foreach obj, $(DOLPHIN_DEPENDENT_O_FILES), $(OBJ_PREFIX)/$(obj))


ifndef SYMBOL_MAP
$(error Error: no symbol map specified)
endif

export SYMBOL_MAP
SYMBOL_OSEV_ADDR = $(shell export SYMBOL_MAP="$(SYMBOL_MAP)" ; line=`grep OSExceptionVector $$SYMBOL_MAP` ; echo $${line#*=})

.PHONY: clean all debug release cleandeps

debug: all
release: all

dolphinDebug: | $(DEBUG_PREFIX)/$(DOLPHIN_PREFIX)
dolphinDebug: OUTPUT_PREFIX := $(DEBUG_PREFIX)
dolphinDebug: $(DEBUG_PREFIX)/$(DOLPHIN_PREFIX)/interrupts.xml $(DEBUG_PREFIX)/$(DOLPHIN_PREFIX)/CustomCode_USA.bin
dolphinRelease: | $(RELEASE_PREFIX)/$(DOLPHIN_PREFIX)
dolphinRelease: OUTPUT_PREFIX := $(RELEASE_PREFIX)
dolphinRelease: $(RELEASE_PREFIX)/$(DOLPHIN_PREFIX)/interrupts.xml $(RELEASE_PREFIX)/$(DOLPHIN_PREFIX)/CustomCode_USA.bin

all: | $(OUTPUT_PREFIX)
all: $(OUTPUT_PREFIX)/interrupts.xml $(OUTPUT_PREFIX)/CustomCode_USA.bin 



clean:
	rm -f $(OBJ_PREFIX)/* $(DEBUG_PREFIX)/* $(RELEASE_PREFIX)/*

cleandeps:
	rm -f $(OBJ_PREFIX)/*.d

#%.o: %.s
#	$(AS) $(ASMFLAGS) -o $@ $<

$(OBJ_PREFIX):
	mkdir -p $(OBJ_PREFIX)

$(DEBUG_PREFIX):
	mkdir -p $(DEBUG_PREFIX)

$(RELEASE_PREFIX):
	mkdir -p $(RELEASE_PREFIX)

$(DOLPHIN_OBJ_PREFIX):
	mkdir -p $(DOLPHIN_OBJ_PREFIX)

$(DEBUG_PREFIX)/$(DOLPHIN_PREFIX):
	mkdir -p $(DEBUG_PREFIX)/$(DOLPHIN_PREFIX)

$(RELEASE_PREFIX)/$(DOLPHIN_PREFIX):
	mkdir -p $(RELEASE_PREFIX)/$(DOLPHIN_PREFIX)

$(DOLPHIN_OBJ_PREFIX)/%.o: $(SOURCE_PREFIX)/%.cpp | $(DOLPHIN_OBJ_PREFIX)
	$(CXX) $(CXXFLAGS) $(DEFINES) $(DOLPHIN_DEFINES) -c -o $@ $<

$(OBJ_PREFIX)/%.o: $(SOURCE_PREFIX)/%.cpp | $(OBJ_PREFIX)
	$(CXX) $(CXXFLAGS) $(DEFINES) -c -o $@ $<

$(OBJ_PREFIX)/%.o: $(SOURCE_PREFIX)/%.c | $(OBJ_PREFIX)
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

$(OUTPUT_PREFIX)/interrupts.xml: $(OBJ_PREFIX)/interruptSubs.o 
	$(KAMEK) -static=$(SYMBOL_OSEV_ADDR) -externals=$(SYMBOL_MAP) -output-riiv=$@ $(OBJ_PREFIX)/interruptSubs.o

$(OUTPUT_PREFIX)/$(DOLPHIN_PREFIX)/interrupts.xml: $(OBJ_PREFIX)/interruptSubs.o 
	$(KAMEK) -static=$(SYMBOL_OSEV_ADDR) -externals=$(SYMBOL_MAP) -output-riiv=$@ $(OBJ_PREFIX)/interruptSubs.o

$(OUTPUT_PREFIX)/CustomCode_USA.bin: $(O_FILES) $(NONDOLPHIN_INSTANCE_O_FILES)
	$(KAMEK) -externals=$(SYMBOL_MAP) -output-kamek=$@ $(O_FILES) $(NONDOLPHIN_INSTANCE_O_FILES)

$(OUTPUT_PREFIX)/$(DOLPHIN_PREFIX)/CustomCode_USA.bin: $(O_FILES) $(DOLPHIN_INSTANCE_O_FILES)
	$(KAMEK) -externals=$(SYMBOL_MAP) -output-kamek=$@ $(O_FILES) $(DOLPHIN_INSTANCE_O_FILES)



ifeq ($(COMPLETE_PREREQUISITES), true) #1

$(OBJ_PREFIX)/%.d: $(SOURCE_PREFIX)/%.c* | $(OBJ_PREFIX)
	@$(CC) $(INCLUDE) $(AUTO_GENERATE_FLAG) $< | sed 's/Z:\\/\//;s?^[^:]*:?$@ $(@:.d=.o):?;s/\\/\//g;s/\/\x0D$$/\\/' > $@
$(DOLPHIN_OBJ_PREFIX)/%.d: $(SOURCE_PREFIX)/%.c* | $(DOLPHIN_OBJ_PREFIX)
	@$(CC) $(INCLUDE) $(AUTO_GENERATE_FLAG) $< | sed 's/Z:\\/\//;s?^[^:]*:?$@ $(@:.d=.o):?;s/\\/\//g;s/\/\x0D$$/\\/' > $@

include $(O_FILES:.o=.d) $(OBJ_PREFIX)/interruptSubs.d $(DOLPHIN_INSTANCE_O_FILES:.o=.d) $(NONDOLPHIN_INSTANCE_O_FILES:.o=.d)

endif #1
