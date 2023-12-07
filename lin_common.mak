#WARN: no incremental build supported yet (use 'all' target carefully)

.PHONY: clean all rebuild

BIN_TARGET    := $(BIN_DIR)/$(TARGET)

CC            := clang
CXX           := clang++
LINK          := clang++
CFLAGS        := $(CFLAGS) -pipe -fvisibility=hidden -DNDEBUG -O3 -gdwarf-2 -target $(ARCH) -Wall -W -fPIC $(DEFINES)
CXXFLAGS      := $(CXXFLAGS) $(CFLAGS) -std=c++11 -fvisibility-inlines-hidden -fno-stack-protector -fno-builtin -Wno-reorder -Wno-switch -Wno-unused-parameter -Wno-unused-variable -Werror
LFLAGS        := $(LFLAGS) -Wl,--as-needed -std=c++11 -target $(ARCH)
SDK_LIBS      := 
SLIBFLAGS     := -crs
COPY_FILE     := cp -f
COPY_DIR      := cp -f -R

DEL_FILE      := rm -f
DEL_DIR       := rm -rf --

HPCH          := precompiled.h
PCH_CPP       := $(PCH_DIR)/$(HPCH).gch
INCFLAGS      := -I ./ $(INCFLAGS)

.PRECIOUS: %/.sentinel  
%/.sentinel:
	test -e $(abspath $@) || (mkdir -p $(abspath ${@D}/) && touch $(abspath $@))

rebuild: clean all

.SUFFIXES:

all:: $(BIN_TARGET)

$(PCH_CPP): precompiled.h $(PCH_DIR)/.sentinel
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -x c++-header -c precompiled.cc -o $(PCH_CPP)
