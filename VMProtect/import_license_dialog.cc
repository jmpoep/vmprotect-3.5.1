#include "../core/objects.h"
#include "../core/lang.h"
#include "import_license_dialog.h"
#include "widgets.h"
#include "moc/moc_import_license_dialog.cc"
#include "help_browser.h"

/**
 * ImportLicenseDialog
 */

ImportLicenseDialog::ImportLicenseDialog(QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setWindowTitle(QString::fromUtf8(language[lsImportLicense].c_str()));

	QLabel *serialLabel = new QLabel(this);
	serialLabel->setObjectName("header");
	serialLabel->setText(QString::fromUtf8(language[lsPasteSerialNumber].c_str()));
	serialEdit_ = new QPlainTextEdit(this);
	QFont font = serialEdit_->font();
	font.setFamily(MONOSPACE_FONT_FAMILY);
	serialEdit_->setFont(font);

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

    okButton_ = new PushButton(QString::fromUtf8(language[lsOK].c_str()));
	okButton_->setEnabled(false);
    QPushButton *cancelButton = new PushButton(QString::fromUtf8(language[lsCancel].c_str()));
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
    
	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(10);
	layout->addWidget(serialLabel);
	layout->addWidget(serialEdit_);
    layout->addLayout(buttonLayout);
	setLayout(layout);

	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton_, SIGNAL(clicked()), this, SLOT(accept()));
	connect(serialEdit_, SIGNAL(textChanged()), this, SLOT(serialChanged()));

	resize(580, 250);
}

void ImportLicenseDialog::serialChanged()
{
	okButton_->setEnabled(!serial().isEmpty());
}

void ImportLicenseDialog::helpClicked()
{
	HelpBrowser::showTopic("project::licenses");
}

QString ImportLicenseDialog::serial() const
{
	QString str = serialEdit_->toPlainText().simplified();
	str.replace(" ", "");
	return str;
}
