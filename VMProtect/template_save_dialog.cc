#include "../core/objects.h"
#include "../core/files.h"
#include "../core/lang.h"
#include "../core/core.h"
//#include "../core/inifile.h"
#include "widgets.h"
#include "template_save_dialog.h"
#include "help_browser.h"
#include "message_dialog.h"
#include "moc/moc_template_save_dialog.cc"

/**
 * TemplateSaveDialog
 */

TemplateSaveDialog::TemplateSaveDialog(ProjectTemplateManager *ptm, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	setWindowTitle(QString::fromUtf8(language[lsSaveTemplateAs].c_str()));

	QList<QString> list;
	for (size_t i = 0; i < ptm->count(); i++) {
		list.append(QString::fromUtf8(ptm->item(i)->display_name().c_str()));
	}

	QFrame *group = new QFrame(this);
	group->setObjectName("gridEditor");
	group->setFrameShape(QFrame::StyledPanel);

	QLabel *label = new QLabel(QString::fromUtf8(language[lsName].c_str()), this);
	label->setObjectName("editor");

	nameEdit_ = new EnumEdit(this, list);
	nameEdit_->setEditable(true);
	nameEdit_->setFrame(false);
	nameEdit_->setObjectName("editor");
	connect(nameEdit_, SIGNAL(editTextChanged(const QString &)), this, SLOT(nameChanged()));

	QGridLayout *layout = new QGridLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setHorizontalSpacing(0);
	layout->setVerticalSpacing(1);
    layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 1);

    layout->addWidget(label, 0, 0);
    layout->addWidget(nameEdit_, 0, 1);
	group->setLayout(layout);

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

    okButton_ = new PushButton(QString::fromUtf8(language[lsOK].c_str()), this);
	okButton_->setEnabled(false);
	connect(okButton_, SIGNAL(clicked()), this, SLOT(okButtonClicked()));

    QPushButton *cancelButton = new PushButton(QString::fromUtf8(language[lsCancel].c_str()), this);
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	buttonLayout->setSpacing(10);
	buttonLayout->addWidget(helpButton);
	buttonLayout->addStretch();
#ifdef __APPLE__
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton_);
#else
    buttonLayout->addWidget(okButton_);
    buttonLayout->addWidget(cancelButton);
#endif

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(10);
	mainLayout->addWidget(group);
    mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	QString tryName, defaultName = QString::fromUtf8(language[lsNewTemplate].c_str());
	int unique = 1;
	do
	{
		tryName = defaultName;
		if (unique > 1)
			tryName += QString(" (%1)").arg(unique);
		unique++;
	} while(nameEdit_->findText(tryName) != -1);
	nameEdit_->setCurrentText(tryName);
	nameEdit_->lineEdit()->selectAll();

	resize(600, 10);
}

void TemplateSaveDialog::okButtonClicked()
{
	if (nameEdit_->findText(nameEdit_->currentText()) != -1)
	{
		if(MessageDialog::question(this, QString::fromUtf8(language[lsOverwriteTemplate].c_str()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;
	}
	accept();
}

void TemplateSaveDialog::helpClicked()
{
	//FIXME HelpBrowser::showTopic("templates");
}

void TemplateSaveDialog::nameChanged()
{
	okButton_->setEnabled(!nameEdit_->currentText().isEmpty());
}

QString TemplateSaveDialog::name()
{
	return nameEdit_->currentText();
}
