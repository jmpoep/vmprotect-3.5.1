#ifdef ULTIMATE
#include "../core/objects.h"
#include "../core/core.h"
#include "../core/lang.h"
#include "widgets.h"
#include "license_dialog.h"
#include "moc/moc_license_dialog.cc"
#include "message_dialog.h"
#include "wait_cursor.h"
#include "help_browser.h"
#include "application.h"

/**
 * LicenseDialog
 */

LicenseDialog::LicenseDialog(LicensingManager *manager, License *license, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint), manager_(manager), license_(license)
{
	setWindowTitle(QString::fromUtf8(language[lsAddLicense].c_str()));

	QFont font;
	font.setBold(true);

	QLabel *details = new QLabel(QString::fromUtf8(language[lsDetails].c_str()), this);
	details->setObjectName("header");
	QFrame *groupDetails = new QFrame(this);
	groupDetails->setObjectName("gridEditor");
	groupDetails->setFrameShape(QFrame::StyledPanel);
	
	QGridLayout *layout = new QGridLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setHorizontalSpacing(0);
	layout->setVerticalSpacing(1);
	layout->setColumnMinimumWidth(0, 180 * Application::stylesheetScaleFactor());
	layout->setColumnStretch(1, 1);

	QLabel *nameLabel = new QLabel(QString::fromUtf8(language[lsCustomerName].c_str()), this);
	nameLabel->setObjectName("editor");
	nameEdit_ = new LineEdit(this);
	nameEdit_->setFrame(false);
	nameEdit_->setFont(font);

	QLabel *emailLabel = new QLabel(QString::fromUtf8(language[lsEmail].c_str()), this);
	emailLabel->setObjectName("editor");
	emailEdit_ = new LineEdit(this);
	emailEdit_->setFrame(false);
	emailEdit_->setFont(font);

	QLabel *dateLabel = new QLabel(QString::fromUtf8(language[lsDate].c_str()), this);
	dateLabel->setObjectName("editor");
	dateEdit_ = new QDateEdit(this);
	dateEdit_->setFrame(false);
	dateEdit_->setDate(QDate::currentDate());
	dateEdit_->setFont(font);

	QLabel *orderLabel = new QLabel(QString::fromUtf8(language[lsOrderRef].c_str()), this);
	orderLabel->setObjectName("editor");
	orderEdit_ = new LineEdit(this);
	orderEdit_->setFrame(false);
	orderEdit_->setFont(font);

	QLabel *commentsLabel = new QLabel(QString::fromUtf8(language[lsComments].c_str()), this);
	commentsLabel->setAlignment(Qt::AlignTop);
	commentsLabel->setObjectName("editor");
	commentsEdit_ = new QPlainTextEdit(this);
	commentsEdit_->setFrameShape(QFrame::NoFrame);
	commentsEdit_->setMaximumHeight(40 * Application::stylesheetScaleFactor());
	commentsEdit_->setFont(font);
	commentsEdit_->setTabChangesFocus(true);

    layout->addWidget(nameLabel, 0, 0);
    layout->addWidget(nameEdit_, 0, 1);
    layout->addWidget(emailLabel, 1, 0);
    layout->addWidget(emailEdit_, 1, 1);
    layout->addWidget(dateLabel, 2, 0);
    layout->addWidget(dateEdit_, 2, 1);
    layout->addWidget(orderLabel, 3, 0);
    layout->addWidget(orderEdit_, 3, 1);
    layout->addWidget(commentsLabel, 4, 0);
    layout->addWidget(commentsEdit_, 4, 1);
	layout->setRowStretch(4, 1);
	groupDetails->setLayout(layout);

	QLabel *serial = new QLabel(QString::fromUtf8(language[lsSerialNumberContents].c_str()), this);
	serial->setObjectName("header");
	QFrame *groupSerial = new QFrame(this);
	groupSerial->setObjectName("gridEditor");
	groupSerial->setFrameShape(QFrame::StyledPanel);

	serialNameCheckBox_ = new QCheckBox(QString::fromUtf8(language[lsCustomerName].c_str()), this);
	serialNameCheckBox_->setObjectName("editor");
	serialNameEdit_ = new LineEdit(this);
	serialNameEdit_->setFrame(false);
	serialNameEdit_->setFont(font);
	serialNameEdit_->setMaxLength(255);

	serialEmailCheckBox_ = new QCheckBox(QString::fromUtf8(language[lsEmail].c_str()), this);
	serialEmailCheckBox_->setObjectName("editor");
	serialEmailEdit_ = new LineEdit(this);
	serialEmailEdit_->setFrame(false);
	serialEmailEdit_->setFont(font);
	serialEmailEdit_->setMaxLength(255);

	serialHWIDCheckBox_ = new QCheckBox(QString::fromUtf8(language[lsHardwareID].c_str()), this);
	serialHWIDCheckBox_->setObjectName("editor");
	serialHWIDEdit_ = new LineEdit(this);
	serialHWIDEdit_->setFrame(false);
	serialHWIDEdit_->setFont(font);

	serialExpirationDateCheckBox_ = new QCheckBox(QString::fromUtf8(language[lsExpirationDate].c_str()), this);
	serialExpirationDateCheckBox_->setObjectName("editor");
	serialExpirationDateEdit_ = new QDateEdit(this);
	serialExpirationDateEdit_->setFrame(false);
	serialExpirationDateEdit_->setDate(QDate::currentDate().addMonths(1));
	serialExpirationDateEdit_->setFont(font);

	serialTimeLimitCheckBox_ = new QCheckBox(QString::fromUtf8(language[lsRunningTimeLimit].c_str()), this);
	serialTimeLimitCheckBox_->setObjectName("editor");
	serialTimeLimitEdit_ = new QSpinBox(this);
	serialTimeLimitEdit_->setFrame(false);
	serialTimeLimitEdit_->setRange(0, 255);
	serialTimeLimitEdit_->setValue(30);
	serialTimeLimitEdit_->setFont(font);

	serialMaxBuildDateCheckBox_ = new QCheckBox(QString::fromUtf8(language[lsMaxBuildDate].c_str()), this);
	serialMaxBuildDateCheckBox_->setObjectName("editor");
	serialMaxBuildDateEdit_ = new QDateEdit(this);
	serialMaxBuildDateEdit_->setFrame(false);
	serialMaxBuildDateEdit_->setDate(QDate::currentDate().addYears(1));
	serialMaxBuildDateEdit_->setFont(font);

	QFrame *serialUserDataFrame = new QFrame(this);
	serialUserDataFrame->setObjectName("editor");
	serialUserDataCheckBox_ = new QCheckBox(QString::fromUtf8(language[lsUserData].c_str()), serialUserDataFrame);
	serialUserDataCheckBox_->setObjectName("editor");
	serialUserDataEdit_ = new BinEditor(this);
	serialUserDataEdit_->setFrameShape(QFrame::NoFrame);
    serialUserDataEdit_->setOverwriteMode(false);
	serialUserDataEdit_->setMaxLength(255);
	serialUserDataEdit_->setMaximumHeight(40 * Application::stylesheetScaleFactor());
	font = serialUserDataEdit_->font();
	font.setBold(true);
	serialUserDataEdit_->setFont(font);

	layout = new QGridLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setHorizontalSpacing(0);
	layout->setVerticalSpacing(1);
	layout->setColumnMinimumWidth(0, 180 * Application::stylesheetScaleFactor());
	layout->setColumnStretch(1, 1);
    layout->addWidget(serialNameCheckBox_, 0, 0);
    layout->addWidget(serialNameEdit_, 0, 1);
    layout->addWidget(serialEmailCheckBox_, 1, 0);
    layout->addWidget(serialEmailEdit_, 1, 1);
    layout->addWidget(serialHWIDCheckBox_, 2, 0);
    layout->addWidget(serialHWIDEdit_, 2, 1);
    layout->addWidget(serialExpirationDateCheckBox_, 3, 0);
    layout->addWidget(serialExpirationDateEdit_, 3, 1);
    layout->addWidget(serialTimeLimitCheckBox_, 4, 0);
    layout->addWidget(serialTimeLimitEdit_, 4, 1);
    layout->addWidget(serialMaxBuildDateCheckBox_, 5, 0);
    layout->addWidget(serialMaxBuildDateEdit_, 5, 1);
	layout->addWidget(serialUserDataFrame, 6, 0);
    layout->addWidget(serialUserDataEdit_, 6, 1);

	groupSerial->setLayout(layout);

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

    QPushButton *okButton = new PushButton(QString::fromUtf8(language[lsAddLicense].c_str()), this);
    QPushButton *cancelButton = new PushButton(QString::fromUtf8(language[lsCancel].c_str()), this);
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
	mainLayout->addWidget(details);
	mainLayout->addWidget(groupDetails);
	mainLayout->addStretch(1);
	mainLayout->addWidget(serial);
	mainLayout->addWidget(groupSerial);
    mainLayout->addLayout(buttonLayout);

	if (license) {
		nameEdit_->setText(QString::fromUtf8(license->customer_name().c_str()));
		emailEdit_->setText(QString::fromUtf8(license->customer_email().c_str()));
		LicenseDate date = license->date();
		dateEdit_->setDate(QDate(date.Year, date.Month, date.Day));
		orderEdit_->setText(QString::fromUtf8(license->order_ref().c_str()));
		commentsEdit_->setPlainText(QString::fromUtf8(license->comments().c_str()));

		LicenseInfo *license_info = license_->info();
		if (license_info) {
			if (license_info->Flags & HAS_USER_NAME) {
				serialNameCheckBox_->setChecked(true);
				serialNameEdit_->setText(QString::fromUtf8(license_info->CustomerName.c_str()));
			}
			if (license_info->Flags & HAS_EMAIL) {
				serialEmailCheckBox_->setChecked(true);
				serialEmailEdit_->setText(QString::fromUtf8(license_info->CustomerEmail.c_str()));
			}
			if (license_info->Flags & HAS_HARDWARE_ID) {
				serialHWIDCheckBox_->setChecked(true);
				serialHWIDEdit_->setText(QString::fromLatin1(license_info->HWID.c_str()));
			}
			if (license_info->Flags & HAS_TIME_LIMIT) {
				serialTimeLimitCheckBox_->setChecked(true);
				serialTimeLimitEdit_->setValue(license_info->RunningTimeLimit);
			}
			if (license_info->Flags & HAS_EXP_DATE) {
				serialExpirationDateCheckBox_->setChecked(true);
				serialExpirationDateEdit_->setDate(QDate(license_info->ExpireDate.Year, license_info->ExpireDate.Month, license_info->ExpireDate.Day));
			}
			if (license_info->Flags & HAS_MAX_BUILD_DATE) {
				serialMaxBuildDateCheckBox_->setChecked(true);
				serialMaxBuildDateEdit_->setDate(QDate(license_info->MaxBuildDate.Year, license_info->MaxBuildDate.Month, license_info->MaxBuildDate.Day));
			}
			if (license_info->Flags & HAS_USER_DATA) {
				serialUserDataCheckBox_->setChecked(true);
				serialUserDataEdit_->setData(QByteArray(license_info->UserData.c_str(), (int)license_info->UserData.size()));
			}
		}
	}

	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(okButtonClicked()));
	connect(nameEdit_, SIGNAL(textChanged(const QString &)), this, SLOT(nameChanged()));
	connect(emailEdit_, SIGNAL(textChanged(const QString &)), this, SLOT(emailChanged()));
	connect(serialNameEdit_, SIGNAL(textChanged(const QString &)), this, SLOT(serialNameChanged()));
	connect(serialEmailEdit_, SIGNAL(textChanged(const QString &)), this, SLOT(serialEmailChanged()));
	connect(serialHWIDEdit_, SIGNAL(textChanged(const QString &)), this, SLOT(HWIDChanged()));
	connect(serialExpirationDateEdit_, SIGNAL(dateChanged(const QDate &)), this, SLOT(expirationDateChanged()));
	connect(serialTimeLimitEdit_, SIGNAL(valueChanged(int)), this, SLOT(timeLimitChanged()));
	connect(serialMaxBuildDateEdit_, SIGNAL(dateChanged(const QDate &)), this, SLOT(maxBuildDateChanged()));
	connect(serialUserDataEdit_, SIGNAL(dataChanged()), this, SLOT(userDataChanged()));

	setLayout(mainLayout);
	setMinimumSize(450 * Application::stylesheetScaleFactor(), 300 * Application::stylesheetScaleFactor());
}

void LicenseDialog::nameChanged()
{
	serialNameEdit_->setText(nameEdit_->text());
}

void LicenseDialog::emailChanged()
{
	serialEmailEdit_->setText(emailEdit_->text());
}

void LicenseDialog::serialNameChanged()
{
	serialNameCheckBox_->setChecked(!serialNameEdit_->text().isEmpty());
}

void LicenseDialog::serialEmailChanged()
{
	serialEmailCheckBox_->setChecked(!serialEmailEdit_->text().isEmpty());
}

void LicenseDialog::HWIDChanged()
{
	serialHWIDCheckBox_->setChecked(!serialHWIDEdit_->text().isEmpty());
}

void LicenseDialog::expirationDateChanged()
{
	serialExpirationDateCheckBox_->setChecked(true);
}

void LicenseDialog::timeLimitChanged()
{
	serialTimeLimitCheckBox_->setChecked(true);
}

void LicenseDialog::maxBuildDateChanged()
{
	serialMaxBuildDateCheckBox_->setChecked(true);
}

void LicenseDialog::userDataChanged()
{
	serialUserDataCheckBox_->setChecked(serialUserDataEdit_->data().size());
}

void LicenseDialog::okButtonClicked()
{
	LicenseInfo licenseInfo;

	if (serialNameCheckBox_->checkState() == Qt::Checked) {
		licenseInfo.Flags |= HAS_USER_NAME;
		licenseInfo.CustomerName = serialNameEdit_->text().toUtf8().constData();
	}

	if (serialEmailCheckBox_->checkState() == Qt::Checked) {
		licenseInfo.Flags |= HAS_EMAIL;
		licenseInfo.CustomerEmail = serialEmailEdit_->text().toUtf8().constData();
	}

	if (serialHWIDCheckBox_->checkState() == Qt::Checked) {
		licenseInfo.Flags |= HAS_HARDWARE_ID;
		licenseInfo.HWID = serialHWIDEdit_->text().toUtf8().constData();
	}

	if (serialExpirationDateCheckBox_->checkState() == Qt::Checked) {
		licenseInfo.Flags |= HAS_EXP_DATE;
		QDate date = serialExpirationDateEdit_->date();
		licenseInfo.ExpireDate = LicenseDate(date.year(), date.month(), date.day());
	}

	if (serialTimeLimitCheckBox_->checkState() == Qt::Checked) {
		licenseInfo.Flags |= HAS_TIME_LIMIT;
		licenseInfo.RunningTimeLimit = (uint8_t)serialTimeLimitEdit_->value();
	}

	if (serialMaxBuildDateCheckBox_->checkState() == Qt::Checked) {
		licenseInfo.Flags |= HAS_MAX_BUILD_DATE;
		QDate date = serialMaxBuildDateEdit_->date();
		licenseInfo.MaxBuildDate = LicenseDate(date.year(), date.month(), date.day());
	}

	if (serialUserDataCheckBox_->checkState() == Qt::Checked) {
		licenseInfo.Flags |= HAS_USER_DATA;
		QByteArray data = serialUserDataEdit_->data();
		licenseInfo.UserData = std::string(data.constData(), data.size());
	}

	try {
		WaitCursor wc;
		std::string serialNumber = manager_->GenerateSerialNumber(licenseInfo);
		QDate date = dateEdit_->date();
		license_ = manager_->Add(LicenseDate(date.year(), date.month(), date.day()), 
					nameEdit_->text().toUtf8().constData(), 
					emailEdit_->text().toUtf8().constData(),
					orderEdit_->text().toUtf8().constData(),
					commentsEdit_->toPlainText().toUtf8().constData(),
					serialNumber,
					false);
	} catch (const std::runtime_error &e) {
		MessageDialog::critical(this, QString("%1:\n%2").arg(QString::fromUtf8(language[lsSerialNumberError].c_str())).arg(QString::fromUtf8(e.what())), QMessageBox::Ok);
		return;
	}

	accept();
}

void LicenseDialog::helpClicked()
{
	HelpBrowser::showTopic("project::licenses");
}

#endif