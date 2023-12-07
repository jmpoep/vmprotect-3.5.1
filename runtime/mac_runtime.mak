SOURCES := core.cc crypto.cc loader.cc licensing_manager.cc string_manager.cc hwid.cc objects.cc utils.cc ../third-party/lzma/LzmaDecode.cc

PROJECT       := mac_runtime
TARGET        := $(PROJECT).dylib
BIN_DIR       := ../bin/$(ARCH_DIR)/Release
LIB_TARGET     := $(BIN_DIR)/$(PROJECT).a
TMP_DIR       := ../tmp/mac/runtime/$(ARCH_DIR)/runtime
PCH_DIR       := $(TMP_DIR)/runtime.gch
DEFINES       :=
LFLAGS        := -dynamiclib
LIBS           = $(SDK_LIBS)  -framework DiskArbitration
DYLIBS        := ../bin/libVMProtectSDK.dylib
OBJCOMP       :=
OBJECTS       := $(addprefix $(TMP_DIR)/, $(SOURCES:.cc=.o))

include ../mac_common.mak

clean: 
	-$(DEL_FILE) $(LIB_TARGET)
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) $(PCH_CPP)
	-$(DEL_FILE) $(BIN_TARGET)

$(LIB_TARGET): $(OBJECTS) $(BIN_DIR)/.sentinel
	libtool $(SLIBFLAGS) -o $(LIB_TARGET) $(abspath $(OBJECTS)) $(OBJCOMP)

$(BIN_TARGET): $(LIB_TARGET) $(OBJCOMP) $(DYLIBS)
	$(LINK) $(LFLAGS) -o $(BIN_TARGET) $(OBJECTS) $(LIBS) $(OBJCOMP) $(DYLIBS)

$(TMP_DIR)/%.o: %.cc $(PCH_CPP) $(TMP_DIR)/%/../.sentinel
	$(CXX) -c -include-pch $(PCH_CPP) $(CXXFLAGS) $(INCFLAGS) -o $(abspath $@) $(abspath $<)
