#include "../core/objects.h"
#include "../core/lang.h"

#include "message_dialog.h"
#include "moc/moc_message_dialog.cc"

/**
 * MessageDialog
 */

MessageDialog::MessageDialog(QMessageBox::Icon icon, const QString &text,
	QMessageBox::StandardButtons /*buttons*/, QWidget *parent)
	: QDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint), clickedButton_(NULL), defaultButton_(NULL)
{
	QLabel *label = new QLabel(text, this);
	label->setObjectName("messageBox");

	QLabel *iconLabel = new QLabel(this);
	iconLabel->setAlignment(Qt::AlignCenter);
	iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    int iconSize = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
    QIcon tmpIcon;
    switch (icon) {
    case QMessageBox::Information:
		setWindowTitle(QString::fromUtf8(language[lsInformation].c_str()));
        tmpIcon = style()->standardIcon(QStyle::SP_MessageBoxInformation, 0, this);
        break;
    case QMessageBox::Warning:
		setWindowTitle(QString::fromUtf8(language[lsWarning].c_str()));
        tmpIcon = style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, this);
        break;
    case QMessageBox::Critical:
		setWindowTitle(QString::fromUtf8(language[lsError].c_str()));
        tmpIcon = style()->standardIcon(QStyle::SP_MessageBoxCritical, 0, this);
        break;
    case QMessageBox::Question:
		setWindowTitle(QString::fromUtf8(language[lsConfirmation].c_str()));
        tmpIcon = style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, this);
    default:
        break;
    }
	if (!tmpIcon.isNull())
		iconLabel->setPixmap(tmpIcon.pixmap(iconSize, iconSize));

	buttonBox_ = new QDialogButtonBox(this);
	buttonBox_->setCenterButtons(true);

	QGridLayout *grid = new QGridLayout;
	grid->setContentsMargins(20, 20, 20, 20);
    grid->setSpacing(20);
	grid->addWidget(iconLabel, 0, 0);
	grid->addWidget(label, 0, 1);
	grid->addWidget(buttonBox_, 2, 0, 1, 2);
	grid->setSizeConstraint(QLayout::SetFixedSize);
	setLayout(grid);

	connect(buttonBox_, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));

	setModal(true);
}

void MessageDialog::buttonClicked(QAbstractButton *button)
{
	clickedButton_ = button;
	int ret = buttonBox_->standardButton(button);
	if (ret == QMessageBox::NoButton)
		ret = -1;
	done(ret);
}

void MessageDialog::setDefaultButton(QPushButton *button)
{
    if (!buttonBox_->buttons().contains(button))
        return;

    defaultButton_ = button;
    button->setDefault(true);
    button->setFocus();
}

QMessageBox::StandardButton MessageDialog::information(QWidget *parent, 
	const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
	return showMessageBox(parent, QMessageBox::Information, text, buttons, defaultButton);
}

QMessageBox::StandardButton MessageDialog::question(QWidget *parent, 
	const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
	return showMessageBox(parent, QMessageBox::Question, text, buttons, defaultButton);
}

QMessageBox::StandardButton MessageDialog::warning(QWidget *parent, 
	const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
	return showMessageBox(parent, QMessageBox::Warning, text, buttons, defaultButton);
}

QMessageBox::StandardButton MessageDialog::critical(QWidget *parent,
	const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
	return showMessageBox(parent, QMessageBox::Critical, text, buttons, defaultButton);
}

QMessageBox::StandardButton MessageDialog::showMessageBox(QWidget *parent, QMessageBox::Icon icon,
	const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
	MessageDialog dialog(icon, text, QMessageBox::NoButton, parent);
    QDialogButtonBox *buttonBox = dialog.findChild<QDialogButtonBox*>();

	assert(buttonBox);
	if (buttonBox == NULL) return QMessageBox::Cancel;

    uint mask = QMessageBox::FirstButton;
    while (mask <= QMessageBox::LastButton) {
        uint sb = buttons & mask;
        mask <<= 1;
        if (!sb)
            continue;

        QPushButton *button = buttonBox->addButton((QDialogButtonBox::StandardButton)sb);
		switch (sb) {
		case QMessageBox::Ok:
			button->setText(QString::fromUtf8(language[lsOK].c_str()));
			break;
		case QMessageBox::Cancel:
			button->setText(QString::fromUtf8(language[lsCancel].c_str()));
			break;
		case QMessageBox::Yes:
			button->setText(QString::fromUtf8(language[lsYes].c_str()));
			break;
		case QMessageBox::No:
			button->setText(QString::fromUtf8(language[lsNo].c_str()));
			break;
		}

        // Choose the first accept role as the default
        if (dialog.defaultButton())
            continue;
        if ((defaultButton == QMessageBox::NoButton && buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
            || (defaultButton != QMessageBox::NoButton && sb == uint(defaultButton)))
            dialog.setDefaultButton(button);
    }

    if (dialog.exec() == -1)
        return QMessageBox::Cancel;
	return QMessageBox::StandardButton(buttonBox->standardButton(dialog.clickedButton()));
}