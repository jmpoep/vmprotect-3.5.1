#WARN: no incremental build supported yet (use 'all' target carefully)

.PHONY: clean all rebuild

BIN_TARGET    := $(BIN_DIR)/$(TARGET)

CC            := clang
CXX           := clang++
LINK          := clang++
CFLAGS        := $(CFLAGS) -pipe -fvisibility=hidden -mmacosx-version-min=10.7 -DNDEBUG -O3 -gdwarf-2 -arch $(ARCH) -Wall -W -fPIC $(DEFINES)
CXXFLAGS      := $(CXXFLAGS) $(CFLAGS) -std=c++11 -stdlib=libc++ -fvisibility-inlines-hidden -fno-stack-protector -fno-builtin -Wno-reorder -Wno-switch -Wno-unused-parameter -Wno-unused-variable -Werror
LFLAGS        := $(LFLAGS) -std=c++11 -stdlib=libc++ -headerpad_max_install_names -mmacosx-version-min=10.7 -arch $(ARCH) -single_module
SDK_LIBS      := -framework CoreServices -framework SystemConfiguration
SLIBFLAGS     := -static -arch_only $(ARCH)
COPY_FILE     := cp -f
COPY_DIR      := cp -f -R

export MACOSX_DEPLOYMENT_TARGET = 10.7

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
