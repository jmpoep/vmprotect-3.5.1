#ifndef WIDGETS_H
#define WIDGETS_H

class HelpContentsKeySequence : public QKeySequence
{
public:
	// HelpContents = Ctrl+? in mac os X, is not working http://www.qtcentre.org/threads/57829-Shortcut-QKeySequence-HelpContents-and-Modal-Dialog
	HelpContentsKeySequence() : QKeySequence(QString("F1")) {}
};

class LineEditHelper : public QLineEdit
{
	Q_OBJECT
public:
	LineEditHelper(QWidget * parent = 0);
signals:
	void doubleClicked();
protected:
	void mouseDoubleClickEvent(QMouseEvent *event);
};

class EnumEdit : public QComboBox
{
	Q_OBJECT
public:
	EnumEdit(QWidget * parent, const QStringList &items);
signals:
	void dropDown();
private slots:
	void editDoubleClicked();
protected:
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void showPopup();
};

class BoolEdit : public EnumEdit
{
	Q_OBJECT
public:
	BoolEdit(QWidget * parent = 0);
	void setChecked(bool value) { setCurrentIndex(value ? 1 : 0); }
	bool checked() const { return currentIndex() == 1; }
signals:
	void toggled(bool value);
private slots:
	void slotCurrentIndexChanged(int value);
};

class StringListEdit : public QPlainTextEdit
{
	Q_OBJECT
public:
	StringListEdit(QWidget * parent = 0);
private slots:
	void slotTextChanged();
signals:
	void textChanged(const QString &string);
};

class LineEdit : public QLineEdit
{
public:
	LineEdit(QWidget *parent = NULL)
		: QLineEdit(parent)
	{
#ifdef __APPLE__
		setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
	}
};

class ButtonLineEdit : public LineEdit
{
	Q_OBJECT
public:
	ButtonLineEdit(QWidget *parent = 0);
protected:
	void resizeEvent(QResizeEvent *);
	void mouseDoubleClickEvent(QMouseEvent *e);
	QToolButton *button() const { return button_; }
protected slots:
	virtual void buttonClicked() { }
private:
	QToolButton *button_;
};

class SearchLineEdit : public ButtonLineEdit
{
    Q_OBJECT
public:
    SearchLineEdit(QWidget *parent = 0);
protected:
	void mouseDoubleClickEvent(QMouseEvent *e);
protected slots:
	virtual void buttonClicked();
private slots:
    void updateButton(const QString &text);
};

class FileDialog
{
public:
	static QString getOpenFileName(QWidget *parent = 0,
		const QString &caption = QString(),
		const QString &dir = QString(),
		const QString &filter = QString(),
		QString *selectedFilter = 0,
		QFileDialog::Options options = 0);

	static QString getSaveFileName(QWidget *parent = 0,
		const QString &caption = QString(),
		const QString &dir = QString(),
		const QString &filter = QString(),
		QString *selectedFilter = 0,
		QFileDialog::Options options = 0);

	static QString defaultPath() { return defaultPath_; }
private:
	static QString defaultPath_;
};

class FileNameEdit : public ButtonLineEdit
{
	Q_OBJECT
public:
	FileNameEdit(QWidget *parent = 0);
	void setFilter(const QString &filter) { filter_ = filter; }
	void setRelativePath(const QString &relativePath) { relativePath_ = relativePath; }
	void setSaveMode(bool value) { saveMode_ = value; }
signals:
	void fileOpened(const QString &value);
protected slots:
	virtual void buttonClicked();

private:

	virtual bool event(QEvent *e);
	QString filter_;
	QString relativePath_;
	bool saveMode_;
};

class WatermarkEdit : public ButtonLineEdit
{
    Q_OBJECT
public:
    WatermarkEdit(QWidget *parent = 0);
protected slots:
	virtual void buttonClicked();
};

class ScriptEdit : public Scintilla::ScintillaEditBase
{
	Q_OBJECT
public:
	ScriptEdit(QWidget *parent = 0);
	void setText(const QString &text);
	QString text() const;
	bool canUndo() const;
	bool canRedo() const;
	bool canPaste() const;
	bool hasSelection() const;
	void del();
	QPoint currentPos() const;
	bool getOverType() const;
	void setOverType(bool value) const;
	void maintainIndentation(int ch);
	void hilightView();
signals:
	void updateUI();
public slots:
	void undo();
	void redo();
	void copy();
	void cut();
	void paste();
};

class TabWidget : public QTabWidget
{
	Q_OBJECT
public:
	explicit TabWidget(QWidget *parent = 0)
		: QTabWidget(parent)
	{
#ifdef __APPLE__
		setAttribute(Qt::WA_LayoutUsesWidgetRect, false);
#endif
	}
	QTabBar* tabBar() const { return QTabWidget::tabBar(); }
signals:
	void resized();
protected:
	virtual void resizeEvent(QResizeEvent *e) 
	{
		QTabWidget::resizeEvent(e);
		emit resized();
	}
};

#define MONOSPACE_FONT_FAMILY "Courier New"

class BinEditor : public QAbstractScrollArea
{
	Q_OBJECT
public:
	BinEditor(QWidget *parent = 0);
	~BinEditor();
	void setData(const QByteArray &data);
	QByteArray data() const { return data_; }
	QString value() const;
	void setValue(const QString &value);
	enum MoveMode {
		MoveAnchor,
		KeepAnchor
	};
	void setCursorPosition(int pos, MoveMode moveMode = MoveAnchor);
	void setOverwriteMode(bool value);
	bool overwriteMode() { return overwriteMode_; }
	int maxLength() const { return maxLength_; }
	void setMaxLength(int value) { maxLength_ = value; }
	void setMaskAllowed(bool value) { maskAllowed_ = value; }
	bool isEmpty() const { return data_.isEmpty(); }
	bool isUndoAvailable() const;
	bool isRedoAvailable() const;
	bool hasSelectedText() const;
public slots:
	void selectAll();
	void copy();
	void cut();
	bool deleteSelection();
	void paste();
	void undo();
	void redo();
signals:
	void dataChanged();
protected:
	struct CheckPoint { 
		CheckPoint(int cp = 0, int ap = 0, const QString &t = "") : cursorPosition(cp), anchorPosition(ap), text(t) {}
		int cursorPosition, anchorPosition;
		QString text;
	};
	CheckPoint CurrentCheckPoint() const { return CheckPoint(cursorPosition_, anchorPosition_, value()); }
	class CheckPointMaker
	{
	public:
		CheckPointMaker(BinEditor *obj) : obj_(obj), original_cp_(obj->CurrentCheckPoint()) {}
		~CheckPointMaker()
		{
			if (obj_->value() != original_cp_.text)
				obj_->makeUndoCheckpoint(original_cp_);
		}
	private:
		BinEditor *obj_;
		CheckPoint original_cp_;
	};
	friend class CheckPointMaker;
	void makeUndoCheckpoint(const CheckPoint &cp);
	void restoreUndoCheckpoint(const CheckPoint &cp);

	void paintEvent(QPaintEvent *e);
	void resizeEvent(QResizeEvent *);
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void focusInEvent(QFocusEvent *);
	void focusOutEvent(QFocusEvent *);
	void timerEvent(QTimerEvent *);
	void keyPressEvent(QKeyEvent *e);
	bool event(QEvent *e);
	void contextMenuEvent(QContextMenuEvent *);
private:
	void init();
	int posAt(const QPoint &pos) const;
	void updateLines();
	void updateLines(int fromPosition, int toPosition);
	void setBlinkingCursorEnabled(bool enable);
	void ensureCursorVisible();
	QRect cursorRect() const;
	void insert(int index, char c);
	void insert(int index, const QString &str);
	void replace(int index, char c, char mask = 0);
	void remove(int index, int len = 1);
	bool inTextArea(const QPoint &pos) const;
	void changed();
	bool deleteSelectionInternal();
	QMenu *createStandardContextMenu();

	int bytesPerLine_;
	int numVisibleLines_;
	int lineHeight_;
	int charWidth_;
	int numLines_;
	QByteArray data_;
	QByteArray mask_;
	int margin_;
	int addressWidth_;
	int columnWidth_;
	int textWidth_;
	int cursorPosition_;
	int anchorPosition_;
	bool cursorVisible_;
	QBasicTimer cursorBlinkTimer_;
	bool overwriteMode_;
	bool hexCursor_;
	int maxLength_;
	bool maskAllowed_;

	QStack<CheckPoint> m_undoStack, m_redoStack; 
};

namespace Qt {
	namespace Vmp {
		enum ItemDataRole {  StaticTextRole = Qt::UserRole + 1, StaticColorRole };
	};
}

class TreeViewItemDelegate : public QStyledItemDelegate
{
public:
	TreeViewItemDelegate(QObject *parent = NULL);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setEditorData(QWidget *editor, const QModelIndex &index) const;
};

template<class T>
class TTemplateView : public T
{
public:
	TTemplateView(QWidget *parent = NULL);
	void copy();
private:
	void getIndexTexts(const QModelIndex &idx,  QString *selected_text_as_html, QString *selected_text);
};

template<class T>
TTemplateView<T>::TTemplateView(QWidget *parent) : T(parent)
{
#ifdef __APPLE__
	T::setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
	T::setItemDelegate(new TreeViewItemDelegate(this));
}

template<class T>
void TTemplateView<T>::getIndexTexts(const QModelIndex &idx, QString *selected_text_as_html, QString *selected_text)
{
	QString text = idx.data().toString();
	QString staticText = idx.data(Qt::Vmp::StaticTextRole).toString().replace('\x1', QChar(0x2190)).replace('\x2', QChar(0x2191)).replace('\x3', QChar(0x2192)).replace('\x4', QChar(0x2193));
	/*
	if (text[0] < 5) {
	// branch symbols
	opt.text = QChar(0x2190 + text[0].unicode() - 1);
	*/
	QColor foreColorOrBlack(Qt::black);
	if (staticText.length() > 0)
	{
		auto mdl = T::model();
		assert(mdl);
		if (!mdl)
			return;
		QVariant foreColor = mdl->data(idx, Qt::Vmp::StaticColorRole);
		if (foreColor.canConvert<QColor>())
		{
			foreColorOrBlack = qvariant_cast<QColor>(foreColor);
		}
		if (foreColorOrBlack != QColor(Qt::black))
		{
			selected_text_as_html->append(QString("%1 <SPAN style='color: rgb(%2,%3,%4);'>%5</SPAN>").arg(text.toHtmlEscaped()).arg(
				foreColorOrBlack.red()).arg(foreColorOrBlack.green()).arg(foreColorOrBlack.blue()).arg(
					staticText.toHtmlEscaped()));
		}
		else
		{
			selected_text_as_html->append(QString("%1 %2").arg(text.toHtmlEscaped()).arg(staticText.toHtmlEscaped()));
		}
		text += QString(" %1").arg(staticText);
	}
	else
	{
		selected_text_as_html->append(text.toHtmlEscaped());
	}
	selected_text->append(text);
}

template<class T>
void TTemplateView<T>::copy()
{
	QModelIndexList indexes = T::selectionModel()->selectedIndexes();

	if (indexes.size() < 1)
		return;

	std::sort(indexes.begin(), indexes.end());
	QString selected_text_as_html;
	QString selected_text;
	selected_text_as_html.prepend("<html><style>br{mso-data-placement:same-cell;}</style><table>");

	QModelIndex previous;
	for (int i = 0; i < indexes.size(); i++) {
		QModelIndex current = indexes[i];
		if (current.row() != previous.row()) {
			if (previous.isValid()) {
				selected_text_as_html.append("</tr>");
				selected_text.append(QLatin1Char('\n'));
			}
			selected_text_as_html.append("<tr>");
		}
		else {
			selected_text.append(QLatin1Char('\t'));
		}

		selected_text_as_html.append("<td>");
		getIndexTexts(current, &selected_text_as_html, &selected_text);
		selected_text_as_html.append("</td>");

		previous = current;
	}

	if (previous.isValid())
		selected_text_as_html.append("</tr>");
	selected_text_as_html.append("</table></html>");

	QMimeData * md = new QMimeData;
	md->setHtml(selected_text_as_html);
	md->setText(selected_text);
	QApplication::clipboard()->setMimeData(md);
}

typedef TTemplateView<QTreeView> TreeView;
typedef TTemplateView<QTableView> TableView;

class LogTreeView : public TreeView
{
public:
	LogTreeView(QWidget *parent = NULL) : TreeView(parent) {}

private:
	/*virtual*/ QStyleOptionViewItem viewOptions() const;
};

class PushButton : public QPushButton
{
public:
	PushButton(QWidget *parent = NULL)
		: QPushButton(parent)
	{
#ifdef __APPLE__
		setAttribute(Qt::WA_LayoutUsesWidgetRect, true);
#endif
	}
	PushButton(const QString &text, QWidget *parent = NULL)
		: QPushButton(text, parent)
	{
#ifdef __APPLE__
		setAttribute(Qt::WA_LayoutUsesWidgetRect, true);
#endif
	}
};

class FindWidget : public QFrame
{
	Q_OBJECT
public:
	FindWidget(QWidget *parent = 0);
	~FindWidget();

	void show();
	void showAndClear(const QString &term);

	QString text() const;
	bool caseSensitive() const;

	void setPalette(bool found);
	void setTextWrappedVisible(bool visible);

signals:
	void findNext();
	void findPrevious();
	void escapePressed();
	void find(const QString &text, bool forward, bool incremental);

private slots:
	void updateButtons();
	void textChanged(const QString &text);

private:
	bool eventFilter(QObject *object, QEvent *e);

private:
	QLineEdit *editFind;
	QCheckBox *checkCase;
	QLabel *labelWrapped;
	QToolButton *toolNext;
	QToolButton *toolClose;
	QToolButton *toolPrevious;
};

class ElidedAction : public QAction
{
private:
	QString fullText_;
public:
	ElidedAction(const QString &text, QObject *parent) : QAction(text, parent) {}
	ElidedAction(QObject *parent) : QAction(parent) {}

	void setFullText(const QString &text);
	const QString &getFullText() { return fullText_; }
};

class ToolButtonElided : public QToolButton
{
private:
	QString fullText_;
public:
	explicit ToolButtonElided(QWidget *parent = NULL) : QToolButton(parent) {}
	void resizeEvent(QResizeEvent *event);
	void setText(const QString &text);

	void elideText(const QSize &widgetSize);
};

class Label : public QLabel
{
	Q_OBJECT
public:
	Label(QWidget * parent = 0);
signals:
	void doubleClicked();
protected:
	void mouseDoubleClickEvent(QMouseEvent *event);
};

class DisasmView : public TableView
{
	Q_OBJECT
public:
	DisasmView(QWidget * parent = 0);
protected:
	void scrollContentsBy(int dx, int dy) override;
};

#endif