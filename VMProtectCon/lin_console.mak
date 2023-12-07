SOURCES := console.cc main.cc


PROJECT       := vmprotect_con
TARGET        := $(PROJECT)
BIN_DIR       := ../bin/$(ARCH_DIR)/$(CFG_DIR)
TMP_ARCH_DIR  := ../tmp/lin/console/$(ARCH_DIR)
TMP_DIR       := $(TMP_ARCH_DIR)/$(CFG_DIR)/$(PROJECT)
PCH_DIR       := $(TMP_DIR)/$(PROJECT).gch
DEFINES       := $(CONFIG) -DTIXML_USE_STL
LFLAGS        := -Wl,-rpath,\$$ORIGIN
LIBS          := $(SDK_LIBS) -Wl,--no-as-needed -L../bin -lVMProtectSDK$(ARCH_DIR) -ldl -Wl,--as-needed
OBJCOMP       :=  ../bin/$(ARCH_DIR)/$(CFG_DIR)/core.a ../bin/$(ARCH_DIR)/invariant_core.a ../third-party/libffi/libffi$(ARCH_DIR).a /usr/lib/$(ARCH)/libcrypto.a
DYLIBS        :=
PCH_DIR       := $(TMP_DIR)

include ../lin_common.mak
include ../gnu_simple.mak