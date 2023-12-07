#ifndef TEMPLATE_SAVE_DIALOG_H
#define TEMPLATE_SAVE_DIALOG_H

class TemplateSaveDialog : public QDialog
{
    Q_OBJECT
public:
	TemplateSaveDialog(ProjectTemplateManager *ptm, QWidget *parent = NULL);
	QString name();
private slots:
	void okButtonClicked();
	void helpClicked();
	void nameChanged();
private:
	EnumEdit *nameEdit_;
	QPushButton *okButton_;
};

#endif