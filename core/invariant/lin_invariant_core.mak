SOURCES := \
 $(addprefix ../../third-party/tinyxml/, \
    tinyxml.cpp \
    tinyxmlparser.cpp \
    tinyxmlerr.cpp) \
 $(addprefix ../../third-party/scintilla/, \
    XPM.cxx \
    WordList.cxx \
    ViewStyle.cxx \
    UniConversion.cxx \
    StyleContext.cxx \
    Style.cxx \
    Selection.cxx \
    ScintillaBase.cxx \
    RunStyles.cxx \
    RESearch.cxx \
    PropSetSimple.cxx \
    PositionCache.cxx \
    PerLine.cxx \
    LineMarker.cxx \
    LexerSimple.cxx \
    LexerModule.cxx \
    LexerBase.cxx \
    LexLua.cxx \
    KeyMap.cxx \
    Indicator.cxx \
    ExternalLexer.cxx \
    Editor.cxx \
    Document.cxx \
    Decoration.cxx \
    ContractionState.cxx \
    CharacterSet.cxx \
    CharClassify.cxx \
    CellBuffer.cxx \
    Catalogue.cxx \
    CaseFolder.cxx \
    CaseConvert.cxx \
    CallTip.cxx \
    AutoComplete.cxx \
    Accessor.cxx)

SOURCES_C := \
 $(addprefix ../../third-party/demangle/, \
    cp-demangle.c \
    undname.c \
    unmangle.c) \
 $(addprefix ../../third-party/lzma/, \
    Alloc.c \
    LzFind.c \
    LzmaEnc.c) \
 $(addprefix ../../third-party/lua/, \
    lapi.c \
    lauxlib.c \
    lstate.c \
    ldebug.c \
    lzio.c \
    llex.c \
    lctype.c \
    lvm.c \
    ldump.c \
    ltm.c \
    lstring.c \
    lundump.c \
    lobject.c \
    lparser.c \
    lcode.c \
    lmem.c \
    ltable.c \
    ldo.c \
    lgc.c \
    lfunc.c \
    linit.c \
    lopcodes.c \
    loadlib.c \
    ltablib.c \
    lstrlib.c \
    liolib.c \
    lmathlib.c \
    loslib.c \
    lbaselib.c \
    lbitlib.c \
    lcorolib.c \
    ldblib.c)

PROJECT       := invariant_core
TARGET        := $(PROJECT).a
BIN_DIR       := ../../bin/$(ARCH_DIR)
TMP_DIR       := ../../tmp/lin/$(PROJECT)/$(ARCH_DIR)/ICORE
DEFINES       := -DSCI_NAMESPACE -DTIXML_USE_STL -DSPV_LIBRARY -DSCI_LEXER -DSPV_LIBRARY -D_7ZIP_ST -DLUA_USE_MKSTEMP -DFFI_BUILDING -DLUA_USE_POSIX
CFLAGS        := -Wno-deprecated-declarations
LFLAGS        :=
LIBS          :=
OBJCOMP       :=

TMP_DIR_CORE_INVAR   := $(TMP_DIR)/CORE_INVAR
TMP_DIR_CORE_INVAR_C := $(TMP_DIR)/CORE_INVAR_C

OBJECTS_CORE_INVAR   := $(addsuffix .o, $(addprefix $(TMP_DIR_CORE_INVAR)/, $(SOURCES)))
OBJECTS_CORE_INVAR_C := $(addsuffix .o, $(addprefix $(TMP_DIR_CORE_INVAR_C)/, $(SOURCES_C)))

OBJECTS              := $(OBJECTS_CORE_INVAR) $(OBJECTS_CORE_INVAR_C)

PCH_DIR              := $(TMP_DIR_CORE_INVAR)

PCH_C                := $(TMP_DIR_CORE_INVAR_C)/precompiled_c.h.gch

$(PCH_C): precompiled_c.h $(TMP_DIR_CORE_INVAR_C)/.sentinel
	$(CC) $(CFLAGS) $(INCFLAGS) -x c-header precompiled_c.c -o $(PCH_C)

include ../../lin_common.mak

clean: 
	-$(DEL_FILE) $(abspath $(OBJECTS))
	-$(DEL_FILE) $(PCH_CPP) $(PCH_C)
	-$(DEL_FILE) $(BIN_TARGET)

$(TMP_DIR_CORE_INVAR)/%.o: % $(PCH_CPP) $(TMP_DIR_CORE_INVAR)/%/../.sentinel
	$(CXX) -c -include-pch $(PCH_CPP) $(CXXFLAGS) $(INCFLAGS) -o $(abspath $@) $(abspath $<)

$(TMP_DIR_CORE_INVAR_C)/%.o: % $(PCH_C) $(TMP_DIR_CORE_INVAR_C)/%/../.sentinel
	$(CC) -c -include-pch $(PCH_C) $(CFLAGS) $(INCFLAGS) -o $(abspath $@) $(abspath $<)

$(BIN_TARGET): $(OBJECTS) $(BIN_DIR)/.sentinel $(OBJCOMP)
	ar $(SLIBFLAGS) $(BIN_TARGET) $(abspath $(OBJECTS)) $(OBJCOMP)
