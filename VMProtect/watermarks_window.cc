#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/core.h"
#include "../core/streams.h"
#include "../core/files.h"
#include "../core/pefile.h"
#include "../core/macfile.h"
#include "../core/elffile.h"
#include "../core/lang.h"
#include "models.h"
#include "widgets.h"
#include "progress_dialog.h"
#include "watermarks_window.h"
#include "moc/moc_watermarks_window.cc"
#include "message_dialog.h"
#include "watermark_dialog.h"
#include "wait_cursor.h"
#include "help_browser.h"
#include "application.h"

WatermarksModel *WatermarksWindow::watermarks_model = NULL;

WatermarksWindow::WatermarksWindow(bool selectMode, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint), processEditLocked(false)
#ifndef VMP_GNU
	, debugPrivilegeEnabled(false)
#endif
{
	setWindowTitle(QString::fromUtf8(language[lsWatermarks].c_str()));

	searchModel = new SearchModel(this);
	scanModel = new WatermarkScanModel(this);

	tabBar = new TabWidget(this);
	tabBar->setIconSize(QSize(18, 18));

	addAct = new QAction(QString::fromUtf8(language[lsAddWatermark].c_str()) + "...", this);
	addAct->setShortcut(QString("Ins"));
	connect(addAct, SIGNAL(triggered()), this, SLOT(addClicked()));
	renameAct = new QAction(QString::fromUtf8(language[lsRename].c_str()), this);
	renameAct->setShortcut(QString("F2"));
	renameAct->setEnabled(false);
	connect(renameAct, SIGNAL(triggered()), this, SLOT(renameClicked()));
	delAct = new QAction(QString::fromUtf8(language[lsDelete].c_str()), this);
	delAct->setShortcut(QString("Del"));
	delAct->setEnabled(false);
	connect(delAct, SIGNAL(triggered()), this, SLOT(delClicked()));
	blockAct = new QAction(QString::fromUtf8(language[lsBlocked].c_str()), this);
	blockAct->setEnabled(false);
	blockAct->setCheckable(true);
	connect(blockAct, SIGNAL(triggered()), this, SLOT(blockClicked()));

	contextMenu = new QMenu(this);
	contextMenu->addAction(addAct);
	contextMenu->addSeparator();
	contextMenu->addAction(delAct);
	contextMenu->addAction(renameAct);
	contextMenu->addSeparator();
	contextMenu->addAction(blockAct);

	pagePanel = new QStackedWidget(this);

	addButton = new QToolButton(this);
	addButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addButton->setAutoRaise(true);
	addButton->setText(addAct->text());
	addButton->setIconSize(QSize(20, 20));
	addButton->setIcon(QIcon(":/images/add_gray.png"));
	connect(addButton, SIGNAL(clicked(bool)), this, SLOT(addClicked()));

	watermarkFilter = new SearchLineEdit(this);
	watermarkFilter->setPlaceholderText(QString::fromUtf8(language[lsSearch].c_str()));
    connect(watermarkFilter, SIGNAL(textChanged(const QString &)), this, SLOT(watermarkSearchChanged()));
	connect(watermarkFilter, SIGNAL(returnPressed()), this, SLOT(watermarkSearchChanged()));

	watermarkTree = new TreeView(this);
	watermarkTree->setObjectName("grid");
	watermarkTree->setRootIsDecorated(false);
	watermarkTree->setUniformRowHeights(true);
	watermarkTree->setIconSize(QSize(18, 18));
	watermarkTree->setContextMenuPolicy(Qt::CustomContextMenu);
	watermarkTree->setItemDelegate(new WatermarksTreeDelegate(this));
	watermarkTree->setModel(watermarks_model);
	watermarkTree->addAction(addAct);
	watermarkTree->addAction(delAct);
    connect(watermarkTree->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(watermarkIndexChanged()));
	connect(watermarkTree, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemDoubleClicked(QModelIndex)));
	connect(watermarkTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenuRequested(const QPoint &)));
	pagePanel->addWidget(watermarkTree);

	searchTree = new TreeView(this);
	searchTree->setObjectName("grid");
	searchTree->setIconSize(QSize(18, 18));
	searchTree->setRootIsDecorated(false);
	searchTree->setUniformRowHeights(true);
	//searchTree->setFrameShape(QFrame::NoFrame);
	searchTree->setModel(searchModel);
	connect(searchTree->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(watermarkIndexChanged()));
	connect(searchTree, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemDoubleClicked(QModelIndex)));
	pagePanel->addWidget(searchTree);

	QTreeView *scanTree = new TreeView(this);
	scanTree->setObjectName("grid");
	scanTree->setRootIsDecorated(false);
	scanTree->setIconSize(QSize(18, 18));
	scanTree->setModel(scanModel);
	scanTree->header()->resizeSection(0, 300 * Application::stylesheetScaleFactor());

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

    okButton = new PushButton(QString::fromUtf8(language[lsOK].c_str()), this);
	okButton->setVisible(selectMode);
	connect(okButton, SIGNAL(clicked()), this, SLOT(okButtonClicked()));

	QPushButton *cancelButton = new PushButton(QString::fromUtf8(language[selectMode ? lsCancel : lsClose].c_str()), this);
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	buttonLayout->setSpacing(10);
	buttonLayout->addWidget(helpButton);
	buttonLayout->addStretch();
#ifdef __APPLE__
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
#else
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
#endif

	QFrame *panel = new QFrame(this);
	QGridLayout *gridLayout = new QGridLayout();
	gridLayout->setContentsMargins(0, 10, 0, 0);
	gridLayout->setVerticalSpacing(10);
	gridLayout->addWidget(addButton, 0, 0);
	gridLayout->addItem(new QSpacerItem(30, 0, QSizePolicy::Expanding, QSizePolicy::Ignored), 0, 1);
	gridLayout->addWidget(watermarkFilter, 0, 2);
	gridLayout->addWidget(pagePanel, 1, 0, 1, 3);
	panel->setLayout(gridLayout);
	tabBar->addTab(panel, QIcon(":/images/tool.png"), QString::fromUtf8(language[lsSetup].c_str()));

	QFrame *filePanel = new QFrame(this);
	filePanel->setObjectName("gridEditor");
	filePanel->setFrameShape(QFrame::StyledPanel);

	fileButton = new QRadioButton(QString::fromUtf8(language[lsSearchInFile].c_str()), this);
	fileButton->setObjectName("editor");
	connect(fileButton, SIGNAL(clicked()), this, SLOT(fileButtonClicked()));

	QFrame *fileSpacer = new QFrame(this);
	fileSpacer->setObjectName("editor");
	fileSpacer->setFixedWidth(17);

	QFont font;
	font.setBold(true);

	fileNameLabel = new QLabel(QString::fromUtf8(language[lsFileName].c_str()), this);
	fileNameLabel->setObjectName("editor");

	fileNameEdit = new FileNameEdit(this);
	fileNameEdit->setObjectName("editor");
	fileNameEdit->setFrame(false);
	fileNameEdit->setFilter(QString(
#ifdef VMP_GNU
		"%1 (*)"
#else
		"%1 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsAllFiles].c_str())));

	processButton = new QRadioButton(QString::fromUtf8(language[lsSearchInModule].c_str()), this);
	processButton->setObjectName("editor");
	connect(processButton, SIGNAL(clicked()), this, SLOT(processButtonClicked()));

	QFrame *processSpacer = new QFrame(this);
	processSpacer->setObjectName("editor");
	processSpacer->setFixedWidth(17);

	processLabel = new QLabel(QString::fromUtf8(language[lsProcess].c_str()), this);
	processLabel->setObjectName("editor");

	processEdit = new EnumEdit(this, QStringList());
	processEdit->setFrame(false);
	processEdit->setObjectName("editor");
	processEdit->setFont(font);
	connect(processEdit, SIGNAL(dropDown()), this, SLOT(processEditDropDown()));
	connect(processEdit, SIGNAL(currentIndexChanged(int)), this, SLOT(processEditChanged()));

	QFrame *moduleSpacer = new QFrame(this);
	moduleSpacer->setObjectName("editor");
	moduleSpacer->setFixedWidth(17);

	moduleLabel = new QLabel(QString::fromUtf8(language[lsModule].c_str()), this);
	moduleLabel->setObjectName("editor");

	moduleEdit = new EnumEdit(this, QStringList());
	moduleEdit->setFrame(false);
	moduleEdit->setObjectName("editor");
	moduleEdit->setFont(font);

	gridLayout = new QGridLayout();
	gridLayout->setContentsMargins(0, 0, 0, 0);
	gridLayout->setHorizontalSpacing(0);
	gridLayout->setVerticalSpacing(1);
	gridLayout->addWidget(fileButton, 0, 0, 1, 3);
	gridLayout->addWidget(fileSpacer, 1, 0);
	gridLayout->addWidget(fileNameLabel, 1, 1);
	gridLayout->addWidget(fileNameEdit, 1, 2);
	gridLayout->addWidget(processButton, 2, 0, 1, 3);
	gridLayout->addWidget(processSpacer, 3, 0);
	gridLayout->addWidget(processLabel, 3, 1);
	gridLayout->addWidget(processEdit, 3, 2);
	gridLayout->addWidget(moduleSpacer, 4, 0);
	gridLayout->addWidget(moduleLabel, 4, 1);
	gridLayout->addWidget(moduleEdit, 4, 2);
	gridLayout->setColumnMinimumWidth(1, 100 * Application::stylesheetScaleFactor());
	gridLayout->setColumnStretch(2, 1);
	filePanel->setLayout(gridLayout);

	QPushButton *scanButton = new PushButton(QString::fromUtf8(language[lsStart].c_str()), this);
	connect(scanButton, SIGNAL(clicked()), this, SLOT(scanButtonClicked()));

	panel = new QFrame(this);
	QBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(0, 10, 0, 0);
	layout->setSpacing(10);
	layout->addWidget(filePanel);
	layout->addWidget(scanButton, 0, Qt::AlignCenter);
	layout->addWidget(scanTree, 1);
	panel->setLayout(layout);
	tabBar->addTab(panel, QIcon(":/images/search_gray.png"), QString::fromUtf8(language[lsSearch].c_str()));

	layout = new QVBoxLayout();
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(10);
	layout->addWidget(tabBar);
	layout->addLayout(buttonLayout);
	setLayout(layout);

	connect(watermarks_model, SIGNAL(nodeUpdated(ProjectNode *)), this, SLOT(watermarkNodeUpdated(ProjectNode *)));
	connect(watermarks_model, SIGNAL(nodeRemoved(ProjectNode *)), this, SLOT(watermarkNodeRemoved(ProjectNode *)));

	fileButton->setChecked(true);
	fileButtonClicked();

	setMinimumSize(450 * Application::stylesheetScaleFactor(), 300 * Application::stylesheetScaleFactor());
}

WatermarksWindow::~WatermarksWindow()
{
	disconnect(watermarks_model, SIGNAL(nodeUpdated(ProjectNode *)), this, SLOT(watermarkNodeUpdated(ProjectNode *)));
	disconnect(watermarks_model, SIGNAL(nodeRemoved(ProjectNode *)), this, SLOT(watermarkNodeRemoved(ProjectNode *)));

	delete searchModel;
	delete scanModel;
}

void WatermarksWindow::watermarkNodeUpdated(ProjectNode *node)
{
	searchModel->updateNode(node);
}

void WatermarksWindow::watermarkNodeRemoved(ProjectNode *node)
{
	searchModel->removeNode(node);
	scanModel->removeWatermark(static_cast<Watermark *>(node->data()));
}

void WatermarksWindow::okButtonClicked()
{
	if (selectedWatermark())
		accept();
}

ProjectNode *WatermarksWindow::currentNode() const
{
	ProjectNode *res = NULL;
	if (pagePanel->currentWidget() == watermarkTree) {
		res = watermarks_model->indexToNode(watermarkTree->currentIndex());
	} else if (pagePanel->currentWidget() == searchTree) {
		res = searchModel->indexToNode(searchTree->currentIndex());
	}
	return res;
}

Watermark *WatermarksWindow::selectedWatermark() const
{
	ProjectNode *node = currentNode();
	return (node && node->type() == NODE_WATERMARK) ? reinterpret_cast<Watermark *>(node->data()) : NULL;
}

void WatermarksWindow::watermarkSearchChanged()
{
	Watermark *watermark = selectedWatermark();
	if (!watermarkFilter->text().isEmpty()) {
		searchModel->search(watermarks_model->root(), watermarkFilter->text());
		pagePanel->setCurrentWidget(searchTree);
		searchTree->setCurrentIndex(searchModel->nodeToIndex(watermarks_model->objectToNode(watermark)));
	} else {
		pagePanel->setCurrentWidget(watermarkTree);
		watermarkTree->setCurrentIndex(watermarks_model->objectToIndex(watermark));
	}

	watermarkIndexChanged();
}

QTreeView *WatermarksWindow::currentTreeView() const
{
	if (pagePanel->currentWidget() == watermarkTree)
		return watermarkTree;
	else
		return searchTree;
}

void WatermarksWindow::watermarkIndexChanged()
{
	Watermark *watermark = selectedWatermark();

	okButton->setEnabled(watermark != NULL);
	delAct->setEnabled(watermark != NULL);
	renameAct->setEnabled(watermark != NULL);
	blockAct->setEnabled(watermark != NULL);
	blockAct->setChecked(watermark != NULL && !watermark->enabled());
}

void WatermarksWindow::itemDoubleClicked(const QModelIndex & /*index*/)
{
	if (!okButton->isVisible())
		return;

	okButtonClicked();
}

QString WatermarksWindow::watermarkName() const
{
	Watermark *watermark = selectedWatermark();
	return watermark ? QString::fromUtf8(watermark->name().c_str()) : QString();
}

void WatermarksWindow::setWatermarkName(const QString &name)
{
	QModelIndex index = watermarks_model->indexByName(name);
	watermarkTree->setCurrentIndex(index);
	if (index.isValid())
		watermarkTree->scrollTo(index, QAbstractItemView::PositionAtTop);
}

std::map<Watermark*, size_t> WatermarksWindow::internalScanWatermarks(IFile *file)
{
	std::map<Watermark*, size_t> res;
	try {
		res = file->SearchWatermarks(*watermarks_model->core()->watermark_manager());
	} catch(canceled_error &error) {
		file->Notify(mtWarning, NULL, error.what());
	} catch(std::runtime_error &error) {
		file->Notify(mtError, NULL, error.what());
	}
	return res;
}

void WatermarksWindow::scanButtonClicked()
{
	if (fileButton->isChecked()) {
		if (fileNameEdit->text().isEmpty()) {
			MessageDialog::critical(this, QString::fromUtf8(language[lsFileNameNotSpecified].c_str()), QMessageBox::Ok);
			fileNameEdit->setFocus();
			return;
		}
	} else {
		if (processEdit->currentIndex() == -1) {
			MessageDialog::critical(this, QString::fromUtf8(language[lsProcessNotSpecified].c_str()), QMessageBox::Ok);
			processEdit->setFocus();
			return;
		}
		if (moduleEdit->currentIndex() == -1) {
			MessageDialog::critical(this, QString::fromUtf8(language[lsModuleNotSpecified].c_str()), QMessageBox::Ok);
			moduleEdit->setFocus();
			return;
		}
	}

	scanModel->clear();
	std::auto_ptr<IFile> file;
	GUILog log;

	if (fileButton->isChecked()) {
		std::string fileName = fileNameEdit->text().toUtf8().data();
		std::auto_ptr<IFile> files[] = { 
			std::auto_ptr<IFile>(new PEFile(qobject_cast<ILog*>(&log)))
			, std::auto_ptr<IFile>(new MacFile(qobject_cast<ILog*>(&log)))
			, std::auto_ptr<IFile>(new ELFFile(qobject_cast<ILog*>(&log)))
			, std::auto_ptr<IFile>(new IFile(qobject_cast<ILog*>(&log)))
		};
		for (size_t i = 0; i < _countof(files); i++) {

			OpenStatus status = files[i]->Open(fileName.c_str(), foRead | foHeaderOnly);
			if (status == osSuccess) {
				file = files[i];
				break;
			}

			if (status == osOpenError) {
				MessageDialog::critical(this, QString::fromUtf8(string_format(language[lsOpenFileError].c_str(), fileName.c_str()).c_str()), QMessageBox::Ok);
				break;
			} else if (status != osUnknownFormat) {
				LangString message_id;
				switch (status) {
				case osUnsupportedCPU:
					message_id = lsFileHasUnsupportedProcessor;
					break;
				case osUnsupportedSubsystem:
					message_id = lsFileHasUnsupportedSubsystem;
					break;
				default:
					message_id = lsFileHasIncorrectFormat;
					break;
				}
				if (MessageDialog::warning(this, QString::fromUtf8(string_format(language[message_id].c_str(), fileName.c_str(), "").append("\n").append(language[lsContinue]).append("?").c_str()), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
					break;
			}
		}
	} else {
		QVariant process = processEdit->itemData(processEdit->currentIndex());
		QVariant module = moduleEdit->itemData(moduleEdit->currentIndex());
		std::string moduleName = moduleEdit->currentText().toUtf8().data();
		file.reset(new IFile(qobject_cast<ILog*>(&log)));
		if (!file->OpenModule(process.toUInt(), reinterpret_cast<HMODULE>(module.toULongLong()))) {
			MessageDialog::critical(this, QString::fromUtf8(string_format(language[lsOpenModuleError].c_str(), moduleName.c_str()).c_str()), QMessageBox::Ok);
		}
	}

	if (file.get()) {
		QFutureWatcher<std::map<Watermark*, size_t>> watcher;
		ProgressDialog progress(this);

		connect(&watcher, SIGNAL(finished()), &progress, SLOT(reject()));
		connect(&progress, SIGNAL(cancel()), &log, SLOT(cancel()));
		connect(&log, SIGNAL(notify(MessageType, IObject *, const QString &)), &progress, SLOT(notify(MessageType, IObject *, const QString &)));
		connect(&log, SIGNAL(startProgress(const QString &, unsigned long long)), &progress, SLOT(startProgress(const QString &, unsigned long long)));
		connect(&log, SIGNAL(stepProgress(unsigned long long)), &progress, SLOT(stepProgress(unsigned long long)));

		watcher.setFuture(QtConcurrent::run(this, &WatermarksWindow::internalScanWatermarks, file.get()));
		progress.exec();
		watcher.waitForFinished();
		scanModel->setWatermarkData(watcher.result());
	}
}

void WatermarksWindow::addClicked()
{
	WatermarkDialog dialog(watermarks_model->manager(), this);

	if (dialog.exec() == QDialog::Accepted) {
		watermarkFilter->clear();
		Watermark *watermark = dialog.watermark();
		watermarkTree->setCurrentIndex(watermarks_model->objectToIndex(watermark));
	}
}

void WatermarksWindow::delClicked()
{
	Watermark *watermark = selectedWatermark();
	if (!watermark)
		return;

	int res;
	if (watermark->use_count()) {
		res = MessageDialog::warning(this, QString("%1.\n%2?").arg(QString::fromUtf8(string_format(language[lsWatermarkIsUsed].c_str(), watermark->name().c_str()).c_str())).arg(QString(QString::fromUtf8(language[lsDeleteWatermark].c_str()))), QMessageBox::Yes | QMessageBox::No);
	} else {
		res = MessageDialog::question(this, QString(QString::fromUtf8(language[lsDeleteWatermark].c_str()) + "?"), QMessageBox::Yes | QMessageBox::No);
	}
	if (res == QMessageBox::Yes)
		delete watermark;
}

void WatermarksWindow::renameClicked()
{
	QTreeView *tree_view = currentTreeView();
	tree_view->edit(tree_view->currentIndex());
}

void WatermarksWindow::blockClicked()
{
	Watermark *watermark = selectedWatermark();
	if (!watermark)
		return;

	watermark->set_enabled(!watermark->enabled());
}

void WatermarksWindow::fileButtonClicked()
{
	fileNameLabel->setEnabled(true);
	fileNameEdit->setEnabled(true);
	processLabel->setEnabled(false);
	processEdit->setEnabled(false);
	moduleLabel->setEnabled(false);
	moduleEdit->setEnabled(false);

	fileNameEdit->setFocus();
	fileNameEdit->selectAll();
}

void WatermarksWindow::processButtonClicked()
{
	fileNameLabel->setEnabled(false);
	fileNameEdit->setEnabled(false);
	processLabel->setEnabled(true);
	processEdit->setEnabled(true);
	moduleLabel->setEnabled(true);
	moduleEdit->setEnabled(true);

	processEdit->setFocus();
	processEdit->lineEdit()->selectAll();
}

void WatermarksWindow::processEditDropDown()
{
#ifndef VMP_GNU
	if (!debugPrivilegeEnabled) {
		HANDLE token;
		if (OpenProcessToken(INVALID_HANDLE_VALUE, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
			LUID value;
			if (LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &value)) {
				TOKEN_PRIVILEGES privileges;
				privileges.PrivilegeCount = 1;
				privileges.Privileges[0].Luid = value;
				privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				debugPrivilegeEnabled = !!(AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), NULL, NULL));
			}
			CloseHandle(token);
		}
	}
#endif

	processEditLocked = true;

	QString oldText = processEdit->currentText();
	processEdit->clear();
	processEdit->setCurrentIndex(-1);

	std::vector<PROCESS_ITEM> processes = os::EnumProcesses();
	for (size_t i = 0; i < processes.size(); i++) {
		processEdit->addItem(
			QString("[%1] ").arg(processes[i].id) + QString::fromUtf8(processes[i].name.c_str()), 
			QVariant(static_cast<qulonglong>(processes[i].id)));
	}

	processEdit->setCurrentIndex(processEdit->findText(oldText));
	if (processEdit->currentIndex() == -1)
		moduleEdit->clear();

	processEditLocked = false;
}

void WatermarksWindow::processEditChanged()
{
	if (processEditLocked)
		return;

	moduleEdit->clear();
	int i = processEdit->currentIndex();
	if (i == -1)
		return;

	QVariant data = processEdit->itemData(i);
	std::vector<MODULE_ITEM> modules = os::EnumModules(data.toInt());
	for (size_t i = 0; i < modules.size(); i++) {
		
		moduleEdit->addItem(
			QString::fromUtf8(modules[i].name.c_str())
#ifdef __unix__ // может быть много строк для одного файла по разным адресам (addr === HANDLE)
				+ QString(" @%1").arg((uint64_t)modules[i].handle, sizeof(void *) * 2, 16, QLatin1Char('0')).toUpper()
#endif
				,
			QVariant(reinterpret_cast<qulonglong>(modules[i].handle)));
	}
	
	if (moduleEdit->count())
		moduleEdit->setCurrentIndex(0);
}

void WatermarksWindow::contextMenuRequested(const QPoint &p)
{
	contextMenu->exec(watermarkTree->viewport()->mapToGlobal(p));
}

void WatermarksWindow::helpClicked()
{
	HelpBrowser::showTopic(tabBar->currentIndex() == 0 ? "watermarks::setup" : "watermarks::search");
}