OBJECTS:= $(addprefix $(TMP_DIR)/, $(SOURCES:.cc=.o))

clean: 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) $(PCH_CPP)
	-$(DEL_FILE) $(BIN_TARGET)

$(BIN_TARGET): $(OBJECTS) $(BIN_DIR)/.sentinel $(OBJCOMP) $(DYLIBS)
	$(LINK) $(LFLAGS) -o $(BIN_TARGET) $(OBJECTS) $(LIBS) $(OBJCOMP) $(DYLIBS)

$(TMP_DIR)/%.o: %.cc $(PCH_CPP) $(TMP_DIR)/%/../.sentinel
	$(CXX) -c -include-pch $(PCH_CPP) $(CXXFLAGS) $(INCFLAGS) -o $(abspath $@) $(abspath $<)
