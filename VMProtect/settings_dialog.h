#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
	SettingsDialog(QWidget *parent = NULL);
private slots:
	void okButtonClicked();
	void helpClicked();
private:
	EnumEdit *languageEdit_;
	BoolEdit *autoSaveEdit_;
#ifndef VMP_GNU
	BoolEdit *shellExtEdit_;
	bool shellExt_;
#endif
};

#endif