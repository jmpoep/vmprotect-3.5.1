#ifndef LICENSE_DIALOG_H
#define LICENSE_DIALOG_H

class LicenseDialog : public QDialog
{
    Q_OBJECT
public:
	LicenseDialog(LicensingManager *manager, License *license, QWidget *parent = NULL);
	License *license() const { return license_; }
private slots:
	void okButtonClicked();
	void nameChanged();
	void emailChanged();
	void serialNameChanged();
	void serialEmailChanged();
	void HWIDChanged();
	void expirationDateChanged();
	void timeLimitChanged();
	void maxBuildDateChanged();
	void userDataChanged();
	void helpClicked();
private:
	LicensingManager *manager_;
	License *license_;

	QLineEdit *nameEdit_;
	QLineEdit *emailEdit_;
	QDateEdit *dateEdit_;
	QLineEdit *orderEdit_;
	QPlainTextEdit *commentsEdit_;

	QCheckBox *serialNameCheckBox_;
	QLineEdit *serialNameEdit_;
	QCheckBox *serialEmailCheckBox_;
	QLineEdit *serialEmailEdit_;
	QCheckBox *serialHWIDCheckBox_;
	QLineEdit *serialHWIDEdit_;
	QCheckBox *serialExpirationDateCheckBox_;
	QDateEdit *serialExpirationDateEdit_;
	QCheckBox *serialTimeLimitCheckBox_;
	QSpinBox *serialTimeLimitEdit_;
	QCheckBox *serialMaxBuildDateCheckBox_;
	QDateEdit *serialMaxBuildDateEdit_;
	QCheckBox *serialUserDataCheckBox_;
	BinEditor *serialUserDataEdit_;
};

#endif