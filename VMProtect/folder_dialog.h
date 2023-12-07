#ifndef FOLDER_DIALOG_H
#define FOLDER_DIALOG_H

#include <QtGui/QDialog>
#include <QtGui/QtGui>

class FolderDialog : public QDialog
{
    Q_OBJECT

public:
	FolderDialog(QWidget *parent = NULL);
	QString nameText() const { return nameEdit->text(); }
	void setNameText(const QString &text) { nameEdit->setText(text); }
private:
	QLineEdit *nameEdit;
};

#endif