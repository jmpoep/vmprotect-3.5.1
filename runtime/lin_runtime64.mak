ARCH     := x86_64-linux-gnu
ARCH_DIR := 64
LFLAGS   := $(LFLAGS) -Wl,-z,max-page-size=4096
include lin_runtime.mak
