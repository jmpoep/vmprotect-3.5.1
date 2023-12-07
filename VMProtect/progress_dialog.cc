#include "../core/objects.h"
#include "../core/files.h"
#include "../core/lang.h"

#include "wait_cursor.h"
#include "progress_dialog.h"
#include "moc/moc_progress_dialog.cc"
#include "message_dialog.h"

/**
 * ProgressDialog
 */

ProgressDialog::ProgressDialog(QWidget *parent)
	: QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::CustomizeWindowHint)
{
	progressBar = new QProgressBar(this);
	progressBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	QFontMetrics fontMetrics = progressBar->fontMetrics();
	QString spaces;
	while (fontMetrics.width(spaces) < 58)
		spaces += QLatin1Char(' ');
	progressBar->setFormat(progressBar->format() + spaces);

	label = new QLabel(this);

	QToolButton *button = new QToolButton(this);
	button->setObjectName("cancel");
	button->setToolTip(QString::fromUtf8(language[lsCancel].c_str()));

	QHBoxLayout *layout = new QHBoxLayout;
	layout->setContentsMargins(20, 0, 20, 0);
	layout->setSpacing(0);
	layout->addWidget(label, 1);
	layout->addWidget(button, 0, Qt::AlignRight);
	progressBar->setLayout(layout);

	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(progressBar);
	setLayout(layout);

	setModal(true);

	connect(button, SIGNAL(clicked()), this, SLOT(cancelClicked()));

	resize(320, 60);
}

void ProgressDialog::cancelClicked()
{
	if(!wait_cursor_.get())
		wait_cursor_.reset(new WaitCursor());
	this->setDisabled(true);
	emit cancel();
}

void ProgressDialog::startProgress(const QString &caption, unsigned long long max)
{
	label->setText(caption);
	progressBar->setValue(0);
	progressBar->setMaximum(max);
}

void ProgressDialog::stepProgress(unsigned long long value)
{
	progressBar->setValue(progressBar->value() + value);
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
	event->ignore();
}

void ProgressDialog::notify(MessageType type, IObject * /*sender*/, const QString &message)
{
	wait_cursor_.reset();
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
}

/**
 * GUILog
 */

GUILog::GUILog(QObject *parent)
	: QObject(parent), is_canceled_(false)
{
	qRegisterMetaType<MessageType>("MessageType");
}

void GUILog::Notify(MessageType type, IObject *sender, const std::string &message)
{
	emit notify(type, sender, QString::fromUtf8(message.c_str()));
}

void GUILog::StartProgress(const std::string &caption, unsigned long long max)
{
	checkCanceled();
	emit startProgress(QString::fromUtf8(caption.c_str()), max);
}

void GUILog::StepProgress(unsigned long long value, bool /*is_project*/)
{
	checkCanceled();
	emit stepProgress(value);
}

void GUILog::EndProgress()
{
	checkCanceled();
	emit endProgress();
}

void GUILog::cancel()
{
	is_canceled_ = true;
}

void GUILog::checkCanceled()
{
	if (is_canceled_)
		throw canceled_error(language[lsOperationCanceledByUser]);
}