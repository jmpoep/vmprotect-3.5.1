#include "../core/objects.h"
#include "../core/files.h"
#include "../core/core.h"
#include "../core/processors.h"
#include "../core/script.h"
#include "../core/lang.h"
#include "../core/inifile.h"

#include "models.h"
#include "widgets.h"
#include "property_editor.h"
#include "progress_dialog.h"
#include "mainwindow.h"
#include "moc/moc_mainwindow.cc"
#include "function_dialog.h"
#ifdef ULTIMATE
#include "license_dialog.h"
#include "import_license_dialog.h"
#include "export_key_pair_dialog.h"
#endif
#include "watermarks_window.h"
#include "settings_dialog.h"
#include "message_dialog.h"
#include "about_dialog.h"
#include "wait_cursor.h"
#include "help_browser.h"
#include "template_save_dialog.h"
#include "templates_window.h"
#include "application.h"

#ifndef VMP_GNU
// cannot include header - name conflict with our "Folder"
#define CSIDL_COMMON_DOCUMENTS          0x002e        // All Users\Documents
SHSTDAPI_(BOOL) SHGetSpecialFolderPathW(__reserved HWND hwnd, __out_ecount(MAX_PATH) LPWSTR pszPath, __in int csidl, __in BOOL fCreate);
#endif

/**
 * MainWindow
 */

MainWindow::MainWindow()
	: QMainWindow(), temp_function_(NULL), fileChanged_(false)
{
	installEventFilter(this);

	VMProtectBeginVirtualization("Caption");
#ifdef DEMO
	caption_ = QString::fromLatin1(Core::edition()).append(' ').append(QString::fromLatin1(VMProtectDecryptStringA("[demo]")));
#else
	bool is_registered = false;
	{
		VMProtectSerialNumberData serial_data;
		if (VMProtectSetSerialNumber(VMProtectDecryptStringA("SerialNumber")) == SERIAL_STATE_SUCCESS && VMProtectGetSerialNumberData(&serial_data, sizeof(serial_data))) {
			if (Core::check_license_edition(serial_data))
				is_registered = true;
			else
				VMProtectSetSerialNumber(NULL);
		}
	}
	caption_ = QString::fromLatin1(Core::edition()).append(' ').append(QString::fromLatin1(is_registered ? VMProtectDecryptStringA("[registered]") : VMProtectDecryptStringA("[unregistered]")));
#endif
	setWindowTitle(caption_);
	VMProtectEnd();

	// create internal objects
	log_ = new GUILog(this);
	connect(log_, SIGNAL(notify(MessageType, IObject *, const QString &)), this, SLOT(notify(MessageType, IObject *, const QString &)));

	core_ = new Core(qobject_cast<ILog*>(log_));

	watermarks_model_ = new WatermarksModel(this);
	watermarks_model_->setCore(core_);
	WatermarksWindow::setModel(watermarks_model_);

	templates_model_ = new TemplatesModel(this);
	templates_model_->setCore(core_);
	TemplatesWindow::setModel(templates_model_);

	project_model_ = new ProjectModel(this);
	connect(project_model_, SIGNAL(modified()), this, SLOT(projectModified()));
	connect(project_model_, SIGNAL(nodeRemoved(ProjectNode *)), this, SLOT(projectNodeRemoved(ProjectNode *)));
	connect(project_model_, SIGNAL(objectRemoved(void *)), this, SLOT(projectObjectRemoved(void *)));

	search_model_ = new SearchModel(this);
	directory_model_ = new DirectoryModel(this);
	log_model_ = new LogModel(this);
	function_property_manager_ = new FunctionPropertyManager(this);
	core_property_manager_ = new CorePropertyManager(this);
#ifndef LITE
	functions_model_ = new FunctionsModel(this);
	info_model_ = new InfoModel(this);
	connect(info_model_, SIGNAL(modified()), this, SLOT(projectModified()));

	dump_model_ = new DumpModel(this);
	disasm_model_ = new DisasmModel(this);
	section_property_manager_ = new SectionPropertyManager(this);
	segment_property_manager_ = new SegmentPropertyManager(this);
	import_property_manager_ = new ImportPropertyManager(this);
	export_property_manager_ = new ExportPropertyManager(this);
	resource_property_manager_ = new ResourcePropertyManager(this);
	loadcommand_property_manager_ = new LoadCommandPropertyManager(this);
	address_calculator_manager_ = new AddressCalculator(this);
#endif
#ifdef ULTIMATE
	license_property_manager_ = new LicensePropertyManager(this);
	internal_file_property_manager_ = new InternalFilePropertyManager(this);
	assembly_property_manager_ = new AssemblyPropertyManager(this);
#endif

	// create actions
	QIcon icon = QIcon(":/images/open.png");
	icon.addPixmap(QPixmap(":/images/open_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/open_disabled.png"), QIcon::Disabled, QIcon::Off);
	open_act_ = new QAction(icon, QString::fromUtf8(language[lsOpen].c_str()) + "...", this);
	open_act_->setShortcut(QString("Ctrl+O"));
	connect(open_act_, SIGNAL(triggered()), this, SLOT(open()));

	icon = QIcon(":/images/disk.png");
	icon.addPixmap(QPixmap(":/images/disk_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/disk_disabled.png"), QIcon::Disabled, QIcon::Off);
	save_act_ = new QAction(icon, QString::fromUtf8(language[lsSaveProject].c_str()), this);
	save_act_->setShortcut(QString("Ctrl+S"));
	save_act_->setEnabled(false);
	connect(save_act_, SIGNAL(triggered()), this, SLOT(save()));

	save_as_act_ = new QAction(QString::fromUtf8(language[lsSaveProjectAs].c_str()) + "...", this);
	save_as_act_->setShortcut(QKeySequence::SaveAs);
	save_as_act_->setEnabled(false);
	connect(save_as_act_, SIGNAL(triggered()), this, SLOT(saveAs()));

	close_act_ = new QAction(QString::fromUtf8(language[lsClose].c_str()), this);
	close_act_->setShortcut(QString("Ctrl+W"));
	close_act_->setEnabled(false);
	connect(close_act_, SIGNAL(triggered()), this, SLOT(closeFile()));

	exit_act_ = new QAction(QString::fromUtf8(language[lsExit].c_str()), this);
	connect(exit_act_, SIGNAL(triggered()), this, SLOT(close()));

	help_act_ = new QAction(QString::fromUtf8(language[lsContents].c_str()), this);
	help_act_->setShortcut(HelpContentsKeySequence());
	connect(help_act_, SIGNAL(triggered()), this, SLOT(help()));

	home_page_act_ = new QAction(QString::fromUtf8(language[lsHomePage].c_str()), this);
	home_page_act_->setToolTip("http://www.vmpsoft.com");
	connect(home_page_act_, SIGNAL(triggered()), this, SLOT(homePage()));

	about_act_ = new QAction(QString::fromUtf8(language[lsAbout].c_str()) + "...", this);
	connect(about_act_, SIGNAL(triggered()), this, SLOT(about()));

	undo_act_ = new QAction(QString::fromUtf8(language[lsUndo].c_str()), this);
	undo_act_->setShortcut(QKeySequence::Undo);
	undo_act_->setEnabled(false);
	connect(undo_act_, SIGNAL(triggered()), this, SLOT(undo()));

	redo_act_ = new QAction(QString::fromUtf8(language[lsRedo].c_str()), this);
	redo_act_->setShortcut(QKeySequence::Redo);
	redo_act_->setEnabled(false);
	connect(redo_act_, SIGNAL(triggered()), this, SLOT(redo()));

	icon = QIcon(":/images/cut.png");
	icon.addPixmap(QPixmap(":/images/cut_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/cut_disabled.png"), QIcon::Disabled, QIcon::Off);
	cut_act_ = new QAction(icon, QString::fromUtf8(language[lsCut].c_str()), this);
	cut_act_->setShortcut(QKeySequence::Cut);
	cut_act_->setEnabled(false);
	connect(cut_act_, SIGNAL(triggered()), this, SLOT(cut()));

	icon = QIcon(":/images/copy.png");
	icon.addPixmap(QPixmap(":/images/copy_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/copy_disabled.png"), QIcon::Disabled, QIcon::Off);
	copy_act_ = new QAction(icon, QString::fromUtf8(language[lsCopy].c_str()), this);
	copy_act_->setShortcut(QKeySequence::Copy);
	copy_act_->setEnabled(false);
	connect(copy_act_, SIGNAL(triggered()), this, SLOT(copy()));

	icon = QIcon(":/images/paste.png");
	icon.addPixmap(QPixmap(":/images/paste_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/paste_disabled.png"), QIcon::Disabled, QIcon::Off);
	paste_act_ = new QAction(icon, QString::fromUtf8(language[lsPaste].c_str()), this);
	paste_act_->setShortcut(QKeySequence::Paste);
	paste_act_->setEnabled(false);
	connect(paste_act_, SIGNAL(triggered()), this, SLOT(paste()));

	cut_act2_ = new QAction(cut_act_->icon(), QString::fromUtf8(language[lsCut].c_str()), this);
	cut_act2_->setEnabled(false);
	cut_act2_->setVisible(false);
	connect(cut_act2_, SIGNAL(triggered()), this, SLOT(cut()));

	copy_act2_ = new QAction(copy_act_->icon(), QString::fromUtf8(language[lsCopy].c_str()), this);
	copy_act2_->setEnabled(false);
	copy_act2_->setVisible(false);
	connect(copy_act2_, SIGNAL(triggered()), this, SLOT(copy()));

	paste_act2_ = new QAction(paste_act_->icon(), QString::fromUtf8(language[lsPaste].c_str()), this);
	paste_act2_->setEnabled(false);
	paste_act2_->setVisible(false);
	connect(paste_act2_, SIGNAL(triggered()), this, SLOT(paste()));

#ifdef LITE
	icon = QIcon(":/images/check_off.png");
	icon.addPixmap(QPixmap(":/images/check_on.png"), QIcon::Normal, QIcon::On);
	icon.addPixmap(QPixmap(":/images/check_off_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/check_on_hover.png"), QIcon::Active, QIcon::On);
	show_act_ = new QAction(icon, QString::fromUtf8(language[lsShowProtectedFunctions].c_str()), this);
	show_act_->setCheckable(true);
	show_act_->setVisible(false);
	connect(show_act_, SIGNAL(triggered()), this, SLOT(showProtected()));
#else
	goto_act_ = new QAction(icon, QString::fromUtf8(language[lsGoTo].c_str()) + "...", this);
	goto_act_->setShortcut(QString("Ctrl+G"));
	goto_act_->setEnabled(false);
	connect(goto_act_, SIGNAL(triggered()), this, SLOT(goTo()));

	icon = QIcon(":/images/goto.png");
	icon.addPixmap(QPixmap(":/images/goto_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/goto_disabled.png"), QIcon::Disabled, QIcon::Off);
	goto_act2_ = new QAction(icon, QString::fromUtf8(language[lsGoTo].c_str()), this);
	goto_act2_->setVisible(false);
	connect(goto_act2_, SIGNAL(triggered()), this, SLOT(goTo()));

	add_function_act_ = new QAction(QString::fromUtf8(language[lsAddFunction].c_str()) + "...", this);
	add_function_act_->setEnabled(false);
	connect(add_function_act_, SIGNAL(triggered()), this, SLOT(addFunction()));

	add_folder_act_ = new QAction(QString::fromUtf8(language[lsAddFolder].c_str()) + "...", this);
	add_folder_act_->setEnabled(false);
	connect(add_folder_act_, SIGNAL(triggered()), this, SLOT(addFolder()));

	icon = QIcon(":/images/add.png");
	icon.addPixmap(QPixmap(":/images/add_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/add_disabled.png"), QIcon::Disabled, QIcon::Off);
	add_function_act2_ = new QAction(icon, QString::fromUtf8(language[lsAddFunction].c_str()), this);
	add_function_act2_->setShortcut(QString("Ins"));
	add_function_act2_->setVisible(false);
	connect(add_function_act2_, SIGNAL(triggered()), this, SLOT(addFunction()));

	watermarks_act_ = new QAction(QString::fromUtf8(language[lsWatermarks].c_str()) + "...", this);
	connect(watermarks_act_, SIGNAL(triggered()), this, SLOT(watermarks()));
#endif

#ifdef ULTIMATE
	save_licenses_act_ = new QAction(QString::fromUtf8(language[lsSaveLicensesAs].c_str()) + "...", this);
	save_licenses_act_->setEnabled(false);
	connect(save_licenses_act_, SIGNAL(triggered()), this, SLOT(saveLicenses()));

	add_license_act_ = new QAction(QString::fromUtf8(language[lsAddLicense].c_str()) + "...", this);
	add_license_act_->setEnabled(false);
	connect(add_license_act_, SIGNAL(triggered()), this, SLOT(addLicense()));
	add_license_act2_ = new QAction(icon, QString::fromUtf8(language[lsAddLicense].c_str()), this);
	add_license_act2_->setShortcut(QString("Ins"));
	add_license_act2_->setVisible(false);
	connect(add_license_act2_, SIGNAL(triggered()), this, SLOT(addLicense()));

	add_file_act_ = new QAction((language[lsAddFile] + "...").c_str(), this);
	add_file_act_->setEnabled(false);
	connect(add_file_act_, SIGNAL(triggered()), this, SLOT(addFile()));
	add_file_act2_ = new QAction(icon, QString::fromUtf8(language[lsAddFile].c_str()), this);
	add_file_act2_->setShortcut(QString("Ins"));
	add_file_act2_->setVisible(false);
	connect(add_file_act2_, SIGNAL(triggered()), this, SLOT(addFile()));

	import_license_act_ = new QAction(QString::fromUtf8(language[lsImportLicense].c_str()) + "...", this);
	import_license_act_->setShortcut(QString("Ctrl+I"));
	import_license_act_->setEnabled(false);
	connect(import_license_act_, SIGNAL(triggered()), this, SLOT(importLicense()));

	import_project_act_ = new QAction(QString::fromUtf8(language[lsImportLicensesFromProjectFile].c_str()) + "...", this);
	import_project_act_->setShortcut(QString("Ctrl+Shift+I"));
	import_project_act_->setEnabled(false);
	connect(import_project_act_, SIGNAL(triggered()), this, SLOT(importProject()));

	export_key_act_ = new QAction((language[lsExportKeyPair] + "...").c_str(), this);
	export_key_act_->setEnabled(false);
	connect(export_key_act_, SIGNAL(triggered()), this, SLOT(exportKeyPair()));
#endif

	delete_act_ = new QAction(QString::fromUtf8(language[lsDelete].c_str()), this);
	delete_act_->setShortcut(QString("Del"));
	delete_act_->setEnabled(false);
	connect(delete_act_, SIGNAL(triggered()), this, SLOT(del()));

	rename_act_ = new QAction(QString::fromUtf8(language[lsRename].c_str()), this);
	rename_act_->setShortcut(QString("F2"));
	rename_act_->setEnabled(false);
	connect(rename_act_, SIGNAL(triggered()), this, SLOT(rename()));

	exclude_act_ = new QAction(QString::fromUtf8(language[lsExcludedFromCompilation].c_str()), this);
	exclude_act_->setEnabled(false);
	exclude_act_->setCheckable(true);
	connect(exclude_act_, SIGNAL(triggered()), this, SLOT(excludeFromCompilation()));

	block_act_ = new QAction(QString::fromUtf8(language[lsBlocked].c_str()), this);
	block_act_->setEnabled(false);
	block_act_->setCheckable(true);
	connect(block_act_, SIGNAL(triggered()), this, SLOT(block()));

	icon = QIcon(":/images/compile.png");
	icon.addPixmap(QPixmap(":/images/compile_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/compile_disabled.png"), QIcon::Disabled, QIcon::Off);
	compile_act_ = new QAction(icon, QString::fromUtf8(language[lsCompile].c_str()), this);
	compile_act_->setShortcut(QString("F9"));
	compile_act_->setEnabled(false);
	connect(compile_act_, SIGNAL(triggered()), this, SLOT(compile()));

	icon = QIcon(":/images/execute.png");
	icon.addPixmap(QPixmap(":/images/execute_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/execute_disabled.png"), QIcon::Disabled, QIcon::Off);
	execute_act_ = new QAction(icon, QString::fromUtf8(language[lsExecute].c_str()), this);
	execute_act_->setShortcut(QString("F5"));
	execute_act_->setEnabled(false);
	connect(execute_act_, SIGNAL(triggered()), this, SLOT(execute()));

	execute_original_act_ = new ElidedAction(this);
	execute_original_act_->setEnabled(false);
	connect(execute_original_act_, SIGNAL(triggered()), this, SLOT(executeOriginal()));

	execute_protected_act_ = new ElidedAction("", this);
	execute_protected_act_->setVisible(false);
	connect(execute_protected_act_, SIGNAL(triggered()), this, SLOT(executeProtected()));

	execute_parameters_act_ = new QWidgetAction(this);

	search_act_ = new QAction(icon, QString::fromUtf8(language[lsSearch].c_str()), this);
	search_act_->setShortcut(QString("Ctrl+F"));
	search_act_->setEnabled(false);
	connect(search_act_, SIGNAL(triggered()), this, SLOT(search()));

    QList<QAction *> action_list = this->findChildren<QAction *>();
	for (QList<QAction *>::ConstIterator it = action_list.constBegin(); it != action_list.constEnd(); it++) {
		(*it)->setIconVisibleInMenu(false);
	}

	settings_act_ = new QAction(QString::fromUtf8(language[lsSettings].c_str()), this);
	connect(settings_act_, SIGNAL(triggered()), this, SLOT(settings()));

	// create menu
	file_menu_ = menuBar()->addMenu(QString::fromUtf8(language[lsFile].c_str()));
	file_menu_->addAction(open_act_);
	file_menu_->addAction(save_act_);
	file_menu_->addAction(save_as_act_);
#ifdef ULTIMATE
	file_menu_->addAction(save_licenses_act_);
#endif
	file_menu_->addAction(close_act_);
	history_separator_ = file_menu_->addSeparator();
	history_separator_->setVisible(false);
	file_menu_->addSeparator();
	file_menu_->addAction(exit_act_);

	edit_menu_ = menuBar()->addMenu(QString::fromUtf8(language[lsEdit].c_str()));
	edit_menu_->addAction(undo_act_);
	edit_menu_->addAction(redo_act_);
	edit_menu_->addSeparator();
	edit_menu_->addAction(cut_act_);
	edit_menu_->addAction(copy_act_);
	edit_menu_->addAction(paste_act_);
	edit_menu_->addAction(delete_act_);

	edit_menu_->addSeparator();
	edit_menu_->addAction(search_act_);
#ifndef LITE
	edit_menu_->addAction(goto_act_);
#endif

	project_menu_ = menuBar()->addMenu(QString::fromUtf8(language[lsProject].c_str()));
#ifndef LITE
	add_menu_ = project_menu_->addMenu(QString::fromUtf8(language[lsAdd].c_str()));
	add_menu_->setEnabled(false);
	add_menu_->addAction(add_function_act_);
#endif
#ifdef ULTIMATE
	add_menu_->addAction(add_license_act_);
	add_menu_->addAction(add_file_act_);
#endif
#ifndef LITE
	add_menu_->addAction(add_folder_act_);
#endif
#ifdef ULTIMATE
	project_menu_->addSeparator();
	project_menu_->addAction(export_key_act_);
	import_menu_ = project_menu_->addMenu(QString::fromUtf8(language[lsImport].c_str()));
	import_menu_->setEnabled(false);
	import_menu_->addAction(import_license_act_);
	import_menu_->addAction(import_project_act_);
#endif
#ifndef LITE
	project_menu_->addSeparator();
#endif
	project_menu_->addAction(compile_act_);
	project_menu_->addAction(execute_act_);

	tools_menu_ = menuBar()->addMenu(QString::fromUtf8(language[lsTools].c_str()));
#ifndef LITE
	tools_menu_->addAction(watermarks_act_);
#endif
	tools_menu_->addAction(settings_act_);

	help_menu_ = menuBar()->addMenu(QString::fromUtf8(language[lsHelp].c_str()));
	help_menu_->addAction(help_act_);
	help_menu_->addSeparator();
	help_menu_->addAction(home_page_act_);
	help_menu_->addAction(about_act_);

	function_ext_address_act_ = new QAction(QString::fromUtf8(language[lsExternalAddress].c_str()), this);
	connect(function_ext_address_act_, SIGNAL(triggered()), this, SLOT(functionExtAddress()));
	function_end_address_act_ = new QAction(QString::fromUtf8(language[lsBreakAddress].c_str()), this);
	connect(function_end_address_act_, SIGNAL(triggered()), this, SLOT(functionEndAddress()));
	function_del_act_ = new QAction(QString::fromUtf8(language[lsDelete].c_str()), this);
	connect(function_del_act_, SIGNAL(triggered()), this, SLOT(functionDel()));

	script_menu_ = new QMenu(this);
	script_menu_->addAction(undo_act_);
	script_menu_->addAction(redo_act_);
	script_menu_->addSeparator();
	script_menu_->addAction(cut_act_);
	script_menu_->addAction(copy_act_);
	script_menu_->addAction(paste_act_);
	script_menu_->addAction(delete_act_);
	script_menu_->addSeparator();
	script_menu_->addAction(search_act_);

	// create widgets
	project_filter_ = new SearchLineEdit(this);
	project_filter_->setFrame(false);
	project_filter_->setFixedWidth(200);
    connect(project_filter_, SIGNAL(textChanged(const QString &)), this, SLOT(projectFilterChanged()));
	connect(project_filter_, SIGNAL(returnPressed()), this, SLOT(projectFilterChanged()));

	QWidget *widget = new QWidget(this);
	QLabel *label = new QLabel(QString::fromUtf8(language[lsParameters].c_str()) + ":", this);
	parameters_edit_ = new LineEdit(this);
	QBoxLayout *layout = new QHBoxLayout();
	layout->setContentsMargins(16, 0, 0, 0);
	layout->addWidget(label);
	layout->addWidget(parameters_edit_);
	widget->setLayout(layout);
	execute_parameters_act_->setDefaultWidget(widget);

	execute_menu_ = new QMenu(this);
	execute_menu_->addAction(execute_original_act_);
	execute_menu_->addAction(execute_protected_act_);
	execute_menu_->addSeparator();
	execute_menu_->addAction(execute_parameters_act_);

	setContextMenuPolicy(Qt::NoContextMenu);

	QToolBar *tool_bar = addToolBar("");
	tool_bar->setMovable(false);
	tool_bar->setIconSize(QSize(20, 20));
	tool_bar->addSeparator();
	tool_bar->addAction(open_act_);
	tool_bar->addAction(save_act_);
	tool_bar->addSeparator();
	spacer_ = new ToolButtonElided(this);
	spacer_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	spacer_->setCursor(Qt::PointingHandCursor);
	spacer_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	spacer_->setFixedWidth(0);
	connect(spacer_, SIGNAL(clicked()), this, SLOT(showFile()));
	tool_bar->addWidget(spacer_);
	tool_bar->addAction(compile_act_);
	tool_bar->addAction(execute_act_);
	QToolButton *button = reinterpret_cast<QToolButton *>(tool_bar->widgetForAction(execute_act_));
	button->setMenu(execute_menu_);
	button->setPopupMode(QToolButton::MenuButtonPopup);
	project_separator_ = tool_bar->addSeparator();
	//project_separator_->setVisible(false);
	project_separator_widget_ = tool_bar->widgetForAction(project_separator_);
	tool_bar->addAction(cut_act2_);
	tool_bar->addAction(copy_act2_);
	tool_bar->addAction(paste_act2_);
#ifdef LITE
	tool_bar->addAction(show_act_);
	reinterpret_cast<QToolButton*>(tool_bar->widgetForAction(show_act_))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#else
	tool_bar->addAction(add_function_act2_);
	reinterpret_cast<QToolButton*>(tool_bar->widgetForAction(add_function_act2_))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	tool_bar->addAction(goto_act2_);
	reinterpret_cast<QToolButton*>(tool_bar->widgetForAction(goto_act2_))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#endif
#ifdef ULTIMATE
	tool_bar->addAction(add_license_act2_);
	reinterpret_cast<QToolButton*>(tool_bar->widgetForAction(add_license_act2_))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	tool_bar->addAction(add_file_act2_);
	reinterpret_cast<QToolButton*>(tool_bar->widgetForAction(add_file_act2_))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#endif

	templates_save_act_ = new QAction(QString::fromUtf8(language[lsSaveTemplateAs].c_str()) + "...", this);
	connect(templates_save_act_, SIGNAL(triggered()), this, SLOT(templatesSave()));

	templates_edit_act_ = new QAction(QString::fromUtf8(language[lsEdit].c_str()) + "...", this);
	connect(templates_edit_act_, SIGNAL(triggered()), this, SLOT(templatesEdit()));

	templates_menu_ = new QMenu(this);
	templates_menu_->addSeparator();
	templates_menu_->addAction(templates_save_act_);
	templates_menu_->addAction(templates_edit_act_);
	connect(templates_menu_, SIGNAL(aboutToShow()), this, SLOT(templatesShow()));
	updateTemplates();

	templates_act_ = new QAction(copy_act_->icon(), QString::fromUtf8(language[lsTemplates].c_str()), this);
	templates_act_->setVisible(false);

	tool_bar->addAction(templates_act_);
	QToolButton *template_button = reinterpret_cast<QToolButton*>(tool_bar->widgetForAction(templates_act_));
	template_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	template_button->setPopupMode(QToolButton::InstantPopup);
	template_button->setMenu(templates_menu_);

	QWidget *spacer = new QWidget(this);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	tool_bar->addWidget(spacer);
	project_filter_act_ = tool_bar->addWidget(project_filter_);
	project_filter_act_->setVisible(false);

	project_frame_ = new QFrame(this);
	status_bar_ = new QStatusBar(this);

	project_tree_ = new TreeView(this);
	project_tree_->setObjectName("project");
	project_tree_->setFrameShape(QFrame::NoFrame);
	project_tree_->header()->setObjectName("project");
	project_tree_->setIconSize(QSize(18, 18));
	project_tree_->setModel(project_model_);
	project_tree_->setContextMenuPolicy(Qt::CustomContextMenu);
	project_tree_->setDragDropMode(QAbstractItemView::DragDrop); 
	project_tree_->setItemDelegate(new ProjectTreeDelegate(this));
	connect(project_tree_->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(projectItemChanged()));
	connect(project_tree_, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(projectContextMenu(const QPoint &)));

#ifndef LITE
	functions_tree_ = new TreeView(this);
	functions_tree_->setObjectName("project");
	functions_tree_->setFrameShape(QFrame::NoFrame);
	functions_tree_->header()->setObjectName("project");
	functions_tree_->setIconSize(QSize(18, 18));
	functions_tree_->setModel(functions_model_);
	functions_tree_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(functions_tree_->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(projectItemChanged()));
	connect(functions_tree_, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(projectContextMenu(const QPoint &)));

	info_tree_ = new TreeView(this);
	info_tree_->setObjectName("project");
	info_tree_->setFrameShape(QFrame::NoFrame);
	info_tree_->header()->setObjectName("project");
	info_tree_->setIconSize(QSize(18, 18));
	info_tree_->setModel(info_model_);
	connect(info_tree_->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(projectItemChanged()));
#endif

	project_tab_ = new TabWidget(this);
	project_tab_->tabBar()->hide();
	project_tab_->resize(240 * Application::stylesheetScaleFactor(), 10);

	QFrame *tab_bar = new QFrame(this);
	tab_bar->setObjectName("desktop");

	icon_project_ = new QRadioButton(this);
	icon_project_->setObjectName("project");
	icon_project_->setChecked(true);
	connect(icon_project_, SIGNAL(toggled(bool)), this, SLOT(projectTabClicked()));

#ifndef LITE
	icon_functions_ = new QRadioButton(this);
	icon_functions_->setObjectName("functions");
	connect(icon_functions_, SIGNAL(toggled(bool)), this, SLOT(projectTabClicked()));

	icon_details_ = new QRadioButton(this);
	icon_details_->setObjectName("details");
	connect(icon_details_, SIGNAL(toggled(bool)), this, SLOT(projectTabClicked()));
#endif

	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(icon_project_);
#ifndef LITE
	layout->addWidget(icon_functions_);
	layout->addWidget(icon_details_);
#endif
	layout->addStretch(1);
	tab_bar->setLayout(layout);

	project_tab_->addTab(project_tree_, "");
#ifndef LITE
	project_tab_->addTab(functions_tree_, "");
	project_tab_->addTab(info_tree_, "");
#endif
	connect(project_tab_, SIGNAL(currentChanged(int)), this, SLOT(projectFilterChanged()));
	connect(project_tab_, SIGNAL(resized()), this, SLOT(projectTabMoved()));

	search_tree_ = new TreeView(this);
	search_tree_->setObjectName("grid");
	search_tree_->setIconSize(QSize(18, 18));
	search_tree_->setRootIsDecorated(false);
	search_tree_->setFrameShape(QFrame::NoFrame);
	search_tree_->setModel(search_model_);
	search_tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	search_tree_->header()->setSectionsMovable(false);
	connect(search_tree_, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(treeItemDoubleClicked(QModelIndex)));

	directory_tree_ = new TreeView(this);
	directory_tree_->setObjectName("grid");
	directory_tree_->setIconSize(QSize(18, 18));
	directory_tree_->setFrameShape(QFrame::NoFrame);
	directory_tree_->setRootIsDecorated(false);
	directory_tree_->setModel(directory_model_);
	directory_tree_->setItemDelegate(new ProjectTreeDelegate(this));
	directory_tree_->header()->setSectionsMovable(false);
	connect(directory_tree_, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(treeItemDoubleClicked(QModelIndex)));
	connect(directory_tree_->header(), SIGNAL(sectionClicked(int)), this, SLOT(treeSectionClicked(int)));
	directory_tree_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(directory_tree_, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(projectContextMenu(const QPoint &)));

#ifndef LITE
	dump_view_ = new TableView(this);
	dump_view_->setObjectName("grid");
	dump_view_->setIconSize(QSize(18, 18));
	dump_view_->setFrameShape(QFrame::NoFrame);
	dump_view_->setModel(dump_model_);
	dump_view_->setShowGrid(false);
	dump_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	dump_view_->horizontalHeader()->resizeSection(0, 150 * Application::stylesheetScaleFactor());
	dump_view_->horizontalHeader()->resizeSection(1, 300 * Application::stylesheetScaleFactor());
	dump_view_->horizontalHeader()->setStretchLastSection(true);
	dump_view_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	dump_view_->horizontalHeader()->setSectionsClickable(false);
	dump_view_->verticalHeader()->setDefaultSectionSize(dump_view_->verticalHeader()->minimumSectionSize());
	dump_view_->setSelectionBehavior(QAbstractItemView::SelectRows);

	disasm_view_ = new DisasmView(this);
	disasm_view_->setObjectName("grid");
	disasm_view_->setIconSize(QSize(18, 18));
	disasm_view_->setFrameShape(QFrame::NoFrame);
	disasm_view_->setModel(disasm_model_);
	disasm_view_->setShowGrid(false);
	disasm_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	disasm_view_->horizontalHeader()->resizeSection(0, 150 * Application::stylesheetScaleFactor());
	disasm_view_->horizontalHeader()->resizeSection(1, 300 * Application::stylesheetScaleFactor());
	disasm_view_->horizontalHeader()->setStretchLastSection(true);
	disasm_view_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	disasm_view_->horizontalHeader()->setSectionsClickable(false);
	disasm_view_->verticalHeader()->setDefaultSectionSize(dump_view_->verticalHeader()->minimumSectionSize());
	disasm_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
	connect(disasm_view_, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(disasmItemDoubleClicked(QModelIndex)));

#endif

	script_editor_ = new ScriptEdit(this);
	script_editor_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(script_editor_, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(scriptContextMenu(const QPoint &)));

	project_file_name_ = new QLabel(this);

	script_line_ = new QLabel(this);
	script_line_->setFixedWidth(100);

	script_column_ = new QLabel(this);
	script_column_->setFixedWidth(100);

	script_mode_ = new Label(this);
	script_mode_->setFixedWidth(30);
	connect(script_mode_, SIGNAL(doubleClicked()), this, SLOT(scriptModeClicked()));

	status_bar_->addPermanentWidget(project_file_name_, 1);
	status_bar_->addPermanentWidget(script_line_);
	status_bar_->addPermanentWidget(script_column_);
	status_bar_->addPermanentWidget(script_mode_);

	core_property_editor_ = new TreePropertyEditor(this);
	core_property_editor_->setModel(core_property_manager_);
	core_property_editor_->expandToDepth(0);
	core_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	core_property_editor_->header()->setSectionsMovable(false);

	function_property_editor_ = new TreePropertyEditor(this);
	function_property_editor_->setModel(function_property_manager_);
	function_property_editor_->expandToDepth(0);
	function_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	function_property_editor_->setContextMenuPolicy(Qt::CustomContextMenu);
	function_property_editor_->setSelectionMode(QTreeView::ExtendedSelection);
	function_property_editor_->header()->setSectionsMovable(false);
	connect(function_property_editor_, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(functionContextMenu(const QPoint &)));
	connect(function_property_editor_, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(functionItemDoubleClicked(QModelIndex)));

#ifndef LITE
	section_property_editor_ = new TreePropertyEditor(this);
	section_property_editor_->setModel(section_property_manager_);
	section_property_editor_->expandToDepth(0);
	section_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	section_property_editor_->header()->setSectionsMovable(false);

	segment_property_editor_ = new TreePropertyEditor(this);
	segment_property_editor_->setModel(segment_property_manager_);
	segment_property_editor_->expandToDepth(0);
	segment_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	segment_property_editor_->header()->setSectionsMovable(false);

	import_property_editor_ = new TreePropertyEditor(this);
	import_property_editor_->setModel(import_property_manager_);
	import_property_editor_->expandToDepth(0);
	import_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	import_property_editor_->header()->setSectionsMovable(false);

	export_property_editor_ = new TreePropertyEditor(this);
	export_property_editor_->setModel(export_property_manager_);
	export_property_editor_->expandToDepth(0);
	export_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	export_property_editor_->header()->setSectionsMovable(false);

	resource_property_editor_ = new TreePropertyEditor(this);
	resource_property_editor_->setModel(resource_property_manager_);
	resource_property_editor_->expandToDepth(0);
	resource_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	resource_property_editor_->header()->setSectionsMovable(false);

	loadcommand_property_editor_ = new TreePropertyEditor(this);
	loadcommand_property_editor_->setModel(loadcommand_property_manager_);
	loadcommand_property_editor_->expandToDepth(0);
	loadcommand_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	loadcommand_property_editor_->header()->setSectionsMovable(false);

	address_calculator_ = new TreePropertyEditor(this);
	address_calculator_->setModel(address_calculator_manager_);
	address_calculator_->expandToDepth(0);
	address_calculator_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	address_calculator_->header()->setSectionsMovable(false);
#endif

	QFont font;
	font.setBold(true);

#ifdef ULTIMATE
	license_property_editor_ = new TreePropertyEditor(this);
	license_property_editor_->setModel(license_property_manager_);
	license_property_editor_->expandToDepth(0);
	license_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	license_property_editor_->header()->setSectionsMovable(false);

	internal_file_property_editor_ = new TreePropertyEditor(this);
	internal_file_property_editor_->setModel(internal_file_property_manager_);
	internal_file_property_editor_->expandToDepth(0);
	internal_file_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	internal_file_property_editor_->header()->setSectionsMovable(false);

	assembly_property_editor_ = new TreePropertyEditor(this);
	assembly_property_editor_->setModel(assembly_property_manager_);
	assembly_property_editor_->expandToDepth(0);
	assembly_property_editor_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	assembly_property_editor_->header()->setSectionsMovable(false);

	licensing_parameters_page_ = new QFrame(this);
	licensing_parameters_page_->setObjectName("editor");

	label = new QLabel(this);
	label->setAlignment(Qt::AlignCenter);
	label->setPixmap(QPixmap(":/images/key_gray.png"));

	licensing_parameters_help_ = new QLabel(this);
	licensing_parameters_help_->setObjectName("note");
	licensing_parameters_help_->setAlignment(Qt::AlignCenter);
	licensing_parameters_help_->setWordWrap(true);
	licensing_parameters_help_->setText(QString::fromUtf8(language[lsKeyPairHelp].c_str()));

	QFrame *key_frame = new QFrame(this);
	key_frame->setObjectName("gridEditor");
	key_frame->setFrameShape(QFrame::NoFrame);

	key_algo_label_ = new QLabel(QString::fromUtf8(language[lsKeyPairAlgorithm].c_str()), this);
	key_algo_label_->setObjectName("editor");

	QComboBox *comboKeyAlgo = new QComboBox(this);
	comboKeyAlgo->setFrame(false);
	comboKeyAlgo->setFont(font);
	comboKeyAlgo->addItem("RSA");
	comboKeyAlgo->setCurrentIndex(0);

	key_len_label_ = new QLabel(QString::fromUtf8(language[lsKeyLength].c_str()), this);
	key_len_label_->setObjectName("editor");

	key_len_ = new QComboBox(this);
	key_len_->setFrame(false);
	key_len_->setFont(font);
	key_len_->addItems(QStringList() << "1024" << "1536" << "2048" << "2560" << "3072" << "3584" << "4096");
	key_len_->setCurrentIndex(2);

	create_key_button_ = new PushButton(QString::fromUtf8(language[lsGenerate].c_str()), this);
	//create_key_button_->setDefault(true);
	connect(create_key_button_, SIGNAL(clicked()), this, SLOT(createKeyPair()));

	use_other_project_button_ = new PushButton(QString::fromUtf8(language[lsUseOtherProject].c_str()), this);
	connect(use_other_project_button_, SIGNAL(clicked()), this, SLOT(useOtherProject()));

    QGridLayout *keyLayout = new QGridLayout();
	keyLayout->setContentsMargins(0, 1, 0, 1);
	keyLayout->setHorizontalSpacing(0);
	keyLayout->setVerticalSpacing(1);
    keyLayout->addWidget(key_algo_label_, 0, 0);
	keyLayout->addWidget(comboKeyAlgo, 0, 1);
    keyLayout->addWidget(key_len_label_, 1, 0);
	keyLayout->addWidget(key_len_, 1, 1);
	key_frame->setLayout(keyLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	buttonLayout->setSpacing(10);
    buttonLayout->addWidget(create_key_button_);
    buttonLayout->addWidget(use_other_project_button_);
	buttonLayout->setAlignment(Qt::AlignCenter);

	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 50, 0, 0);
	layout->setSpacing(30);
	layout->addWidget(label);
	layout->addWidget(licensing_parameters_help_);
	layout->addWidget(key_frame);
	layout->addLayout(buttonLayout);
	layout->addStretch(1);
	licensing_parameters_page_->setLayout(layout);
#endif

	QSplitter *splitter;
#ifndef LITE
	splitter = new QSplitter(this);
	splitter->setOrientation(Qt::Vertical);
	dump_page_ = new QFrame(this);
	dump_page_->setObjectName("gridEditor");
	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(disasm_view_);
	layout->addWidget(splitter);
	layout->addWidget(dump_view_);
	dump_page_->setLayout(layout);

	splitter->addWidget(disasm_view_);
	splitter->addWidget(dump_view_);
    splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 0);
#endif

	project_page_ = new QStackedWidget(this);
	project_page_->addWidget(search_tree_);
	project_page_->addWidget(directory_tree_);
	project_page_->addWidget(function_property_editor_);
	project_page_->addWidget(script_editor_);
	project_page_->addWidget(core_property_editor_);
#ifndef LITE
	project_page_->addWidget(section_property_editor_);
	project_page_->addWidget(segment_property_editor_);
	project_page_->addWidget(import_property_editor_);
	project_page_->addWidget(export_property_editor_);
	project_page_->addWidget(resource_property_editor_);
	project_page_->addWidget(loadcommand_property_editor_);
	project_page_->addWidget(address_calculator_);
	project_page_->addWidget(dump_page_);
#endif
#ifdef ULTIMATE
	project_page_->addWidget(licensing_parameters_page_);
	project_page_->addWidget(license_property_editor_);
	project_page_->addWidget(internal_file_property_editor_);
	project_page_->addWidget(assembly_property_editor_);
#endif

	context_find_ = new FindWidget(this);
	context_find_->hide();
	connect(context_find_, SIGNAL(findNext()), this, SLOT(contextFindNext()));
	connect(context_find_, SIGNAL(findPrevious()), this, SLOT(contextFindPrevious()));
	connect(context_find_, SIGNAL(find(QString, bool, bool)), this,
		SLOT(contextFind(QString, bool, bool)));
	connect(context_find_, SIGNAL(escapePressed()), this, SLOT(contextFindClosed()));

	QWidget *project_find = new QWidget(this);
	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(project_page_);
	layout->addWidget(context_find_);
	project_find->setLayout(layout);

	log_tree_ = new LogTreeView(this);
	log_tree_->setFrameShape(QFrame::NoFrame);
	log_tree_->setRootIsDecorated(false);
	log_tree_->setModel(log_model_);
	connect(log_tree_, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(logItemDoubleClicked(QModelIndex)));

	splitter = new QSplitter(this);
	splitter->setOrientation(Qt::Vertical);

	QFrame *project_right = new QFrame(this);
	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(project_find);
	layout->addWidget(splitter);
	layout->addWidget(log_tree_);
	project_right->setLayout(layout);

	splitter->addWidget(project_find);
	splitter->addWidget(log_tree_);
    splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 0);

	splitter = new QSplitter(this);

	QGridLayout *gridLayout = new QGridLayout();
	gridLayout->setContentsMargins(0, 0, 0, 0);
	gridLayout->setSpacing(0);
	gridLayout->addWidget(tab_bar, 0, 0);
	gridLayout->addWidget(project_tab_, 0, 1);
	gridLayout->addWidget(splitter, 0, 2);
	gridLayout->addWidget(project_right, 0, 3);
	gridLayout->addWidget(status_bar_, 1, 0, 1, 4);
	project_frame_->setLayout(gridLayout);

	splitter->addWidget(project_tab_);
	splitter->addWidget(project_right);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

	boot_frame_ = new QFrame(this);
	boot_frame_->setObjectName("desktop");

	boot_panel_ = new QFrame(this);
	boot_panel_->setObjectName("boot");
	boot_panel_->setMinimumSize(500, 300);

	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(boot_panel_);
	layout->setAlignment(boot_panel_, Qt::AlignCenter);
	boot_frame_->setLayout(layout);

	QFrame *top = new QFrame(this);
	top->setObjectName("bootTop");

	QLabel *logo = new QLabel(this);
	logo->setPixmap(QPixmap(":/images/logo.png"));

	welcome_label_ = new QLabel(QString::fromUtf8(language[lsWelcome].c_str()), this);
	welcome_label_->setObjectName("version");

	QLabel *version = new QLabel(QString::fromLatin1(Core::edition()) + " v " + QString::fromLatin1(Core::version()), this);
	version->setObjectName("version");

	QGridLayout *grid_layout = new QGridLayout();
	grid_layout->setContentsMargins(30, 30, 30, 20);
	grid_layout->setSpacing(0);
	grid_layout->setColumnStretch(1, 1);
	grid_layout->addWidget(logo, 0, 0, 2, 1);
	grid_layout->addWidget(welcome_label_, 0, 1, Qt::AlignCenter);
	grid_layout->addWidget(version, 1, 1, Qt::AlignCenter);
	top->setLayout(grid_layout);

	QFrame *line1 = new QFrame(this);
	line1->setObjectName("bootHSeparator");
	line1->setFixedHeight(1);

	recent_files_label_ = new QLabel(QString::fromUtf8(language[lsRecentFiles].c_str()), this);
	recent_files_label_->setObjectName("boot");

	quick_start_label_ = new QLabel(QString::fromUtf8(language[lsQuickStart].c_str()), this);
	quick_start_label_->setObjectName("boot");

	QFrame *left = new QFrame(this);
	left->setFixedWidth(boot_panel_->minimumSize().width() / 2);
	recent_files_layout_ = new QVBoxLayout();
	recent_files_layout_->setContentsMargins(10, 10, 10, 10);
	recent_files_layout_->setSpacing(0);
	recent_files_layout_->addWidget(recent_files_label_);
	recent_files_layout_->addSpacing(10);
	recent_files_layout_->addStretch(1);
	left->setLayout(recent_files_layout_);

	recent_file_open_act_ = new QAction(QString::fromUtf8(language[lsOpen].c_str()), this);
	connect(recent_file_open_act_, SIGNAL(triggered()), this, SLOT(openRecentFile()));

	recent_file_remove_act_ = new QAction(QString::fromUtf8(language[lsDelete].c_str()), this);
	connect(recent_file_remove_act_, SIGNAL(triggered()), this, SLOT(removeRecentFile()));

	recent_file_menu_ = new QMenu(this);
	recent_file_menu_->addAction(recent_file_open_act_);
	recent_file_menu_->addAction(recent_file_remove_act_);

	open_button_ = new QToolButton(this);
	open_button_->setObjectName("boot");
	open_button_->setIconSize(QSize(24, 24));
	open_button_->setIcon(QIcon(":/images/boot_open.png"));
	open_button_->setText(QString::fromUtf8(language[lsOpen].c_str()) + "...");
	open_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	open_button_->setAutoRaise(true);
	open_button_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	connect(open_button_, SIGNAL(clicked(bool)), this, SLOT(open()));

	examples_button_ = new QToolButton(this);
	examples_button_->setObjectName("boot");
	examples_button_->setIconSize(QSize(24, 24));
	examples_button_->setIcon(QIcon(":/images/boot_examples.png"));
	examples_button_->setText(QString::fromUtf8(language[lsExamples].c_str()) + "...");
	examples_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	examples_button_->setAutoRaise(true);
	examples_button_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	connect(examples_button_, SIGNAL(clicked(bool)), this, SLOT(examples()));

	help_button_ = new QToolButton(this);
	help_button_->setObjectName("boot");
	help_button_->setIconSize(QSize(24, 24));
	help_button_->setIcon(QIcon(":/images/boot_help.png"));
	help_button_->setText(QString::fromUtf8(language[lsHelp].c_str()));
	help_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	help_button_->setAutoRaise(true);
	help_button_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	connect(help_button_, SIGNAL(clicked(bool)), this, SLOT(help()));

	QFrame *right = new QFrame(this);
	layout = new QVBoxLayout();
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(0);
	layout->addWidget(quick_start_label_);
	layout->addSpacing(10);
	layout->addWidget(open_button_);
	layout->addWidget(examples_button_);
	layout->addWidget(help_button_);
	layout->addStretch(1);
	right->setLayout(layout);

	QFrame *line3 = new QFrame(this);
	line3->setObjectName("bootVSeparator");
	line3->setFixedWidth(1);

	QFrame *center = new QFrame(this);
	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(left);
	layout->addWidget(line3);
	layout->addWidget(right);
	center->setLayout(layout);

	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(top);
	layout->addWidget(line1);
	layout->addWidget(center, 1);
	boot_panel_->setLayout(layout);

	desktop_page_ = new QStackedWidget(this);
	desktop_page_->addWidget(boot_frame_);
	desktop_page_->addWidget(project_frame_);
	desktop_page_->setCurrentWidget(boot_frame_);
	setCentralWidget(desktop_page_);

	connect(QAbstractEventDispatcher::instance(), SIGNAL(awake()), SLOT(updateEditActions()));
	connect(&fs_watcher_, SIGNAL(fileChanged(const QString &)), this, SLOT(fileChanged(const QString &)));

	setAcceptDrops(true);

	std::vector<std::string> project_list = settings_file().project_list();
	if (project_list.size() > 10)
		project_list.resize(10);
	for (int i = 0; i < (int)project_list.size(); i++) {
		addRecentFile(i, QString::fromUtf8(project_list[i].c_str()));
	}

	localize();

	QStringList args = QCoreApplication::arguments();
	if (args.length() > 1) {
		QString fileName(QFileInfo(args[1]).canonicalFilePath());
		if (fileName.isEmpty()) // no file found -> will use original argument in error message
			fileName = args[1];
		loadFile(QDir::toNativeSeparators(fileName));
	}
}

MainWindow::~MainWindow()
{
	delete core_;
	delete log_;
}

void MainWindow::addRecentFile(int index, const QString file_name)
{
	history_separator_->setVisible(true);

	QAction *action = new QAction(file_name, this);
	connect(action, SIGNAL(triggered()), this, SLOT(loadFileFromHistory()));
	int file_pos = file_menu_->actions().indexOf(history_separator_) + 1;
	file_menu_->insertAction(file_menu_->actions().at(file_pos + index), action);

	QFileInfo fileInfo(file_name);

	QToolButton *button = new QToolButton(this);
	button->setObjectName("boot");
	button->setIconSize(QSize(16, 16));
	button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	button->setAutoRaise(true);
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	button->setText(fileInfo.fileName());
	button->setToolTip(file_name);
	button->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(button, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(recentFileContextMenu(const QPoint &)));

	QFileIconProvider ip;
	QIcon icon = ip.icon(fileInfo);
	if (icon.isNull())
		icon = ip.icon(QFileIconProvider::File);
	button->setIcon(icon);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(loadFileFromBoot()));

	recent_files_layout_->insertWidget(2 + index, button);
}

void MainWindow::recentFileContextMenu(const QPoint &p)
{
	recent_file_ = -1;
	QToolButton *widget = qobject_cast<QToolButton*>(sender());
	if (widget) {
		for (int i = 2; i < recent_files_layout_->count(); ++i) {
			if (recent_files_layout_->itemAt(i)->widget() == widget) {
				recent_file_ = i - 2;
				break;
			}
		}
		if (recent_file_ >= 0) {
			widget->setDown(true);
			recent_file_menu_->exec(qobject_cast<QWidget*>(sender())->mapToGlobal(p));
			if (recent_file_ >= 0)
				widget->setDown(false);
		}
	}
}

void MainWindow::openRecentFile()
{
	if (recent_file_ < 0)
		return;

	QToolButton *button = qobject_cast<QToolButton *>(recent_files_layout_->itemAt(2 + recent_file_)->widget());
	if (button)
		loadFile(button->toolTip());
}

void MainWindow::removeRecentFile()
{
	if (recent_file_ < 0)
		return;

	int file_pos = file_menu_->actions().indexOf(history_separator_) + 1;
	QAction *action = file_menu_->actions().at(file_pos + recent_file_);
	file_menu_->removeAction(action);
	delete action;

	QWidget *widget = recent_files_layout_->itemAt(2 + recent_file_)->widget();
	recent_files_layout_->removeWidget(widget);
	delete widget;

	recent_file_ = -1;

	saveRecentFiles();
}

void MainWindow::saveRecentFiles()
{
	std::vector<std::string> project_list;
	int file_pos = file_menu_->actions().indexOf(history_separator_) + 1;
	QList<QAction*> actions = file_menu_->actions();
	for (int i = file_pos; i < actions.size() - 2; i++) {
		QAction *action = actions[i];
		project_list.push_back(action->text().toUtf8().constData());
	}
	settings_file().set_project_list(project_list);
}

void MainWindow::localize()
{
	file_menu_->setTitle(QString::fromUtf8(language[lsFile].c_str()));
	open_act_->setText(QString::fromUtf8(language[lsOpen].c_str()) + "...");
	save_act_->setText(QString::fromUtf8(language[lsSaveProject].c_str()));
	save_as_act_->setText(QString::fromUtf8(language[lsSaveProjectAs].c_str()) + "...");
	close_act_->setText(QString::fromUtf8(language[lsClose].c_str()));
	exit_act_->setText(QString::fromUtf8(language[lsExit].c_str()));
	project_filter_->setPlaceholderText(QString::fromUtf8(language[lsSearch].c_str()));

	edit_menu_->setTitle(QString::fromUtf8(language[lsEdit].c_str()));
	redo_act_->setText(QString::fromUtf8(language[lsRedo].c_str()));
	undo_act_->setText(QString::fromUtf8(language[lsUndo].c_str()));
	cut_act_->setText(QString::fromUtf8(language[lsCut].c_str()));
	copy_act_->setText(QString::fromUtf8(language[lsCopy].c_str()));
	paste_act_->setText(QString::fromUtf8(language[lsPaste].c_str()));
	exclude_act_->setText(QString::fromUtf8(language[lsExcludedFromCompilation].c_str()));
	block_act_->setText(QString::fromUtf8(language[lsBlocked].c_str()));
	rename_act_->setText(QString::fromUtf8(language[lsRename].c_str()));
	delete_act_->setText(QString::fromUtf8(language[lsDelete].c_str()));
	cut_act2_->setText(cut_act_->text());
	copy_act2_->setText(copy_act_->text());
	paste_act2_->setText(paste_act_->text());
	search_act_->setText(QString::fromUtf8(language[lsSearch].c_str()));

	project_menu_->setTitle(QString::fromUtf8(language[lsProject].c_str()));
#ifdef LITE
	show_act_->setText(QString::fromUtf8(language[lsShowProtectedFunctions].c_str()));
#else
	add_menu_->setTitle(QString::fromUtf8(language[lsAdd].c_str()));
	add_function_act_->setText(QString::fromUtf8(language[lsAddFunction].c_str()) + "...");
	add_function_act2_->setText(add_function_act_->text());
	add_folder_act_->setText(QString::fromUtf8(language[lsAddFolder].c_str()) + "...");
	goto_act_->setText(QString::fromUtf8(language[lsGoTo].c_str()) + "...");
	goto_act2_->setText(goto_act_->text());
	templates_act_->setText(QString::fromUtf8(language[lsTemplates].c_str()));
	templates_save_act_->setText(QString::fromUtf8(language[lsSaveTemplateAs].c_str()) + "...");
	templates_edit_act_->setText(QString::fromUtf8(language[lsEdit].c_str()) + "...");
	if (templates_menu_->actions().size() > 3)
		templates_menu_->actions().at(0)->setText("(" + QString::fromUtf8(language[lsDefault].c_str()) + ")");
#endif
#ifdef ULTIMATE
	save_licenses_act_->setText(QString::fromUtf8(language[lsSaveLicensesAs].c_str()) + "...");
	add_license_act_->setText(QString::fromUtf8(language[lsAddLicense].c_str()) + "...");
	add_file_act_->setText(QString::fromUtf8(language[lsAddFile].c_str()) + "...");
	add_license_act2_->setText(add_license_act_->text());
	add_file_act2_->setText(add_file_act_->text());
	import_menu_->setTitle(QString::fromUtf8(language[lsImport].c_str()));
	import_license_act_->setText(QString::fromUtf8(language[lsImportLicenseFromSerialNumber].c_str()) + "...");
	import_project_act_->setText(QString::fromUtf8(language[lsImportLicensesFromProjectFile].c_str()) + "...");
	export_key_act_->setText(QString::fromUtf8(language[lsExportKeyPair].c_str()) + "...");
#endif
	compile_act_->setText(QString::fromUtf8(language[lsCompile].c_str()));
	execute_act_->setText(QString::fromUtf8(language[lsExecute].c_str()));

	tools_menu_->setTitle(QString::fromUtf8(language[lsTools].c_str()));
#ifndef LITE
	watermarks_act_->setText(QString::fromUtf8(language[lsWatermarks].c_str()) + "...");
#endif
	settings_act_->setText(QString::fromUtf8(language[lsSettings].c_str()) + "...");

	help_menu_->setTitle(QString::fromUtf8(language[lsHelp].c_str()));
	help_act_->setText(QString::fromUtf8(language[lsContents].c_str()));
	home_page_act_->setText(QString::fromUtf8(language[lsHomePage].c_str()));
	about_act_->setText(QString::fromUtf8(language[lsAbout].c_str()) + "...");

	icon_project_->setToolTip(QString::fromUtf8(language[lsProject].c_str()));
#ifndef LITE
	icon_functions_->setToolTip(QString::fromUtf8(language[lsFunctions].c_str()));
	icon_details_->setToolTip(QString::fromUtf8(language[lsDetails].c_str()));
#endif

	function_end_address_act_->setText(QString::fromUtf8(language[lsBreakAddress].c_str()));
	function_ext_address_act_->setText(QString::fromUtf8(language[lsExternalAddress].c_str()));
	function_del_act_->setText(QString::fromUtf8(language[lsDelete].c_str()));

#ifdef ULTIMATE
	licensing_parameters_help_->setText(QString::fromUtf8(language[lsKeyPairHelp].c_str()));
	key_algo_label_->setText(QString::fromUtf8(language[lsKeyPairAlgorithm].c_str()));
	key_len_label_->setText(QString::fromUtf8(language[lsKeyLength].c_str()));
	create_key_button_->setText(QString::fromUtf8(language[lsGenerate].c_str()));
	use_other_project_button_->setText(QString::fromUtf8(language[lsUseOtherProject].c_str()));
#endif

	open_button_->setText(QString::fromUtf8(language[lsOpen].c_str()) + "...");
	examples_button_->setText(QString::fromUtf8(language[lsExamples].c_str()) + "...");
	help_button_->setText(QString::fromUtf8(language[lsHelp].c_str()));

	recent_files_label_->setText(QString::fromUtf8(language[lsRecentFiles].c_str()));
	recent_file_open_act_->setText(QString::fromUtf8(language[lsOpen].c_str()));
	recent_file_remove_act_->setText(QString::fromUtf8(language[lsDelete].c_str()));

	quick_start_label_->setText(QString::fromUtf8(language[lsQuickStart].c_str()));
	welcome_label_->setText(QString::fromUtf8(language[lsWelcome].c_str()));

	function_property_manager_->localize();
	core_property_manager_->localize();
	project_model_->localize();
#ifndef LITE
	info_model_->localize();
	section_property_manager_->localize();
	segment_property_manager_->localize();
	import_property_manager_->localize();
	export_property_manager_->localize();
	resource_property_manager_->localize();
	loadcommand_property_manager_->localize();
	address_calculator_manager_->localize();
#endif
#ifdef ULTIMATE
	license_property_manager_->localize();
	internal_file_property_manager_->localize();
	assembly_property_manager_->localize();
#endif
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
	if (!event->mimeData()->hasUrls())
		return;

	loadFile(QDir::toNativeSeparators(event->mimeData()->urls().at(0).toLocalFile()));
}

bool MainWindow::internalLoadFile(const QString &filename)
{
	bool res = false;
	try {
		res = core_->Open(filename.toUtf8().constData());
	} catch(canceled_error &error) {
		core_->Notify(mtWarning, NULL, error.what());
	} catch(std::runtime_error &error) {
		core_->Notify(mtError, NULL, error.what());
	}
	return res;
}

void MainWindow::loadFile(const QString &filename)
{
	if (!closeFile())
		return;

	boot_panel_->hide();
	{
		QFutureWatcher<bool> watcher;
		ProgressDialog progress(this);
		log_->reset();

		connect(&watcher, SIGNAL(finished()), &progress, SLOT(reject()));
		connect(&progress, SIGNAL(cancel()), log_, SLOT(cancel()));
		connect(log_, SIGNAL(startProgress(const QString &, unsigned long long)), &progress, SLOT(startProgress(const QString &, unsigned long long)));
		connect(log_, SIGNAL(stepProgress(unsigned long long)), &progress, SLOT(stepProgress(unsigned long long)));

		watcher.setFuture(QtConcurrent::run(this, &MainWindow::internalLoadFile, filename));
		progress.exec();
		watcher.waitForFinished();
		if (!watcher.result()) {
			boot_panel_->show();
			return;
		}
	}

	if (core_->input_file())
		fs_watcher_.addPath(QString::fromUtf8(core_->input_file()->file_name().c_str()));

	QString input_file_name = QDir::toNativeSeparators(QDir::cleanPath(core_->input_file() ? QString::fromUtf8(core_->input_file()->file_name().c_str()) : filename));
	QFileInfo fileInfo(input_file_name);
	spacer_->setText(fileInfo.fileName());
	spacer_->setToolTip(input_file_name);
	QFileIconProvider ip;
	QIcon icon = ip.icon(fileInfo);
	if (icon.isNull())
		icon = ip.icon(QFileIconProvider::File);
	QPixmap pixmap = icon.pixmap(QSize(20, 20));
	QImage image = pixmap.toImage();
	for (int i = 0; i < pixmap.width(); ++i) {
		for (int j = 0; j < pixmap.height(); ++j) {
            QRgb col = image.pixel(i, j);
            int gray = qGray(col);
            image.setPixel(i, j, qRgba(gray, gray, gray, qAlpha(col)));
        }
    }
	QIcon newIcon;
	newIcon.addPixmap(pixmap, QIcon::Active, QIcon::Off);
	newIcon.addPixmap(QPixmap::fromImage(image), QIcon::Normal, QIcon::Off);
	spacer_->setIcon(newIcon);

	IFile *file = core_->input_file();
	if (file)
		file->map_function_list()->ReadFromFile(*file);

	bool is_found = false;
	QList<QAction*> actions = file_menu_->actions();
	int file_pos = actions.indexOf(history_separator_) + 1;
	for (int i = file_pos; i < actions.size() - 2; i++) {
		QAction *action = actions[i];
		if (action->text() == filename) {
			file_menu_->removeAction(action);
			file_menu_->insertAction(file_menu_->actions().at(file_pos), action);

			QWidget *button = recent_files_layout_->itemAt(2 + i - file_pos)->widget();
			recent_files_layout_->removeWidget(button);
			recent_files_layout_->insertWidget(2, button);
			is_found = true;
			break;
		}
	}
	if (!is_found) {
		addRecentFile(0, filename);

		if (actions.size() - file_pos - 2 == 10) {
			QAction *action = actions[9 + file_pos];
			file_menu_->removeAction(action);
			delete action;
		}

		if (recent_files_layout_->count() == 14) {
			QWidget *widget = recent_files_layout_->itemAt(12)->widget();
			recent_files_layout_->removeWidget(widget);
			delete widget;
		}
	}

	saveRecentFiles();

	project_model_->setCore(core_);
	ProjectNode *options = project_model_->indexToNode(project_model_->optionsIndex());
	if (options)
		options->setPropertyManager(core_property_manager_);
	project_tree_->setCurrentIndex(project_model_->index(0, 0));

#ifndef LITE
	functions_model_->setCore(core_);
	functions_tree_->setCurrentIndex(functions_model_->index(0, 0));

	info_model_->setCore(core_);
	info_tree_->setCurrentIndex(info_model_->index(0, 0));
#endif

	script_editor_->setText(QString(core_->script()->text().c_str()));
	connect(script_editor_, SIGNAL(notify(SCNotification *)), this, SLOT(scriptNotify(SCNotification *)));
	core_property_manager_->setCore(core_);

	desktop_page_->setCurrentWidget(project_frame_);
	icon_project_->setChecked(true);
	project_tree_->setFocus();
	project_filter_act_->setVisible(true);
	save_as_act_->setEnabled(true);
	close_act_->setEnabled(true);
	search_act_->setEnabled(true);

	bool file_loaded = core_->input_file() != NULL;
	compile_act_->setEnabled(file_loaded);
#ifndef LITE
	icon_functions_->setVisible(file_loaded);
	icon_details_->setVisible(file_loaded);
	add_menu_->setEnabled(file_loaded);
	add_function_act_->setEnabled(file_loaded);
	add_folder_act_->setEnabled(file_loaded);
	goto_act_->setEnabled(file_loaded);
#endif

	bool is_executable = core_->input_file() && core_->input_file()->is_executable();
	execute_act_->setEnabled(is_executable);
	execute_original_act_->setEnabled(is_executable);
	execute_original_act_->setFullText(input_file_name);
	execute_menu_->setDefaultAction(execute_original_act_);
#ifdef ULTIMATE
	add_file_act_->setEnabled(file_loaded && (core_->input_file()->disable_options() & cpVirtualFiles) == 0);
	import_menu_->setEnabled(true);
	import_project_act_->setEnabled(true);
	updateLicensingActions();
#endif
	projectTabMoved();
	
	updateCaption();
}

void MainWindow::loadFileFromHistory()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		loadFile(action->text());
}

void MainWindow::loadFileFromBoot()
{
	QToolButton *button = qobject_cast<QToolButton *>(sender());
	if (button)
		loadFile(button->toolTip());
}

bool MainWindow::closeFile()
{
	if (project_model_->isEmpty())
		return true;

	if (save_act_->isEnabled()) {
		switch (MessageDialog::question(this, QString("%1 \"%2\"?").arg(QString::fromUtf8(language[lsSave].c_str())).arg(QString::fromUtf8(core_->project_file_name().c_str())), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)) {
		case QMessageBox::Yes:
			if (!save())
				return false;
			break;
		case QMessageBox::No:
			// do nothing
			break;
		default:
			return false;
		}
	}

	fs_watcher_.removePaths(fs_watcher_.files());
	fileChanged_ = false;

	log_model_->clear();
	project_model_->setCore(NULL);
	directory_model_->setDirectory(NULL);
	search_model_->clear();
	disconnect(script_editor_, SIGNAL(notify(SCNotification *)), this, SLOT(scriptNotify(SCNotification *)));
	script_editor_->setText("");
	core_property_manager_->setCore(NULL);
	function_property_manager_->setValue(NULL);
#ifndef LITE
	functions_model_->setCore(NULL);
	info_model_->setCore(NULL);
	disasm_model_->setFile(NULL);
	dump_model_->setFile(NULL);
	section_property_manager_->setValue(NULL);
	segment_property_manager_->setValue(NULL);
	import_property_manager_->setValue(NULL);
	export_property_manager_->setValue(NULL);
	resource_property_manager_->setValue(NULL);
	loadcommand_property_manager_->setValue(NULL);
	address_calculator_manager_->setValue(NULL);
#endif
#ifdef ULTIMATE
	license_property_manager_->setValue(NULL);
	internal_file_property_manager_->setValue(NULL);
	assembly_property_manager_->setValue(NULL);
#endif
	if (temp_function_) {
		for (size_t i = 0; i < temp_function_->count(); i++) {
			delete temp_function_->item(i)->func();
		}
		delete temp_function_;
		temp_function_ = NULL;
	}
	core_->Close();

	spacer_->setFixedWidth(0);
	spacer_->setText("");
	spacer_->setToolTip("");
	spacer_->setIcon(QIcon());
	project_filter_act_->setVisible(false);
	project_filter_->clear();
	boot_panel_->show();
	desktop_page_->setCurrentWidget(boot_frame_);
	save_act_->setEnabled(false);
	save_as_act_->setEnabled(false);
	close_act_->setEnabled(false);
	redo_act_->setEnabled(false);
	undo_act_->setEnabled(false);
	cut_act_->setEnabled(false);
	copy_act_->setEnabled(false);
	paste_act_->setEnabled(false);
	block_act_->setEnabled(false);
	exclude_act_->setEnabled(false);
	rename_act_->setEnabled(false);
	delete_act_->setEnabled(false);
	search_act_->setEnabled(false);
	//project_separator_->setVisible(false);
	cut_act2_->setVisible(false);
	copy_act2_->setVisible(false);
	paste_act2_->setVisible(false);
#ifdef LITE
	show_act_->setVisible(false);
#else
	add_menu_->setEnabled(false);
	add_function_act2_->setVisible(false);
	goto_act_->setEnabled(false);
	goto_act2_->setVisible(false);
	add_function_act_->setEnabled(false);
	add_folder_act_->setEnabled(false);
#endif
	templates_act_->setVisible(false);
#ifdef ULTIMATE
	save_licenses_act_->setEnabled(false);
	add_license_act2_->setEnabled(false);
	add_license_act2_->setVisible(false);
	add_file_act2_->setVisible(false);
	import_license_act_->setEnabled(false);
	import_project_act_->setEnabled(false);
	add_license_act_->setEnabled(false);
	export_key_act_->setEnabled(false);
	import_menu_->setEnabled(false);
#endif
	compile_act_->setEnabled(false);
	execute_act_->setEnabled(false);
	execute_protected_act_->setVisible(false);
	updateCaption();

	return true;
}

void MainWindow::open()
{
	QString fileName = FileDialog::getOpenFileName(this, QString::fromUtf8(language[lsOpen].c_str()), FileDialog::defaultPath(),
		QString("%1 ("
#ifdef VMP_GNU
				"*.app *.dylib *.exe *.dll *.bpl *.ocx *.sys *.scr *.so);;%2 (*.vmp);;%3 (*)"
#else
				"*.exe *.dll *.bpl *.ocx *.sys *.scr *.dylib *.so);;%2 (*.vmp);;%3 (*.*)"
#endif
				).arg(QString::fromUtf8(language[lsExecutableFiles].c_str())).arg(QString::fromUtf8(language[lsProjectFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));
	
	if (fileName.isEmpty())
		return;

	loadFile(fileName);
}

bool MainWindow::save()
{
	bool ret = core_->Save();
	if (ret) {
		save_act_->setEnabled(false);
		updateCaption();
	} else {
		MessageDialog::critical(this, QString::fromUtf8(string_format(language[lsSaveFileError].c_str(), core_->project_file_name().c_str()).c_str()), QMessageBox::Ok);
	}

	return ret;
}

void MainWindow::saveAs()
{
	QString fileName = FileDialog::getSaveFileName(this, QString::fromUtf8(language[lsSave].c_str()), QString::fromUtf8(core_->project_file_name().c_str()),
		QString(
#ifdef VMP_GNU
		"%1 (*.vmp);;%2 (*)"
#else
		"%1 (*.vmp);;%2 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsProjectFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));

	if (fileName.isEmpty())
		return;

	if (core_->SaveAs(fileName.toUtf8().constData())) {
		save_act_->setEnabled(false);
		updateCaption();
	} else {
		MessageDialog::critical(this, QString::fromUtf8(string_format(language[lsSaveFileError].c_str(), fileName.toUtf8().constData()).c_str()), QMessageBox::Ok);
	}
}

void MainWindow::saveLicenses()
{
#ifdef ULTIMATE
	QString fileName = FileDialog::getSaveFileName(this, QString::fromUtf8(language[lsSave].c_str()), QString::fromUtf8(core_->project_file_name().c_str()),
		QString(
#ifdef VMP_GNU
		"%1 (*.vmp);;%2 (*)"
#else
		"%1 (*.vmp);;%2 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsProjectFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));

	if (fileName.isEmpty())
		return;

	if (!core_->licensing_manager()->SaveAs(fileName.toUtf8().constData())) {
		MessageDialog::critical(this, QString::fromUtf8(string_format(language[lsSaveFileError].c_str(), fileName.toUtf8().constData()).c_str()), QMessageBox::Ok);
		return;
	}
#endif
}

void MainWindow::homePage()
{
	QDesktopServices::openUrl(QUrl("http://www.vmpsoft.com"));
}

void MainWindow::help()
{
	QString keywordId;
	ProjectNode *current = currentProjectNode();
	if (current) {
		switch (current->type()) {
		case NODE_ROOT:
			break;
		case NODE_OPTIONS:
			keywordId = "project::options";
			break;
		case NODE_SCRIPT:
		case NODE_SCRIPT_BOOKMARK:
			keywordId = "project::script";
			break;
		case NODE_FILES:
		case NODE_FILE:
		case NODE_FILE_FOLDER:
		case NODE_ASSEMBLIES:
			keywordId = "project::files";
			break;
		case NODE_LICENSES:
		case NODE_LICENSE:
			keywordId = "project::licenses";
			break;
		case NODE_FUNCTIONS:
		case NODE_FUNCTION:
		case NODE_FOLDER:
			keywordId = "project::functions";
			break;
		case NODE_MAP_FUNCTION:
		case NODE_MAP_FOLDER:
			keywordId = "project::mapfunctions";
			break;
		default:
			keywordId = "project::details";
			break;
		}
	}
	HelpBrowser::showTopic(keywordId);
}

void MainWindow::about()
{
	AboutDialog dialog(this);
	dialog.exec();
}

void MainWindow::examples()
{
#ifdef VMP_GNU
	QString path = QCoreApplication::applicationDirPath() + "/examples";
#else
	wchar_t buf[MAX_PATH];
	QString path;
	// приоритетная папка, сюда дистрибутив пишет
	if (SHGetSpecialFolderPathW(0, buf, CSIDL_COMMON_DOCUMENTS, FALSE))
	{
		path = QDir::fromNativeSeparators(QString::fromWCharArray(buf)) + "/VMProtect";
		if (!QDir(path).exists())
			path.clear();
	}
	if (path.isEmpty())
	{
		// если несколько юзеров работают одновременно, надо каждому скопировать папку из CSIDL_COMMON_DOCUMENTS к себе в документы,
		// а исходную грохнуть - в итоге каждый работает со своим каталогом
		path = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first() + "/VMProtect";
	}
#endif

	QString fileName = FileDialog::getOpenFileName(this, QString::fromUtf8(language[lsOpen].c_str()), path, QString("%1 ("
#ifdef VMP_GNU
				"*.app *.dylib *.exe *.dll *.bpl *.ocx *.sys *.scr *.so);;%2 (*.vmp);;%3 (*)"
#else
				"*.exe *.dll *.bpl *.ocx *.sys *.scr *.dylib *.so);;%2 (*.vmp);;%3 (*.*)"
#endif
				).arg(QString::fromUtf8(language[lsExecutableFiles].c_str())).arg(QString::fromUtf8(language[lsProjectFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));
	
	if (fileName.isEmpty())
		return;

	loadFile(fileName);
}

void MainWindow::watermarks()
{
#ifndef LITE
	WatermarksWindow dialog(false, this);
	dialog.exec();
#endif
}

void MainWindow::showFile()
{
	QString path = spacer_->toolTip();
#ifdef __APPLE__
	QStringList scriptArgs;
	scriptArgs << QLatin1String("-e")
		<< QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
		.arg(path);
	QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
	scriptArgs.clear();
	scriptArgs << QLatin1String("-e")
		<< QLatin1String("tell application \"Finder\" to activate");
	QProcess::execute("/usr/bin/osascript", scriptArgs);
#else 
#ifndef VMP_GNU
	QString param;
	if (!QFileInfo(path).isDir())
		param = QLatin1String("/select,");
	param += QDir::toNativeSeparators(path);
	QProcess::startDetached("explorer " + param);
#else
	// we cannot select a file here, because no file browser really supports it...
	const QString folder = QDir::toNativeSeparators(QFileInfo(path).absoluteDir().absolutePath());
	QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
#endif
#endif
}

void MainWindow::settings()
{
	std::string lang_id = settings_file().language();
	SettingsDialog dialog(this);
	if (dialog.exec() == QDialog::Accepted) {
		if (settings_file().language() != lang_id)
			localize();
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (closeFile())
		event->accept();
    else
		event->ignore();
}

void MainWindow::projectModified()
{
	save_act_->setEnabled(true);
	updateCaption();
}

void MainWindow::projectNodeRemoved(ProjectNode *node)
{
	directory_model_->removeNode(node);
	search_model_->removeNode(node);
}

void MainWindow::projectObjectRemoved(void *object)
{
	if (function_property_manager_->value() && function_property_manager_->value() == object)
		function_property_manager_->setValue(NULL);
	log_model_->removeObject(object);
}

void MainWindow::updateCaption()
{
	QString caption = caption_;
	if (!project_model_->isEmpty()) {
		QString fileName = QString::fromUtf8(core_->project_file_name().c_str());
		QFileInfo fileInfo(fileName);
		caption = QString("%1%2 %3 %4").arg(fileInfo.fileName()).arg(save_act_->isEnabled() ? "*" : "").arg(QString::fromUtf8("\xE2\x80\x94")).arg(caption_);
		project_file_name_->setText(fileName);
	} else {
		project_file_name_->setText("");
	}
	setWindowTitle(caption);
}

QTreeView *MainWindow::currentTreeView() const
{
#ifdef LITE
	return project_tree_;
#else
	if (project_tab_->currentWidget() == project_tree_)
		return project_tree_;
	else if (project_tab_->currentWidget() == functions_tree_)
		return functions_tree_;
	else
		return info_tree_;
#endif
}

ProjectNode *MainWindow::currentProjectNode(bool focusedTree) const
{
	QTreeView *tree_view = (focusedTree && focusWidget() == directory_tree_ && directory_tree_->selectionModel()->selectedIndexes().count()) ? directory_tree_ : currentTreeView();
	IProjectNodesModel *model = dynamic_cast<IProjectNodesModel *>(tree_view->model());
	return model ? model->indexToNode(tree_view->currentIndex()) : NULL;
}

void MainWindow::projectItemChanged()
{
	if (project_filter_->text().isEmpty())
		showCurrentObject();
	else
		project_filter_->clear();
}

void MainWindow::projectFilterChanged()
{
	if (project_filter_->text().isEmpty()) {
		showCurrentObject();
		return;
	}

	QAbstractItemModel *model = currentTreeView()->model();
	assert(model);
	if (model == NULL) return;

	search_model_->search(reinterpret_cast<BaseModel *>(model)->root(), project_filter_->text()
#ifdef LITE
		, (show_act_->isVisible() && show_act_->isChecked())
#endif
		);
	project_page_->setCurrentWidget(search_tree_);
}

void MainWindow::showCurrentObject()
{
    ProjectNode *current = currentProjectNode();
	if (!current)
		return;

	cut_act2_->setVisible(current->type() == NODE_SCRIPT || current->type() == NODE_SCRIPT_BOOKMARK);
	copy_act2_->setVisible(current->type() == NODE_SCRIPT || current->type() == NODE_SCRIPT_BOOKMARK);
	paste_act2_->setVisible(current->type() == NODE_SCRIPT || current->type() == NODE_SCRIPT_BOOKMARK);
	script_line_->setVisible(current->type() == NODE_SCRIPT || current->type() == NODE_SCRIPT_BOOKMARK);
	script_column_->setVisible(current->type() == NODE_SCRIPT || current->type() == NODE_SCRIPT_BOOKMARK);
	script_mode_->setVisible(current->type() == NODE_SCRIPT || current->type() == NODE_SCRIPT_BOOKMARK);
	templates_act_->setVisible(current->type() == NODE_OPTIONS);
#ifdef LITE
	show_act_->setVisible(current->type() == NODE_FUNCTIONS || current->type() == NODE_MAP_FOLDER);
#else
	add_folder_act_->setEnabled(current->type() == NODE_FUNCTIONS || current->type() == NODE_FOLDER || current->type() == NODE_FUNCTION || current->type() == NODE_FILES || current->type() == NODE_FILE_FOLDER || current->type() == NODE_FILE || current->type() == NODE_ASSEMBLIES);
	goto_act2_->setVisible(icon_details_->isChecked());
#endif

#ifdef LITE
	if (show_act_->isVisible() && show_act_->isChecked()) {
		ProjectNode *root = reinterpret_cast<BaseModel *>(currentTreeView()->model())->root();
		search_model_->search(root, project_filter_->text(), true);
		project_page_->setCurrentWidget(search_tree_);
		context_find_->hide();
		return;
	}
#endif

	switch (current->type()) {
#ifdef ULTIMATE
	case NODE_LICENSES:
		{
			LicensingManager *licensingManager = static_cast<LicensingManager *>(current->data());
			if (licensingManager->empty()) {
				project_page_->setCurrentWidget(licensing_parameters_page_);
				break;
			}
		}
#endif
	case NODE_ARCHITECTURE:
	case NODE_LOAD_COMMANDS:
	case NODE_SEGMENTS:
	case NODE_FUNCTIONS:
	case NODE_FOLDER:
	case NODE_RESOURCES:
	case NODE_RESOURCE_FOLDER:
	case NODE_IMPORTS:
	case NODE_IMPORT:
	case NODE_EXPORTS:
	case NODE_MAP_FOLDER:
	case NODE_IMPORT_FOLDER:
	case NODE_EXPORT_FOLDER:
	case NODE_FILES:
	case NODE_FILE_FOLDER:
	case NODE_ASSEMBLIES:
		directory_model_->setDirectory(current);
		for (int i = 0; i < directory_tree_->header()->count(); i++) {
			directory_tree_->header()->resizeSection(i,  Application::stylesheetScaleFactor() *((i == 0) ? 240 : 100));
		}

		directory_tree_->header()->setSectionsClickable(current->type() == NODE_FUNCTIONS || current->type() == NODE_FOLDER || current->type() == NODE_FILES || current->type() == NODE_FILE_FOLDER || current->type() == NODE_LICENSES || current->type() == NODE_ASSEMBLIES);
		project_page_->setCurrentWidget(directory_tree_);
		break;

	case NODE_OPTIONS:
		project_page_->setCurrentWidget(core_property_editor_);
		break;

#ifndef LITE
	case NODE_SCRIPT:
		project_page_->setCurrentWidget(script_editor_);
		break;

	case NODE_SCRIPT_BOOKMARK:
		project_page_->setCurrentWidget(script_editor_);
		script_editor_->setFocus();
		script_editor_->send(SCI_GOTOPOS, project_model_->bookmarkNodeToPos(current));
		break;
#endif

	case NODE_MAP_FUNCTION:
		{
			MapFunctionBundle *map = static_cast<MapFunctionBundle *>(current->data());
			FunctionBundle *func = core_->input_file()->function_list()->GetFunctionByName(map->name());
			FunctionBundle *old_temp_function = NULL;
			if (!func) {
				old_temp_function = temp_function_;
				temp_function_ = NULL;
				func = new FunctionBundle(core_->input_file()->function_list(), map->full_name(), false);
				for (size_t i = 0; i < map->count(); i++) {
					MapFunctionArch *map_arch = map->item(i);
					FunctionArch *func_arch = func->Add(map_arch->arch(), map_arch->arch()->function_list()->CreateFunction());
					func_arch->func()->set_need_compile(false);
					func_arch->func()->set_compilation_type(ctVirtualization);
					func_arch->func()->ReadFromFile(*map_arch->arch(), map_arch->func()->address());
				}
				temp_function_ = func;
			}
			function_property_manager_->setValue(func);

			if (old_temp_function) {
				for (size_t i = 0; i < old_temp_function->count(); i++) {
					delete old_temp_function->item(i)->func();
				}
				delete old_temp_function;
			}
		}
		project_page_->setCurrentWidget(function_property_editor_);
		break;

	case NODE_FUNCTION:
		{
			FunctionBundle *object = static_cast<FunctionBundle *>(current->data());
			function_property_manager_->setValue(object);
		}
		project_page_->setCurrentWidget(function_property_editor_);
		break;

#ifndef LITE
	case NODE_SEGMENT:
		{
			ISection *object = static_cast<ISection *>(current->data());
			segment_property_manager_->setValue(object);
		}
		project_page_->setCurrentWidget(segment_property_editor_);
		break;

	case NODE_SECTION:
		{
			ISection *object = static_cast<ISection *>(current->data());
			section_property_manager_->setValue(object);
		}
		project_page_->setCurrentWidget(section_property_editor_);
		break;

	case NODE_IMPORT_FUNCTION:
		{
			IImportFunction *object = static_cast<IImportFunction *>(current->data());
			import_property_manager_->setValue(object);
		}
		project_page_->setCurrentWidget(import_property_editor_);
		break;

	case NODE_EXPORT:
		{
			IExport *object = static_cast<IExport *>(current->data());
			export_property_manager_->setValue(object);
		}
		project_page_->setCurrentWidget(export_property_editor_);
		break;

	case NODE_RESOURCE:
		{
			IResource *object = static_cast<IResource *>(current->data());
			resource_property_manager_->setValue(object);
		}
		project_page_->setCurrentWidget(resource_property_editor_);
		break;

	case NODE_LOAD_COMMAND:
		{
			ILoadCommand *object = static_cast<ILoadCommand *>(current->data());
			loadcommand_property_manager_->setValue(object);
		}
		project_page_->setCurrentWidget(loadcommand_property_editor_);
		break;

	case NODE_CALC:
		{
			IArchitecture *file = static_cast<IArchitecture *>(current->data());
			address_calculator_manager_->setValue(file);
		}
		project_page_->setCurrentWidget(address_calculator_);
		break;

	case NODE_DUMP:
		{
			IArchitecture *file = static_cast<IArchitecture *>(current->data());
			QModelIndex index = disasm_view_->currentIndex();
			disasm_model_->setFile(file);
			if (!index.isValid() && file->entry_point())
				goToDump(file, file->entry_point());
			dump_model_->setFile(file);
		}
		project_page_->setCurrentWidget(dump_page_);
		break;
#endif

#ifdef ULTIMATE
	case NODE_LICENSE:
		{
			License *object = reinterpret_cast<License *>(current->data());
			license_property_manager_->setValue(object);
		}
		project_page_->setCurrentWidget(license_property_editor_);
		break;

	case NODE_FILE:
		if (project_model_->objectToNode(core_->file_manager()->folder_list())->type() == NODE_ASSEMBLIES) {
			InternalFile *object = reinterpret_cast<InternalFile *>(current->data());
			assembly_property_manager_->setValue(object);
			project_page_->setCurrentWidget(assembly_property_editor_);
		}
		else {
			InternalFile *object = reinterpret_cast<InternalFile *>(current->data());
			internal_file_property_manager_->setValue(object);
			project_page_->setCurrentWidget(internal_file_property_editor_);
		}
		break;
#endif
	}
	if(!isContextSearchApplicable())
		context_find_->hide();
}

void MainWindow::projectContextMenu(const QPoint &p)
{
	ProjectNode *current = currentProjectNode(true);
	if (!current)
		return;

	QMenu menu;
	switch (current->type()) {
	case NODE_FUNCTIONS:
	case NODE_FUNCTION:
	case NODE_FOLDER:
#ifndef LITE
		menu.addAction(add_function_act2_);
		menu.addAction(add_folder_act_);
#endif
		break;
#ifdef ULTIMATE
	case NODE_LICENSE:
		{
			License *object = reinterpret_cast<License *>(current->data());
			block_act_->setChecked(object->blocked());
		}
	case NODE_LICENSES:
		menu.addAction(add_license_act2_);
		break;
	case NODE_FILES:
	case NODE_FILE_FOLDER:
	case NODE_FILE:
	case NODE_ASSEMBLIES:
		menu.addAction(add_file_act2_);
		menu.addAction(add_folder_act_);
		break;
#endif
	}

	switch (current->type()) {
	case NODE_FUNCTION:
	case NODE_FOLDER:
	case NODE_LICENSE:
	case NODE_FILE:
	case NODE_FILE_FOLDER:
		menu.addSeparator();
		menu.addAction(delete_act_);
		if (current->type() != NODE_FUNCTION)
			menu.addAction(rename_act_);
		break;
	}

	switch (current->type()) {
	case NODE_LICENSE:
		menu.addSeparator();
		menu.addAction(block_act_);
		break;
	case NODE_FILES:
	case NODE_ASSEMBLIES:
#ifdef ULTIMATE
		{
			FileManager *object = static_cast<FileManager *>(current->data());
			exclude_act_->setChecked(!object->need_compile());
		}
#endif
		menu.addSeparator();
		menu.addAction(exclude_act_);
		break;
	case NODE_FUNCTION:
		{
			FunctionBundle *object = static_cast<FunctionBundle *>(current->data());
			exclude_act_->setChecked(!object->need_compile());
		}
		menu.addSeparator();
		menu.addAction(exclude_act_);
		break;
	case NODE_SCRIPT:
	case NODE_SCRIPT_BOOKMARK:
		{
			Script *object = static_cast<Script *>(current->data());
			exclude_act_->setChecked(!object->need_compile());
		}
		menu.addAction(exclude_act_);
		break;
	case NODE_MAP_FUNCTION:
		{
			MapFunctionBundle *map = static_cast<MapFunctionBundle *>(current->data());
			FunctionBundle *func = core_->input_file()->function_list()->GetFunctionByName(map->name());
			exclude_act_->setChecked(func ? !func->need_compile() : true);
		}
		menu.addAction(exclude_act_);
		break;
	}

	if (!menu.isEmpty())
	{
		rename_act_->setEnabled(current->type() == NODE_FOLDER || current->type() == NODE_LICENSE || current->type() == NODE_FILE_FOLDER || current->type() == NODE_FILE);
		block_act_->setEnabled(current->type() == NODE_LICENSE);
		exclude_act_->setEnabled(current->type() == NODE_FUNCTION || current->type() == NODE_SCRIPT_BOOKMARK || current->type() == NODE_SCRIPT || current->type() == NODE_FILES || current->type() == NODE_MAP_FUNCTION || current->type() == NODE_ASSEMBLIES);
		menu.exec((qobject_cast<QAbstractScrollArea *>(QObject::sender()))->viewport()->mapToGlobal(p));
	}
}

void MainWindow::scriptContextMenu(const QPoint &p)
{
	script_menu_->exec(script_editor_->viewport()->mapToGlobal(p));
}

void MainWindow::functionContextMenu(const QPoint &p)
{
	ICommand *command = function_property_manager_->indexToCommand(function_property_editor_->currentIndex());
	if (command) {
		QMenu menu;
		menu.addAction(copy_act_);
		menu.addSeparator();
		menu.addAction(function_ext_address_act_);
		menu.addAction(function_end_address_act_);
		menu.exec(function_property_editor_->viewport()->mapToGlobal(p));
		return;
	}

	IFunction *func = function_property_manager_->indexToFunction(function_property_editor_->currentIndex());
	if (func) {
		QMenu menu;
		menu.addAction(function_del_act_);
		menu.exec(function_property_editor_->viewport()->mapToGlobal(p));
		return;
	}
}

void MainWindow::functionExtAddress()
{
	ICommand *command = function_property_manager_->indexToCommand(function_property_editor_->currentIndex());
	if (!command)
		return;

	ExtCommandList *extCommandList = command->owner()->ext_command_list();
	ExtCommand *extCommand = extCommandList->GetCommandByAddress(command->address());
	if (extCommand) {
		delete extCommand;
	} else {
		extCommandList->Add(command->address());
	}
}

void MainWindow::functionEndAddress()
{
	ICommand *command = function_property_manager_->indexToCommand(function_property_editor_->currentIndex());
	if (!command)
		return;

	IFunction *func = command->owner();
	func->set_break_address(func->is_breaked_address(command->address()) ? 0 : command->address());
}

void MainWindow::functionDel()
{
	IFunction *func = function_property_manager_->indexToFunction(function_property_editor_->currentIndex());
	if (func)
		delete func;
}

void MainWindow::treeItemDoubleClicked(const QModelIndex &index)
{
	ProjectNode *current = static_cast<ProjectNode *>(index.internalPointer());
	if (!current)
		return;

	QString term;
	if (project_page_->currentWidget() == search_tree_) {
		term = project_filter_->text();
		project_filter_->clear();
	}

	QTreeView *tree_view = currentTreeView();
	BaseModel *model = reinterpret_cast<BaseModel*>(tree_view->model());
	assert(model);
	if (model == NULL) return;

	if (current->type() == NODE_PROPERTY) {
		Property *prop = static_cast<Property *>(current->data());
		Property *root = prop;
		while (root && root->parent()) {
			root = root->parent();
		}
		if (core_property_manager_->root() == root) {
			current = project_model_->indexToNode(project_model_->optionsIndex());
			core_property_editor_->setCurrentIndex(core_property_manager_->propertyToIndex(prop));
		}
	}

	tree_view->setCurrentIndex(reinterpret_cast<BaseModel*>(model)->nodeToIndex(current));

	if (!term.isEmpty() && (current->type() == NODE_SCRIPT || current->type() == NODE_SCRIPT_BOOKMARK))
		context_find_->showAndClear(term);
}

void MainWindow::notify(MessageType type, IObject *sender, const QString &message)
{
	if (sender) {
		if (Watermark *watermark = dynamic_cast<Watermark *>(sender)) {
			switch (type) {
			case mtAdded:
				watermarks_model_->addWatermark(watermark);
				break;
			case mtChanged:
				watermarks_model_->updateWatermark(watermark);
				break;
			case mtDeleted:
				watermarks_model_->removeWatermark(watermark);
				break;
			}
			return;
		}
		if (ProjectTemplate *pt = dynamic_cast<ProjectTemplate *>(sender)) {
			switch (type) {
			case mtAdded:
				templates_model_->addTemplate(pt);
				break;
			case mtChanged:
				templates_model_->updateTemplate(pt);
				break;
			case mtDeleted:
				templates_model_->removeTemplate(pt);
				break;
			}
			updateTemplates();
			return;
		}
	}

	if (project_model_->isEmpty()) {
		QApplication::restoreOverrideCursor();
		switch (type) {
		case mtError:
			MessageDialog::critical(this, message, QMessageBox::Ok);
			break;
		case mtWarning:
			MessageDialog::warning(this, message, QMessageBox::Ok);
			break;
		case mtInformation:
			MessageDialog::information(this, message, QMessageBox::Ok);
			break;
		}
		return;
	}

	QString add;
	if (sender) {
		if (dynamic_cast<Core *>(sender)) {
			core_property_manager_->update();
			projectModified();
		} else if (IFunction *func = dynamic_cast<IFunction *>(sender)) {
			switch (type) {
			case mtAdded:
				project_model_->addFunction(func);
#ifndef LITE
				functions_model_->updateFunction(func);
#endif
				function_property_manager_->addFunction(func);
				break;
			case mtChanged:
				if (temp_function_ && temp_function_->GetArchByFunction(func)) {
					for (size_t i = 0; i < temp_function_->count(); i++) {
						FunctionArch *func_arch = temp_function_->item(i);
						IFunction *func_src = func_arch->func();
						IFunction *func_dst = func_arch->arch()->function_list()->AddByAddress(func_src->address(), temp_function_->compilation_type(), temp_function_->compilation_options(), temp_function_->need_compile(), NULL);
						if (func_dst) {
							func_dst->set_break_address(func_src->break_address());
							for (size_t i = 0; i < func_src->ext_command_list()->count(); i++) {
								func_dst->ext_command_list()->Add(func_src->ext_command_list()->item(i)->address());
							}
						}
					}
				}
				function_property_manager_->updateFunction(func);
				project_model_->updateFunction(func);
#ifndef LITE
				functions_model_->updateFunction(func);
#endif
				break;
			case mtDeleted:
				{
					bool needUpdate = function_property_manager_->removeFunction(func);
					if (temp_function_) {
						FunctionArch *func_arch = temp_function_->GetArchByFunction(func);
						if (func_arch)
							delete func_arch;
					}
					project_model_->removeFunction(func);
					if (needUpdate)
						function_property_manager_->update();
#ifndef LITE
					functions_model_->removeFunction(func);
#endif
				}
				break;
			default:
				{
					IArchitecture *arch = func->owner()->owner();
					if (core_->input_file()->IndexOf(arch) == NOT_ID) {
						sender = NULL;
						arch = core_->input_file()->GetArchitectureByName(arch->name());
						if (arch)
							sender = (func->type() == otUnknown) ? arch->function_list()->GetUnknownByName(func->name()) : arch->function_list()->GetFunctionByAddress(func->address());
					}
				}
				break;
			}
		} else if (Folder *folder = dynamic_cast<Folder *>(sender)) {
			switch (type) {
			case mtAdded:
				project_model_->addFolder(folder);
				break;
			case mtChanged:
				project_model_->updateFolder(folder);
				break;
			case mtDeleted:
				project_model_->removeFolder(folder);
				break;
			}
		} 
#ifdef ULTIMATE
		else if (License *license = dynamic_cast<License *>(sender)) {
			switch (type) {
			case mtAdded:
				project_model_->addLicense(license);
				break;
			case mtChanged:
				project_model_->updateLicense(license);
				if (license_property_manager_->value() == license)
					license_property_manager_->update();
				break;
			case mtDeleted:
				project_model_->removeLicense(license);
				if (license_property_manager_->value() == license)
					license_property_manager_->setValue(NULL);
				break;
			}
		} else if (FileFolder *folder = dynamic_cast<FileFolder *>(sender)) {
			switch (type) {
			case mtAdded:
				project_model_->addFileFolder(folder);
				break;
			case mtChanged:
				project_model_->updateFileFolder(folder);
				break;
			case mtDeleted:
				project_model_->removeFileFolder(folder);
				break;
			}
		} else if (InternalFile *file = dynamic_cast<InternalFile *>(sender)) {
			switch (type) {
			case mtAdded:
				project_model_->addFile(file);
				break;
			case mtChanged:
				project_model_->updateFile(file);
				if (internal_file_property_manager_->value() == file)
					internal_file_property_manager_->update();
				else if (assembly_property_manager_->value() == file)
					assembly_property_manager_->update();
				break;
			case mtDeleted:
				project_model_->removeFile(file);
				if (internal_file_property_manager_->value() == file)
					internal_file_property_manager_->update();
				else if (assembly_property_manager_->value() == file)
					assembly_property_manager_->update();
				break;
			}
		} else if (/*LicensingManager *licensingManager = */dynamic_cast<LicensingManager *>(sender)) {
			if (type == mtChanged) {
				updateLicensingActions();
				core_property_manager_->update();
				projectModified();
				ProjectNode *current = currentProjectNode();
				if (current && current->type() == NODE_LICENSES)
					showCurrentObject();
			}
		} else if (dynamic_cast<FileManager *>(sender)) {
			project_model_->updateFiles();
			projectModified();
		} 
#endif
		else if (dynamic_cast<Script *>(sender)) {
			if (type == mtChanged) {
				project_model_->updateScript();
				projectModified();
				}
		} else if (ExtCommand *extCommand = dynamic_cast<ExtCommand *>(sender)) {
			if (!extCommand->owner())
				return;

			IFunction *func = extCommand->owner()->owner();
			if (temp_function_ && temp_function_->GetArchByFunction(func))
				notify(mtChanged, func, "");

			project_model_->updateFunction(func);
			if (function_property_manager_->value() && function_property_manager_->value()->GetArchByFunction(func))
				function_property_manager_->update();
		} else if (ICommand *command = dynamic_cast<ICommand *>(sender)) {
			IFunction *func = command->owner();
			IArchitecture *arch = func->owner()->owner();

			std::string func_name = func->display_name();
			if (!func_name.empty())
				func_name += ".";
			add = QString::fromUtf8(string_format("%s%.8llX: ", func_name.c_str(), command->address()).c_str());
			if (core_->input_file()->IndexOf(arch) == NOT_ID) {
				sender = NULL;
				arch = core_->input_file()->GetArchitectureByName(arch->name());
				if (arch)
					sender = (func->type() == otUnknown) ? arch->function_list()->GetFunctionByName(func->name()) : arch->function_list()->GetFunctionByAddress(func->address());
			} else {
				sender = func;
			}
		} 
#ifndef LITE
		else if (IResource *resource = dynamic_cast<IResource *>(sender)) {
			switch (type) {
			case mtChanged:
				info_model_->updateResource(resource);
				if (resource_property_manager_->value() == resource)
					resource_property_manager_->update();
				break;
			}
		} else if (ISection *section = dynamic_cast<ISection *>(sender)) {
			switch (type) {
			case mtChanged:
				info_model_->updateSegment(section);
				if (segment_property_manager_->value() == section)
					segment_property_manager_->update();
				break;
			}
		} else if (IImport *import = dynamic_cast<IImport *>(sender)) {
			switch (type) {
			case mtChanged:
				info_model_->updateImport(import);
				break;
			}
		}
#endif
	}

	if (type == mtInformation || type == mtWarning || type == mtError || type == mtScript)
		log_model_->addMessage(type, sender, add + message);
}

void MainWindow::projectTabClicked()
{
#ifdef LITE
	icon_project_->setChecked(true);
#else
	if (icon_details_->isChecked())
		project_tab_->setCurrentWidget(info_tree_);
	else if (icon_functions_->isChecked())
		project_tab_->setCurrentWidget(functions_tree_);
	else
#endif
		project_tab_->setCurrentWidget(project_tree_);
}

void MainWindow::projectTabMoved()
{
	spacer_->setFixedWidth((spacer_->width() > project_separator_widget_->pos().x() ? 0 : spacer_->width()) + project_tab_->width() + icon_project_->width() - project_separator_widget_->pos().x());
}

void MainWindow::scriptNotify(SCNotification *notification)
{
	switch (notification->nmhdr.code) {
	case SCN_MODIFIED:
		if (notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
			core_->script()->set_text(script_editor_->text().toUtf8().constData());
			projectModified();
#ifndef LITE
			project_model_->updateScriptBookmarks();
#endif
		}
		break;

	case SCN_UPDATEUI:
		undo_act_->setEnabled(script_editor_->canUndo());
		redo_act_->setEnabled(script_editor_->canRedo());
		cut_act_->setEnabled(script_editor_->hasSelection());
		copy_act_->setEnabled(script_editor_->hasSelection());
		paste_act_->setEnabled(script_editor_->canPaste());

		cut_act2_->setEnabled(cut_act_->isEnabled());
		copy_act2_->setEnabled(copy_act_->isEnabled());
		paste_act2_->setEnabled(paste_act_->isEnabled());

		script_line_->setText(QString("Ln: %1").arg(script_editor_->currentPos().y() + 1));
		script_column_->setText(QString("Col: %2").arg(script_editor_->currentPos().x() + 1));
		script_mode_->setText(script_editor_->getOverType() ? "OVR" : "INS");

		script_editor_->hilightView();
		break;

	case SCN_CHARADDED:
		script_editor_->maintainIndentation(notification->ch);
		break;
	}
}

void MainWindow::scriptModeClicked()
{
	script_editor_->setOverType(!script_editor_->getOverType());
	script_mode_->setText(script_editor_->getOverType() ? "OVR" : "INS");
}

void MainWindow::undo()
{
	QWidget *widget = focusWidget();
	if (widget == script_editor_)
		script_editor_->undo();
	else if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget))
		edit->undo();
	else if (QPlainTextEdit *edit = qobject_cast<QPlainTextEdit *>(widget))
		edit->undo();
	else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget))
		comboBox->lineEdit()->undo();
}

void MainWindow::redo()
{
	QWidget *widget = focusWidget();
	if (widget == script_editor_)
		script_editor_->redo();
	else if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget))
		edit->redo();
	else if (QPlainTextEdit *edit = qobject_cast<QPlainTextEdit *>(widget))
		edit->redo();
	else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget))
		comboBox->lineEdit()->redo();
}

void MainWindow::copy()
{
	QWidget *widget = focusWidget();
	if (widget == script_editor_)
		script_editor_->copy();
	else if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget))
		edit->copy();
	else if (QPlainTextEdit *edit = qobject_cast<QPlainTextEdit *>(widget))
		edit->copy();
	else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget))
		comboBox->lineEdit()->copy();
#ifdef ULTIMATE
	else if (widget == license_property_editor_ && license_property_manager_->indexToProperty(license_property_editor_->currentIndex()) == license_property_manager_->serialNumber())
		QApplication::clipboard()->setText(license_property_manager_->serialNumber()->value());
#endif
	else if (TreeView *tree = dynamic_cast<TreeView *>(widget))
		tree->copy();
	else if (TableView *table = dynamic_cast<TableView *>(widget))
		table->copy();
}

void MainWindow::cut()
{
	QWidget *widget = focusWidget();
	if (widget == script_editor_)
		script_editor_->cut();
	else if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget))
		edit->cut();
	else if (QPlainTextEdit *edit = qobject_cast<QPlainTextEdit *>(widget))
		edit->cut();
	else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget))
		comboBox->lineEdit()->cut();
}

void MainWindow::paste()
{
	QWidget *widget = focusWidget();
	if (widget == script_editor_)
		script_editor_->paste();
	else if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget))
		edit->paste();
	else if (QPlainTextEdit *edit = qobject_cast<QPlainTextEdit *>(widget))
		edit->paste();
	else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget))
		comboBox->lineEdit()->paste();
}

void MainWindow::addFunction()
{
	FunctionDialog dialog(core_->input_file(), NULL, smAddFunction, this);

	if (dialog.exec() == QDialog::Accepted) {
		QList<AddressInfo> addresses = dialog.addresses();
		CompilationType compilationType = dialog.compilationType();
		uint32_t compilationOptions = dialog.compilationOptions();
		if (addresses.isEmpty())
			return;

		ProjectNode *node = currentProjectNode();
		assert(node);
		if (node == NULL) return;

		while (node->type() == NODE_FUNCTION) {
			node = node->parent();
		}
		Folder *folder = (node->type() == NODE_FOLDER) ? reinterpret_cast<Folder *>(node->data()) : NULL;

		WaitCursor wc;
		/*
		if (addresses.size() > 1)
			ProgressDialog::Start((language[lsLoading] + "...").c_str(), addresses.size());
			

		IFile *file = core_->input_file();*/
		IFunction *func = NULL;
		for (int i = 0; i < addresses.size(); i++) {
			//ProgressDialog::Step(1);
			AddressInfo info = addresses[i];
			IFunction *tmp = info.arch->function_list()->AddByAddress(info.address, compilationType, compilationOptions, true, folder);
			if (!func)
				func = tmp;
		}
		//ProgressDialog::End();

		if (func)
			project_tree_->setCurrentIndex(project_model_->objectToIndex(func));
	}
}

void MainWindow::addFolder()
{
	ProjectNode *node = currentProjectNode();
	assert(node);
	if (node == NULL) return;

	switch (node->type()) {
	case NODE_FUNCTIONS:
	case NODE_FOLDER:
	case NODE_FUNCTION:
		addFunctionFolder();
		break;
	case NODE_FILES:
	case NODE_FILE_FOLDER:
	case NODE_FILE:
	case NODE_ASSEMBLIES:
		addFileFolder();
		break;
	}
}

void MainWindow::addFunctionFolder()
{
	ProjectNode *node = currentProjectNode();
	assert(node);
	if (node == NULL) return;

	while (node->type() == NODE_FUNCTION) {
		node = node->parent();
	}

	Folder *parent = (node->type() == NODE_FOLDER) ? reinterpret_cast<Folder *>(node->data()) : core_->input_file()->folder_list();

	QModelIndex index = project_model_->objectToIndex(parent->Add(""));
	project_tree_->setCurrentIndex(index);
	project_tree_->edit(index);
}

#ifdef ULTIMATE
void MainWindow::updateLicensingActions()
{
	bool is_ok = !core_->licensing_manager()->empty();
	save_licenses_act_->setEnabled(is_ok);
	export_key_act_->setEnabled(is_ok);
	import_license_act_->setEnabled(is_ok);
	add_license_act_->setEnabled(is_ok);
	add_license_act2_->setEnabled(is_ok);
}

void MainWindow::createLicense(License *license)
{
	LicenseDialog dialog(core_->licensing_manager(), license, this);

	if (dialog.exec() == QDialog::Accepted) {
		license = dialog.license();
		project_tree_->setCurrentIndex(project_model_->objectToIndex(license));
	}
}
#endif

void MainWindow::addLicense()
{
#ifdef ULTIMATE
	createLicense(NULL);
#endif
}

void MainWindow::createKeyPair()
{
#ifdef ULTIMATE
	WaitCursor wc;
	core_->licensing_manager()->Init(key_len_->currentText().toInt());
#endif
}

void MainWindow::exportKeyPair()
{
#ifdef ULTIMATE
	ExportKeyPairDialog dialog(this, core_->licensing_manager());

	dialog.exec();
#endif
}

void MainWindow::useOtherProject()
{
#ifdef ULTIMATE
	QString fileName = FileDialog::getOpenFileName(this, QString::fromUtf8(language[lsOpen].c_str()), FileDialog::defaultPath(),
		QString(
#ifdef VMP_GNU
		"%1 (*.vmp);;%2 (*)"
#else
		"%1 (*.vmp);;%2 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsProjectFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));

	if (fileName.isEmpty())
		return;

	LicensingManager manager;
	if (!manager.Open(fileName.toUtf8().constData())) {
		MessageDialog::critical(this, QString::fromUtf8(string_format(language[lsOpenFileError].c_str(), fileName.toUtf8().constData()).c_str()), QMessageBox::Ok);
		return;
	}

	if (manager.algorithm() != alRSA) {
		MessageDialog::critical(this, QString("The license data not found in the file \"%1\"").arg(fileName), QMessageBox::Ok);
		return;
	}

	core_->set_license_data_file_name(fileName.toUtf8().constData());
#endif
}

void MainWindow::importLicense()
{
#ifdef ULTIMATE
	ImportLicenseDialog dialog(this);

	if (dialog.exec() == QDialog::Accepted) {
		std::string serialNumber = dialog.serial().toLatin1().constData();
		LicensingManager *licensingManager = core_->licensing_manager();
		License *license = licensingManager->GetLicenseBySerialNumber(serialNumber);
		if (!license) {
			LicenseInfo licenseInfo;
			if (!licensingManager->DecryptSerialNumber(serialNumber, licenseInfo)) {
				MessageDialog::critical(this, QString::fromUtf8(language[lsImportSerialNumberError].c_str()), QMessageBox::Ok);
				return;
			}
			QDate date = QDate::currentDate();
			license = licensingManager->Add(LicenseDate(date.year(), date.month(), date.day()), 
											licenseInfo.CustomerName, 
											licenseInfo.CustomerEmail, 
											"", 
											"",
											serialNumber,
											false);
		}
		project_tree_->setCurrentIndex(project_model_->objectToIndex(license));
	}
#endif
}

void MainWindow::importProject()
{
#ifdef ULTIMATE
	QString fileName = FileDialog::getOpenFileName(this, QString::fromUtf8(language[lsOpen].c_str()), FileDialog::defaultPath(),
		QString(
#ifdef VMP_GNU
		"%1 (*.vmp);;%2 (*)"
#else
		"%1 (*.vmp);;%2 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsProjectFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));
	if (fileName.isEmpty())
		return;

	LicensingManager manager;
	if (!manager.Open(fileName.toUtf8().constData())) {
		MessageDialog::critical(this, QString::fromUtf8(string_format(language[lsOpenFileError].c_str(), fileName.toUtf8().constData()).c_str()), QMessageBox::Ok);
		return;
	}

	if (manager.algorithm() != alRSA) {
		MessageDialog::critical(this, QString("The licensing parameters are not found in the file \"%1\"").arg(fileName), QMessageBox::Ok);
		return;
	}

	if (!core_->licensing_manager()->CompareParameters(manager)) {
		MessageDialog::critical(this, QString::fromUtf8(language[lsImportLicensesError].c_str()), QMessageBox::Ok);
		return;
	}

	size_t c = 0;
	size_t b = 0;
	for (size_t i = 0; i < manager.count(); i++) {
		License *src_license = manager.item(i);
		License *dst_license = core_->licensing_manager()->GetLicenseBySerialNumber(src_license->serial_number());
		if (dst_license) {
			if (!dst_license->blocked() && src_license->blocked()) {
				dst_license->set_blocked(src_license->blocked());
				b++;
			}
		} else {
			/*dst_license =*/ core_->licensing_manager()->Add(src_license->date(), 
															src_license->customer_name(), 
															src_license->customer_email(), 
															src_license->order_ref(), 
															src_license->comments(), 
															src_license->serial_number(),
															src_license->blocked());
			c++;
		}
	}
	MessageDialog::information(this, QString::fromUtf8(string_format(language[lsImportLicensesResult].c_str(), c, b).c_str()), QMessageBox::Ok);
#endif
}

void MainWindow::addFileFolder()
{
#ifdef ULTIMATE
	ProjectNode *node = currentProjectNode();
	assert(node);
	if (node == NULL) return;

	while (node->type() == NODE_FILE) {
		node = node->parent();
	}

	FileFolder *parent = (node->type() == NODE_FILE_FOLDER) ? reinterpret_cast<FileFolder *>(node->data()) : core_->file_manager()->folder_list();

	QModelIndex index = project_model_->objectToIndex(parent->Add(""));
	project_tree_->setCurrentIndex(index);
	project_tree_->edit(index);
#endif
}

void MainWindow::addFile()
{
#ifdef ULTIMATE
	QString filter;
	if (project_model_->objectToNode(core_->file_manager()->folder_list())->type() == NODE_ASSEMBLIES) {
		filter = QString(
			"%1 (*.dll);;%2 (*.*)"
		).arg(QString::fromUtf8(language[lsAssemblies].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str()));
	}
	else {
		filter = QString(
#ifdef VMP_GNU
			"%1 (*)"
#else
			"%1 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsAllFiles].c_str()));
	}
	
	QString fileName = FileDialog::getOpenFileName(this, QString::fromUtf8(language[lsOpen].c_str()), FileDialog::defaultPath(), filter);
	if (fileName.isEmpty())
		return;

	QString tmp = QDir(QString::fromUtf8(core_->project_path().c_str())).relativeFilePath(fileName);
	if (tmp.mid(0, 2) != "..")
		fileName = tmp;

	ProjectNode *node = currentProjectNode();
	while (node->type() == NODE_FILE) {
		node = node->parent();
	}
	FileFolder *folder = (node->type() == NODE_FILE_FOLDER) ? reinterpret_cast<FileFolder *>(node->data()) : NULL;

	FileManager *file_manager = core_->file_manager();
	InternalFile *file = file_manager->Add(QFileInfo(fileName).fileName().toUtf8().constData(), fileName.toUtf8().constData(), faNone, folder);
	if (file)
		project_tree_->setCurrentIndex(project_model_->objectToIndex(file));
#endif
}

void MainWindow::rename()
{
	QTreeView *tree_view = qobject_cast<QTreeView *>(focusWidget());
	if (tree_view)
		tree_view->edit(tree_view->currentIndex());
}

void MainWindow::excludeFromCompilation()
{
	ProjectNode *node = currentProjectNode(true);
	if (!node)
		return;

	switch (node->type()) {
	case NODE_FUNCTION:
		{
			FunctionBundle *object = reinterpret_cast<FunctionBundle *>(node->data());
			object->set_need_compile(!object->need_compile());
		}
		break;
	case NODE_SCRIPT:
	case NODE_SCRIPT_BOOKMARK:
		{
			Script *object = reinterpret_cast<Script *>(node->data());
			object->set_need_compile(!object->need_compile());
		}
		break;
#ifdef ULTIMATE
	case NODE_FILES:
	case NODE_ASSEMBLIES:
		{
			FileManager *object = reinterpret_cast<FileManager *>(node->data());
			object->set_need_compile(!object->need_compile());
		}
		break;
#endif
	case NODE_MAP_FUNCTION:
		{
			MapFunctionBundle *object = static_cast<MapFunctionBundle *>(node->data());
			FunctionBundle *func = core_->input_file()->function_list()->GetFunctionByName(object->name());
			if (func) {
				func->set_need_compile(!func->need_compile());
			} else {
				for (size_t i = 0; i < object->count(); i++) {
					MapFunctionArch *map_arch = object->item(i);
					map_arch->arch()->function_list()->AddByAddress(map_arch->func()->address(), ctVirtualization, 0, true, NULL);
				}
			}
		}
		break;
	}
}

void MainWindow::block()
{
#ifdef ULTIMATE
	ProjectNode *node = currentProjectNode(true);
	if (!node)
		return;

	switch (node->type()) {
	case NODE_LICENSE:
		{
			License *license = reinterpret_cast<License *>(node->data());
			license->set_blocked(!license->blocked());
		}
		break;
	}
#endif
}

void MainWindow::del()
{
	ProjectNode *node = currentProjectNode(true);
	if (!node)
		return;

	switch (node->type()) {
	case NODE_SCRIPT:
	case NODE_SCRIPT_BOOKMARK:
		script_editor_->del();
		break;
	case NODE_FOLDER:
		{
			Folder *folder = reinterpret_cast<Folder *>(node->data());
			if (MessageDialog::question(this, QString::fromUtf8(language[lsDeleteFolder].c_str()) + "?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
				delete folder;
		}
		break;
	case NODE_FUNCTION:
		{
			FunctionBundle *func = reinterpret_cast<FunctionBundle *>(node->data());
			if (MessageDialog::question(this, QString::fromUtf8(language[lsDeleteFunction].c_str()) + "?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
				for (size_t i = func->count(); i > 0; i--) {
					delete func->item(i - 1)->func();
				}
			}
		}
		break;
#ifdef ULTIMATE
	case NODE_LICENSE:
		{
			License *license = reinterpret_cast<License *>(node->data());
			if (MessageDialog::question(this, QString::fromUtf8(language[lsDeleteLicense].c_str()) + "?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
				delete license;
		}
		break;
	case NODE_FILE_FOLDER:
		{
			FileFolder *folder = reinterpret_cast<FileFolder *>(node->data());
			if (MessageDialog::question(this, QString::fromUtf8(language[lsDeleteFolder].c_str()) + "?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
				delete folder;
		}
		break;
	case NODE_FILE:
		{
			InternalFile *file = reinterpret_cast<InternalFile *>(node->data());
			if (MessageDialog::question(this, QString::fromUtf8(language[lsDeleteFile].c_str()) + "?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
				delete file;
		}
		break;
#endif
	default:
		{
			QWidget *widget = focusWidget();
			if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget))
				edit->del();
			else if (QPlainTextEdit *edit = qobject_cast<QPlainTextEdit *>(widget))
				edit->textCursor().removeSelectedText();
			else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget))
				comboBox->lineEdit()->del();
		}
	}
}

bool MainWindow::internalCompile()
{
	bool res = false;
	try {
		res = core_->Compile();
		if (res && QFileInfo(QString::fromUtf8(core_->input_file()->file_name().c_str())) == QFileInfo(QString::fromUtf8(core_->absolute_output_file_name().c_str())))
			fileChanged_ = false;
	} catch(canceled_error &error) {
		core_->Notify(mtWarning, NULL, error.what());
	} catch(std::runtime_error &error) {
		core_->Notify(mtError, NULL, error.what());
	}
	return res;
}

void MainWindow::compile()
{
	log_model_->clear();

	{
		QFutureWatcher<bool> watcher;
		ProgressDialog progress(this);
		log_->reset();

		connect(&watcher, SIGNAL(finished()), &progress, SLOT(reject()));
		connect(&progress, SIGNAL(cancel()), log_, SLOT(cancel()));
		connect(log_, SIGNAL(startProgress(const QString &, unsigned long long)), &progress, SLOT(startProgress(const QString &, unsigned long long)));
		connect(log_, SIGNAL(stepProgress(unsigned long long)), &progress, SLOT(stepProgress(unsigned long long)));

		disconnect(log_, SIGNAL(notify(MessageType, IObject *, const QString &)), this, SLOT(notify(MessageType, IObject *, const QString &)));
		connect(log_, SIGNAL(notify(MessageType, IObject *, const QString &)), this, SLOT(notify(MessageType, IObject *, const QString &)), Qt::BlockingQueuedConnection);

		watcher.setFuture(QtConcurrent::run(this, &MainWindow::internalCompile));
		progress.exec();
		watcher.waitForFinished();

		disconnect(log_, SIGNAL(notify(MessageType, IObject *, const QString &)), this, SLOT(notify(MessageType, IObject *, const QString &)));
		connect(log_, SIGNAL(notify(MessageType, IObject *, const QString &)), this, SLOT(notify(MessageType, IObject *, const QString &)));

		if (!watcher.result())
			return;
	}

	if (save_act_->isEnabled() && settings_file().auto_save_project())
		save();

	if (execute_act_->isEnabled()) {
		execute_protected_act_->setFullText(QString::fromUtf8(core_->absolute_output_file_name().c_str()));
		execute_protected_act_->setVisible(true);
		execute_menu_->setDefaultAction(execute_protected_act_);

		if (MessageDialog::question(this, QString("%1.\n%2 \"%3\"?").arg(QString::fromUtf8(language[lsCompiled].c_str())).arg(QString::fromUtf8(language[lsExecute].c_str())).arg(execute_protected_act_->getFullText()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			executeProtected();
	} else {
		MessageDialog::information(this, QString::fromUtf8(language[lsCompiled].c_str()), QMessageBox::Ok);
	}
}

void MainWindow::execute()
{
	if (execute_protected_act_->isVisible())
		executeProtected();
	else
		executeOriginal();
}

void MainWindow::executeOriginal()
{
	executeFile(execute_original_act_->getFullText());
}

void MainWindow::executeProtected()
{
	executeFile(execute_protected_act_->getFullText());
}

void MainWindow::executeFile(const QString &fileName)
{
	QString commandLine = parameters_edit_->text().trimmed();
#ifdef VMP_GNU
	QStringList arguments;
	if (!commandLine.isEmpty())
		arguments << commandLine;
	QProcess::startDetached(fileName, arguments, QFileInfo(fileName).absolutePath());
#else
	QString execCommand;
	if (core_->input_file())
		execCommand = QString::fromUtf8(core_->input_file()->exec_command().c_str());
	if (execCommand.isEmpty())
		ShellExecuteW(0, L"open", (LPCWSTR)fileName.utf16(), (LPCWSTR)commandLine.utf16(), (LPCWSTR)QFileInfo(fileName).absolutePath().utf16(), SW_SHOWNORMAL);
	else
		ShellExecuteW(0, L"open", (LPCWSTR)execCommand.utf16(), (LPCWSTR)(fileName + " " + commandLine).utf16(), (LPCWSTR)QFileInfo(fileName).absolutePath().utf16(), SW_SHOWNORMAL);
#endif
}

#ifndef LITE
static int luaErrGetLineNumber(const QString &text)
{
	int line = -1;
	bool afterFirstSemicolon = false;
	for(int pos = 0; pos < text.length(); pos++)
	{
		QChar ch = text.at(pos);
		if(afterFirstSemicolon)
		{
			if(ch.isDigit())
			{
				if(line < 0)
					line = 0;
				line = 10 * line + ch.digitValue();
			} else
			{
				break;
			}
		} else if(ch.toLatin1() == ':') 
		{
			afterFirstSemicolon = true;
		}
	}
	return line;
}
#endif

void MainWindow::logItemDoubleClicked(const QModelIndex &index)
{
	ProjectNode *current = static_cast<ProjectNode *>(index.internalPointer());
	if (!current || !current->data())
		return;

	IObject *sender = static_cast<IObject *>(current->data());
	if (sender == core_->script()) {
		icon_project_->setChecked(true);
#ifndef LITE
		project_tree_->setCurrentIndex(project_model_->scriptIndex());
		int line = luaErrGetLineNumber(current->text());
		if(line >= 1) 
			script_editor_->send(SCI_GOTOLINE, line - 1);
#endif
	} else if (sender == core_->watermark_manager()) {
		icon_project_->setChecked(true);
		project_tree_->setCurrentIndex(project_model_->optionsIndex());
#ifndef LITE
		core_property_editor_->setFocus();
		core_property_editor_->setCurrentIndex(core_property_manager_->watermarkNameIndex());
#endif
	} else if (dynamic_cast<IFunction *>(sender)) {
		icon_project_->setChecked(true);
		project_tree_->setCurrentIndex(project_model_->objectToIndex(sender));
	} 
#ifdef ULTIMATE
	else if (sender == core_->licensing_manager()) {
		icon_project_->setChecked(true);
		project_tree_->setCurrentIndex(project_model_->licensesIndex());
	} else if (dynamic_cast<InternalFile *>(sender)) {
		icon_project_->setChecked(true);
		project_tree_->setCurrentIndex(project_model_->objectToIndex(sender));
		internal_file_property_editor_->setCurrentIndex(internal_file_property_manager_->fileNameIndex());
	}
#endif
	else
		return;

	auto cw = project_page_->currentWidget();
	if (cw)
		cw->setFocus();
}

void MainWindow::functionItemDoubleClicked(const QModelIndex &index)
{
	Property *prop = function_property_manager_->indexToProperty(index);
	if (!prop)
		return;

	if (CommandProperty *commandProp = qobject_cast<CommandProperty *>(prop)) {
		ICommand *command = commandProp->value();
		if (qobject_cast<CommandProperty *>(commandProp->parent())) {
			// link
			function_property_editor_->setCurrentIndex(function_property_manager_->commandToIndex(command));
			return;
		}

		uint64_t branchAddress = command->link() ? command->link()->to_address() : 0;
		if (branchAddress) {
			IArchitecture *file = command->owner()->owner()->owner();
			if (core_->input_file()->IndexOf(file) != NOT_ID) {
				command = command->owner()->GetCommandByAddress(branchAddress);
				if (command) {
					function_property_editor_->setCurrentIndex(function_property_manager_->commandToIndex(command));
					return;
				}
				goToAddress(file, branchAddress);
			}
		}
	}
}

void MainWindow::disasmItemDoubleClicked(const QModelIndex &index)
{
#ifndef LITE
	ICommand *command = disasm_model_->indexToCommand(index);
	if (!command)
		return;

	uint64_t branchAddress = command->link() ? command->link()->to_address() : 0;
	if (branchAddress)
		goToAddress(disasm_model_->file(), branchAddress);
#endif
}

void MainWindow::goToDump(IArchitecture *file, uint64_t address, bool mode)
{
#ifndef LITE
	info_tree_->setCurrentIndex(info_model_->dumpIndex(file));
	if (mode) {
		QModelIndex index = dump_model_->addressToIndex(address);
		if (index.isValid())
			dump_view_->setCurrentIndex(index);
	} else {
		QModelIndex index = disasm_model_->addressToIndex(address);
		if (index.isValid()) {
			disasm_view_->scrollTo(index, QAbstractItemView::PositionAtTop);
			// sometimes PositionAtTop works like PositionAtCenter
			disasm_view_->scrollTo(index, QAbstractItemView::PositionAtTop);
			disasm_view_->setCurrentIndex(index);
		}
	}
#endif
}

void MainWindow::goToAddress(IArchitecture *file, uint64_t address)
{
#ifdef LITE
	MapFunctionBundle *mapFunction = file->owner()->map_function_list()->GetFunctionByAddress(file, address);
	if (mapFunction) {
		project_tree_->setCurrentIndex(project_model_->objectToIndex(mapFunction));
		if (function_property_manager_->value()) {
			ICommand *command = function_property_manager_->value()->GetCommandByAddress(file, address);
			if (command) {
				function_property_editor_->setCurrentIndex(function_property_manager_->commandToIndex(command));
				return;
			}
		}
	}
#else
	if (icon_details_->isChecked()) {
		goToDump(file, address, dump_view_->hasFocus());
		return;
	}

	ICommand *command = file->function_list()->GetCommandByAddress(address, false);
	IFunction *project_function = command ? command->owner() : NULL;
	MapFunctionBundle *mapFunction = file->owner()->map_function_list()->GetFunctionByAddress(file, address);

	if (project_function && (icon_project_->isChecked() || !mapFunction)) {
		icon_project_->setChecked(true);
		project_tree_->setCurrentIndex(project_model_->objectToIndex(project_function));
		function_property_editor_->setCurrentIndex(function_property_manager_->commandToIndex(command));
		return;
	} else if (mapFunction) {
		icon_functions_->setChecked(true);
		functions_tree_->setCurrentIndex(functions_model_->objectToIndex(mapFunction));
		if (function_property_manager_->value()) {
			command = function_property_manager_->value()->GetCommandByAddress(file, address);
			if (command) {
				function_property_editor_->setFocus();
				function_property_editor_->setCurrentIndex(function_property_manager_->commandToIndex(command));
				return;
			}
		}
	}

	icon_details_->setChecked(true);
	goToDump(file, address);
#endif
}

void MainWindow::goTo()
{
#ifndef LITE
	FunctionDialog dialog(core_->input_file(), disasm_model_->file(), smGotoAddress, this);

	if (dialog.exec() == QDialog::Accepted) {
		QList<AddressInfo> addresses = dialog.addresses();
		if (addresses.isEmpty())
			return;

		AddressInfo info = addresses[0];
		goToAddress(info.arch, info.address);
	}
#endif
}

bool MainWindow::isContextSearchApplicable()
{
	return project_page_->currentWidget() == script_editor_ || 
#ifndef LITE
		project_page_->currentWidget() == dump_page_ || 
#endif
		dynamic_cast<QTreeView *>(project_page_->currentWidget());
}

void MainWindow::search()
{
	if (isContextSearchApplicable()) {
		if (!context_find_->isVisible()) {
			context_find_->showAndClear(project_filter_->text());
		} else {
			context_find_->show();
		}
		return;
	}
	project_filter_->selectAll();
	project_filter_->setFocus();
}

void MainWindow::showProtected()
{
	showCurrentObject();
}

void MainWindow::contextFindNext()
{
	contextFind(context_find_->text(), true, false);
}

void MainWindow::contextFindPrevious()
{
	contextFind(context_find_->text(), false, false);
}

void MainWindow::contextFind(const QString &ttf, bool forward, bool incremental)
{
	bool found = true;
	if(!ttf.isEmpty())
	{
		if (project_page_->currentWidget() == script_editor_)
		{
			found = findInScript(ttf, forward, incremental);
		} else if (QAbstractItemView *tv =
#ifndef LITE
				(project_page_->currentWidget() == dump_page_) ? (disasm_view_->geometry().height() ? (QAbstractItemView *)disasm_view_ : (QAbstractItemView *)dump_view_) :
#endif
				dynamic_cast<QAbstractItemView *>(project_page_->currentWidget()))
		{
			found = findInView(tv, ttf, forward, incremental);
		}
	}
	if (!context_find_->isVisible())
		context_find_->show();
	context_find_->setPalette(found);
}

bool MainWindow::findInScript(const QString &ttf, bool forward, bool incremental)
{
	bool found = false;

	uptr_t pos = script_editor_->send(SCI_GETCURRENTPOS), 
	anc = script_editor_->send(SCI_GETANCHOR);
	if(!incremental)
	{
		if (forward) // set caret to the end of selection
		{
			script_editor_->send(SCI_GOTOPOS, std::max(anc, pos));
		} else // set caret to the beginning of selection
		{
			script_editor_->send(SCI_GOTOPOS, std::min(anc, pos));
		}
	}

	unsigned int iMessage = SCI_SEARCHPREV;
	if (forward) 
		iMessage = SCI_SEARCHNEXT;
	uptr_t wParam = 0;
	if (context_find_->caseSensitive())
		wParam = SCFIND_MATCHCASE;

	while(!found) 
	{
		script_editor_->send(SCI_SEARCHANCHOR);
		found = (script_editor_->send(iMessage, wParam, (sptr_t)ttf.toUtf8().data()) != -1);

		if (found)
		{
			script_editor_->send(SCI_SCROLLCARET);
		} else
		{
			if(incremental) // try from the beginning
			{
				script_editor_->send(SCI_GOTOPOS);
				incremental = false;
			} else
			{
				script_editor_->send(SCI_SETSEL, anc, pos);
				break;
			}
		}
	}
	return found;
}

bool MainWindow::findInView(QAbstractItemView *tv, const QString &ttf, bool forward, bool incremental)
{
	WaitCursor wc;

	QRegExp term(ttf, context_find_->caseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
	ItemModelSearcher searcher(
		tv->model(),
		tv->currentIndex(),
		&term,
		forward,
		incremental);

	QModelIndex ci = searcher.result();
	bool found = ci.isValid();
	if(found && ci != tv->currentIndex()) {
		tv->selectionModel()->setCurrentIndex(ci, QItemSelectionModel::ClearAndSelect);
	}
	return found;
}

void MainWindow::contextFindClosed()
{
	QWidget *page = project_page_->currentWidget();
	if (page) 
		page->setFocus();
}

void MainWindow::updateTemplates()
{
	QList<QAction*> actions = templates_menu_->actions();
	ProjectTemplateManager *template_manager = core_->template_manager();
	int separator_pos = actions.size() - 3;
	size_t c = 0;
	for (int i = 0; i < separator_pos; i++) {
		QAction *action = actions.at(i);
		if (i < (int)template_manager->count()) {
			ProjectTemplate *pt = template_manager->item(i);
			action->setText(QString::fromUtf8(pt->display_name().c_str()));
			c++;
		} else {
			templates_menu_->removeAction(action);
		}
	}
	for (size_t i = c; i < template_manager->count(); i++) {
		ProjectTemplate *pt = template_manager->item(i);
		QAction *action = new QAction(QString::fromUtf8(pt->display_name().c_str()), this);
		action->setCheckable(true);
		connect(action, SIGNAL(triggered()), this, SLOT(templateSelect()));
		templates_menu_->insertAction(actions.at(separator_pos), action);
	}
}

void MainWindow::templatesShow()
{
	QList<QAction*> actions = templates_menu_->actions();
	ProjectTemplate current(NULL, "");
	ProjectTemplateManager *template_manager = core_->template_manager();
	current.ReadFromCore(*core_);
	bool isChecked = false;
	for (int i = 0; i < (int)template_manager->count(); i++) {
		ProjectTemplate *pt = template_manager->item(i);
		if (!isChecked && *pt == current) {
			isChecked = true;
			actions.at(i)->setChecked(true);
		} else {
			actions.at(i)->setChecked(false);
		}
	}
}

void MainWindow::templatesSave()
{
	TemplateSaveDialog dialog(core_->template_manager(), this);
	if (dialog.exec() == QDialog::Accepted)
		core_->template_manager()->Add(dialog.name().toUtf8().constData(), *core_);
}

void MainWindow::templatesEdit()
{
	TemplatesWindow dialog(this);
	dialog.exec();
}

void MainWindow::templateSelect()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		core_property_editor_->closePersistentEditor(core_property_editor_->currentIndex());
		core_->LoadFromTemplate(*core_->template_manager()->item(templates_menu_->actions().indexOf(action)));
	}
}

void MainWindow::updateEditActions()
{
	if (project_model_->isEmpty())
		return;

	if (QApplication::activeWindow() != this)
		return;

	QWidget *widget = focusWidget();
	bool canCopy = false;
#ifdef ULTIMATE
	if (widget == license_property_editor_ && license_property_manager_->indexToProperty(license_property_editor_->currentIndex()) == license_property_manager_->serialNumber())
		canCopy = true;
#endif
	bool canCut = false;
	bool canPaste = false;
	bool canRedo = false;
	bool canUndo = false;
	ProjectNode *current = currentProjectNode();
	assert(current);
	if (!current)
		return;
#ifdef ULTIMATE
	add_license_act2_->setVisible(current->type() == NODE_LICENSE || current->type() == NODE_LICENSES);
	add_file_act2_->setVisible(current->type() == NODE_FILE || current->type() == NODE_FILE_FOLDER || current->type() == NODE_FILES || current->type() == NODE_ASSEMBLIES);
	add_function_act2_->setVisible(!add_license_act2_->isEnabled() && !add_file_act2_->isEnabled());
#endif
#ifndef LITE
	add_function_act2_->setVisible(current->type() == NODE_FUNCTIONS || current->type() == NODE_FOLDER || current->type() == NODE_FUNCTION);
#endif

	bool canDel = current->type() == NODE_SCRIPT || current->type() == NODE_SCRIPT_BOOKMARK || current->type() == NODE_FUNCTION || current->type() == NODE_FOLDER || current->type() == NODE_LICENSE || current->type() == NODE_FILE_FOLDER || current->type() == NODE_FILE;
	if (widget == script_editor_) {
		canUndo = script_editor_->canUndo();
		canRedo = script_editor_->canRedo();
		canCopy = script_editor_->hasSelection();
		canCut = script_editor_->hasSelection();
		canDel = script_editor_->hasSelection();
		canPaste = script_editor_->canPaste();
	} else if (QAbstractItemView *view = qobject_cast<QAbstractItemView *>(widget)) {
		canCopy = view->selectionModel()->selectedIndexes().size() > 0;
		canDel = canCopy && currentTreeView() == project_tree_;
#ifdef ULTIMATE
	} else if (widget == license_property_editor_ && license_property_manager_->indexToProperty(license_property_editor_->currentIndex()) == license_property_manager_->serialNumber()) {
		canCopy = true;
#endif
	} else if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget)) {
		canCopy = edit->hasSelectedText();
		if (!edit->isReadOnly()) {
			canPaste = !QApplication::clipboard()->text().isEmpty();
			canCut = edit->hasSelectedText();
			canDel = edit->hasSelectedText();
			canRedo = edit->isRedoAvailable();
			canUndo = edit->isUndoAvailable();
		}
	} else if (QPlainTextEdit *pt_edit = qobject_cast<QPlainTextEdit *>(widget)) {
		canCopy = pt_edit->textCursor().hasSelection();
		if (!pt_edit->isReadOnly()) {
			canPaste = !QApplication::clipboard()->text().isEmpty();
			canCut = pt_edit->textCursor().hasSelection();
			canDel = pt_edit->textCursor().hasSelection();
			canRedo = pt_edit->document()->isRedoAvailable();
			canUndo = pt_edit->document()->isUndoAvailable();
		}
	} else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget)) {
		if (comboBox->isEditable()) {
			QLineEdit *edit = comboBox->lineEdit();
			canCopy = edit->hasSelectedText();
			if (!edit->isReadOnly()) {
				canPaste = !QApplication::clipboard()->text().isEmpty();
				canCut = edit->hasSelectedText();
				canDel = edit->hasSelectedText();
				canRedo = edit->isRedoAvailable();
				canUndo = edit->isUndoAvailable();
			}
		}
	}
	copy_act_->setEnabled(canCopy);
	cut_act_->setEnabled(canCut);
	paste_act_->setEnabled(canPaste);
	redo_act_->setEnabled(canRedo);
	undo_act_->setEnabled(canUndo);
	delete_act_->setEnabled(canDel);

	if (fileChanged_) {
		fileChanged_ = false;
		if (MessageDialog::question(this, QString::fromUtf8(string_format(language[lsWatchedFileChange].c_str(), core_->input_file()->file_name().c_str()).c_str()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
			recent_file_ = 0;
			openRecentFile();
		}
	}
}

void MainWindow::treeSectionClicked(int index)
{
	ProjectNode *current = currentProjectNode();
	if (!current)
		return;

	project_model_->sortNode(current, index);
	directory_model_->setDirectory(current);
}

void MainWindow::fileChanged(const QString & path)
{
	fileChanged_ = true;
}