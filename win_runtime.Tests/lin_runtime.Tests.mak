SOURCES := ../runtime/core.cc ../runtime/crypto.cc ../runtime/loader.cc ../runtime/licensing_manager.cc ../runtime/string_manager.cc ../runtime/hwid.cc ../runtime/objects.cc ../runtime/utils.cc core_tests.cc crypto_tests.cc main.cc ../third-party/lzma/LzmaDecode.cc

PROJECT       := runtime.Tests
TARGET        := $(PROJECT)
BIN_DIR       := ../bin/$(ARCH_DIR)/Release
TMP_DIR       := ../tmp/lin/$(PROJECT)/$(ARCH_DIR)/$(PROJECT)
PCH_DIR       := $(TMP_DIR)/$(PROJECT).gch
DEFINES       := -D TIXML_USE_STL -D _CONSOLE
LFLAGS        := 
LIBS           = $(SDK_LIBS) -lpthread -L../bin -lVMProtectSDK$(ARCH_DIR) ~/curl-7.35.0-$(ARCH_DIR)/lib/libcurl.so -ldl
OBJCOMP       := ../bin/$(ARCH_DIR)/Release/gtests.a
DYLIBS        :=
INCFLAGS      := -I ../third-party/gmock/include/ -I ../third-party/gtest/include/ -I ../third-party/gmock/ -I ../third-party/gtest/ -I ~/curl-7.35.0-$(ARCH_DIR)/include

include ../lin_common.mak
include ../gnu_simple.mak
