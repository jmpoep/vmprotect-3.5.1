#include "../core/objects.h"
#include "../core/core.h"
#include "../core/lang.h"
#include "../core/files.h"
#include "../core/processors.h"

#include "widgets.h"
#include "models.h"
#include "property_editor.h"
#include "function_dialog.h"
#include "message_dialog.h"
#include "help_browser.h"
#include "moc/moc_function_dialog.cc"
#include "application.h"

/**
 * FunctionDialog
 */

FunctionDialog::FunctionDialog(IFile *file, IArchitecture *file_arch, ShowMode mode, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint), file_(file), mode_(mode), defaultArch_(NULL)
{
	setWindowTitle(QString::fromUtf8(language[mode_ == smAddFunction ? lsAddFunction : lsGoTo].c_str()));

	if (file_arch) { 
		functionListModel_.reset(new MapFunctionListModel(*file_arch, (mode_ == smAddFunction), this));
		defaultArch_ = file_arch;
	} else {
		functionBundleListModel_.reset(new MapFunctionBundleListModel(*file, (mode_ == smAddFunction), this));
		if (!file_->map_function_list()->show_arch_name()) {
			for (size_t i = 0; i < file_->count(); i++) {
				IArchitecture *arch = file_->item(i);
				if (!arch->visible())
					continue;
	
				defaultArch_ = arch;
				break;
			}
		}
	}

    QLabel *addressLabel = new QLabel(QString::fromUtf8(language[lsAddress].c_str()), this);
	addressEdit_ = new LineEdit(this);
	tabBar_ = new TabWidget(this);
	tabBar_->setIconSize(QSize(18, 18));

	QFrame *panel;
	QLayout *panelLayout;

	functionsTree_ = new TreeView(this);
	functionsTree_->setObjectName("grid");
	functionsTree_->setIconSize(QSize(18, 18));
	functionsTree_->setRootIsDecorated(false);
	functionsTree_->setUniformRowHeights(true);
	if (functionBundleListModel_.get())
		functionsTree_->setModel(functionBundleListModel_.get());
	else
		functionsTree_->setModel(functionListModel_.get());
	if (mode_ == smAddFunction)
		functionsTree_->setSelectionMode(QTreeView::ExtendedSelection);
	functionsTree_->setSelectionBehavior(QTreeView::SelectRows);
	functionsTree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	functionsTree_->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
	functionsTree_->header()->setSectionsMovable(false);
	functionsTree_->header()->setSectionsClickable(true);
    connect(functionsTree_->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(functionsTreeSelectionChanged(QItemSelection, QItemSelection)));
    connect(functionsTree_, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(okButtonClicked()));
	connect(functionsTree_->header(), SIGNAL(sectionClicked(int)), this, SLOT(sectionClicked(int)));

	functionsFilter_ = new SearchLineEdit(this);
	functionsFilter_->setPlaceholderText(QString::fromUtf8(language[lsSearch].c_str()));
    connect(functionsFilter_, SIGNAL(textChanged(const QString &)), this, SLOT(functionsFilterChanged()));
	connect(functionsFilter_, SIGNAL(returnPressed()), this, SLOT(functionsFilterChanged()));

	panel = new QFrame(this);
	panelLayout = new QVBoxLayout();
	panelLayout->setContentsMargins(0, 10, 0, 0);
	panelLayout->setSpacing(10);
	panelLayout->addWidget(functionsFilter_);
	panelLayout->addWidget(functionsTree_);
	panel->setLayout(panelLayout);
	tabBar_->addTab(panel, QIcon(":/images/star_gray.png"), QString::fromUtf8(language[lsFunctions].c_str()));

	if (mode_ == smAddFunction) {
		PropertyManager *propertyManager = new PropertyManager(this);
		GroupProperty *protection = propertyManager->addGroupProperty(NULL, QString::fromUtf8(language[lsProtection].c_str()));
		compilationType_ = propertyManager->addEnumProperty(protection, QString::fromUtf8(language[lsCompilationType].c_str()), 
			QStringList() << QString::fromUtf8(language[lsVirtualization].c_str()) 
			<< QString::fromUtf8(language[lsMutation].c_str()) 
			<< QString("%1 (%2 + %3)").arg(QString::fromUtf8(language[lsUltra].c_str())).arg(QString::fromUtf8(language[lsMutation].c_str())).arg(QString::fromUtf8(language[lsVirtualization].c_str())), 0);
		lockToKey_ = propertyManager->addBoolProperty(protection, QString::fromUtf8(language[lsLockToSerialNumber].c_str()), false);

		TreePropertyEditor *optionsTree = new TreePropertyEditor(this);
		optionsTree->setFrameShape(QFrame::Box);
		optionsTree->setModel(propertyManager);
		optionsTree->expandToDepth(0);
		optionsTree->header()->resizeSection(0, 240 * Application::stylesheetScaleFactor());
		optionsTree->header()->setSectionsMovable(false);

		panel = new QFrame(this);
		panelLayout = new QVBoxLayout();
		panelLayout->setContentsMargins(0, 10, 0, 0);
		panelLayout->addWidget(optionsTree);
		panel->setLayout(panelLayout);
		tabBar_->addTab(panel, QIcon(":/images/gear.png"), QString::fromUtf8(language[lsOptions].c_str()));
	}

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

    QPushButton *okButton = new PushButton(windowTitle(), this);
	connect(okButton, SIGNAL(clicked()), this, SLOT(okButtonClicked()));

    QPushButton *cancelButton = new PushButton(QString::fromUtf8(language[lsCancel].c_str()), this);
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
    
	QGridLayout *layout = new QGridLayout();
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(10);
	layout->addWidget(addressLabel, 0, 0);
	layout->addWidget(addressEdit_, 0, 1);
	layout->addWidget(tabBar_, 1, 0, 1, 2);
    layout->addLayout(buttonLayout, 2, 0, 1, 2);
	setLayout(layout);

	setMinimumSize(480 * Application::stylesheetScaleFactor(), 300 * Application::stylesheetScaleFactor());
}

bool FunctionDialog::checkAddress(const AddressInfo &info)
{
	if (!info.arch)
		return false;

	if (mode_ == smAddFunction) {
		if (info.arch->segment_list()->GetMemoryTypeByAddress(info.address) & mtExecutable) {
			return true;
		} else {
			MapFunction *func = info.arch->map_function_list()->GetFunctionByAddress(info.address);
			if (func && func->type() == otString)
				return true;
		}
	} else {
		if (info.arch->segment_list()->GetSectionByAddress(info.address))
			return true;
	}
	return false;
}

void FunctionDialog::okButtonClicked()
{
	addresses_.clear();

	bool show_arch_name = file_->map_function_list()->show_arch_name();

	QStringList address_list = addressEdit_->text().split(',');
	if (address_list.isEmpty())
		address_list.append("0");

	QString address_str;
	AddressInfo info;
	info.arch = NULL;
	int p = 0;
	for (int i = 0; i < address_list.size(); i++) {
		address_str = address_list[i].trimmed();
		info.arch = defaultArch_;
		if (show_arch_name) {
			int dot_pos = address_str.indexOf('.');
			if (dot_pos > -1) {
				info.arch = file_->GetArchitectureByName(address_str.left(dot_pos).toUtf8().constData());
				address_str.remove(0, dot_pos + 1);
			}
		}
		bool is_valid;
		info.address = address_str.toULongLong(&is_valid, 16);

		if (!is_valid || !checkAddress(info)) {
			MessageDialog::critical(this, "Invalid address", QMessageBox::Ok);
			addressEdit_->setFocus();
			addressEdit_->setCursorPosition(p);
			addressEdit_->cursorForward(true, address_list[i].size());
			return;
		}
		addresses_.append(info);
		p += address_list[i].size() + 1;
	}

	accept();
}

void FunctionDialog::functionsTreeSelectionChanged(const QItemSelection & /*selected*/, const QItemSelection & /*deselected*/)
{
	QModelIndexList list;

	list = functionsTree_->selectionModel()->selectedIndexes();

	QString address;
	for (int i = 0; i < list.size(); i++) {
		QModelIndex index = list.at(i);
		if (index.column() != 0)
			continue;

		if (!address.isEmpty())
			address.append(", ");
		if (functionBundleListModel_.get()) {
			MapFunctionBundle *func = static_cast<MapFunctionBundle *>(index.internalPointer());
			address.append(QString::fromLatin1(func->display_address().c_str()));
		} else {
			MapFunction *func = static_cast<MapFunction *>(index.internalPointer());
			address.append(QString::fromLatin1(func->display_address("").c_str()));
		}
	}
	addressEdit_->setText(address);
}

CompilationType FunctionDialog::compilationType() const 
{ 
	return static_cast<CompilationType>(compilationType_->value()); 
}

uint32_t FunctionDialog::compilationOptions() const 
{ 
	uint32_t res = 0;
	if (lockToKey_->value())
		res |= coLockToKey;
	return res; 
}

void FunctionDialog::functionsFilterChanged()
{
	if (functionBundleListModel_.get())
		functionBundleListModel_->search(functionsFilter_->text());
	else
		functionListModel_->search(functionsFilter_->text());
}

void FunctionDialog::helpClicked()
{
	HelpBrowser::showTopic(tabBar_->currentIndex() == 0 ? "functions::search" : "functions::setup");
}

void FunctionDialog::sectionClicked(int index)
{
	auto mdl = functionsTree_->model();
	assert(mdl);
	if (mdl) 
		mdl->sort(index);
}