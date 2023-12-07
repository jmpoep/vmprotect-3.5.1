#ifndef TEMPLATES_WINDOW_H
#define TEMPLATES_WINDOW_H

class TemplatesWindow : public QDialog
{
	Q_OBJECT
public:
	TemplatesWindow(QWidget *parent = NULL);
	static void setModel(TemplatesModel *model) { templates_model_ = model; }
private slots:
	void helpClicked();
	void contextMenuRequested(const QPoint &p);
	void delClicked();
	void renameClicked();
	void templateIndexChanged();
private:
	ProjectTemplate *selectedTemplate() const;
	static TemplatesModel *templates_model_;
	QPushButton *closeButton_;
	QAction *renameAct_;
	QAction *delAct_;
	QMenu *contextMenu_;
	QTreeView *templateTree_;
};

#endif