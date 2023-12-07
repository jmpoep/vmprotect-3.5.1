#ifndef MESSAGE_DIALOG_H
#define MESSAGE_DIALOG_H

class MessageDialog : public QDialog
{
    Q_OBJECT
public:
	MessageDialog(QMessageBox::Icon icon, const QString &text,
		QMessageBox::StandardButtons buttons = QMessageBox::NoButton, QWidget *parent = 0);
	static QMessageBox::StandardButton information(QWidget *parent, 
		const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
	static QMessageBox::StandardButton question(QWidget *parent, 
		const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
	static QMessageBox::StandardButton warning(QWidget *parent, 
		const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
	static QMessageBox::StandardButton critical(QWidget *parent, 
		const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
	QAbstractButton *clickedButton() const { return clickedButton_; }
	void setDefaultButton(QPushButton *button);
	QAbstractButton *defaultButton() const { return defaultButton_; }
private slots:
	void buttonClicked(QAbstractButton *button);
private:
	static QMessageBox::StandardButton showMessageBox(QWidget *parent, QMessageBox::Icon icon, 
		const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
	QDialogButtonBox *buttonBox_;
	QAbstractButton *clickedButton_;
	QAbstractButton *defaultButton_;
};

#endif