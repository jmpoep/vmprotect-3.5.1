SOURCES := core.cc crypto.cc loader.cc licensing_manager.cc string_manager.cc hwid.cc objects.cc utils.cc ../third-party/lzma/LzmaDecode.cc

PROJECT       := lin_runtime
TARGET        := $(PROJECT).so
BIN_DIR       := ../bin/$(ARCH_DIR)/Release
LIB_TARGET    := $(BIN_DIR)/$(PROJECT).a
TMP_DIR       := ../tmp/lin/runtime/$(ARCH_DIR)/runtime
PCH_DIR       := $(TMP_DIR)/runtime.gch
DEFINES       :=
LFLAGS        := $(LFLAGS) -Wl,--no-undefined -shared -Wl,--wrap=memcpy -Wl,--wrap=__poll_chk -Wl,--wrap=__fdelt_chk
LIBS           = ~/curl-7.35.0-$(ARCH_DIR)/lib/libcurl.so -ldl -lrt -L../bin/ -lVMProtectSDK$(ARCH_DIR)
DYLIBS        := ../bin/libVMProtectSDK$(ARCH_DIR).so
OBJCOMP       :=
OBJECTS       := $(addprefix $(TMP_DIR)/, $(SOURCES:.cc=.o))
INCFLAGS      := -I ~/curl-7.35.0-$(ARCH_DIR)/include

include ../lin_common.mak

clean: 
	-$(DEL_FILE) $(LIB_TARGET)
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) $(PCH_CPP)
	-$(DEL_FILE) $(BIN_TARGET)

$(LIB_TARGET): $(OBJECTS) $(BIN_DIR)/.sentinel
	ar $(SLIBFLAGS) $(LIB_TARGET) $(abspath $(OBJECTS)) $(OBJCOMP)

$(BIN_TARGET): $(LIB_TARGET) $(OBJCOMP) $(DYLIBS)
	$(LINK) $(LFLAGS) -o $(BIN_TARGET) $(OBJECTS) $(LIBS) $(OBJCOMP)

$(TMP_DIR)/%.o: %.cc $(PCH_CPP) $(TMP_DIR)/%/../.sentinel
	$(CXX) -c -include-pch $(PCH_CPP) $(CXXFLAGS) $(INCFLAGS) -o $(abspath $@) $(abspath $<)
