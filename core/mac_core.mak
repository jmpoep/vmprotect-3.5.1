SOURCES := \
 $(addprefix ../core/, \
	core.cc \
	files.cc \
	inifile.cc \
	dotnetfile.cc \
	dwarf.cc \
	elffile.cc \
	intel.cc \
	il.cc \
	lang.cc \
	objc.cc \
	macfile.cc \
	objects.cc \
	osutils.cc \
	packer.cc \
	pefile.cc \
	processors.cc \
	script.cc \
	streams.cc) \
	../runtime/crypto.cc
   

PROJECT       := core
TARGET        := $(PROJECT).a
BIN_DIR       := ../bin/$(ARCH_DIR)/$(CFG_DIR)
TMP_DIR       := ../tmp/mac/$(PROJECT)/$(ARCH_DIR)/$(CFG_DIR)/$(PROJECT)
DEFINES       := $(CONFIG) -DTIXML_USE_STL -DSPV_LIBRARY -DFFI_BUILDING
LFLAGS        :=
LIBS          :=
OBJCOMP       :=

OBJECTS       := $(addsuffix .o, $(addprefix $(TMP_DIR)/, $(SOURCES)))

PCH_DIR       := $(TMP_DIR)

include ../mac_common.mak

clean: 
	-$(DEL_FILE) $(abspath $(OBJECTS))
	-$(DEL_FILE) $(PCH_CPP)
	-$(DEL_FILE) $(BIN_TARGET)

$(TMP_DIR)/%.o: % $(PCH_CPP) $(TMP_DIR)/%/../.sentinel
	$(CXX) -c -include-pch $(PCH_CPP) $(CXXFLAGS) $(INCFLAGS) -o $(abspath $@) $(abspath $<)

$(BIN_TARGET): $(OBJECTS) $(BIN_DIR)/.sentinel $(LIBS) $(OBJCOMP)
	libtool $(SLIBFLAGS) -o $(BIN_TARGET) $(abspath $(OBJECTS)) $(OBJCOMP)