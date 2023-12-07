#include "../core/objects.h"
#include "../core/lang.h"
#include "../core/core.h"
#include "widgets.h"
#include "watermark_dialog.h"
#include "moc/moc_watermark_dialog.cc"
#include "help_browser.h"
#include "application.h"

/**
 * WatermarkDialog
 */

WatermarkDialog::WatermarkDialog(WatermarkManager *manager, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint), manager_(manager), watermark_(NULL)
{
	setWindowTitle(QString::fromUtf8(language[lsAddWatermark].c_str()));

	QFont font;
	font.setBold(true);

	QLabel *details = new QLabel(QString::fromUtf8(language[lsDetails].c_str()), this);
	details->setObjectName("header");
	QFrame *groupDetails = new QFrame(this);
	groupDetails->setObjectName("gridEditor");
	groupDetails->setFrameShape(QFrame::StyledPanel);
	
	QLabel *nameLabel = new QLabel(QString::fromUtf8(language[lsName].c_str()), this);
	nameLabel->setObjectName("editor");
	nameEdit_ = new LineEdit(this);
	nameEdit_->setFrame(false);
	nameEdit_->setFont(font);

	QFrame *valueFrame = new QFrame(this);
	valueFrame->setObjectName("editor");
	QLabel *valueLabel = new QLabel(QString::fromUtf8(language[lsValue].c_str()), valueFrame);
	valueLabel->setObjectName("editor");
	valueEdit_ = new BinEditor(this);
	valueEdit_->setFrameShape(QFrame::NoFrame);
    valueEdit_->setOverwriteMode(false);
	valueEdit_->setMaskAllowed(true);
	font = valueEdit_->font();
	font.setBold(true);
	valueEdit_->setFont(font);
	valueEdit_->setMaximumHeight(60 * Application::stylesheetScaleFactor());

	QGridLayout *layout = new QGridLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setHorizontalSpacing(0);
	layout->setVerticalSpacing(1);
	layout->setColumnMinimumWidth(0, 180 * Application::stylesheetScaleFactor());
	layout->setColumnStretch(1, 1);
	layout->setRowStretch(1, 1);
    layout->addWidget(nameLabel, 0, 0);
    layout->addWidget(nameEdit_, 0, 1);
	layout->addWidget(valueFrame, 1, 0);
    layout->addWidget(valueEdit_, 1, 1);

	groupDetails->setLayout(layout);

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

	okButton_ = new PushButton(QString::fromUtf8(language[lsAddWatermark].c_str()), this);
	okButton_->setEnabled(false);
    QPushButton *cancelButton = new PushButton(QString::fromUtf8(language[lsCancel].c_str()), this);
	QPushButton *generateButton = new PushButton(QString::fromUtf8(language[lsGenerate].c_str()), this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	buttonLayout->setSpacing(10);
	buttonLayout->addWidget(helpButton);
	buttonLayout->addStretch();
	buttonLayout->addWidget(generateButton);
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
	mainLayout->addWidget(details);
	mainLayout->addWidget(groupDetails);
    mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	connect(nameEdit_, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
	connect(valueEdit_, SIGNAL(dataChanged()), this, SLOT(changed()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(generateButton, SIGNAL(clicked()), this, SLOT(generateClicked()));
	connect(okButton_, SIGNAL(clicked()), this, SLOT(okButtonClicked()));

	setMinimumSize(450 * Application::stylesheetScaleFactor(), 100 * Application::stylesheetScaleFactor());
}

void WatermarkDialog::okButtonClicked()
{
	watermark_ = manager_->Add(nameEdit_->text().toUtf8().constData(), valueEdit_->value().toUtf8().constData());

	accept();
}

void WatermarkDialog::changed()
{
	okButton_->setEnabled(!nameEdit_->text().isEmpty() && !valueEdit_->isEmpty());
}

void WatermarkDialog::generateClicked()
{
	valueEdit_->setValue(QString::fromUtf8(manager_->CreateValue().c_str()));
}

void WatermarkDialog::helpClicked()
{
	HelpBrowser::showTopic("watermarks::setup");
}