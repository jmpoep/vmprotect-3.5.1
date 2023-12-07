#ifndef MAINWINDOW_H
#define MAINWINDOW_H

class GUILog;
class FunctionBundle;
class FunctionPropertyManager;
class CorePropertyManager;
class SectionPropertyManager;
class SegmentPropertyManager;
class ImportPropertyManager;
class ExportPropertyManager;
class ResourcePropertyManager;
class LoadCommandPropertyManager;
class AddressCalculator;
class ProjectNode;
class IArchitecture;
class Core;
class WatermarksModel;
class TemplatesModel;
class ProjectModel;
class SearchModel;
class DirectoryModel;
class LogModel;
class FunctionsModel;
class InfoModel;
class DumpModel;
class DisasmModel;
class TabWidget;
class TreePropertyEditor;
class SearchLineEdit;
class ScriptEdit;
class License;
class LicensePropertyManager;
class InternalFilePropertyManager;
class AssemblyPropertyManager;
class FindWidget;
class ElidedAction;
class ToolButtonElided;

using namespace Scintilla;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
	MainWindow();
	~MainWindow();

protected:
	virtual void closeEvent(QCloseEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dropEvent(QDropEvent *event);
private slots:
	void notify(MessageType type, IObject *sender, const QString &message);
	void projectFilterChanged();
	void projectItemChanged();
	void treeItemDoubleClicked(const QModelIndex &index);
	void open();
	void loadFile(const QString &filenameOrBundle);
	bool save();
	void saveAs();
	void saveLicenses();
	bool closeFile();
	void projectTabMoved();
	void projectTabClicked();
	void scriptNotify(SCNotification *scn);
	void undo();
	void redo();
	void copy();
	void cut();
	void paste();
	void addFunction();
	void addFolder();
	void addFunctionFolder();
	void rename();
	void excludeFromCompilation();
	void block();
	void del();
	void homePage();
	void help();
	void about();
	void projectModified();
	void projectNodeRemoved(ProjectNode *node);
	void projectObjectRemoved(void *object);
	void compile();
	void execute();
	void executeOriginal();
	void executeProtected();
	void projectContextMenu(const QPoint &p);
	void scriptContextMenu(const QPoint &p);
	void functionContextMenu(const QPoint &p);
	void functionExtAddress();
	void functionEndAddress();
	void functionDel();
	void loadFileFromHistory();
	void loadFileFromBoot();
	void addLicense();
	void addFileFolder();
	void addFile();
	void createKeyPair();
	void exportKeyPair();
	void useOtherProject();
	void importLicense();
	void importProject();
	void examples();
	void watermarks();
	void settings();
	void showFile();
	void logItemDoubleClicked(const QModelIndex &index);
	void functionItemDoubleClicked(const QModelIndex &index);
	void disasmItemDoubleClicked(const QModelIndex &index);
	void goTo();
	void search();
	void showProtected();
	void contextFindNext();
	void contextFindPrevious();
	void contextFind(const QString &ttf, bool forward, bool incremental);
	void contextFindClosed();
	void scriptModeClicked();
	void templatesShow();
	void templatesSave();
	void templatesEdit();
	void templateSelect();
	void updateEditActions();
	void treeSectionClicked(int index);
	void recentFileContextMenu(const QPoint &p);
	void openRecentFile();
	void removeRecentFile();
	void fileChanged(const QString & path);
private:
	bool isContextSearchApplicable();
	bool findInScript(const QString &ttf, bool forward, bool incremental);
	bool findInView(QAbstractItemView *tv, const QString &ttf, bool forward, bool incremental);
	void showCurrentObject();
	void addRecentFile(int index, const QString file_name);
	void saveRecentFiles();
	bool internalLoadFile(const QString &filename);
	bool internalCompile();
	QTreeView *currentTreeView() const;
	ProjectNode *currentProjectNode(bool focusedTree = false) const;
	void updateCaption();
	void executeFile(const QString &fileName);
	void localize();
	void goToAddress(IArchitecture *file, uint64_t address);
	void goToDump(IArchitecture *file, uint64_t address, bool mode = false);
#ifdef ULTIMATE
	void createLicense(License *license);
	void updateLicensingActions();
#endif
	void updateTemplates();

	QString caption_;
	GUILog *log_;
	Core *core_;
	FunctionBundle *temp_function_;
	WatermarksModel *watermarks_model_;
	TemplatesModel *templates_model_;
	ProjectModel *project_model_;
	SearchModel *search_model_;
	DirectoryModel *directory_model_;
	LogModel *log_model_;
	FunctionPropertyManager *function_property_manager_;
	CorePropertyManager *core_property_manager_;
#ifndef LITE
	FunctionsModel *functions_model_;
	InfoModel *info_model_;
	DumpModel *dump_model_;
	DisasmModel *disasm_model_;
	SectionPropertyManager *section_property_manager_;
	SegmentPropertyManager *segment_property_manager_;
	ImportPropertyManager *import_property_manager_;
	ExportPropertyManager *export_property_manager_;
	ResourcePropertyManager *resource_property_manager_;
	LoadCommandPropertyManager *loadcommand_property_manager_;
	AddressCalculator *address_calculator_manager_;
	QTreeView *info_tree_;
#endif
#ifdef ULTIMATE
	LicensePropertyManager *license_property_manager_;
	InternalFilePropertyManager *internal_file_property_manager_;
	AssemblyPropertyManager *assembly_property_manager_;
#endif
	QFileSystemWatcher fs_watcher_;

	QAction *open_act_;
	QAction *save_act_;
	QAction *save_as_act_;
	QAction *close_act_;
	QAction *exit_act_;
	QAction *undo_act_;
	QAction *redo_act_;
	QAction *cut_act_;
	QAction *copy_act_;
	QAction *paste_act_;
	QAction *cut_act2_;
	QAction *copy_act2_;
	QAction *paste_act2_;
#ifdef LITE
	QAction *show_act_;
#else
	QAction *add_function_act_;
	QAction *add_function_act2_;
	QAction *add_folder_act_;
	QAction *goto_act_;
	QAction *goto_act2_;
	QAction *watermarks_act_;
#endif
#ifdef ULTIMATE
	QAction *save_licenses_act_;
	QAction *add_license_act_;
	QAction *add_license_act2_;
	QAction *add_file_act_;
	QAction *add_file_act2_;
	QAction *import_license_act_;
	QAction *import_project_act_;
	QAction *export_key_act_;
#endif
	QAction *block_act_;
	QAction *exclude_act_;
	QAction *rename_act_;
	QAction *delete_act_;
	QAction *project_filter_act_;
	QAction *compile_act_;
	QAction *execute_act_;
	ElidedAction *execute_original_act_;
	ElidedAction *execute_protected_act_;
	QWidgetAction *execute_parameters_act_;
	QAction *function_ext_address_act_;
	QAction *function_end_address_act_;
	QAction *function_del_act_;
	QAction *help_act_;
	QAction *home_page_act_;
	QAction *about_act_;
	QAction *history_separator_;
	QAction *project_separator_;
	QAction *settings_act_;
	QAction *search_act_;

	QMenu *file_menu_;
	QMenu *edit_menu_;
	QMenu *project_menu_;
#ifdef ULTIMATE
	QMenu *import_menu_;
#endif
	QMenu *help_menu_;
	QMenu *script_menu_;
	QMenu *tools_menu_;

	QStackedWidget *desktop_page_;
	QFrame *boot_frame_;
	QFrame *boot_panel_;
	QFrame *project_frame_;
	TabWidget *project_tab_;
	QTreeView *project_tree_;
	QTreeView *search_tree_;
	QTreeView *directory_tree_;
	QRadioButton *icon_project_;
	QStatusBar *status_bar_;
#ifndef LITE
	QMenu *add_menu_;
	QRadioButton *icon_functions_;
	QRadioButton *icon_details_;
	QTreeView *functions_tree_;
	QFrame *dump_page_;
	QTableView *dump_view_;
	QTableView *disasm_view_;
	TreePropertyEditor *section_property_editor_;
	TreePropertyEditor *segment_property_editor_;
	TreePropertyEditor *import_property_editor_;
	TreePropertyEditor *export_property_editor_;
	TreePropertyEditor *resource_property_editor_;
	TreePropertyEditor *loadcommand_property_editor_;
	TreePropertyEditor *address_calculator_;
#endif
	SearchLineEdit *project_filter_;
	QStackedWidget *project_page_;
	ScriptEdit *script_editor_;
	QLabel *project_file_name_;
	QLabel *script_line_;
	QLabel *script_column_;
	QLabel *script_mode_;
	FindWidget *context_find_;
	TreePropertyEditor *function_property_editor_;
	TreePropertyEditor *core_property_editor_;
#ifdef ULTIMATE
	TreePropertyEditor *license_property_editor_;
	TreePropertyEditor *internal_file_property_editor_;
	TreePropertyEditor *assembly_property_editor_;
	QFrame *licensing_parameters_page_;
	QLabel *licensing_parameters_help_;
	QLabel *key_algo_label_;
	QLabel *key_len_label_;
	QComboBox *key_len_;
	QPushButton *create_key_button_;
	QPushButton *use_other_project_button_;
#endif
	ToolButtonElided *spacer_;
	QWidget *project_separator_widget_;
	QTreeView *log_tree_;
	QMenu *execute_menu_;
	QLineEdit *parameters_edit_;

	QBoxLayout *recent_files_layout_;
	QToolButton *open_button_;
	QToolButton *examples_button_;
	QToolButton *help_button_;
	QLabel *recent_files_label_;
	QLabel *quick_start_label_;
	QLabel *welcome_label_;
	QAction *templates_act_;
	QAction *templates_save_act_;
	QAction *templates_edit_act_;
	QMenu *templates_menu_;
	QAction *recent_file_open_act_;
	QAction *recent_file_remove_act_;
	int recent_file_;
	QMenu *recent_file_menu_;
	bool fileChanged_;
};

#endif