#ifndef EXPORT_KEY_PAIR_DIALOG_H
#define EXPORT_KEY_PAIR_DIALOG_H

class ExportKeyPairDialog : public QDialog
{
    Q_OBJECT
public:
	ExportKeyPairDialog(QWidget *parent, LicensingManager *manager);
private slots:
	void formatChanged(int format);
	void copyClicked();
	void helpClicked();
private:
	size_t bits_;
	std::vector<uint8_t> public_exp_;
	std::vector<uint8_t> private_exp_;
	std::vector<uint8_t> modulus_;
	std::vector<uint8_t> product_code_;

	QPlainTextEdit *edit_;
};

#endif