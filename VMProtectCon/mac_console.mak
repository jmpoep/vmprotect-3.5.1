SOURCES := console.cc main.cc


PROJECT       := vmprotect_con
TARGET        := $(PROJECT)
BIN_DIR       := ../bin/$(ARCH_DIR)/$(CFG_DIR)
TMP_ARCH_DIR  := ../tmp/mac/console/$(ARCH_DIR)
TMP_DIR       := $(TMP_ARCH_DIR)/$(CFG_DIR)/$(PROJECT)
PCH_DIR       := $(TMP_DIR)/$(PROJECT).gch
DEFINES       := $(CONFIG) -DTIXML_USE_STL
LFLAGS        :=
LIBS          :=  -framework CoreServices -framework Security
OBJCOMP       := ../bin/$(ARCH_DIR)/invariant_core.a ../bin/$(ARCH_DIR)/$(CFG_DIR)/core.a /usr/local/opt/libffi/lib/libffi.a
DYLIBS        := ../bin/libVMProtectSDK.dylib
PCH_DIR       := $(TMP_DIR)

include ../mac_common.mak
include ../gnu_simple.mak