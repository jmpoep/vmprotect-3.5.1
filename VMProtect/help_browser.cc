#include "help_browser.h" 
#include "message_dialog.h" 
#include "widgets.h" 
#include "moc/moc_help_browser.cc"
#include "../core/objects.h"
#include "../core/lang.h"
#include "../core/inifile.h"
#include "remotecontrol.h"

/**
 * HelpContentModel
 */

HelpContentModel::HelpContentModel(QHelpEngine *engine, QObject *parent)
	: QIdentityProxyModel(parent)
{
	setSourceModel(engine->contentModel());
}

QVariant HelpContentModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::FontRole) {
		if (!index.parent().isValid()) {
			// top level
			QFont font;
			font.setBold(true);
			return font;
		}
	}
	return QIdentityProxyModel::data(index, role);
}

QVariant HelpContentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0)
			return QString::fromUtf8(language[lsContents].c_str());
	}
	return QIdentityProxyModel::headerData(section, orientation, role);
}

QHelpContentItem *HelpContentModel::contentItemAt(const QModelIndex &index) const
{
	QHelpContentModel *model = qobject_cast<QHelpContentModel*>(sourceModel());
	if (model)
		return model->contentItemAt(index);
	return NULL;
}

QModelIndex HelpContentModel::indexOf(const QUrl &link)
{
	if (link.scheme() != QLatin1String("qthelp"))
		return QModelIndex();

	QString fragment = link.fragment();
	for (int i = 0; i < rowCount(); ++i) {
		QHelpContentItem *item = contentItemAt(index(i, 0));
		if (item && item->url().host() == link.host()) {
			QString path = link.path();
			//if (path.startsWith(QLatin1Char('/')))
			//	path = path.mid(1);
			QModelIndex res = searchContentItem(index(i, 0), path);
			if (res.isValid()) {
				if (!fragment.isEmpty()) {
					item = contentItemAt(res);
					assert(item);
					for (int j = 0; item && (j < item->childCount()); ++j) {
						auto ch = item->child(j);
						assert(ch);
						if (ch && (ch->url().fragment() == fragment)) {
							res = index(j, 0, res);
							break;
						}
					}
				}
				return res;
			}
		}
	}
	return QModelIndex();
}

QModelIndex HelpContentModel::searchContentItem(const QModelIndex &parent, const QString &path)
{
	QHelpContentItem *parentItem = contentItemAt(parent);
	if (!parentItem)
		return QModelIndex();

	QString cpath = parentItem->url().path();
	if (cpath == path)
		return parent;

	for (int i = 0; i < parentItem->childCount(); ++i) {
		QModelIndex res = searchContentItem(index(i, 0, parent), path);
		if (res.isValid())
			return res;
	}
	return QModelIndex();
}

/**
 * HelpResultItem
 */

HelpResultItem::HelpResultItem(const QString &title, const QString &url, const QString &toolTip)
	: title_(title), url_(url), toolTip_(toolTip), parent_(NULL)
{

}

HelpResultItem::~HelpResultItem()
{
	clear();
	if (parent_)
		parent_->removeChild(this);
}

void HelpResultItem::clear()
{
	for (int i = 0; i < children_.size(); i++) {
		HelpResultItem *item = children_[i];
		item->parent_ = NULL;
		delete item;
	}
	children_.clear();
}

void HelpResultItem::addChild(HelpResultItem *child)
{
	insertChild(childCount(), child);
}

void HelpResultItem::insertChild(int index, HelpResultItem *child)
{ 
	if (child->parent())
		return;

	child->parent_ = this;
	children_.insert(index, child);
}

void HelpResultItem::removeChild(HelpResultItem *child)
{
	if (children_.removeOne(child))
		child->parent_ = NULL;
}

/**
 * HelpResultModel
 */

HelpResultModel::HelpResultModel(QHelpEngine *engine, QObject *parent)
	: QAbstractItemModel(parent), engine_(engine)
{
	root_ = new HelpResultItem(QString(), QString(), QString());

	connect(engine_->searchEngine(), SIGNAL(searchingFinished(int)), this, SLOT(searchingFinished()));
}

HelpResultModel::~HelpResultModel()
{
	delete root_;
}

QModelIndex HelpResultModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	HelpResultItem *childItem = itemAt(index);
	HelpResultItem *parentItem = childItem->parent();
	if (parentItem == root_)
		return QModelIndex();

	return createIndex(parentItem->parent()->children().indexOf(parentItem), 0, parentItem);
}

int HelpResultModel::columnCount(const QModelIndex & /* parent */) const
{
	return 1;
}

QVariant HelpResultModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0)
			return QString::fromUtf8(language[lsSearchResult].c_str());
	}
	return QVariant();
}

int HelpResultModel::rowCount(const QModelIndex &parent) const
{
	HelpResultItem *parentItem = itemAt(parent);
	return parentItem->childCount();
}

QVariant HelpResultModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	HelpResultItem *item = itemAt(index);
	switch (role) {
	case Qt::DisplayRole:
		return item->title();
	case Qt::ToolTipRole:
		return item->toolTip();
	case Qt::FontRole:
		if (!index.parent().isValid()) {
			// top level
			QFont font;
			font.setBold(true);
			return font;
		}
		break;
	}
	return QVariant();
}

QModelIndex HelpResultModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	HelpResultItem *parentItem = itemAt(parent);
	HelpResultItem *childItem = parentItem->child(row);
	if (!childItem)
		return QModelIndex();

	return createIndex(row, column, childItem);
}

HelpResultItem *HelpResultModel::itemAt(const QModelIndex &index) const
{
	if (index.isValid())
		return static_cast<HelpResultItem *>(index.internalPointer());
	return root_;
}

void HelpResultModel::search(const QString &text)
{
	QHelpSearchQuery q(QHelpSearchQuery::DEFAULT, QStringList(text));
	QList<QHelpSearchQuery> ql;
	ql.append(q);
	engine_->searchEngine()->search(ql);
}

void HelpResultModel::searchingFinished()
{
	beginResetModel();

	root_->clear();
	QList<QHelpSearchEngine::SearchHit> hits = engine_->searchEngine()->hits(0, engine_->searchEngine()->hitsCount());
	foreach(const QHelpSearchEngine::SearchHit &hit, hits) {
		QString toolTip;
		QModelIndex index = engine_->contentWidget()->indexOf(hit.first);
		while (index.isValid()) {
			QHelpContentItem *item = engine_->contentModel()->contentItemAt(index);
			if (item) {
				if (!toolTip.isEmpty())
					toolTip = " > " + toolTip;
				toolTip = item->title() + toolTip;
			}
			index = index.parent();
		}
		root_->addChild(new HelpResultItem(hit.second, hit.first, toolTip));
	}

	endResetModel();

	emit finished(root_->childCount());
}

class TextBrowser : public QTextBrowser 
{
public:
	TextBrowser(QHelpEngine *helpEngine, QWidget *parent_ = 0): QTextBrowser(parent_), helpEngine_(helpEngine) {}
	QVariant loadResource(int type, const QUrl &url)
	{
		const QString &scheme = url.scheme();
		if(scheme == "qthelp")
			return QVariant(helpEngine_->fileData(url));
		else
			return QTextBrowser::loadResource(type, url);
	}
	bool navigateToKeyword(const QString &keywordId)
	{
		auto links = helpEngine_->linksForIdentifier(keywordId);
		if(links.count() > 0)
		{
			setSource(links.begin().value());
			return true;
		}
		return false;
	}
	void setSource(const QUrl &src)
	{
		const QUrl &prevSrc = source();
		if(src == prevSrc) return;

		const QString &scheme = src.scheme();
		if(scheme != "qthelp")
		{
			QDesktopServices::openUrl(src);
			return;
		}

		QTextBrowser::setSource(src);
	}

	bool findText(const QString &text, QTextDocument::FindFlags flags, bool incremental, bool fromSearch)
	{
		QTextDocument *doc = document();
		QTextCursor cursor = textCursor();
		if (!doc || cursor.isNull())
			return false;

		const int position = cursor.selectionStart();
		if (incremental)
			cursor.setPosition(position);

		QTextCursor found = doc->find(text, cursor, flags);
		if (found.isNull()) {
			if ((flags & QTextDocument::FindBackward) == 0)
				cursor.movePosition(QTextCursor::Start);
			else
				cursor.movePosition(QTextCursor::End);
			found = doc->find(text, cursor, flags);
		}

		if (fromSearch) {
			cursor.beginEditBlock();
			viewport()->setUpdatesEnabled(false);

			QTextCharFormat marker;
			marker.setForeground(Qt::red);
			marker.setBackground(Qt::yellow);
			cursor.movePosition(QTextCursor::Start);
			setTextCursor(cursor);

			while (find(text)) {
				QTextCursor hit = textCursor();
				hit.mergeCharFormat(marker);
			}

			viewport()->setUpdatesEnabled(true);
			cursor.endEditBlock();
		}

		bool cursorIsNull = found.isNull();
		if (cursorIsNull) {
			found = textCursor();
			found.setPosition(position);
		}
		setTextCursor(found);
		return !cursorIsNull;
	}

private:
	QHelpEngine *helpEngine_;
};

HelpBrowser::HelpBrowser(const QString &fileName) 
	: QMainWindow(NULL), fileName_(fileName), contentsCreated_(false)
{
	settings_file();

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	QToolBar *toolBar = new QToolBar(this);
	QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

	QIcon icon = QIcon(":/images/back.png");
	icon.addPixmap(QPixmap(":/images/back_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/back_disabled.png"), QIcon::Disabled, QIcon::Off);
	backButton_ = new QAction(icon, QString::fromUtf8(language[lsBack].c_str()), this);
	backButton_->setEnabled(false);

	icon = QIcon(":/images/forward.png");
	icon.addPixmap(QPixmap(":/images/forward_hover.png"), QIcon::Active, QIcon::Off);
	icon.addPixmap(QPixmap(":/images/forward_disabled.png"), QIcon::Disabled, QIcon::Off);
	fwdButton_ = new QAction(icon, QString::fromUtf8(language[lsForward].c_str()), this);
	fwdButton_->setEnabled(false);

	searchBox_ = new SearchLineEdit(this);
	searchBox_->setFrame(false);
	searchBox_->setFixedWidth(200);
	searchBox_->setPlaceholderText(QString::fromUtf8(language[lsSearch].c_str()));
	connect(searchBox_, SIGNAL(textChanged(const QString &)), this, SLOT(searchBoxChanged()));
	connect(searchBox_, SIGNAL(returnPressed()), this, SLOT(searchBoxChanged()));

	toolBar->setMovable(false);
	toolBar->setIconSize(QSize(22, 22));
	toolBar->addSeparator();
	toolBar->addAction(backButton_);
	toolBar->addAction(fwdButton_);
	toolBar->addSeparator();
	QWidget *spacer = new QWidget(this);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	toolBar->addWidget(spacer);
	toolBar->addWidget(searchBox_);

	QVBoxLayout *vLayout = new QVBoxLayout(this);
	vLayout->setContentsMargins(0, 0, 0, 0);
	vLayout->setSpacing(0);

	helpEngine_ = new QHelpEngine(fileName_, splitter);
	connect(helpEngine_->contentModel(), SIGNAL(contentsCreated()), this, SLOT(contentsCreated()));
	if (!helpEngine_->setupData()) {
		contentsCreated_ = true;
		MessageDialog::critical(this, QString("Can not open file \"%1\"").arg(fileName_), QMessageBox::Ok);
	}

	contentModel_ = new HelpContentModel(helpEngine_, this);
	resultModel_ = new HelpResultModel(helpEngine_, this);
	
	contentWidget_ = new TreeView(this);
	contentWidget_->setObjectName("project");
	contentWidget_->setFrameShape(QFrame::NoFrame);
	contentWidget_->header()->setObjectName("project");
	contentWidget_->setUniformRowHeights(true);
	contentWidget_->setModel(contentModel_);

	resultWidget_ = new TreeView(this);
	resultWidget_->setObjectName("project");
	resultWidget_->setFrameShape(QFrame::NoFrame);
	resultWidget_->header()->setObjectName("project");
	resultWidget_->setUniformRowHeights(true);
	resultWidget_->setModel(resultModel_);

	QToolButton *closeButton = new QToolButton(this);
	closeButton->setObjectName("cancel");
	closeButton->setToolTip(QString::fromUtf8(language[lsClose].c_str()));

	QHBoxLayout *layout = new QHBoxLayout;
	layout->setContentsMargins(5, 0, 5, 0);
	layout->setSpacing(0);
	layout->addWidget(closeButton, 0, Qt::AlignRight);
	resultWidget_->header()->setLayout(layout);

	tabWidget_ = new QStackedWidget;
	tabWidget_->addWidget(contentWidget_);
	tabWidget_->addWidget(resultWidget_);

	splitter->addWidget(tabWidget_);
	helpBrowser_ = new TextBrowser(helpEngine_);
	helpBrowser_->setFrameShape(QFrame::NoFrame);
	
	QWidget *right = new QWidget();
	vLayout->addWidget(helpBrowser_);
	findWidget_ = new FindWidget(this);
	vLayout->addWidget(findWidget_);
	findWidget_->hide();
	right->setLayout(vLayout);
	splitter->addWidget(right); 
	splitter->setStretchFactor(1, 1);

	mainLayout->addWidget(toolBar);
	mainLayout->addWidget(splitter);

	QWidget *window = new QWidget();
	window->setLayout(mainLayout);
	setCentralWidget(window);

	RemoteControl *rc = new RemoteControl(this);
	connect(helpBrowser_, SIGNAL(sourceChanged(const QUrl &)), this, SLOT(syncTree(const QUrl &)));
	connect(rc, SIGNAL(handleNavigateToKeywordCommand(const QString &)), this, SLOT(handleNavigateToKeywordCommand(const QString &)));

	connect(contentWidget_, SIGNAL(clicked(const QModelIndex &)), this, SLOT(contentWidgetClicked(const QModelIndex &)));
	connect(resultWidget_, SIGNAL(clicked(const QModelIndex &)), this, SLOT(resultWidgetClicked(const QModelIndex &)));
	connect(backButton_, SIGNAL(triggered()), helpBrowser_, SLOT(backward()));
	connect(fwdButton_, SIGNAL(triggered()), helpBrowser_, SLOT(forward()));
	connect(helpBrowser_, SIGNAL(backwardAvailable(bool)), backButton_, SLOT(setEnabled(bool)));
	connect(helpBrowser_, SIGNAL(forwardAvailable(bool)), fwdButton_, SLOT(setEnabled(bool)));
	connect(helpBrowser_, SIGNAL(sourceChanged(const QUrl &)), this, SLOT(highlightSearchTerms()));
	connect(resultModel_, SIGNAL(finished(int)), this, SLOT(searchingFinished()));
	connect(closeButton, SIGNAL(clicked(bool)), this, SLOT(resultCloseClicked()));

	connect(findWidget_, SIGNAL(findNext()), this, SLOT(findNext()));
	connect(findWidget_, SIGNAL(findPrevious()), this, SLOT(findPrevious()));
	connect(findWidget_, SIGNAL(find(QString, bool, bool)), this,
		SLOT(find(QString, bool, bool)));
	connect(findWidget_, SIGNAL(escapePressed()), helpBrowser_, SLOT(setFocus()));

	resize(890, 440);
}

void HelpBrowser::handleNavigateToKeywordCommand(const QString &arg)
{
	if(arg.length()) navigateToKeyword(arg);
	raise();
	activateWindow();
}

void HelpBrowser::findNext()
{
	find(findWidget_->text(), true, false);
}

void HelpBrowser::findPrevious()
{
	find(findWidget_->text(), false, false);
}

void HelpBrowser::find(const QString &ttf, bool forward, bool incremental)
{
	bool found = false;
	QTextDocument::FindFlags flags = (QTextDocument::FindFlags)0;
	if (!forward)
		flags |= QTextDocument::FindBackward;
	if (findWidget_->caseSensitive())
		flags |= QTextDocument::FindCaseSensitively;
	found = helpBrowser_->findText(ttf, flags, incremental, false);

	if (!found && ttf.isEmpty())
		found = true;   // the line edit is empty, no need to mark it red...

	if (!findWidget_->isVisible())
		findWidget_->show();
	findWidget_->setPalette(found);
}

void HelpBrowser::highlightSearchTerms()
{
	if (tabWidget_->currentIndex() != 1)
		return;

	QHelpSearchEngine *searchEngine = helpEngine_->searchEngine();
	QList<QHelpSearchQuery> queryList = searchEngine->query();

	QStringList terms;
	foreach (const QHelpSearchQuery &query, queryList) {
		switch (query.fieldName) 
		{
			default: break;
		case QHelpSearchQuery::ALL:
		case QHelpSearchQuery::PHRASE:
		case QHelpSearchQuery::DEFAULT:
		case QHelpSearchQuery::ATLEAST:
			foreach (QString term, query.wordList)
				terms.append(term.remove(QLatin1Char('"')));
		}
	}

	foreach (const QString& term, terms)
		helpBrowser_->findText(term, 0, false, true);
}

void HelpBrowser::keyPressEvent(QKeyEvent *e)
{
	if ((e->modifiers() & Qt::ControlModifier) && e->key()==Qt::Key_F) {
		if (!findWidget_->isVisible()) {
			findWidget_->showAndClear(searchBox_->text());
		} else {
			findWidget_->show();
		}
	} else {
		QWidget::keyPressEvent(e);
	}
}

void HelpBrowser::searchBoxChanged()
{
	QString term = searchBox_->text();
	if(term.length() < 3) {
		tabWidget_->setCurrentIndex(0);
		return;
	}

	resultModel_->search(term);
}

void HelpBrowser::searchingFinished()
{
	tabWidget_->setCurrentIndex(1);
}

void HelpBrowser::syncTree(const QUrl &src)
{
	setWindowTitle(QString("%1: %2").arg(QString::fromUtf8(language[lsHelp].c_str())).arg(helpBrowser_->documentTitle()));

	// Synchronize with help content widget
	QModelIndex indexOfSource = contentModel_->indexOf(src);
	if (contentWidget_->currentIndex() != indexOfSource)
		contentWidget_->setCurrentIndex(indexOfSource);

	if(indexOfSource.isValid())
		contentWidget_->expand(indexOfSource);
	else
		contentWidget_->expandAll();
}

static QString helpFileName(const QString &lang)
{
	return QString("%1/Help/%2.qhc").arg(QCoreApplication::applicationDirPath()).arg(lang);
}

void HelpBrowser::showTopic(const QString &keywordId)
{
	QString fileName = helpFileName(QString::fromUtf8(settings_file().language().c_str()));
	if (!QFileInfo(fileName).exists())
		fileName = helpFileName("en");

	bool bOldProcess = true;
	if (help_browser_process_ == NULL || help_browser_process_->state() == QProcess::NotRunning || help_browser_process_->fileName_ != fileName) {
		delete help_browser_process_;
		help_browser_process_ = new QHelpBrowserProcess(qApp, fileName);
		if (!help_browser_process_->waitForStarted())
		{
			MessageDialog::critical(NULL, QString("Can not launch help system"), QMessageBox::Ok);
			delete help_browser_process_;
			help_browser_process_ = NULL;
			return;
		}
		bOldProcess = false;
	}
	QByteArray ba;
	ba += "navigateToKeyword#";
	ba += (keywordId.length() || bOldProcess) ? keywordId : QString("default");
	ba += ";\n";
	help_browser_process_->write(ba);
}

void HelpBrowser::navigateToKeyword(const QString &keywordId)
{
	for (int i = 0; i < 50 && !contentsCreated_; i++)
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	if(!helpBrowser_->navigateToKeyword(keywordId))
		MessageDialog::critical(this, QString("Can not find keyword \"%1\" for \"%2\"").arg(keywordId, fileName_), QMessageBox::Ok);
}

void HelpBrowser::contentWidgetClicked(const QModelIndex &index)
{
	QHelpContentItem *item = contentModel_->contentItemAt(index);
	if (!item)
		return;

	QUrl url = item->url();
	if (url.isValid())
		helpBrowser_->setSource(url);
}

void HelpBrowser::resultWidgetClicked(const QModelIndex &index)
{
	HelpResultItem *item = resultModel_->itemAt(index);
	if (!item)
		return;

	QUrl url = item->url();
	if (url.isValid())
		helpBrowser_->setSource(url);
}

void HelpBrowser::pathClicked(const QString &link)
{
	helpBrowser_->setSource(link);
}

void HelpBrowser::resultCloseClicked()
{
	searchBox_->clear();
}

void HelpBrowser::contentsCreated()
{
	contentsCreated_ = true;
}

/**
 * QHelpBrowserProcess
 */

QHelpBrowserProcess *HelpBrowser::help_browser_process_;

QHelpBrowserProcess::QHelpBrowserProcess(QObject *parent, const QString &fileName) : QProcess(parent), fileName_(fileName)
{
	QStringList args;
	args << "--help" << fileName;
	start(QCoreApplication::applicationFilePath(), args);
}

QHelpBrowserProcess::~QHelpBrowserProcess()
{
	if(state() != NotRunning)
		kill();
}