SOURCES := about_dialog.cc export_key_pair_dialog.cc function_dialog.cc import_license_dialog.cc license_dialog.cc main.cc mainwindow.cc message_dialog.cc remotecontrol.cc template_save_dialog.cc templates_window.cc \
 models.cc progress_dialog.cc property_editor.cc resources.cc settings_dialog.cc watermark_dialog.cc watermarks_window.cc widgets.cc help_browser.cc \
 ../third-party/scintilla/ScintillaQt.cc ../third-party/scintilla/ScintillaEditBase.cc \
 ../third-party/scintilla/PlatQt.cc

MOC_HEADERS := application.h mainwindow.h property_editor.h widgets.h models.h about_dialog.h watermark_dialog.h message_dialog.h settings_dialog.h watermarks_window.h remotecontrol.h \
 export_key_pair_dialog.h function_dialog.h import_license_dialog.h license_dialog.h progress_dialog.h help_browser.h template_save_dialog.h templates_window.h

SC_MOC_HEADERS := ScintillaQt.h ScintillaEditBase.h

PROJECT       := vmprotect_gui
TARGET        := $(PROJECT)
QT_DIR        := /home/vano/Qt/5.6/gcc_$(ARCH_DIR)
BIN_DIR       := ../bin/$(ARCH_DIR)/$(CFG_DIR)
APP_NAME      := vmprotect_gui.unarc
APP_DIR       := $(BIN_DIR)/$(APP_NAME)
SDK_DYLIB     := libVMProtectSDK$(ARCH_DIR).so
TMP_ARCH_DIR  := ../tmp/lin/gui/$(ARCH_DIR)
TMP_DIR       := $(TMP_ARCH_DIR)/$(CFG_DIR)/$(PROJECT)
PCH_DIR       := $(TMP_DIR)/$(PROJECT).gch
DEFINES       := $(CONFIG) -DTIXML_USE_STL -DSCI_NAMESPACE
LFLAGS        := -Wl,-rpath,\$$ORIGIN,-rpath-link,$(QT_DIR)/lib
LIBS          := $(SDK_LIBS) -L$(QT_DIR)/lib -lQt5Gui -lQt5Core -lQt5Help -lQt5Widgets -Wl,--no-as-needed  -L../bin -lVMProtectSDK$(ARCH_DIR) -ldl -Wl,--as-needed
OBJCOMP       := ../bin/$(ARCH_DIR)/$(CFG_DIR)/core.a ../bin/$(ARCH_DIR)/invariant_core.a ../third-party/libffi/libffi$(ARCH_DIR).a /usr/lib/$(ARCH)/libcrypto.a
DYLIBS        :=
PCH_DIR       := $(TMP_DIR)
MOC_TEMPLATE  := moc/moc_
SC_MOC_TEMPLATE  := ../VMProtect/moc/moc_
INCFLAGS      := -I$(QT_DIR)/include/

include ../lin_common.mak

OBJECTS      := $(addprefix $(TMP_DIR)/, $(SOURCES:.cc=.o))
MOC_RESULTS  := $(addprefix $(MOC_TEMPLATE), $(MOC_HEADERS:.h=.cc))
SC_MOC_RESULTS  := $(addprefix $(SC_MOC_TEMPLATE), $(SC_MOC_HEADERS:.h=.cc))

clean:
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) $(PCH_CPP)
	-$(DEL_FILE) $(BIN_TARGET)
	-$(DEL_FILE) $(MOC_RESULTS)
	-$(DEL_FILE) $(SC_MOC_RESULTS)
	-$(DEL_DIR) $(APP_DIR)

all:: $(BIN_TARGET)

$(BIN_TARGET): moc/.sentinel $(MOC_RESULTS) $(SC_MOC_RESULTS) $(OBJECTS) $(BIN_DIR)/.sentinel $(OBJCOMP)
	$(LINK) $(LFLAGS) -o $(BIN_TARGET) $(OBJECTS) $(LIBS) $(OBJCOMP)

$(TMP_DIR)/%.o: %.cc $(PCH_CPP) $(TMP_DIR)/%/../.sentinel
	$(CXX) -c -include-pch $(PCH_CPP) $(CXXFLAGS) $(INCFLAGS) -o $(abspath $@) $(abspath $<)

$(MOC_TEMPLATE)%.cc: %.h
	moc -DSCI_NAMESPACE -DUNICODE -DQT_LARGEFILE_SUPPORT -DQT_DLL -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_HAVE_MMX -DQT_HAVE_3DNOW -DQT_HAVE_SSE -DQT_HAVE_MMXEXT -DQT_HAVE_SSE2 -DQT_THREAD_SUPPORT $(abspath $<) -o $(abspath $@)

$(SC_MOC_TEMPLATE)%.cc: ../third-party/scintilla/%.h
	moc -DSCI_NAMESPACE -DUNICODE -DQT_LARGEFILE_SUPPORT -DQT_DLL -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_HAVE_MMX -DQT_HAVE_3DNOW -DQT_HAVE_SSE -DQT_HAVE_MMXEXT -DQT_HAVE_SSE2 -DQT_THREAD_SUPPORT $(abspath $<) -o $(abspath $@)
