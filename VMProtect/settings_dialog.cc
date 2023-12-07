#include "../core/objects.h"
//#include "../core/files.h"
#include "../core/lang.h"
#include "../core/core.h"
#include "../core/inifile.h"
#include "widgets.h"
#include "settings_dialog.h"
#include "help_browser.h"
#include "moc/moc_settings_dialog.cc"
#include "application.h"

/**
 * SettingsDialog
 */

SettingsDialog::SettingsDialog(QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint)
{
	setWindowTitle(QString::fromUtf8(language[lsSettings].c_str()));

	QList<QString> languageList;
	LanguageManager *languageManager = settings_file().language_manager();
	size_t lang_index = 0;
	std::string lang_id = settings_file().language();
	for (size_t i = 0; i < languageManager->count(); i++) {
		Language *lang = languageManager->item(i);
		if (lang->id() == lang_id)
			lang_index = i;
		languageList.append(QString::fromUtf8(lang->name().c_str()));
	}

	QFrame *group = new QFrame(this);
	group->setObjectName("gridEditor");
	group->setFrameShape(QFrame::StyledPanel);

	QLabel *languageLabel = new QLabel(QString::fromUtf8(language[lsLanguage].c_str()), this);
	languageLabel->setObjectName("editor");

	languageEdit_ = new EnumEdit(this, languageList);
	languageEdit_->setFrame(false);
	languageEdit_->setObjectName("editor");

	QLabel *autoSaveLabel = new QLabel(QString::fromUtf8(language[lsAutoSaveProject].c_str()), this);
	autoSaveLabel->setObjectName("editor");

	autoSaveEdit_ = new BoolEdit(this);
	autoSaveEdit_->setFrame(false);
	autoSaveEdit_->setObjectName("editor");

	QGridLayout *layout = new QGridLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setHorizontalSpacing(0);
	layout->setVerticalSpacing(1);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 1);

	layout->addWidget(languageLabel, 0, 0);
	layout->addWidget(languageEdit_, 0, 1);
	layout->addWidget(autoSaveLabel, 1, 0);
	layout->addWidget(autoSaveEdit_, 1, 1);
	group->setLayout(layout);

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

	QPushButton *okButton = new PushButton(QString::fromUtf8(language[lsOK].c_str()), this);
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

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(10);
	mainLayout->addWidget(group);
    mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	languageEdit_->setCurrentIndex((int)lang_index);
	autoSaveEdit_->setCurrentIndex((int)settings_file().auto_save_project());

	resize(400 * Application::stylesheetScaleFactor(), 10);
}

void SettingsDialog::okButtonClicked()
{
	std::string lang_id = settings_file().language_manager()->item(languageEdit_->currentIndex())->id();
	bool auto_save_project = (autoSaveEdit_->currentIndex() == 1);

	if (settings_file().language() != lang_id)
		settings_file().set_language(lang_id);

	if (settings_file().auto_save_project() != auto_save_project)
		settings_file().set_auto_save_project(auto_save_project);

	accept();
}

void SettingsDialog::helpClicked()
{
	HelpBrowser::showTopic("settings");
}