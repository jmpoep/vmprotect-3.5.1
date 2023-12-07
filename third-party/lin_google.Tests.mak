SOURCES := gtest/src/gtest-all.cc gmock/src/gmock-all.cc

PROJECT       := gtests
TARGET        := $(PROJECT).a
BIN_DIR       := ../bin/$(ARCH_DIR)/Release
TMP_DIR       := ../tmp/lin/gtests/$(ARCH_DIR)/$(PROJECT)
DEFINES       :=
LFLAGS        :=
LIBS          :=
OBJCOMP       :=
INCFLAGS      := -I ../third-party/gmock/include/ -I ../third-party/gtest/include/ -I ../third-party/gmock/ -I ../third-party/gtest/
OBJECTS       := $(addprefix $(TMP_DIR)/, $(SOURCES:.cc=.o))

include ../lin_common.mak

clean: 
	-$(DEL_FILE) $(abspath $(OBJECTS))
	-$(DEL_FILE) $(BIN_TARGET)

$(TMP_DIR)/%.o: %.cc $(TMP_DIR)/%/../.sentinel
	$(CXX) -c $(CXXFLAGS) $(INCFLAGS) -o $(abspath $@) $(abspath $<)

$(BIN_TARGET): $(OBJECTS) $(BIN_DIR)/.sentinel $(OBJCOMP)
	ar $(SLIBFLAGS) $(BIN_TARGET) $(abspath $(OBJECTS)) $(OBJCOMP)
