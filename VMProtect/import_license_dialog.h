#ifndef IMPORT_LICENSE_DIALOG_H
#define IMPORT_LICENSE_DIALOG_H

class ImportLicenseDialog : public QDialog
{
    Q_OBJECT
public:
	ImportLicenseDialog(QWidget *parent = NULL);
	QString serial() const;
private slots:
	void serialChanged();
	void helpClicked();
private:
	QPlainTextEdit *serialEdit_;
	QPushButton *okButton_;
};

#endif