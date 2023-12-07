#ifndef WATERMARK_DIALOG_H
#define WATERMARK_DIALOG_H

class WatermarkDialog : public QDialog
{
    Q_OBJECT
public:
	WatermarkDialog(WatermarkManager *manager, QWidget *parent = NULL);
	Watermark *watermark() const { return watermark_; }
private slots:
	void okButtonClicked();
	void changed();
	void generateClicked();
	void helpClicked();
private:
	WatermarkManager *manager_;
	Watermark *watermark_;

	QPushButton *okButton_;
	QLineEdit *nameEdit_;
	BinEditor *valueEdit_;
};

#endif