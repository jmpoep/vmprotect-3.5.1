SOURCES := sdk.cc

TARGET        := libVMProtectSDK$(ARCH_DIR).dylib
BIN_DIR       := ../bin
TMP_DIR       := ../tmp/mac/sdk/$(ARCH_DIR)/SDK
PCH_DIR       := $(TMP_DIR)/sdk.gch
DEFINES       := 
LFLAGS        := -install_name @executable_path/libVMProtectSDK.dylib -dynamiclib
LIBS          := -F/Library/Frameworks -L/Library/Frameworks
OBJCOMP       :=

include ../mac_common.mak
include ../gnu_simple.mak
