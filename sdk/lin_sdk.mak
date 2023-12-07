SOURCES := sdk.cc

TARGET        := libVMProtectSDK$(ARCH_DIR).so
BIN_DIR       := ../bin
TMP_DIR       := ../tmp/lin/sdk/$(ARCH_DIR)/SDK
PCH_DIR       := $(TMP_DIR)/sdk.gch
DEFINES       := 
LFLAGS        := -shared -fPIC
LIBS          := 
OBJCOMP       :=

include ../lin_common.mak
include ../gnu_simple.mak
