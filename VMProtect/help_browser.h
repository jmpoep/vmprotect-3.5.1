#ifndef HELP_BROWSER_H
#define HELP_BROWSER_H

class TextBrowser;
class QHelpEngine;
class QHelpSearchEngine;
class QPushButton;
class SearchLineEdit;
class QHelpContentItem;
class FindWidget;

class HelpContentModel : public QIdentityProxyModel
{
public:
	HelpContentModel(QHelpEngine *engine, QObject *parent = 0);
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QHelpContentItem *contentItemAt(const QModelIndex &index) const;
	QModelIndex indexOf(const QUrl &link);
private:
	QModelIndex searchContentItem(const QModelIndex &parent, const QString &path);
};

class HelpResultItem
{
public:
	HelpResultItem(const QString &title, const QString &url, const QString &toolTip);
	~HelpResultItem();
	QString title() const { return title_; }
	QString url() const { return url_; }
	QString toolTip() const { return toolTip_; }
	HelpResultItem *child(int index) const { return children_.value(index); };
	int childCount() const { return children_.size(); }
	QList<HelpResultItem *> children() const { return children_; };
	HelpResultItem *parent() const { return parent_; };
	void clear();
	void addChild(HelpResultItem *child);
	void insertChild(int index, HelpResultItem *child);
	void removeChild(HelpResultItem *child);
private:
	HelpResultItem *parent_;
	QString title_;
	QString url_;
	QString toolTip_;
	QList<HelpResultItem *> children_;
};

class HelpResultModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	HelpResultModel(QHelpEngine *engine, QObject *parent = 0);
	~HelpResultModel();
	void search(const QString &text);
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	HelpResultItem *itemAt(const QModelIndex &index) const;
signals:
	void finished(int hits);
protected slots:
	void searchingFinished();
private:
	QHelpEngine *engine_;
	HelpResultItem *root_;
};

class QHelpBrowserProcess : public QProcess
{
public:
	QHelpBrowserProcess(QObject *parent, const QString &fileName);
	const QString fileName_;
	~QHelpBrowserProcess();
};

class RemoteControl;
class HelpBrowser : public QMainWindow
{ 
	Q_OBJECT 
public:
	HelpBrowser(const QString &fileName);
	static void showTopic(const QString &keywordId);

private:
	void navigateToKeyword(const QString &keywordId);
	void keyPressEvent(QKeyEvent *e);
	void updatePath();

	static QHelpBrowserProcess *help_browser_process_;

	QString fileName_;
	QHelpEngine *helpEngine_;
	TextBrowser *helpBrowser_;
	QAction *backButton_;
	QAction *fwdButton_;
	SearchLineEdit *searchBox_;
	QStackedWidget *tabWidget_;
	FindWidget *findWidget_;
	QTreeView *contentWidget_;
	QTreeView *resultWidget_;
	HelpContentModel *contentModel_;
	HelpResultModel *resultModel_;
	RemoteControl *rc_;
	bool contentsCreated_;

protected Q_SLOTS:
	void syncTree(const QUrl &);
	void searchBoxChanged();
	void searchingFinished();
	void highlightSearchTerms();
	void findNext();
	void findPrevious();
	void find(const QString &ttf, bool forward, bool incremental);
	void contentWidgetClicked(const QModelIndex &index);
	void resultWidgetClicked(const QModelIndex &index);
	void pathClicked(const QString &link);
	void resultCloseClicked();
	void contentsCreated();
	void handleNavigateToKeywordCommand(const QString &arg);
};

#endif