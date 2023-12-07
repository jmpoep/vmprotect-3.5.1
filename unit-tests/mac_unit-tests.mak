SOURCES := core_tests.cc files_tests.cc intel_tests.cc il_tests.cc macfile_tests.cc pefile_tests.cc elffile_tests.cc test1.cc ../third-party/lzma/LzmaDecode.cc

PROJECT       := unit.Tests
TARGET        := $(PROJECT)
BIN_DIR       := ../bin/$(ARCH_DIR)/$(CFG_DIR)
TMP_DIR       := ../tmp/mac/$(PROJECT)/$(ARCH_DIR)/TESTS
PCH_DIR       := $(TMP_DIR)/$(PROJECT).gch
DEFINES       := $(CONFIG) -D TIXML_USE_STL -D _CONSOLE -D FFI_BUILDING
LFLAGS        := 
LIBS           = $(SDK_LIBS) -framework Security
OBJCOMP       := ../bin/$(ARCH_DIR)/Release/gtests.a ../bin/$(ARCH_DIR)/invariant_core.a ../bin/$(ARCH_DIR)/$(CFG_DIR)/core.a /usr/local/opt/libffi/lib/libffi.a
DYLIBS        := ../bin/libVMProtectSDK.dylib
INCFLAGS      := -I ../third-party/gmock/include/ -I ../third-party/gtest/include/ -I ../third-party/gmock/ -I ../third-party/gtest/

include ../mac_common.mak
include ../gnu_simple.mak
