#include "../core/objects.h"
#include "../core/lang.h"
#include "widgets.h"
#include "moc/moc_widgets.cc"
#include "models.h"
#include "watermarks_window.h"
#include "application.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

/**
 * SearchLineEdit
 */

SearchLineEdit::SearchLineEdit(QWidget *parent)
    : ButtonLineEdit(parent)
{
	button()->setIcon(QIcon(":/images/search.png"));
	setPlaceholderText(language[lsSearch].c_str());

	connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateButton(const QString&)));
}

void SearchLineEdit::updateButton(const QString& text)
{
	QIcon icon = text.isEmpty() ? QIcon(":/images/search.png") : QIcon(":/images/cancel.png");
	button()->setIcon(icon);
}

void SearchLineEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
	LineEdit::mouseDoubleClickEvent(e);
}

void SearchLineEdit::buttonClicked()
{
	clear();
}

/**
 * LineEditHelper
 */

LineEditHelper::LineEditHelper(QWidget * parent)
	: QLineEdit(parent)
{

}

void LineEditHelper::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		emit doubleClicked();
}

/**
 * EnumEdit
 */

EnumEdit::EnumEdit(QWidget * parent, const QStringList &items)
	: QComboBox(parent)
{
	setInsertPolicy(QComboBox::NoInsert);
	LineEditHelper *edit = new LineEditHelper(this);
	setLineEdit(edit);
	connect(edit, SIGNAL(doubleClicked()), this, SLOT(editDoubleClicked()));

	addItems(items);
}

void EnumEdit::keyPressEvent(QKeyEvent *event)
{
	QString text = event->text();
	if (!text.isEmpty()) {
		int i = findText(text.left(1), Qt::MatchStartsWith);
		if (i != -1) {
			setCurrentIndex(i);
			lineEdit()->selectAll();
			return;
		}
	}
	QComboBox::keyPressEvent(event);
}

void EnumEdit::showPopup() 
{
	emit dropDown();
	QComboBox::showPopup();
}

void EnumEdit::editDoubleClicked()
{
	int i = currentIndex() + 1;
	if (i >= count())
		i = 0;
	setCurrentIndex(i);
}

/**
 * BoolEdit
 */

BoolEdit::BoolEdit(QWidget * parent)
	: EnumEdit(parent, QStringList() << QString::fromUtf8(language[lsNo].c_str()) << QString::fromUtf8(language[lsYes].c_str()))
{
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentIndexChanged(int)));
}

void BoolEdit::slotCurrentIndexChanged(int value)
{
	emit toggled(value == 1);
}

/**
 * StringListEdit
 */

StringListEdit::StringListEdit(QWidget * parent) 
	: QPlainTextEdit(parent)
{
	connect(this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
}

void StringListEdit::slotTextChanged()
{
	emit textChanged(toPlainText());
}

/**
 * ButtonLineEdit
 */

ButtonLineEdit::ButtonLineEdit(QWidget *parent)
    : LineEdit(parent)
{
    button_ = new QToolButton(this);
	button_->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button_->setIcon(QIcon(":/images/browse.png"));
	QSize sz = sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
	button_->resize(QSize(sz.height() - frameWidth * 2, sz.height() - frameWidth * 2));
    button_->setCursor(Qt::ArrowCursor);
    connect(button_, SIGNAL(clicked()), this, SLOT(buttonClicked()));
}

void ButtonLineEdit::resizeEvent(QResizeEvent *)
{
    QSize sz = button_->size();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
    button_->move(rect().right() - sz.width(), rect().top() + frameWidth);
}

void ButtonLineEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton) {
		e->accept();
		buttonClicked();
		return;
	}

	LineEdit::mouseDoubleClickEvent(e);
}

/**
 * FileDialog
 */

QString FileDialog::defaultPath_ = QString();

QString FileDialog::getOpenFileName(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options)
{
	QString result = QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options);
	if (!result.isEmpty()) {
		result = QDir::toNativeSeparators(result);
		defaultPath_ = QFileInfo(result).path();
	}
	return result;
}

QString FileDialog::getSaveFileName(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options)
{
	QString result = QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
	if (!result.isEmpty()) {
		result = QDir::toNativeSeparators(result);
		defaultPath_ = QFileInfo(result).path();
	}
	return result;
}

/**
 * FileNameEdit
 */

FileNameEdit::FileNameEdit(QWidget *parent)
    : ButtonLineEdit(parent), saveMode_(false)
{

}

void FileNameEdit::buttonClicked()
{
	QDir dir(relativePath_);
	
	// to prevent self-destroy while QFileDialog-ing
	QIntValidator invalid(1, 0);
	setValidator(&invalid);
	QString fileName = saveMode_ ? FileDialog::getSaveFileName(this, QString::fromUtf8(language[lsSave].c_str()), dir.absoluteFilePath(text()), filter_)
								: FileDialog::getOpenFileName(this, QString::fromUtf8(language[lsOpen].c_str()), dir.absoluteFilePath(text()), filter_);
	setValidator(NULL);
	if (fileName.isEmpty())
		return;

	if (!relativePath_.isEmpty()) {
		QString tmp = dir.relativeFilePath(fileName);
		if (tmp.mid(0, 2) != "..")
			fileName = tmp;
	}
	setText(fileName);
}

bool FileNameEdit::event(QEvent *e)
{
	if(e->type() == QEvent::DeferredDelete && validator())
	{
		// to prevent self-destroy while QFileDialog-ing
		return true;
	}
	return ButtonLineEdit::event(e);
}

/**
 * WatermarkEdit
 */

WatermarkEdit::WatermarkEdit(QWidget *parent)
    : ButtonLineEdit(parent)
{

}

void WatermarkEdit::buttonClicked()
{
	WatermarksWindow dialog(true, this);
	QString watermarkName = text();
	if (!watermarkName.isEmpty())
		dialog.setWatermarkName(watermarkName);
	if (dialog.exec() == QDialog::Accepted)
		setText(dialog.watermarkName());
}

/**
 * ScriptEdit
 */

static const char *keywords[] = {
	"sand break do else elseif end false for function goto if in local nil not or repeat return then true until while",
	"_ENV _G _VERSION assert collectgarbage dofile error getfenv getmetatable ipairs load loadfile loadstring module next pairs pcall print rawequal rawget rawlen rawset require select setfenv setmetatable tonumber tostring type unpack xpcall string table math bit32 coroutine io os debug package __index __newindex __call __add __sub __mul __div __mod __pow __unm __concat __len __eq __lt __le __gc __mode",
	"byte char dump find format gmatch gsub len lower match rep reverse sub upper abs acos asin atan atan2 ceil cos cosh deg exp floor fmod frexp ldexp log log10 max min modf pow rad random randomseed sin sinh sqrt tan tanh arshift band bnot bor btest bxor extract lrotate lshift replace rrotate rshift shift string.byte string.char string.dump string.find string.format string.gmatch string.gsub string.len string.lower string.match string.rep string.reverse string.sub string.upper table.concat table.insert table.maxn table.pack table.remove table.sort table.unpack math.abs math.acos math.asin math.atan math.atan2 math.ceil math.cos math.cosh math.deg math.exp math.floor math.fmod math.frexp math.huge math.ldexp math.log math.log10 math.max math.min math.modf math.pi math.pow math.rad math.random math.randomseed math.sin math.sinh math.sqrt math.tan math.tanh bit32.arshift bit32.band bit32.bnot bit32.bor bit32.btest bit32.bxor bit32.extract bit32.lrotate bit32.lshift bit32.replace bit32.rrotate bit32.rshift",
	"close flush lines read seek setvbuf write clock date difftime execute exit getenv remove rename setlocale time tmpname coroutine.create coroutine.resume coroutine.running coroutine.status coroutine.wrap coroutine.yield io.close io.flush io.input io.lines io.open io.output io.popen io.read io.tmpfile io.type io.write io.stderr io.stdin io.stdout os.clock os.date os.difftime os.execute os.exit os.getenv os.remove os.rename os.setlocale os.time os.tmpname debug.debug debug.getfenv debug.gethook debug.getinfo debug.getlocal debug.getmetatable debug.getregistry debug.getupvalue debug.getuservalue debug.setfenv debug.sethook debug.setlocal debug.setmetatable debug.setupvalue debug.setuservalue debug.traceback debug.upvalueid debug.upvaluejoin package.cpath package.loaded package.loaders package.loadlib package.path package.preload package.seeall",
};

ScriptEdit::ScriptEdit(QWidget *parent)
	: ScintillaEditBase(parent)
{
	setFrameShape(StyledPanel);
	setFrameShadow(Sunken);

	send(SCI_SETCODEPAGE, SC_CP_UTF8);
	send(SCI_SETLEXER, SCLEX_LUA);

	send(SCI_SETMARGINWIDTHN, 0, 40);
	send(SCI_SETMARGINWIDTHN, 1, 0);
	send(SCI_SETMARGINWIDTHN, 2, 18);
	send(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
	send(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	send(SCI_SETMARGINSENSITIVEN, 2, 1);
	
	send(SCI_STYLESETFORE, STYLE_LINENUMBER, 0x808080);
	send(SCI_STYLESETBACK, STYLE_LINENUMBER, 0xFFFFFF);

	send(SCI_SETFOLDMARGINCOLOUR, true, 0xFFFFFF);
	send(SCI_SETFOLDMARGINHICOLOUR, true, 0xFFFFFF);//xE9E9E9);
	
	send(SCI_SETPROPERTY, (sptr_t)"fold", (sptr_t)"1");
	send(SCI_SETPROPERTY, (sptr_t)"fold.compact", (sptr_t)"0");
	send(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED);
	send(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CLICK | SC_AUTOMATICFOLD_CHANGE);

	send(SCI_SETSCROLLWIDTHTRACKING, true);
	send(SCI_SETSCROLLWIDTH, 1);

	send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
	send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
	send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
	send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
	send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

	send(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPEN, 0xFFFFFF);
	send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPEN, 0x808080);
	send(SCI_MARKERSETFORE, SC_MARKNUM_FOLDER, 0xFFFFFF);
	send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDER, 0x808080);
	send(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERSUB, 0xFFFFFF);
	send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERSUB, 0x808080);
	send(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERTAIL, 0xFFFFFF);
	send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERTAIL, 0x808080);
	send(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEREND, 0xFFFFFF);
	send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEREND, 0x808080);
	send(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPENMID, 0xFFFFFF);
	send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPENMID, 0x808080);
	send(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERMIDTAIL, 0xFFFFFF);
	send(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERMIDTAIL, 0x808080);

	for (size_t style = 0; style < _countof(keywords); style++) {
		send(SCI_SETKEYWORDS, style, reinterpret_cast<sptr_t>(keywords[style]));
	}
	
	send(SCI_STYLESETFONT, STYLE_DEFAULT, (sptr_t)MONOSPACE_FONT_FAMILY);
	int fontSize = int(1100 * Application::devicePixelRatio() + 0.5);
	send(SCI_STYLESETSIZEFRACTIONAL, STYLE_DEFAULT, fontSize);

	send(SCI_STYLESETFORE, SCE_LUA_COMMENT, 0x008000);
	send(SCI_STYLESETFORE, SCE_LUA_COMMENTLINE, 0x008000);
	send(SCI_STYLESETFORE, SCE_LUA_COMMENTDOC, 0x808000);
	send(SCI_STYLESETFORE, SCE_LUA_NUMBER, 0x0080FF);
	send(SCI_STYLESETFORE, SCE_LUA_WORD, 0xFF0000);
	send(SCI_STYLESETFORE, SCE_LUA_STRING, 0x1515A3);
	send(SCI_STYLESETFORE, SCE_LUA_CHARACTER, 0x1515A3);
	send(SCI_STYLESETFORE, SCE_LUA_LITERALSTRING, 0x1515A3);
	send(SCI_STYLESETFORE, SCE_LUA_PREPROCESSOR, 0x004080);
	send(SCI_STYLESETFORE, SCE_LUA_OPERATOR, 0x800000);
	send(SCI_STYLESETFORE, SCE_LUA_WORD2, 0xC08000);
	send(SCI_STYLESETFORE, SCE_LUA_WORD3, 0xC08000);
	send(SCI_STYLESETFORE, SCE_LUA_WORD4, 0xC08000);

	send(SCI_INDICSETSTYLE, 0, INDIC_STRAIGHTBOX);
}

void ScriptEdit::setText(const QString &text)
{
	send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(text.toUtf8().constData()));
	send(SCI_EMPTYUNDOBUFFER);
}

QString ScriptEdit::text() const
{
	QString res;
	int len = send(SCI_GETTEXTLENGTH);
	if (len) {
		char *buf = new char[len + 1];
		send(SCI_GETTEXT, len + 1, reinterpret_cast<sptr_t>(buf));
		res = QString().fromUtf8(buf);
		delete [] buf;
	}
	return res;
}

void ScriptEdit::redo()
{
    send(SCI_REDO);
}

void ScriptEdit::undo()
{
    send(SCI_UNDO);
}

void ScriptEdit::copy()
{
    send(SCI_COPY);
}

void ScriptEdit::cut()
{
    send(SCI_CUT);
}

void ScriptEdit::paste()
{
    send(SCI_PASTE);
}

bool ScriptEdit::canRedo() const
{
    return send(SCI_CANREDO);
}

bool ScriptEdit::canUndo() const
{
    return send(SCI_CANUNDO);
}

bool ScriptEdit::canPaste() const
{
    return send(SCI_CANPASTE);
}

bool ScriptEdit::hasSelection() const
{
	return (send(SCI_GETSELECTIONSTART) != send(SCI_GETSELECTIONEND));
}

void ScriptEdit::del()
{
	send(SCI_CLEAR);
}

QPoint ScriptEdit::currentPos() const
{
	return QPoint(send(SCI_GETCOLUMN, send(SCI_GETCURRENTPOS)), send(SCI_LINEFROMPOSITION, send(SCI_GETCURRENTPOS)));
};

bool ScriptEdit::getOverType() const
{
	return send(SCI_GETOVERTYPE);
};

void ScriptEdit::setOverType(bool value) const
{
	send(SCI_SETOVERTYPE, value);
};

void ScriptEdit::maintainIndentation(int ch)
{
	int eolMode = int(send(SCI_GETEOLMODE));
	int curLine = int(send(SCI_LINEFROMPOSITION, send(SCI_GETCURRENTPOS)));
	int prevLine = curLine - 1;
	int indentAmountPrevLine = 0;
	//int tabWidth = send(SCI_GETTABWIDTH);

	if (((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
		(eolMode == SC_EOL_CR && ch == '\r'))
	{
		// Search the non-empty previous line
		while (prevLine >= 0 && send(SCI_LINELENGTH, prevLine) <= (eolMode == SC_EOL_CRLF ? 2 : 1))
			prevLine--;

		// Get previous line's Indent
		if (prevLine >= 0)
		{
			indentAmountPrevLine = send(SCI_GETLINEINDENTATION, prevLine);
		}

		send(SCI_SETLINEINDENTATION, curLine, indentAmountPrevLine);
		send(SCI_SETEMPTYSELECTION, send(SCI_GETLINEINDENTPOSITION, curLine));
	}
}

void ScriptEdit::hilightView()
{
	long docLen = (long)send(SCI_GETTEXTLENGTH);
	//Clear marks
	send(SCI_SETINDICATORCURRENT, 0);
	send(SCI_INDICATORCLEARRANGE, 0, docLen);

	long selStart = (long)send(SCI_GETSELECTIONSTART);
	if (selStart != (long)send(SCI_WORDSTARTPOSITION, selStart, 1))
		return;
	long selEnd = (long)send(SCI_GETSELECTIONEND);
	if (selEnd != (long)send(SCI_WORDENDPOSITION, selEnd, 1))
		return;

	//If nothing selected, don't mark anything
	if (selStart == selEnd)
		return;

	int textlen = selEnd - selStart;
	char *bytes2Find = new char[textlen + 1];
	send(SCI_GETSELTEXT, 0, reinterpret_cast<sptr_t>(bytes2Find));

	// Get the range of text visible and highlight everything in it
	int firstLine =	(int)send(SCI_GETFIRSTVISIBLELINE);
	int nrLines =	(int)send(SCI_LINESONSCREEN) + 1;
	Scintilla::Sci_TextToFind ttf;
	ttf.chrg.cpMin = (int)send(SCI_POSITIONFROMLINE, firstLine);
	ttf.chrg.cpMax = (int)send(SCI_POSITIONFROMLINE, firstLine + nrLines + 1);
	if (ttf.chrg.cpMax == -1)
		ttf.chrg.cpMax = (int)docLen;
	ttf.lpstrText = bytes2Find;
	while (send(SCI_FINDTEXT, SCFIND_WHOLEWORD, reinterpret_cast<sptr_t>(&ttf)) != -1)
	{
		send(SCI_INDICATORFILLRANGE, ttf.chrgText.cpMin, textlen);
		ttf.chrg.cpMin = ttf.chrgText.cpMin + 1;
		if (ttf.chrg.cpMin >= ttf.chrg.cpMax)
			break;
	}

	delete [] bytes2Find;
}

/**
 * BinEditor
 */

BinEditor::BinEditor(QWidget *parent)
	: QAbstractScrollArea(parent), maskAllowed_(false)
{
	bytesPerLine_ = 8;

	QFont font(MONOSPACE_FONT_FAMILY, 14, QFont::Bold);
#ifdef __APPLE__
	font.setStyleStrategy(QFont::ForceIntegerMetrics);
#endif
	setFont(font);

    setFocusPolicy(Qt::WheelFocus);
    setFrameStyle(QFrame::Plain);
	cursorPosition_ = 0;
	anchorPosition_ = 0;
	overwriteMode_ = false;
	hexCursor_ = true;
	cursorVisible_ = false;
	maxLength_ = 0;

	init();
}

BinEditor::~BinEditor()
{

}

void BinEditor::setData(const QByteArray &data)
{
	m_undoStack.empty();
	m_redoStack.empty();
	cursorPosition_ = 0;
	anchorPosition_ = 0;
	data_ = data;
	mask_.clear();
	for (int i = 0; i < data_.size(); i++) {
		mask_.append('0');
	}
	changed();
}

void BinEditor::setValue(const QString &value)
{
	m_undoStack.empty();
	m_redoStack.empty();
	cursorPosition_ = 0;
	anchorPosition_ = 0;
	data_.clear();
	mask_.clear();
	insert(0, value);
}

QString BinEditor::value() const
{
	QString res = data().toHex();
	if (maskAllowed_) {
		for (int i = 0; i < mask_.size(); i++) {
			uint8_t mask = mask_[i];
			if (mask & 0x10)
				res[i * 2] = QChar('?');
			if (mask & 1)
				res[i * 2 + 1] = QChar('?');
		}
	}
	return res;
}

void BinEditor::init()
{
	QFontMetrics fm(fontMetrics());
    lineHeight_ = fm.lineSpacing();
    charWidth_ = fm.width(QChar(QLatin1Char('M')));
	margin_ = charWidth_;
	addressWidth_ = charWidth_ * 4;
	columnWidth_ = 2 * charWidth_ + fm.width(QChar(QLatin1Char(' ')));
    textWidth_ = bytesPerLine_ * charWidth_ + charWidth_;
    
	numLines_ = data_.size() / bytesPerLine_ + 1;
    numVisibleLines_ = viewport()->height() / lineHeight_;

    horizontalScrollBar()->setRange(0, 2 * charWidth_ + bytesPerLine_ * columnWidth_
                                    + addressWidth_ + textWidth_ - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());
    verticalScrollBar()->setRange(0, numLines_ - numVisibleLines_);
    verticalScrollBar()->setPageStep(numVisibleLines_);
}

void BinEditor::resizeEvent(QResizeEvent *)
{
    init();
}

void BinEditor::changed()
{
	if (maxLength_ > 0 && data_.size() > maxLength_)
		data_.resize(maxLength_);
	numLines_ = data_.size() / bytesPerLine_ + 1;
	verticalScrollBar()->setRange(0, numLines_ - numVisibleLines_);
	setCursorPosition(cursorPosition_);
	viewport()->update();

	emit dataChanged();
}

void BinEditor::paintEvent(QPaintEvent *e)
{
    QPainter painter(viewport());
    const int topLine = verticalScrollBar()->value();
    const int xoffset = horizontalScrollBar()->value();
    const int x1 = -xoffset + margin_ + addressWidth_ - charWidth_/2 - 1;
    const int x2 = -xoffset + margin_ + addressWidth_ + bytesPerLine_ * columnWidth_ + charWidth_/2;
	painter.setPen(QColor(0xe5, 0xe5, 0xe5));
    painter.drawLine(x1, 0, x1, viewport()->height());
    painter.drawLine(x2, 0, x2, viewport()->height());

	int selStart = qMin(anchorPosition_, cursorPosition_) / 2;
	int selEnd = qMax(anchorPosition_, cursorPosition_) / 2;

    QString itemString(bytesPerLine_*3, QLatin1Char(' '));
    QChar *itemStringData = itemString.data();
	const char *hex = "0123456789abcdef";

    painter.setPen(palette().text().color());
    const QFontMetrics &fm = painter.fontMetrics();
	int ascent = fm.ascent();
	int descent = fm.descent();
    for (int i = 0; i <= numVisibleLines_; ++i) {
        int line = topLine + i;
        if (line >= numLines_)
            break;

        //int lineAddress = 0 + uint(line) * bytesPerLine_;
        int y = i * lineHeight_ + ascent;
        if (y - ascent > e->rect().bottom())
            break;
        if (y + descent < e->rect().top())
            continue;

        painter.drawText(-xoffset, i * lineHeight_ + ascent, QString("%1").arg(line * bytesPerLine_, 4, 16, QChar('0')));

		QString printable;
		QRect selectionRect;
		QRect printableSelectionRect;
		int cursor = -1;
        if (line == cursorPosition_ / (bytesPerLine_ * 2))
            cursor = cursorPosition_ % (bytesPerLine_ * 2);
        for (int c = 0; c < bytesPerLine_; ++c) {
            int pos = line * bytesPerLine_ + c;
            if (pos >= data_.size()) {
                while (c < bytesPerLine_) {
                    itemStringData[c*3] = itemStringData[c*3+1] = QLatin1Char(' ');
					printable += QLatin1Char(' ');
                    ++c;
                }
                break;
            }

			unsigned char value = data_[pos];
			unsigned char mask = mask_[pos];
			itemStringData[c*3] = (mask & 0x10) ? QLatin1Char('?') : QLatin1Char(hex[value >> 4]);
			itemStringData[c*3+1] = (mask & 1) ? QLatin1Char('?') : QLatin1Char(hex[value & 0xf]);

            QChar qc(mask ? '?' : value);
            if (qc.unicode() >= 127 || !qc.isPrint())
				qc = 0xB7;
            printable += qc;

			if (selStart != selEnd && pos >= selStart && pos < selEnd) {
				if (hexCursor_) {
					int item_x = -xoffset + margin_  + addressWidth_ + c * columnWidth_;
					selectionRect |= QRect(item_x - charWidth_/2, y - ascent, columnWidth_, lineHeight_);
				} else {
					int item_x = -xoffset + margin_ + addressWidth_ + bytesPerLine_ * columnWidth_ + charWidth_ + c * charWidth_;
					printableSelectionRect |= QRect(item_x, y - ascent, charWidth_, lineHeight_);
				}
			}
		}
		int x = -xoffset +  margin_ + addressWidth_;
		painter.drawText(x, y, itemString);

        if (!selectionRect.isEmpty()) {
            painter.save();
            painter.fillRect(selectionRect, palette().highlight());
            painter.setPen(palette().highlightedText().color());
            painter.setClipRect(selectionRect);
            painter.drawText(x, y, itemString);
            painter.restore();
        }

		int text_x = -xoffset + margin_ + addressWidth_ + bytesPerLine_ * columnWidth_ + charWidth_;
		painter.drawText(text_x, y, printable);

		if (!printableSelectionRect.isEmpty()) {
            painter.save();
            painter.fillRect(printableSelectionRect, palette().highlight());
            painter.setPen(palette().highlightedText().color());
            painter.setClipRect(printableSelectionRect);
            painter.drawText(text_x, y, printable);
            painter.restore();
        }

		if (cursor >= 0 && cursorVisible_) {
			int pos = cursor / 2;
			if (hexCursor_) {
				int c = cursor % 2;
				QRect cursorRect(x + pos * columnWidth_ + c * charWidth_, y - ascent, fm.boundingRect(itemString.mid(pos * 3 + c, 1)).width() + 1, lineHeight_);
				if (!overwriteMode_)
					cursorRect.setWidth(1);
				painter.save();
				painter.setClipRect(cursorRect);
				painter.fillRect(cursorRect, palette().text().color());
				painter.setPen(palette().highlightedText().color());
				painter.drawText(x, y, itemString);
				painter.restore();
			} else {
				QRect cursorRect = QRect(text_x + fm.width(printable.left(pos)), y - ascent, fm.width(printable.at(pos)), lineHeight_);
				if (!overwriteMode_)
					cursorRect.setWidth(1);
				painter.save();
                painter.setClipRect(cursorRect);
				painter.fillRect(cursorRect, palette().text().color());
				painter.setPen(palette().highlightedText().color());
				painter.drawText(text_x, y, printable);
	            painter.restore();
			}
		}
	}
}

int BinEditor::posAt(const QPoint &pos) const
{
    int xoffset = horizontalScrollBar()->value();
    int x = xoffset + pos.x() - margin_ - addressWidth_;
    int topLine = verticalScrollBar()->value();
    int line = pos.y() / lineHeight_;
    int column = (qMax(0,x + charWidth_)) / columnWidth_;
	int c = (x - column * columnWidth_ > charWidth_) ? 1 : 0;
	if (x > bytesPerLine_ * columnWidth_ + charWidth_/2) {
		c = 0;
        x -= bytesPerLine_ * columnWidth_ + charWidth_;
        for (column = 0; column < 15; ++column) {
            int posn = (topLine + line) * bytesPerLine_ + column;
            if (posn < 0 || posn >= data_.size())
                break;
            QChar qc(data_[posn]);
            if (!qc.isPrint())
                qc = 0xB7;
            x -= fontMetrics().width(qc);
            if (x <= 0)
                break;
        }
	}

    return (topLine + line) * bytesPerLine_ * 2 + column * 2 + c;
}

bool BinEditor::inTextArea(const QPoint &pos) const
{
    int xoffset = horizontalScrollBar()->value();
    int x = xoffset + pos.x() - margin_ - addressWidth_;
    return (x > bytesPerLine_ * columnWidth_ + charWidth_/2);
}

void BinEditor::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;
    MoveMode moveMode = e->modifiers() & Qt::ShiftModifier ? KeepAnchor : MoveAnchor;
    setCursorPosition(posAt(e->pos()), moveMode);
    setBlinkingCursorEnabled(true);
    if (hexCursor_ == inTextArea(e->pos())) {
        hexCursor_ = !hexCursor_;
        updateLines();
    }
}

void BinEditor::mouseMoveEvent(QMouseEvent *e)
{
    if (!(e->buttons() & Qt::LeftButton))
        return;
    setCursorPosition(posAt(e->pos()), KeepAnchor);
}

void BinEditor::setOverwriteMode(bool value)
{
	if (overwriteMode_ != value) {
		overwriteMode_ = value;
		updateLines();
	}
}

void BinEditor::setCursorPosition(int pos, MoveMode moveMode)
{
    if (overwriteMode_) {
        if (pos > (data_.size() * 2 - 1))
            pos = data_.size() * 2 - 1;
    } else {
        if (pos > (data_.size() * 2))
            pos = data_.size() * 2;
    }
	if (pos < 0)
		return;

    int oldCursorPosition = cursorPosition_;
    cursorPosition_ = pos;
    if (moveMode == MoveAnchor) {
        updateLines(anchorPosition_, oldCursorPosition);
        anchorPosition_ = cursorPosition_;
    }

    updateLines(oldCursorPosition, cursorPosition_);
    ensureCursorVisible();
}

void BinEditor::setBlinkingCursorEnabled(bool enable)
{
    if (enable && QApplication::cursorFlashTime() > 0)
        cursorBlinkTimer_.start(QApplication::cursorFlashTime() / 2, this);
    else
        cursorBlinkTimer_.stop();
    cursorVisible_ = enable;

	if (cursorPosition_ != anchorPosition_)
		updateLines(anchorPosition_, cursorPosition_);
	else
		updateLines(cursorPosition_, cursorPosition_);
}

void BinEditor::focusInEvent(QFocusEvent *e)
{
	setBlinkingCursorEnabled(true);
    bool first = true;
    switch (e->reason()) {
    case Qt::BacktabFocusReason:
        first = false;
        break;
    case Qt::TabFocusReason:
		break;
    default:
		return;
    }
	hexCursor_ = first;
}

void BinEditor::focusOutEvent(QFocusEvent *)
{
    setBlinkingCursorEnabled(false);
}

void BinEditor::timerEvent(QTimerEvent *e)
{
	if (e->timerId() == cursorBlinkTimer_.timerId()) {
        cursorVisible_ = !cursorVisible_;
        updateLines();
	}
    QAbstractScrollArea::timerEvent(e);
}

void BinEditor::updateLines()
{
    updateLines(cursorPosition_ , cursorPosition_);
}

void BinEditor::updateLines(int fromPosition, int toPosition)
{
    int topLine = verticalScrollBar()->value();
    int firstLine = qMin(fromPosition, toPosition) / (bytesPerLine_ * 2);
    int lastLine = qMax(fromPosition, toPosition) / (bytesPerLine_ * 2);
    int y = (firstLine - topLine) * lineHeight_;
    int h = (lastLine - firstLine + 1 ) * lineHeight_;

    viewport()->update(0, y, viewport()->width(), h);
}

void BinEditor::ensureCursorVisible()
{
    QRect cr = cursorRect();
    QRect vr = viewport()->rect();
    if (!vr.contains(cr)) {
        if (cr.top() < vr.top())
            verticalScrollBar()->setValue(cursorPosition_ / (bytesPerLine_ * 2));
        else if (cr.bottom() > vr.bottom())
            verticalScrollBar()->setValue(cursorPosition_ / (bytesPerLine_ * 2) - numVisibleLines_ + 1);
    }
}

QRect BinEditor::cursorRect() const
{
    int topLine = verticalScrollBar()->value();
    int line = cursorPosition_ / (bytesPerLine_ * 2);
    int y = (line - topLine) * lineHeight_;
    int xoffset = horizontalScrollBar()->value();
    int column = cursorPosition_ % (bytesPerLine_ * 2);
    int x = hexCursor_
            ? (-xoffset + margin_ + addressWidth_ + column * columnWidth_ + (cursorPosition_ & 1) * charWidth_)
            : (-xoffset + margin_ + addressWidth_ + bytesPerLine_ * columnWidth_ + charWidth_ + column * charWidth_);
    int w = hexCursor_ ? columnWidth_ : charWidth_;
    return QRect(x, y, w, lineHeight_);
}

void BinEditor::insert(int index, char c)
{
	data_.insert(index, c);
	mask_.insert(index, '0');
	changed();
}

void BinEditor::insert(int index, const QString &value)
{
	QByteArray insData;
	QByteArray insMask;
    bool odd_digit = false;
    for (int i = 0; i < value.size(); i++) {
		int ch = value.at(i).unicode();
        int tmp;
		int mask = 0;
        if (ch >= '0' && ch <= '9')
            tmp = ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            tmp = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F')
            tmp = ch - 'A' + 10;
		else if (maskAllowed_ && ch == '?') {
			tmp = 0;
			mask = 1;
		} else
            continue;

        if (odd_digit) {
			int pos = insData.size() - 1;
			insData[pos] = insData[pos] | tmp;
			insMask[pos] = insMask[pos] | mask;
            odd_digit = false;
        } else {
			insData.append(tmp << 4);
			insMask.append(mask << 4);
            odd_digit = true;
        }
    }
	data_.insert(index, insData);
	mask_.insert(index, insMask);
	changed();
}

void BinEditor::replace(int index, char c, char mask)
{
	data_[index] = c;
	mask_[index] = mask;
	changed();
}

void BinEditor::remove(int index, int len)
{
	data_.remove(index, len);
	mask_.remove(index, len);
	changed();
}

void BinEditor::selectAll()
{
    setCursorPosition(0);
	setCursorPosition(data_.size() * 2, KeepAnchor);
}

void BinEditor::copy()
{
	int selStart = qMin(anchorPosition_, cursorPosition_) / 2;
	int selEnd = qMax(anchorPosition_, cursorPosition_) / 2;
	QByteArray data = data_.mid(selStart, selEnd - selStart);
	QByteArray mask = mask_.mid(selStart, selEnd - selStart);
	QString res;
	if (hexCursor_) {
		const char * const hex = "0123456789abcdef";
		res.reserve(3 * data.size());
		for (int i = 0; i < data.size(); ++i) {
			unsigned char d = data[i];
			unsigned char m = mask[i];
			res.append(QLatin1Char((m & 0x10) ? '?' : hex[d >> 4])).append(QLatin1Char((m & 1) ? '?' : hex[d & 0xf])).append(QLatin1Char(' '));
		}
		res.chop(1);
	} else {
		for (int i = 0; i < data.size(); ++i) {
			if (mask[i])
				data[i] = '?';
		}
		res = QString::fromLatin1(data);
	}
	QClipboard *clipboard = QApplication::clipboard();
	if (clipboard)
		clipboard->setText(res);
}

void BinEditor::cut()
{
	copy();
	deleteSelection();
}

void BinEditor::paste()
{
	QClipboard *clipboard = QApplication::clipboard();
	if (clipboard) {
		CheckPointMaker maker(this);
		deleteSelectionInternal();
		QString text = clipboard->text();
		if (!hexCursor_)
			text = QByteArray(text.toLatin1()).toHex();
		insert(cursorPosition_ / 2, text);
		setCursorPosition(cursorPosition_ + text.size());
	}
}

bool BinEditor::deleteSelection()
{
	CheckPointMaker maker(this);
	return deleteSelectionInternal();
}

bool BinEditor::deleteSelectionInternal()
{
	int selStart = qMin(anchorPosition_, cursorPosition_) / 2;
	int selEnd = qMax(anchorPosition_, cursorPosition_) / 2;
	if (selStart == selEnd)
		return false;

	remove(selStart, selEnd - selStart);
	setCursorPosition(selStart * 2);
	return true;
}

void BinEditor::keyPressEvent(QKeyEvent *e)
{
	if (e == QKeySequence::SelectAll) {
		selectAll();
		e->accept();
		return;
    } else if (e == QKeySequence::Copy) {
		copy();
		e->accept();
		return;
	} else if (e == QKeySequence::Cut) {
		cut();
		e->accept();
		return;
	} else if (e == QKeySequence::Paste) {
		paste();
		e->accept();
		return;
	} else if (e == QKeySequence::Undo) {
		undo();
		e->accept();
		return;
	} else if (e == QKeySequence::Redo) {
		redo();
		e->accept();
		return;
	}

	CheckPointMaker maker(this);
    MoveMode moveMode = e->modifiers() & Qt::ShiftModifier ? KeepAnchor : MoveAnchor;
	int cursorStep = hexCursor_ ? 1 : 2;
    switch (e->key()) {
    case Qt::Key_Up:
        setCursorPosition(cursorPosition_ - bytesPerLine_ * 2, moveMode);
        break;
    case Qt::Key_Down:
        setCursorPosition(cursorPosition_ + bytesPerLine_ * 2, moveMode);
        break;
    case Qt::Key_Right:
        setCursorPosition(cursorPosition_ + cursorStep, moveMode);
        break;
    case Qt::Key_Left:
        setCursorPosition(cursorPosition_ - cursorStep, moveMode);
        break;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
		{
			int line = qMax(0, cursorPosition_ / (bytesPerLine_ * 2) - verticalScrollBar()->value());
			verticalScrollBar()->triggerAction(e->key() == Qt::Key_PageUp ? QScrollBar::SliderPageStepSub : QScrollBar::SliderPageStepAdd);
			setCursorPosition((verticalScrollBar()->value() + line) * bytesPerLine_ + cursorPosition_ % (bytesPerLine_ * 2), moveMode);
		} 
		break;
    case Qt::Key_Home:
		{
			int pos;
			if (e->modifiers() & Qt::ControlModifier)
				pos = 0;
			else
				pos = cursorPosition_ - cursorPosition_ % (bytesPerLine_ * 2);
			setCursorPosition(pos, moveMode);
		} 
		break;
    case Qt::Key_End:
		{
			int pos;
			if (e->modifiers() & Qt::ControlModifier)
				pos = data_.size();
			else
				pos = cursorPosition_ - cursorPosition_ % (bytesPerLine_ * 2) + (bytesPerLine_ * 2 - 1);
			setCursorPosition(pos, moveMode);
		}
		break;
	case Qt::Key_Insert:
		if (e->modifiers() == Qt::NoModifier) {
			overwriteMode_ = !overwriteMode_;
			setCursorPosition(cursorPosition_, KeepAnchor);
		}
		break;
	case Qt::Key_Backspace:
		{
			if (!deleteSelectionInternal()) {
				int pos = cursorPosition_ / 2;
				if (pos > 0) {
					setCursorPosition(cursorPosition_ - 2);
					if (overwriteMode_)
						replace(pos - 1, char(0));
					else
						remove(pos - 1, 1);
				}
			}
		}
		break;
	case Qt::Key_Escape:
		e->ignore();
		return;
	case Qt::Key_Delete:
		{
			if (!deleteSelectionInternal()) {
				int pos = cursorPosition_ / 2;
				if (overwriteMode_)
					replace(pos, char(0));
				else
					remove(pos, 1);
				setCursorPosition(cursorPosition_);
			}
		}
		break;
    default:
        {
			QString text = e->text();
			for (int i = 0; i < text.length(); ++i) {
				QChar c = text.at(i);
				if (hexCursor_) {
					c = c.toLower();
					int nibble = -1;
					int mask = 0;
					if (c.unicode() >= 'a' && c.unicode() <= 'f')
						nibble = c.unicode() - 'a' + 10;
					else if (c.unicode() >= '0' && c.unicode() <= '9')
						nibble = c.unicode() - '0';
					else if (maskAllowed_ && c.unicode() == '?') {
						mask = 1;
						nibble = 0;
					}

					if (nibble < 0)
						continue;

					deleteSelectionInternal();

					int pos = cursorPosition_ / 2;
					if (!overwriteMode_ && cursorPosition_ % 2 == 0)
						insert(pos, 0);

					if (cursorPosition_ & 1) {
						replace(pos, nibble + (data_[pos] & 0xf0), mask + (mask_[pos] & 0xf0));
						setCursorPosition(cursorPosition_ + 1);
					} else {
						replace(pos, (nibble << 4) + (data_[pos] & 0x0f), (mask << 4) + (mask_[pos] & 0x0f));
						setCursorPosition(cursorPosition_ + 1);
					}
				} else {
					if (c.unicode() >= 128 || !c.isPrint())
						continue;

					deleteSelectionInternal();

					int pos = cursorPosition_ / 2;
					if (!overwriteMode_)
						insert(pos, 0);

					replace(pos, c.unicode());
					setCursorPosition(cursorPosition_ + 2);
				}
				setBlinkingCursorEnabled(true);
			}
		}
	}

	e->accept();
}

bool BinEditor::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        switch (static_cast<QKeyEvent*>(e)->key()) {
        case Qt::Key_Tab:
			if (hexCursor_) {
				hexCursor_ = !hexCursor_;
				setBlinkingCursorEnabled(true);
				ensureCursorVisible();
				e->accept();
				return true;
			}
			break;
        case Qt::Key_Backtab:
			if (!hexCursor_) {
				hexCursor_ = !hexCursor_;
				setBlinkingCursorEnabled(true);
				ensureCursorVisible();
				e->accept();
				return true;
			}
			break;
		}
	}

    return QAbstractScrollArea::event(e);
}

void BinEditor::makeUndoCheckpoint(const CheckPoint &cp)
{
	m_redoStack.clear();
	m_undoStack.push(cp);
}

void BinEditor::undo() 
{
	if (!m_undoStack.isEmpty()) {
		m_redoStack.push(CurrentCheckPoint());
		restoreUndoCheckpoint(m_undoStack.pop());
	}
} 

void BinEditor::redo() 
{ 
	if (!m_redoStack.isEmpty()) {
		m_undoStack.push(CurrentCheckPoint()); 
		restoreUndoCheckpoint(m_redoStack.pop()); 
	}
} 

void BinEditor::restoreUndoCheckpoint(const CheckPoint &cp)
{
	cursorPosition_ = cp.cursorPosition;
	anchorPosition_ = cp.anchorPosition;
	data_.clear();
	mask_.clear();
	insert(0, cp.text);
}

void BinEditor::contextMenuEvent(QContextMenuEvent *event)
{
    if (QMenu *menu = createStandardContextMenu()) {
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(event->globalPos());
    }
}

bool BinEditor::isUndoAvailable() const
{
	return !m_undoStack.isEmpty();
}

bool BinEditor::isRedoAvailable() const
{
	return !m_redoStack.isEmpty();
}

bool BinEditor::hasSelectedText() const
{
	return anchorPosition_ != cursorPosition_;
}

QMenu *BinEditor::createStandardContextMenu()
{
	QMenu *popup = new QMenu(this);
	QAction *action = 0;

	action = popup->addAction(QLineEdit::tr("&Undo"));
	action->setShortcut(QKeySequence::Undo);
	action->setEnabled(isUndoAvailable());
	connect(action, SIGNAL(triggered()), this, SLOT(undo()));

	action = popup->addAction(QLineEdit::tr("&Redo"));
	action->setShortcut(QKeySequence::Redo);
	action->setEnabled(isRedoAvailable());
	connect(action, SIGNAL(triggered()), this, SLOT(redo()));

	popup->addSeparator();

	action = popup->addAction(QLineEdit::tr("Cu&t"));
	action->setShortcut(QKeySequence::Cut);
	action->setEnabled(hasSelectedText());
	connect(action, SIGNAL(triggered()), this, SLOT(cut()));

    action = popup->addAction(QLineEdit::tr("&Copy"));
	action->setShortcut(QKeySequence::Copy);
    action->setEnabled(hasSelectedText());
    connect(action, SIGNAL(triggered()), this, SLOT(copy()));

	action = popup->addAction(QLineEdit::tr("&Paste"));
	action->setShortcut(QKeySequence::Paste);
	action->setEnabled(!QApplication::clipboard()->text().isEmpty());
	connect(action, SIGNAL(triggered()), this, SLOT(paste()));

	action = popup->addAction(QLineEdit::tr("Delete"));
	action->setEnabled(!isEmpty() && hasSelectedText());
	connect(action, SIGNAL(triggered()), this, SLOT(deleteSelection()));

    if (!popup->isEmpty())
        popup->addSeparator();

    action = popup->addAction(QLineEdit::tr("Select All"));
	action->setShortcut(QKeySequence::SelectAll);
    action->setEnabled(!isEmpty());
    connect(action, SIGNAL(triggered()), this, SLOT(selectAll()));

    return popup;
}

/**
 * FindWidget
 */

FindWidget::FindWidget(QWidget *parent)
	: QFrame(parent)
{
	installEventFilter(this);
	QHBoxLayout *hboxLayout = new QHBoxLayout(this);
	QString resourcePath = QLatin1String(":/images/");

	hboxLayout->setMargin(5);
	hboxLayout->setSpacing(5);

	toolClose = new QToolButton(this);
	toolClose->setIcon(QIcon(resourcePath + QLatin1String("/cancel_gray.png")));
	toolClose->setToolTip(QString::fromUtf8(language[lsClose].c_str()));
	connect(toolClose, SIGNAL(clicked()), SLOT(hide()));
	hboxLayout->addWidget(toolClose);

	editFind = new LineEdit(this);
	hboxLayout->addWidget(editFind);
	editFind->setMinimumSize(QSize(150, 0));
	connect(editFind, SIGNAL(textChanged(QString)), this,
		SLOT(textChanged(QString)));
	connect(editFind, SIGNAL(returnPressed()), this, SIGNAL(findNext()));
	connect(editFind, SIGNAL(textChanged(QString)), this, SLOT(updateButtons()));

	toolPrevious = new QToolButton(this);
	toolPrevious->setIcon(QIcon(resourcePath + QLatin1String("/previous.png")));
	toolPrevious->setToolTip(QString::fromUtf8(language[lsPrevious].c_str()));
	connect(toolPrevious, SIGNAL(clicked()), this, SIGNAL(findPrevious()));
	hboxLayout->addWidget(toolPrevious);

	toolNext = new QToolButton(this);
	toolNext->setIcon(QIcon(resourcePath + QLatin1String("/next.png")));
	toolNext->setToolTip(QString::fromUtf8(language[lsNext].c_str()));
	connect(toolNext, SIGNAL(clicked()), this, SIGNAL(findNext()));
	hboxLayout->addWidget(toolNext);

	checkCase = new QCheckBox(QString::fromUtf8(language[lsCaseSensitive].c_str()), this);
	hboxLayout->addWidget(checkCase);

	labelWrapped = new QLabel(this);
	labelWrapped->setScaledContents(true);
	labelWrapped->setTextFormat(Qt::RichText);
	labelWrapped->setMinimumSize(QSize(0, 20));
	labelWrapped->setMaximumSize(QSize(105, 20));
	labelWrapped->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
	labelWrapped->setText("<img src=\":/images/wrap.png\""
		">&nbsp;" + QString::fromUtf8(language[lsSearchWrapped].c_str()));
	hboxLayout->addWidget(labelWrapped);

	QSpacerItem *spacerItem = new QSpacerItem(20, 20, QSizePolicy::Expanding,
		QSizePolicy::Minimum);
	hboxLayout->addItem(spacerItem);
	setMinimumWidth(minimumSizeHint().width());
	labelWrapped->hide();

	updateButtons();
}

FindWidget::~FindWidget()
{
}

void FindWidget::show()
{
	QWidget::show();
	editFind->selectAll();
	editFind->setFocus(Qt::ShortcutFocusReason);
}

void FindWidget::showAndClear(const QString &term)
{
	show();
	editFind->setText(term);
}

QString FindWidget::text() const
{
	return editFind->text();
}

bool FindWidget::caseSensitive() const
{
	return checkCase->isChecked();
}

void FindWidget::setPalette(bool found)
{
	editFind->setStyleSheet(found ? "" : "QLineEdit{color: #ff3c08; background: #fff6f4;}");
}

void FindWidget::setTextWrappedVisible(bool visible)
{
	labelWrapped->setVisible(visible);
}

void FindWidget::updateButtons()
{
	const bool enable = !editFind->text().isEmpty();
	toolNext->setEnabled(enable);
	toolPrevious->setEnabled(enable);
}

void FindWidget::textChanged(const QString &text)
{
	emit find(text, true, true);
}

bool FindWidget::eventFilter(QObject *object, QEvent *e)
{
	if (e->type() == QEvent::KeyPress) {
		if ((static_cast<QKeyEvent*>(e))->key() == Qt::Key_Escape) {
			hide();
			emit escapePressed();
		}
	}
	return QWidget::eventFilter(object, e);
}

/**
 * TreeViewItemDelegate
 */

TreeViewItemDelegate::TreeViewItemDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{

}

void TreeViewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	QStyleOptionViewItemV4 opt = option;
	opt.state &= ~QStyle::State_HasFocus;
	initStyleOption(&opt, index);
	const QWidget *widget = opt.widget;
	QStyle *style = widget ? widget->style() : QApplication::style();
	assert(style);
	if (style == NULL) return;
	
	style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
	QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);

	QVariant value = index.data(Qt::Vmp::StaticTextRole);
	if (value.isValid()) {
		QString text = value.toString();

		painter->save();
		value = index.data(Qt::FontRole);
		if (value.canConvert<QFont>())
			painter->setFont(qvariant_cast<QFont>(value));
		else
			painter->setFont(opt.font);

		const QSize textMargin(
			style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1, 1);
		if (!opt.text.isEmpty())
			opt.text += " ";
		textRect.adjust(painter->fontMetrics().width(opt.text) + textMargin.width(), 0, 0, 0);

		value = index.data(Qt::Vmp::StaticColorRole);
		if (value.canConvert<QBrush>())
			painter->setPen(qvariant_cast<QBrush>(value).color());

		if (text[0] < 5) {
			// branch symbols
			opt.text = QChar(0x2190 + text[0].unicode() - 1);
			style->drawItemText(painter, textRect, opt.displayAlignment, opt.palette, true, opt.text);
			textRect.adjust(painter->fontMetrics().width(opt.text), 0, 0, 0);
			text = text.mid(1);
		}
		opt.text = text;
		style->drawItemText(painter, textRect, opt.displayAlignment, opt.palette, true, elidedText(opt.fontMetrics, textRect.width(), Qt::ElideRight, opt.text));
		painter->restore();
	}
}

QSize TreeViewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItemV4 opt = option;
	const int border = (opt.widget->objectName().startsWith("grid") ? 1 : 0);
	const int min_height = 18 * Application::stylesheetScaleFactor() + border;

	QSize ret = QStyledItemDelegate::sizeHint(option, index) + QSize(0, 2);
	if(ret.height() < min_height)
		ret.setHeight(min_height);
	return ret;
}

void TreeViewItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	bool mac;
#ifdef __APPLE__
	mac = true;
#else
	mac = false;
#endif
	editor->setProperty("OSX", mac);
	QStyledItemDelegate::updateEditorGeometry(editor, option, index);
}

void TreeViewItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	ProjectNode *node = static_cast<ProjectNode *>(index.internalPointer());
	if (!node)
		return;

	auto ed = qobject_cast<QLineEdit *>(editor);
	assert(ed);
	if (ed)
		ed->setText(node->text());
}

/**
 * LogTreeView
 */

QStyleOptionViewItem LogTreeView::viewOptions() const
{
	QStyleOptionViewItem opt = TreeView::viewOptions();
	opt.decorationPosition = QStyleOptionViewItem::Right;
	return opt;
}

/**
 * QElidedAction
 */

void ElidedAction::setFullText(const QString &text)
{
	fullText_ = text;
	int screenWidth = QApplication::desktop()->screenGeometry().width();
	QString elidedText = QApplication::fontMetrics().elidedText(text, Qt::ElideMiddle, screenWidth * 0.9);
	setText(elidedText);
}

/**
 * Label
 */

Label::Label(QWidget * parent)
	: QLabel(parent)
{

}

void Label::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		emit doubleClicked();
}

/**
* ToolButtonElided
*/

void ToolButtonElided::resizeEvent(QResizeEvent *event)
{
	elideText(event->size());
	QToolButton::resizeEvent(event);
}

void ToolButtonElided::elideText(const QSize &widgetSize)
{
	const int width = widgetSize.width();
	if (width == 0)
		return;
	const int iconWidth = icon().isNull() ? 0 : (iconSize().width() + 8);

	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);
	int padding = left + right + 8;
	int textWidth = width - (iconWidth + padding);
	
	QFontMetrics fm(font());
	QString elidedText = fm.elidedText(fullText_, Qt::ElideRight, textWidth);
	QToolButton::setText(elidedText);
}

void ToolButtonElided::setText(const QString &text)
{
	fullText_ = text;
	QToolButton::setText(text);
}

/**
* DisasmView
*/

DisasmView::DisasmView(QWidget * parent)
	: TableView(parent)
{

}

void DisasmView::scrollContentsBy(int dx, int dy)
{
	TableView::scrollContentsBy(dx, dy);
	viewport()->update();
}