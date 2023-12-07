#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/files.h"
#include "../core/lang.h"
#include "../core/core.h"
#include "../core/processors.h"
#include "widgets.h"
#include "models.h"
#include "property_editor.h"
#include "moc/moc_property_editor.cc"

std::string formatBase64(const std::string &value)
{
	std::string res = value;
	size_t len = res.size();
	for (size_t i = 0; i < len / 76; i++) {
		res.insert(res.begin() + (i + 1) * 76 + i, '\n');
	}
	return res;
}

std::string formatVector(const std::vector<uint8_t> &value)
{
	std::string dst;
	size_t dst_len = Base64EncodeGetRequiredLength(value.size());
	if (dst_len) {
		dst.resize(dst_len);
		Base64Encode(&value[0], value.size(), &dst[0], dst_len);
		if (dst_len != dst.size())
			dst.resize(dst_len);
	}
	return formatBase64(dst);
}

/**
 * Property
 */

Property::Property(Property *parent, const QString &name)
	: parent_(NULL), name_(name), readOnly_(false)
{
	if (parent)
		parent->addChild(this);
}

Property::~Property()
{
	if (parent_)
		parent_->removeChild(this);
	clear();
	emit destroyed(this);
}

void Property::clear()
{
	while (!children_.isEmpty()) {
		QListIterator<Property *> it(children_);
		Property *prop = it.next();
		delete prop;
	}
}

QWidget *Property::createEditor(QWidget * /*parent*/)
{
	return NULL;
}

QString Property::valueText() const
{
	return QString();
}

void Property::setName(const QString &name)
{
	if (name_ == name)
		return;

	name_ = name;
	emit changed(this);
}

void Property::setReadOnly(bool value)
{
	if (readOnly_ == value)
		return;

	readOnly_ = value;
	emit changed(this);
}

void Property::addChild(Property *child)
{
	insertChild(childCount(), child);
}

void Property::insertChild(int index, Property *child)
{
	if (child->parent())
		return;

	child->parent_ = this;
	children_.insert(index, child);
}

void Property::removeChild(Property *child)
{
	children_.removeOne(child);
}

/**
 * StringProperty
 */

StringProperty::StringProperty(Property *parent, const QString &name, const QString &value)
	: Property(parent, name), value_(value)
{
	
}

QString StringProperty::valueText() const
{
	return value_;
}

QWidget *StringProperty::createEditor(QWidget *parent)
{
	QLineEdit *editor = new LineEdit(parent);
	editor->setFrame(false);
	editor->setText(value_);
	connect(editor, SIGNAL(textChanged(const QString &)), this, SLOT(editorChanged(const QString &)));

	return editor;
}

void StringProperty::editorChanged(const QString &value)
{
	setValue(value);
}

void StringProperty::setValue(const QString &value)
{
	if (value_ != value) {
		value_ = value;
		emit valueChanged(value);
		emit changed(this);
	}
}

/**
 * BoolProperty
 */

BoolProperty::BoolProperty(Property *parent, const QString &name, bool value)
	: Property(parent, name), value_(value)
{

}

QString BoolProperty::valueText() const
{
	return value_ ? QString::fromUtf8(language[lsYes].c_str()) : QString::fromUtf8(language[lsNo].c_str());
}

QWidget *BoolProperty::createEditor(QWidget *parent)
{
	BoolEdit *editor = new BoolEdit(parent);
	editor->setFrame(false);
	editor->setChecked(value_);

	connect(editor, SIGNAL(toggled(bool)), this, SLOT(editorChanged(bool)));

	return editor;
}

void BoolProperty::editorChanged(bool value)
{
	setValue(value);
}

void BoolProperty::setValue(bool value)
{
	if (value_ != value) {
		value_ = value;
		emit valueChanged(value);
		emit changed(this);
	}
}

/**
 * DateProperty
 */

DateProperty::DateProperty(Property *parent, const QString &name, const QDate &value)
	: Property(parent, name), value_(value)
{
	
}

QString DateProperty::valueText() const
{
	return value_.toString(Qt::SystemLocaleShortDate);
}

QWidget *DateProperty::createEditor(QWidget *parent)
{
	QDateEdit *editor = new QDateEdit(parent);
	editor->setFrame(false);
	editor->setDate(value_);
	connect(editor, SIGNAL(dateChanged(const QDate &)), this, SLOT(editorChanged(const QDate &)));

	return editor;
}

void DateProperty::editorChanged(const QDate &value)
{
	setValue(value);
}

void DateProperty::setValue(const QDate &value)
{
	if (value_ != value) {
		value_ = value;
		emit valueChanged(value);
		emit changed(this);
	}
}

/**
 * StringListProperty
 */

StringListProperty::StringListProperty(Property *parent, const QString &name, const QString &value)
	: StringProperty(parent, name, value)
{

}

QWidget *StringListProperty::createEditor(QWidget *parent)
{
	StringListEdit *editor = new StringListEdit(parent);
	editor->document()->setDocumentMargin(0);
	editor->setFrameShape(QFrame::NoFrame);
	editor->setPlainText(value());
	connect(editor, SIGNAL(textChanged(const QString &)), this, SLOT(editorChanged(const QString &)));

	return editor;
}

/**
 * HexProperty
 */

	/*
HexProperty::HexProperty(Property *parent, const QString &name, const QByteArray &value)
	: Property(parent, name)
{
	edit_ = new QHexEdit();
	edit_->setFrameShape(QFrame::NoFrame);
	edit_->setData(value);
}

HexProperty::~HexProperty()
{
	delete edit_;
}
	*/

/**
 * GroupProperty
 */

GroupProperty::GroupProperty(Property *parent, const QString &name)
	: Property(parent, name)
{

}

/**
 * CommandProperty
 */

CommandProperty::CommandProperty(Property *parent, ICommand *value)
	: Property(parent, QString::fromLatin1(value->display_address().c_str())), value_(value)
{
	text_ = QString::fromUtf8(value_->text().c_str());
}

QString CommandProperty::valueText() const
{
	return text_;
}

bool CommandProperty::hasStaticText(int column) const 
{ 
	if (!value_)
		return false;

	if (column == 0)
		return true;

	if (column == 1)
		return (value_->comment().type > ttNone);

	return false;
}

QString CommandProperty::staticText(int column) const 
{ 
	if (!value_)
		return QString();

	if (column == 0)
		return QString::fromLatin1(value_->dump_str().c_str());

	if (column == 1)
		return QString::fromUtf8(value_->comment().display_value().c_str());

	return QString();
}

QColor CommandProperty::staticColor(int column) const 
{
	if (!value_)
		return QColor();

	if (column == 0)
		return Qt::gray;

	if (column == 1) {
		switch (value_->comment().type) {
		case ttFunction: case ttJmp:
			return Qt::blue;
		case ttImport:
			return Qt::darkRed;
		case ttExport:
			return Qt::red;
		case ttString:
			return Qt::darkGreen;
		case ttVariable:
			return Qt::magenta;
		case ttComment:
			return Qt::gray;
		}
	}
	return QColor();
}

/**
 * EnumProperty
 */

EnumProperty::EnumProperty(Property *parent, const QString &name, const QStringList &items, int value)
	: Property(parent, name), items_(items), value_(value)
{

}
	
void EnumProperty::setValue(int value)
{
	if (value_ != value) {
		value_ = value;
		emit valueChanged(value);
		emit changed(this);
	}
}

void EnumProperty::editorChanged(int value)
{
	setValue(value);
}

QString EnumProperty::valueText() const
{
	if (value_ >= 0 && value_ < items_.size())
		return items_[value_];
	return QString();
}

QWidget *EnumProperty::createEditor(QWidget *parent)
{
	EnumEdit *editor = new EnumEdit(parent, items_);
	editor->setFrame(false);
	editor->setCurrentIndex(value_);

	connect(editor, SIGNAL(currentIndexChanged(int)), this, SLOT(editorChanged(int)));

	return editor;
}

/**
 * CompilationTypeProperty
 */

CompilationTypeProperty::CompilationTypeProperty(Property *parent, const QString &name, CompilationType value)
	: Property(parent, name), value_(value), defaultValue_(ctNone)
{
	items_ << QString::fromUtf8(language[lsNone].c_str());
	items_ << QString::fromUtf8(language[lsVirtualization].c_str());
	items_ << QString::fromUtf8(language[lsMutation].c_str());
	items_ << QString("%1 (%2 + %3)").arg(QString::fromUtf8(language[lsUltra].c_str())).arg(QString::fromUtf8(language[lsMutation].c_str())).arg(QString::fromUtf8(language[lsVirtualization].c_str()));
}
	
void CompilationTypeProperty::setValue(CompilationType value)
{
	if (value_ != value) {
		value_ = value;
		emit valueChanged(value);
		emit changed(this);
	}
}

void CompilationTypeProperty::editorChanged(int value)
{
	CompilationType new_value = ctNone;
	if (value > 0) {
		if (defaultValue_ == ctNone)
			new_value = static_cast<CompilationType>(value - 1);
		else
			new_value = (value == 1) ? defaultValue_ : ctNone;
	}
		
	setValue(new_value);
}

QString CompilationTypeProperty::valueText() const
{
	switch (value_) {
	case ctNone:
		return items_[0];
	case ctVirtualization:
		return items_[1];
	case ctMutation:
		return items_[2];
	case ctUltra:
		return items_[3];
	}

	return QString();
}

QWidget *CompilationTypeProperty::createEditor(QWidget *parent)
{
	QStringList items;
	items << items_[0];
	switch (defaultValue_) {
	case ctNone:
		items << items_[1];
		items << items_[2];
		items << items_[3];
		break;
	case ctVirtualization:
		items << items_[1];
		break;
	case ctMutation:
		items << items_[2];
		break;
	case ctUltra:
		items << items_[3];
		break;
	}

	int index = 0;
	if (defaultValue_ != ctNone) {
		if (value_ == defaultValue_)
			index = 1;
	} else if (value_ != ctNone) {
		index = value_ + 1;
	}

	EnumEdit *editor = new EnumEdit(parent, items);
	editor->setFrame(false);
	editor->setCurrentIndex(index);

	connect(editor, SIGNAL(currentIndexChanged(int)), this, SLOT(editorChanged(int)));

	return editor;
}

/**
 * FileNameProperty
 */

FileNameProperty::FileNameProperty(Property *parent, const QString &name, const QString &filter, const QString &value, bool saveMode)
	: Property(parent, name), filter_(filter), value_(value), saveMode_(saveMode)
{
	
}

QString FileNameProperty::valueText() const
{
	return value_;
}

QWidget *FileNameProperty::createEditor(QWidget *parent)
{
	FileNameEdit *editor = new FileNameEdit(parent);
	editor->setFrame(false);
	editor->setFilter(filter_);
	editor->setRelativePath(relativePath_);
	editor->setText(value_);
	editor->setSaveMode(saveMode_);
	connect(editor, SIGNAL(textChanged(const QString &)), this, SLOT(editorChanged(const QString &)));

	return editor;
}

void FileNameProperty::editorChanged(const QString &value)
{
	setValue(value);
}

void FileNameProperty::setValue(const QString &value)
{
	if (value_ != value) {
		value_ = value;
		emit valueChanged(value);
		emit changed(this);
	}
}

/**
 * FileNameProperty
 */

WatermarkProperty::WatermarkProperty(Property *parent, const QString &name, const QString &value)
	: Property(parent, name), value_(value)
{
	
}

QString WatermarkProperty::valueText() const
{
	return value_;
}

QWidget *WatermarkProperty::createEditor(QWidget *parent)
{
	WatermarkEdit *editor = new WatermarkEdit(parent);
	editor->setFrame(false);
	editor->setText(value_);
	connect(editor, SIGNAL(textChanged(const QString &)), this, SLOT(editorChanged(const QString &)));

	return editor;
}

void WatermarkProperty::editorChanged(const QString &value)
{
	setValue(value);
}

void WatermarkProperty::setValue(const QString &value)
{
	if (value_ != value) {
		value_ = value;
		emit valueChanged(value);
		emit changed(this);
	}
}

/**
 * PropertyManager
 */

PropertyManager::PropertyManager(QObject *parent)
	: QAbstractItemModel(parent)
{
	root_ = new GroupProperty(NULL, "");
}

PropertyManager::~PropertyManager()
{
	delete root_;
}

QModelIndex PropertyManager::index(int row, int column, const QModelIndex &parent) const
{
    //if (parent.isValid() && parent.column() != 0)
    //    return QModelIndex();

	Property *parentProp = indexToProperty(parent);
	Property *childProp = parentProp->children().value(row);
	if (!childProp)
		return QModelIndex();

	return createIndex(row, column, childProp);
}

QModelIndex PropertyManager::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	Property *childProp = indexToProperty(index);
	Property *parentProp = childProp->parent();
	if (parentProp == root_)
		return QModelIndex();

	return createIndex(parentProp->parent()->children().indexOf(parentProp), 0, parentProp);
}

int PropertyManager::columnCount(const QModelIndex & /* parent */) const
{
	return 2;
}

int PropertyManager::rowCount(const QModelIndex &parent) const
{
	Property *parentProp = indexToProperty(parent);
	return parentProp->childCount();
}

QVariant PropertyManager::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	Property *prop = indexToProperty(index);
	switch (role) {
	case Qt::DisplayRole:
		if (index.column() == 0) {
			return prop->name();
		} else if (index.column() == 1) {
			return prop->valueText();
		}
		break;
	case Qt::DecorationRole:
		if (CommandProperty *c = qobject_cast<CommandProperty *>(prop)) {
			if (index.column() == 0) {
				ICommand *command = c->value();
				IFunction *func = command->owner();
				if (qobject_cast<CommandProperty *>(c->parent())) {
					return QIcon(":/images/reference.png");
				} else if (func->ext_command_list()->GetCommandByAddress(command->address())) {
					return QIcon(":/images/link.png");
				} else if (func->is_breaked_address(command->address())) {
					return QIcon(":/images/disabled.png");
				}
				QIcon res;
				QPixmap pixmap(18, 1);
				pixmap.fill(Qt::transparent);
				res.addPixmap(pixmap);
				return res;
			}
		}
		break;
	case Qt::TextColorRole:
		if (prop->readOnly() && index.column() == 1) {
			QPalette palette;
			return palette.color(QPalette::Disabled, QPalette::Text);
		}
		break;
	case Qt::FontRole:
		if (!prop->readOnly() && index.column() == 1) {
			QFont font;
			font.setBold(true);
			return font;
		}
		break;
	case Qt::Vmp::StaticTextRole:
		if (prop->hasStaticText(index.column()))
			return prop->staticText(index.column());
		break;
	case Qt::Vmp::StaticColorRole:
		if (prop->hasStaticText(index.column()))
			return prop->staticColor(index.column());
		break;
	}

	return QVariant();
}

QVariant PropertyManager::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0) {
			return QString::fromUtf8(language[lsName].c_str());
		} else {
			return QString::fromUtf8(language[lsValue].c_str());
		}
	}
	return QVariant();
}

void PropertyManager::clear()
{
	root_->clear();
}

BoolProperty *PropertyManager::addBoolProperty(Property *parent, const QString &name, bool value)
{
	BoolProperty *prop = new BoolProperty(parent ? parent : root_, name, value);
	addProperty(prop);
	return prop;
}

StringProperty *PropertyManager::addStringProperty(Property *parent, const QString &name, const QString &value)
{
	StringProperty *prop = new StringProperty(parent ? parent : root_, name, value);
	addProperty(prop);
	return prop;
}

DateProperty *PropertyManager::addDateProperty(Property *parent, const QString &name, const QDate &value)
{
	DateProperty *prop = new DateProperty(parent ? parent : root_, name, value);
	addProperty(prop);
	return prop;
}

StringListProperty *PropertyManager::addStringListProperty(Property *parent, const QString &name, const QString &value)
{
	StringListProperty *prop = new StringListProperty(parent ? parent : root_, name, value);
	addProperty(prop);
	return prop;
}

GroupProperty *PropertyManager::addGroupProperty(Property *parent, const QString &name)
{
	GroupProperty *prop = new GroupProperty(parent ? parent : root_, name);
	addProperty(prop);
	return prop;
}

EnumProperty *PropertyManager::addEnumProperty(Property *parent, const QString &name, const QStringList &items, int value)
{
	EnumProperty *prop = new EnumProperty(parent ? parent : root_, name, items, value);
	addProperty(prop);
	return prop;
}

FileNameProperty *PropertyManager::addFileNameProperty(Property *parent, const QString &name, const QString &filter, const QString &value, bool saveMode)
{
	FileNameProperty *prop = new FileNameProperty(parent ? parent : root_, name, filter, value, saveMode);
	addProperty(prop);
	return prop;
}

CommandProperty *PropertyManager::addCommandProperty(Property *parent, ICommand *value)
{
	CommandProperty *prop = new CommandProperty(parent ? parent : root_, value);
	addProperty(prop);
	return prop;
}

WatermarkProperty *PropertyManager::addWatermarkProperty(Property *parent, const QString &name, const QString &value)
{
	WatermarkProperty *prop = new WatermarkProperty(parent ? parent : root_, name, value);
	addProperty(prop);
	return prop;
}

CompilationTypeProperty *PropertyManager::addCompilationTypeProperty(Property *parent, const QString &name, CompilationType value)
{
	CompilationTypeProperty *prop = new CompilationTypeProperty(parent ? parent : root_, name, value);
	addProperty(prop);
	return prop;
}

void PropertyManager::addProperty(Property *prop)
{
	if (!prop)
		return;

	connect(prop, SIGNAL(destroyed(Property *)), this, SLOT(slotPropertyDestroyed(Property *)));
	connect(prop, SIGNAL(changed(Property *)), this, SLOT(slotPropertyChanged(Property *)));
}

Property *PropertyManager::indexToProperty(const QModelIndex &index) const
{
	if (index.isValid())
		return static_cast<Property *>(index.internalPointer());
	return root_;
}

QModelIndex PropertyManager::propertyToIndex(Property *prop) const
{
	if (!prop)
		return QModelIndex();

	return createIndex(prop->parent()->children().indexOf(prop), 0, prop);
}

void PropertyManager::slotPropertyChanged(Property *prop)
{
	QModelIndex index = propertyToIndex(prop);
	emit dataChanged(index, index.sibling(index.row(), 1));
}

void PropertyManager::slotPropertyDestroyed(Property * /*prop*/)
{
	//
}

Qt::ItemFlags PropertyManager::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	Qt::ItemFlags res = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	Property *prop = indexToProperty(index);
	if (prop && prop->hasValue() && !prop->readOnly())
		res |= Qt::ItemIsEditable;
	return res;
}

/**
 * PropertyEditorDelegate
 */

PropertyEditorDelegate::PropertyEditorDelegate(QObject *parent)
	: TreeViewItemDelegate(parent)
{

}

Property *PropertyEditorDelegate::indexToProperty(const QModelIndex &index) const
{
	return static_cast<Property *>(index.internalPointer());
}

QWidget *PropertyEditorDelegate::createEditor(QWidget *parent,
        const QStyleOptionViewItem &, const QModelIndex &index) const
{
	if (index.column() != 1)
		return NULL;

	Property *prop = indexToProperty(index);
	if (!prop)
		return NULL;

	QWidget *editor = prop->createEditor(parent);
	if (editor) {
		QWidget *corrBearing = dynamic_cast<QLineEdit *>(editor);
		editor->setObjectName("editor");
		editor->setAutoFillBackground(true);
		QVariant value = index.data(Qt::FontRole);
		if (value.isValid() && value.canConvert<QFont>())
			editor->setFont(qvariant_cast<QFont>(value));
		if (QComboBox *c = qobject_cast<QComboBox *>(editor)) {
			c->lineEdit()->selectAll();
			corrBearing = c;
		} else if (QPlainTextEdit *c = qobject_cast<QPlainTextEdit *>(editor)) {
			c->selectAll();
		}
		if(corrBearing)
		{
			corrBearing->setProperty("corrBearing", 
#ifdef __APPLE__
				false
#else
				true
#endif
			);
			corrBearing->style()->unpolish(corrBearing);
			corrBearing->style()->polish(corrBearing);
			corrBearing->update();
		}
	}
	return editor;
}

void PropertyEditorDelegate::setEditorData(QWidget * /*widget*/, const QModelIndex &) const
{
	//do nothing
}

/**
 * TreePropertyEditor
 */

TreePropertyEditor::TreePropertyEditor(QWidget *parent)
	: TreeView(parent)
{
	setObjectName("grid");
	setIconSize(QSize(18, 18));
	setFrameShape(QFrame::NoFrame);
	setItemDelegate(new PropertyEditorDelegate(this));
}

Property *TreePropertyEditor::indexToProperty(const QModelIndex &index) const
{
	return static_cast<Property *>(index.internalPointer());
}

void TreePropertyEditor::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Return:
	case Qt::Key_Enter:
	case Qt::Key_Space: // Trigger Edit
		{
			QModelIndex index = currentIndex();
			if ((index.flags() & (Qt::ItemIsEditable | Qt::ItemIsEnabled)) == (Qt::ItemIsEditable | Qt::ItemIsEnabled)) {
				event->accept();
				// If the current position is at column 0, move to 1.
				if (index.column() == 0) {
					index = index.sibling(index.row(), 1);
					setCurrentIndex(index);
				}
				edit(index);
				return;
			}
		}
		break;
	default:
		break;
	}

	TreeView::keyPressEvent(event);
}

void TreePropertyEditor::mousePressEvent(QMouseEvent *event)
{
    QTreeView::mousePressEvent(event);
    QModelIndex index = indexAt(event->pos());

	if (index.isValid()) {
		Property *prop = indexToProperty(index);
		if ((event->button() == Qt::LeftButton) && (index.column() == 1) && ((index.flags() & (Qt::ItemIsEditable | Qt::ItemIsEnabled)) == (Qt::ItemIsEditable | Qt::ItemIsEnabled))) {
			edit(index);
		} else if (!rootIsDecorated() && prop->childCount()) {
			QRect rect = visualRect(index);
			if (event->pos().x() >= rect.left() && event->pos().x() <= rect.left() + 20)
				setExpanded(index, !isExpanded(index));
        }
    }
}

void TreePropertyEditor::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> &roles /*= QVector<int>()*/)
{
	QRect rect = visualRect(topLeft);
	QTreeView::dataChanged(topLeft, bottomRight, roles);
	if (topLeft.row() == bottomRight.row() && topLeft.isValid()) {
		if (rect != visualRect(bottomRight))
			updateEditorGeometries();
	}
}

/**
 * LicensePropertyManager
 */

LicensePropertyManager::LicensePropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL), lock_(false)
{
	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));
	licenseName_ = addStringProperty(details_, QString::fromUtf8(language[lsCustomerName].c_str()), QString());
	licenseEmail_ = addStringProperty(details_, QString::fromUtf8(language[lsEmail].c_str()), QString());
	licenseDate_ = addDateProperty(details_, QString::fromUtf8(language[lsDate].c_str()), QDate());
	licenseOrder_ = addStringProperty(details_, QString::fromUtf8(language[lsOrderRef].c_str()), QString());
	licenseComments_ = addStringListProperty(details_, QString::fromUtf8(language[lsComments].c_str()), QString());
	licenseBlocked_ = addBoolProperty(details_, QString::fromUtf8(language[lsBlocked].c_str()), false);
	licenseSerialNumber_ = addStringProperty(details_, QString::fromUtf8(language[lsSerialNumber].c_str()), QString());
	licenseSerialNumber_->setReadOnly(true);
	
	contents_ = addGroupProperty(NULL, QString::fromUtf8(language[lsSerialNumberContents].c_str()));
	serialName_ = addStringProperty(contents_, QString::fromUtf8(language[lsCustomerName].c_str()), QString());
	serialName_->setReadOnly(true);
	serialEmail_ = addStringProperty(contents_, QString::fromUtf8(language[lsEmail].c_str()), QString());
	serialEmail_->setReadOnly(true);
	serialHWID_ = addStringProperty(contents_, QString::fromUtf8(language[lsHardwareID].c_str()), QString());
	serialHWID_->setReadOnly(true);
	serialExpirationDate_ = addDateProperty(contents_, QString::fromUtf8(language[lsExpirationDate].c_str()), QDate());
	serialExpirationDate_->setReadOnly(true);
	serialTimeLimit_ = addStringProperty(contents_, QString::fromUtf8(language[lsRunningTimeLimit].c_str()), QString());
	serialTimeLimit_->setReadOnly(true);
	serialMaxBuildDate_ = addDateProperty(contents_, QString::fromUtf8(language[lsMaxBuildDate].c_str()), QDate());
	serialMaxBuildDate_->setReadOnly(true);
	serialUserData_ = addStringProperty(contents_, QString::fromUtf8(language[lsUserData].c_str()), QString());
	serialUserData_->setReadOnly(true);

	connect(licenseName_, SIGNAL(valueChanged(const QString &)), this, SLOT(nameChanged(const QString &)));
	connect(licenseEmail_, SIGNAL(valueChanged(const QString &)), this, SLOT(emailChanged(const QString &)));
	connect(licenseDate_, SIGNAL(valueChanged(const QDate &)), this, SLOT(dateChanged(const QDate &)));
	connect(licenseOrder_, SIGNAL(valueChanged(const QString &)), this, SLOT(orderChanged(const QString &)));
	connect(licenseComments_, SIGNAL(valueChanged(const QString &)), this, SLOT(commentsChanged(const QString &)));
	connect(licenseBlocked_, SIGNAL(valueChanged(bool)), this, SLOT(blockedChanged(bool)));

	localize();
}

void LicensePropertyManager::localize()
{
	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	licenseName_->setName(QString::fromUtf8(language[lsCustomerName].c_str()));
	licenseEmail_->setName(QString::fromUtf8(language[lsEmail].c_str()));
	licenseDate_->setName(QString::fromUtf8(language[lsDate].c_str()));
	licenseComments_->setName(QString::fromUtf8(language[lsComments].c_str()));
	licenseOrder_->setName(QString::fromUtf8(language[lsOrderRef].c_str()));
	licenseBlocked_->setName(QString::fromUtf8(language[lsBlocked].c_str()));
	licenseSerialNumber_->setName(QString::fromUtf8(language[lsSerialNumber].c_str()));

	contents_->setName(QString::fromUtf8(language[lsSerialNumberContents].c_str()));
	serialName_->setName(QString::fromUtf8(language[lsCustomerName].c_str()));
	serialEmail_->setName(QString::fromUtf8(language[lsEmail].c_str()));
	serialHWID_->setName(QString::fromUtf8(language[lsHardwareID].c_str()));
	serialExpirationDate_->setName(QString::fromUtf8(language[lsExpirationDate].c_str()));
	serialTimeLimit_->setName(QString::fromUtf8(language[lsRunningTimeLimit].c_str()));
	serialMaxBuildDate_->setName(QString::fromUtf8(language[lsMaxBuildDate].c_str()));
	serialUserData_->setName(QString::fromUtf8(language[lsUserData].c_str()));
}

QVariant LicensePropertyManager::data(const QModelIndex &index, int role) const
{
	Property *prop = indexToProperty(index);
	if (role == Qt::FontRole) {
		if (index.column() == 1 && (prop == licenseSerialNumber_ || prop == serialUserData_))
			return QFont(MONOSPACE_FONT_FAMILY);
	}

	return PropertyManager::data(index, role);
}

void LicensePropertyManager::nameChanged(const QString &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_customer_name(value.toUtf8().constData());
#endif
}

void LicensePropertyManager::emailChanged(const QString &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_customer_email(value.toUtf8().constData());
#endif
}

void LicensePropertyManager::orderChanged(const QString &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_order_ref(value.toUtf8().constData());
#endif
}

void LicensePropertyManager::dateChanged(const QDate &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_date(LicenseDate(value.year(), value.month(), value.day()));
#endif
}

void LicensePropertyManager::commentsChanged(const QString &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_comments(value.toUtf8().constData());
#endif
}

void LicensePropertyManager::blockedChanged(bool value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_blocked(value);
#endif
}

void LicensePropertyManager::setValue(License *value)
{ 
	value_ = value;
	update();
}

void LicensePropertyManager::update()
{
#ifdef ULTIMATE
	lock_ = true;

	licenseName_->setValue(value_ ? QString::fromUtf8(value_->customer_name().c_str()) : QString());
	licenseEmail_->setValue(value_ ? QString::fromUtf8(value_->customer_email().c_str()) : QString());
	licenseDate_->setValue(value_ ? QDate(value_->date().Year, value_->date().Month, value_->date().Day) : QDate());
	licenseOrder_->setValue(value_ ? QString::fromUtf8(value_->order_ref().c_str()) : QString());
	licenseComments_->setValue(value_ ? QString::fromUtf8(value_->comments().c_str()) : QString());
	licenseBlocked_->setValue(value_ ? value_->blocked() : false);
	licenseSerialNumber_->setValue(value_ ? QString::fromLatin1(formatBase64(value_->serial_number()).c_str()) : QString());

	LicenseInfo *license_info = value_ ? value_->info() : NULL;
	serialName_->setValue(license_info && (license_info->Flags & HAS_USER_NAME) ? QString::fromUtf8(license_info->CustomerName.c_str()) : QString());
	serialEmail_->setValue(license_info && (license_info->Flags & HAS_EMAIL) ? QString::fromUtf8(license_info->CustomerEmail.c_str()) : QString());
	serialHWID_->setValue(license_info && (license_info->Flags & HAS_HARDWARE_ID) ? QString::fromLatin1(license_info->HWID.c_str()) : QString());
	serialTimeLimit_->setValue(license_info && (license_info->Flags & HAS_TIME_LIMIT) ? QString::number(license_info->RunningTimeLimit) : QString());
	serialExpirationDate_->setValue(license_info && (license_info->Flags & HAS_EXP_DATE) ? QDate(license_info->ExpireDate.Year, license_info->ExpireDate.Month, license_info->ExpireDate.Day) : QDate());
	serialMaxBuildDate_->setValue(license_info && (license_info->Flags & HAS_MAX_BUILD_DATE) ? QDate(license_info->MaxBuildDate.Year, license_info->MaxBuildDate.Month, license_info->MaxBuildDate.Day) : QDate());
	QString userData;
	if (license_info && license_info->Flags & HAS_USER_DATA) {
		int bytes = 16;
		for (size_t line = 0; line <= license_info->UserData.size() / bytes; line++) {
			QString hex;
			QString text;
			for (int i = 0; i < bytes; i++) {
				size_t pos = line * bytes + i;
				if (pos < license_info->UserData.size()) {
					uint8_t value = static_cast<uint8_t>(license_info->UserData[pos]);
					hex.append(QString("%1 ").arg(value, 2, 16, QChar('0')));
					QChar qc(value);
					if (qc.unicode() >= 127 || !qc.isPrint())
						qc = 0xB7;
					text += qc;
				} else {
					if (i == 0)
						break;
					hex.append("   ");
					text.append(" ");
				}
			}
			if (!hex.isEmpty()) {
				if (!userData.isEmpty())
					userData.append('\n');
				userData.append(QString("%1 ").arg(line * bytes, 4, 16, QChar('0'))).append(hex).append(text);
			}
		}
	}
	serialUserData_->setValue(userData);

	if (serialHWID_->childCount()) {
		beginRemoveRows(propertyToIndex(serialHWID_), 0, serialHWID_->childCount());
		serialHWID_->clear();
		endRemoveRows();
	}
	if (license_info && (license_info->Flags & HAS_HARDWARE_ID) && !license_info->HWID.empty()) {
		size_t len = license_info->HWID.size();
		uint8_t *hwid = new uint8_t[len];
		Base64Decode(license_info->HWID.c_str(), license_info->HWID.size(), hwid, len);
		beginInsertRows(propertyToIndex(serialHWID_), 0, (int)(len / 4));
		for (size_t i = 0; i < len; i += 4) {
			uint32_t value = *reinterpret_cast<uint32_t *>(&hwid[i]);
			QString name;
			switch (value & 3) {
			case 0:
				name = QString::fromUtf8(language[lsCPU].c_str());
				break;
			case 1:
				name = QString::fromUtf8(language[lsHost].c_str());
				break;
			case 2:
				name = QString::fromUtf8(language[lsEthernet].c_str());
				break;
			case 3:
				name = QString::fromUtf8(language[lsHDD].c_str());
				break;
			}
			StringProperty *prop = addStringProperty(serialHWID_, name, QString("%1").arg(value & ~3, 8, 16, QChar('0')).toUpper());
			prop->setReadOnly(true);
		}
		delete [] hwid;
		endInsertRows();
	}

	lock_ = false;
#endif
}

/**
 * FilePropertyManager
 */

InternalFilePropertyManager::InternalFilePropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL), lock_(false)
{
	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));
	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	fileName_ = addFileNameProperty(details_, QString::fromUtf8(language[lsFileName].c_str()), QString(
#ifdef VMP_GNU
		"%1 (*)"
#else
		"%1 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsAllFiles].c_str())), QString());

	action_ = addEnumProperty(details_, QString::fromUtf8(language[lsAction].c_str()), QStringList() << QString::fromUtf8(language[lsNo].c_str())
		<< QString::fromUtf8(language[lsLoadAtStart].c_str()) << QString::fromUtf8(language[lsRegisterCOMServer].c_str()) << QString::fromUtf8(language[lsInstallCOMServer].c_str()), 0);

	connect(name_, SIGNAL(valueChanged(const QString &)), this, SLOT(nameChanged(const QString &)));
	connect(fileName_, SIGNAL(valueChanged(const QString &)), this, SLOT(fileNameChanged(const QString &)));
	connect(action_, SIGNAL(valueChanged(int)), this, SLOT(actionChanged(int)));
}

void InternalFilePropertyManager::localize()
{
	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	fileName_->setName(QString::fromUtf8(language[lsFileName].c_str()));
	fileName_->setFilter(QString(
#ifdef VMP_GNU
		"%1 (*)"
#else
		"%1 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsAllFiles].c_str())));
	action_->setName(QString::fromUtf8(language[lsAction].c_str()));
	action_->setText(0, QString::fromUtf8(language[lsNo].c_str()));
	action_->setText(1, QString::fromUtf8(language[lsLoadAtStart].c_str()));
	action_->setText(2, QString::fromUtf8(language[lsRegisterCOMServer].c_str()));
	action_->setText(3, QString::fromUtf8(language[lsInstallCOMServer].c_str()));
}

void InternalFilePropertyManager::nameChanged(const QString &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_name(value.toUtf8().constData());
#endif
}

void InternalFilePropertyManager::fileNameChanged(const QString &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_file_name(value.toUtf8().constData());
#endif
}

void InternalFilePropertyManager::actionChanged(int value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_action(static_cast<InternalFileAction>(value));
#endif
}

void InternalFilePropertyManager::setValue(InternalFile *value)
{ 
	value_ = value;

#ifdef ULTIMATE
	fileName_->setRelativePath(value_ ? QString::fromUtf8(value_->owner()->owner()->project_path().c_str()) : QString());
	update();
#endif
}

void InternalFilePropertyManager::update()
{
#ifdef ULTIMATE
	lock_ = true;

	name_->setValue(value_ ? QString::fromUtf8(value_->name().c_str()) : QString());
	fileName_->setValue(value_ ? QString::fromUtf8(value_->file_name().c_str()) : QString());
	int i = 0;
	if (value_) {
		switch (value_->action()) {
		case faLoad:
			i = 1;
			break;
		case faRegister:
			i = 2;
			break;
		case faInstall:
			i = 3;
			break;
		}
	}
	action_->setValue(value_ ? i : 0);

	lock_ = false;
#endif
}

/**
* AssemblyPropertyManager
*/

AssemblyPropertyManager::AssemblyPropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL), lock_(false)
{
	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));
	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	fileName_ = addFileNameProperty(details_, QString::fromUtf8(language[lsFileName].c_str()), 
		QString("%1 (*.dll);;%2 (*.*)").arg(QString::fromUtf8(language[lsAssemblies].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())), QString());

	connect(name_, SIGNAL(valueChanged(const QString &)), this, SLOT(nameChanged(const QString &)));
	connect(fileName_, SIGNAL(valueChanged(const QString &)), this, SLOT(fileNameChanged(const QString &)));
}

void AssemblyPropertyManager::localize()
{
	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	fileName_->setName(QString::fromUtf8(language[lsFileName].c_str()));
	fileName_->setFilter(QString("%1 (*.dll);;%2 (*.*)").arg(QString::fromUtf8(language[lsAssemblies].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));
}

void AssemblyPropertyManager::nameChanged(const QString &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_name(value.toUtf8().constData());
#endif
}

void AssemblyPropertyManager::fileNameChanged(const QString &value)
{
	if (!value_ || lock_)
		return;

#ifdef ULTIMATE
	value_->set_file_name(value.toUtf8().constData());
#endif
}

void AssemblyPropertyManager::setValue(InternalFile *value)
{
	value_ = value;

#ifdef ULTIMATE
	fileName_->setRelativePath(value_ ? QString::fromUtf8(value_->owner()->owner()->project_path().c_str()) : QString());
	update();
#endif
}

void AssemblyPropertyManager::update()
{
#ifdef ULTIMATE
	lock_ = true;

	name_->setValue(value_ ? QString::fromUtf8(value_->name().c_str()) : QString());
	fileName_->setValue(value_ ? QString::fromUtf8(value_->file_name().c_str()) : QString());

	lock_ = false;
#endif
}

/**
 * FunctionPropertyManager
 */

FunctionPropertyManager::FunctionPropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL), lock_(false)
{
	protection_ = addGroupProperty(NULL, QString::fromUtf8(language[lsProtection].c_str()));
	compilationType_ = addCompilationTypeProperty(protection_, QString::fromUtf8(language[lsCompilationType].c_str()), ctNone);
	lockToKey_ = addBoolProperty(protection_, QString::fromUtf8(language[lsLockToSerialNumber].c_str()), false);

	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));
	type_ = addStringProperty(details_, QString::fromUtf8(language[lsType].c_str()), QString());
	type_->setReadOnly(true);
	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	name_->setReadOnly(true);
	address_ = addStringProperty(details_, QString::fromUtf8(language[lsAddress].c_str()), QString());
	address_->setReadOnly(true);

	connect(compilationType_, SIGNAL(valueChanged(CompilationType)), this, SLOT(compilationTypeChanged(CompilationType)));
	connect(lockToKey_, SIGNAL(valueChanged(bool)), this, SLOT(lockToKeyChanged(bool)));
}

void FunctionPropertyManager::localize()
{
	protection_->setName(QString::fromUtf8(language[lsProtection].c_str()));
	compilationType_->setName(QString::fromUtf8(language[lsCompilationType].c_str()));
	compilationType_->setText(0, QString::fromUtf8(language[lsNone].c_str()));
	compilationType_->setText(1, QString::fromUtf8(language[lsVirtualization].c_str()));
	compilationType_->setText(2, QString::fromUtf8(language[lsMutation].c_str()));
	compilationType_->setText(3, QString("%1 (%2 + %3)").arg(QString::fromUtf8(language[lsUltra].c_str())).arg(QString::fromUtf8(language[lsMutation].c_str())).arg(QString::fromUtf8(language[lsVirtualization].c_str())));
	compilationType_->setToolTip(QString::fromUtf8(language[lsCompilationTypeHelp].c_str()));

	lockToKey_->setName(QString::fromUtf8(language[lsLockToSerialNumber].c_str()));
	lockToKey_->setToolTip(QString::fromUtf8(language[lsLockToSerialNumberHelp].c_str()));

	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	type_->setName(QString::fromUtf8(language[lsType].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	address_->setName(QString::fromUtf8(language[lsAddress].c_str()));

	if (value_) {
		for (QMap<IArchitecture *, ArchInfo>::const_iterator arch = map_.begin(); arch != map_.end(); arch++) {
			QString str = QString::fromUtf8(language[lsCode].c_str());
			if (value_->show_arch_name())
				str.append(" ").append(QString::fromLatin1(arch.key()->name().c_str()));
			arch.value().code->setName(str);
		}
		update();
	}
}

QVariant FunctionPropertyManager::data(const QModelIndex &index, int role) const
{
	Property *prop = indexToProperty(index);
	if (role == Qt::FontRole) {
		if (qobject_cast<CommandProperty*>(prop))
			return QFont(MONOSPACE_FONT_FAMILY);
	}

	return PropertyManager::data(index, role);
}

void FunctionPropertyManager::setValue(FunctionBundle *value)
{
	if (value_ == value)
		return;

	value_ = value;

	for (QMap<IArchitecture *, ArchInfo>::iterator arch = map_.begin(); arch != map_.end(); arch++) {
		Property *code = arch.value().code;
		if (code->childCount()) {
			beginRemoveRows(propertyToIndex(code), 0, code->childCount());
			code->clear();
			endRemoveRows();
		}
		arch.value().map.clear();
	}

	if (value_) {
		for (size_t i = 0; i < value_->count(); i++) {
			internalAddFunction(value_->item(i)->func());
		}
	} else {
		for (QMap<IArchitecture *, ArchInfo>::const_iterator arch = map_.begin(); arch != map_.end(); arch++) {
			Property *code = arch.value().code;
			int index = code->parent()->children().indexOf(code);
			beginRemoveRows(propertyToIndex(code->parent()), index, index + 1);
			delete code;
			endRemoveRows();
		}
		map_.clear();
	}

	update();
}

void FunctionPropertyManager::addFunction(IFunction *func)
{
	if (internalAddFunction(func))
		update();
}

void FunctionPropertyManager::createCommands(IFunction *func, Property *code)
{
	size_t i;
	QMap<uint64_t, QList<ICommand*> > references;

	for (i = 0; i < func->link_list()->count(); i++) {
		CommandLink *link = func->link_list()->item(i);
		if (link->to_address()) {
			QMap<uint64_t, QList<ICommand*> >::iterator it = references.find(link->to_address());
			if (it == references.end())
				it = references.insert(link->to_address(), QList<ICommand*>() << link->from_command());
			else
				it.value().push_back(link->from_command());
		}
	}

	beginInsertRows(propertyToIndex(code), 0, (int)func->count());
	for (i = 0; i < func->count(); i++) {
		ICommand *command = func->item(i);
		Property *prop = addCommandProperty(code, command);
		prop->setReadOnly(true);

		QMap<uint64_t, QList<ICommand*> >::ConstIterator it = references.find(command->address());
		if (it != references.end()) {
			for (int j = 0; j < it.value().size(); j++) {
				Property *link_prop = addCommandProperty(prop, it.value().at(j));
				link_prop->setReadOnly(true);
			}
		}
	}
	endInsertRows();
}

bool FunctionPropertyManager::internalAddFunction(IFunction *func)
{
	if (!value_)
		return false;

	FunctionArch *func_arch = value_->GetArchByFunction(func);
	if (!func_arch)
		return false;

	QMap<IArchitecture *, ArchInfo>::iterator arch = map_.find(func_arch->arch());
	if (arch == map_.end()) {
		beginInsertRows(propertyToIndex(details_), details_->childCount(), details_->childCount() + 1);
		QString str = QString::fromUtf8(language[lsCode].c_str());
		if (value_->show_arch_name())
			str.append(" ").append(QString::fromLatin1(func_arch->arch()->name().c_str()));
		ArchInfo info;
		info.code = addStringProperty(details_, str, QString());
		info.code->setReadOnly(true);
		endInsertRows();
		arch = map_.insert(func_arch->arch(), info);
	}
	QMap<IFunction *, StringProperty *>::const_iterator it = arch.value().map.find(func);
	if (it == arch.value().map.end()) {
		StringProperty *code = arch.value().code;
		if (!arch.value().map.empty()) {
			if (arch.value().map.size() == 1) {
				QMap<IFunction *, StringProperty *>::iterator tmp = arch.value().map.begin();
				if (tmp.value() == code) {
					beginRemoveRows(propertyToIndex(code), 0, code->childCount());
					code->clear();
					endRemoveRows();

					beginInsertRows(propertyToIndex(code), code->childCount(), code->childCount() + 1);
					StringProperty *newCode = addStringProperty(code, QString::fromLatin1(tmp.key()->display_address("").c_str()), QString());
					newCode->setReadOnly(true);
					endInsertRows();
					tmp.value() = newCode;

					createCommands(tmp.key(), newCode);
				}
			}
			beginInsertRows(propertyToIndex(code), code->childCount(), code->childCount() + 1);
			code = addStringProperty(code, QString::fromLatin1(func->display_address("").c_str()), QString());
			code->setReadOnly(true);
			endInsertRows();
		}
		it = arch.value().map.insert(func, code);
	}
	createCommands(func, it.value());

	return true;
}

void FunctionPropertyManager::updateFunction(IFunction *func)
{
	if (value_ && value_->GetArchByFunction(func))
		update();
}

bool FunctionPropertyManager::removeFunction(IFunction *func)
{
	if (internalRemoveFunction(func)) {
		update();
		return true;
	}
	return false;
}

bool FunctionPropertyManager::internalRemoveFunction(IFunction *func)
{
	if (!value_)
		return false;

	FunctionArch *func_arch = value_->GetArchByFunction(func);
	if (!func_arch)
		return false;

	QMap<IArchitecture *, ArchInfo>::iterator arch = map_.find(func_arch->arch());
	if (arch != map_.end()) {
		QMap<IFunction *, StringProperty *>::iterator it = arch.value().map.find(func);
		if (it != arch.value().map.end()) {
			StringProperty *code = it.value();
			if (code == arch.value().code) {
				beginRemoveRows(propertyToIndex(code), 0, code->childCount());
				code->clear();
				endRemoveRows();
			} else {
				int index = code->parent()->children().indexOf(code);
				beginRemoveRows(propertyToIndex(code->parent()), index, index + 1);
				delete code;
				endRemoveRows();
			}
			arch.value().map.erase(it);
		}
	}

	return true;
}

void FunctionPropertyManager::update()
{
	lock_ = true;

	name_->setValue(value_ ? QString::fromUtf8(value_->display_name().c_str()) : QString());

	QString str;
	if (value_) {
		switch (value_->type()) {
		case otAPIMarker:
		case otMarker:
			str = QString::fromUtf8(language[lsMarker].c_str());
			break;
		case otString:
			str = QString::fromUtf8(language[lsString].c_str());
			break;
		case otCode:
			str = QString::fromUtf8(language[lsFunction].c_str());
			break;
		case otExport:
			str = QString::fromUtf8(language[lsExport].c_str());
			break;
		default:
			str = QString::fromUtf8(language[lsUnknown].c_str());
			break;
		}
	} else {
		str = "";
	}
	type_->setValue(str);
	address_->setValue(value_ ? QString::fromUtf8(value_->display_address().c_str()) : QString());

	compilationType_->setValue(value_ && value_->need_compile() ? value_->compilation_type() : ctNone);
	compilationType_->setDefaultValue(value_ ? value_->default_compilation_type() : ctNone);
	lockToKey_->setReadOnly(value_ && (!value_->need_compile() || value_->compilation_type() == ctMutation));
	lockToKey_->setValue((value_ && !lockToKey_->readOnly()) ? (value_->compilation_options() & coLockToKey) != 0 : false);

	for (QMap<IArchitecture *, ArchInfo>::const_iterator arch = map_.begin(); arch != map_.end(); arch++) {
		ArchInfo info = arch.value();
		if (info.code->childCount())
			info.code->setValue(QString::number(info.code->childCount()) + " " + QString::fromUtf8(language[lsItems].c_str()));
		else
			info.code->setValue(QString());
		for (QMap<IFunction *, StringProperty *>::const_iterator it = info.map.begin(); it != info.map.end(); it++) {
			IFunction *func = it.key();
			StringProperty *code = it.value();

			str.clear();
			if (func->type() != otUnknown) {
				str = QString::number(func->count()) + " " + QString::fromUtf8(language[lsItems].c_str());
				QString hint;
				if (func->ext_command_list()->count()) {
					if (!hint.isEmpty())
						hint.append(", ");
					hint.append(QString::fromUtf8(language[lsExternalAddress].c_str())).append(": ").append(QString::number(func->ext_command_list()->count()));
				}
				if (func->break_address()) {
					if (!hint.isEmpty())
						hint.append(", ");
					hint.append(QString::fromUtf8(language[lsBreakAddress].c_str()));
				}
				if (!hint.isEmpty())
					str.append(" (").append(hint).append(")");
			}
			code->setValue(str);
		}
	}

	lock_ = false;
}

void FunctionPropertyManager::compilationTypeChanged(CompilationType value)
{
	if (!value_ || lock_)
		return;

	if (value == ctNone) {
		value_->set_need_compile(false);
	} else {
		value_->set_need_compile(true);
		value_->set_compilation_type(value);
	}
}

void FunctionPropertyManager::lockToKeyChanged(bool value)
{
	if (!value_ || lock_)
		return;

	uint32_t options = value_->compilation_options();
	if (value) {
		options |= coLockToKey;
	} else {
		options &= ~coLockToKey;
	}
	value_->set_compilation_options(options);
}

ICommand *FunctionPropertyManager::indexToCommand(const QModelIndex &index) const
{
	if (!index.isValid())
		return NULL;

	Property *prop = indexToProperty(index);
	if (prop) {
		if (CommandProperty *c = qobject_cast<CommandProperty*>(prop))
			return c->value();
	}
	return NULL;
}

IFunction *FunctionPropertyManager::indexToFunction(const QModelIndex &index) const
{
	if (!index.isValid())
		return NULL;

	Property *prop = indexToProperty(index);
	if (prop) {
		for (QMap<IArchitecture *, ArchInfo>::const_iterator arch = map_.begin(); arch != map_.end(); arch++) {
			for (QMap<IFunction *, StringProperty *>::const_iterator it = arch.value().map.begin(); it != arch.value().map.end(); it++) {
				if (it.value() == prop)
					return it.key();
			}
		}
	}
	return NULL;
}

QModelIndex FunctionPropertyManager::commandToIndex(ICommand *command) const
{
	for (QMap<IArchitecture *, ArchInfo>::const_iterator arch = map_.begin(); arch != map_.end(); arch++) {
		ArchInfo info = arch.value();
		for (QMap<IFunction *, StringProperty *>::const_iterator it = info.map.begin(); it != info.map.end(); it++) {
			QList<Property *> propList = it.value()->children();
			for (int j = 0; j < propList.size(); j++) {
				Property *prop = propList[j];
				if (reinterpret_cast<CommandProperty*>(prop)->value() == command)
					return propertyToIndex(prop);
			}
		}
	}
	return QModelIndex();
}

/**
 * SectionPropertyManager
 */

SectionPropertyManager::SectionPropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL)
{
	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));

	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	name_->setReadOnly(true);
	address_ = addStringProperty(details_, QString::fromUtf8(language[lsAddress].c_str()), QString());
	address_->setReadOnly(true);
	virtual_size_ = addStringProperty(details_, QString::fromUtf8(language[lsSize].c_str()), QString());
	virtual_size_->setReadOnly(true);
	physical_offset_ = addStringProperty(details_, QString::fromUtf8(language[lsRawAddress].c_str()), QString());
	physical_offset_->setReadOnly(true);
	physical_size_ = addStringProperty(details_, QString::fromUtf8(language[lsRawSize].c_str()), QString());
	physical_size_->setReadOnly(true);
	flags_ = addStringProperty(details_, QString::fromUtf8(language[lsFlags].c_str()), QString());
	flags_->setReadOnly(true);
}

void SectionPropertyManager::localize()
{
	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	address_->setName(QString::fromUtf8(language[lsAddress].c_str()));
	virtual_size_->setName(QString::fromUtf8(language[lsSize].c_str()));
	physical_offset_->setName(QString::fromUtf8(language[lsRawAddress].c_str()));
	physical_size_->setName(QString::fromUtf8(language[lsRawSize].c_str()));
	flags_->setName(QString::fromUtf8(language[lsFlags].c_str()));
}

void SectionPropertyManager::setValue(ISection *value)
{
	value_ = value;

	update();
}

void SectionPropertyManager::update()
{
	name_->setValue(value_ ? QString::fromUtf8(value_->name().c_str()) : QString());
	address_->setValue(value_ ? QString::fromUtf8(DisplayValue(value_->address_size(), value_->address()).c_str()) : QString());
	virtual_size_->setValue(value_ ? QString::fromUtf8(DisplayValue(value_->address_size(), value_->size()).c_str()) : QString());
	physical_offset_->setValue(value_ ? QString::fromUtf8(DisplayValue(osDWord, value_->physical_offset()).c_str()) : QString());
	physical_size_->setValue(value_ ? QString::fromUtf8(DisplayValue(osDWord, value_->physical_size()).c_str()) : QString());
	flags_->setValue(value_ ? QString::fromLatin1(DisplayValue(osDWord, value_->flags()).c_str()) : QString());
}

/**
 * SegmentPropertyManager
 */

SegmentPropertyManager::SegmentPropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL), lock_(false)
{
	options_ = addGroupProperty(NULL, QString::fromUtf8(language[lsOptions].c_str()));
	excluded_from_memory_protection_ = addBoolProperty(options_, QString::fromUtf8(language[lsExcludedFromMemoryProtection].c_str()), false);
	excluded_from_packing_ = addBoolProperty(options_, QString::fromUtf8(language[lsExcludedFromPacking].c_str()), false);

	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));
	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	name_->setReadOnly(true);
	address_ = addStringProperty(details_, QString::fromUtf8(language[lsAddress].c_str()), QString());
	address_->setReadOnly(true);
	virtual_size_ = addStringProperty(details_, QString::fromUtf8(language[lsSize].c_str()), QString());
	virtual_size_->setReadOnly(true);
	physical_offset_ = addStringProperty(details_, QString::fromUtf8(language[lsRawAddress].c_str()), QString());
	physical_offset_->setReadOnly(true);
	physical_size_ = addStringProperty(details_, QString::fromUtf8(language[lsRawSize].c_str()), QString());
	physical_size_->setReadOnly(true);
	flags_ = addStringProperty(details_, QString::fromUtf8(language[lsFlags].c_str()), QString());
	flags_->setReadOnly(true);
	flag_readable_ = addBoolProperty(flags_, "Readable", false);
	flag_readable_->setReadOnly(true);
	flag_writable_ = addBoolProperty(flags_, "Writable", false);
	flag_writable_->setReadOnly(true);
	flag_executable_ = addBoolProperty(flags_, "Executable", false);
	flag_executable_->setReadOnly(true);
	flag_shared_ = addBoolProperty(flags_, "Shared", false);
	flag_shared_->setReadOnly(true);
	flag_discardable_ = addBoolProperty(flags_, "Discardable", false);
	flag_discardable_->setReadOnly(true);
	flag_notpaged_ = addBoolProperty(flags_, "NotPaged", false);
	flag_notpaged_->setReadOnly(true);

	connect(excluded_from_packing_, SIGNAL(valueChanged(bool)), this, SLOT(excludedFromPackingChanged(bool)));
	connect(excluded_from_memory_protection_, SIGNAL(valueChanged(bool)), this, SLOT(excludedFromMemoryProtectionChanged(bool)));
}

void SegmentPropertyManager::localize()
{
	options_->setName(QString::fromUtf8(language[lsOptions].c_str()));
	excluded_from_packing_->setName(QString::fromUtf8(language[lsExcludedFromPacking].c_str()));
	excluded_from_memory_protection_->setName(QString::fromUtf8(language[lsExcludedFromMemoryProtection].c_str()));

	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	virtual_size_->setName(QString::fromUtf8(language[lsSize].c_str()));
	address_->setName(QString::fromUtf8(language[lsAddress].c_str()));
	physical_offset_->setName(QString::fromUtf8(language[lsRawAddress].c_str()));
	physical_size_->setName(QString::fromUtf8(language[lsRawSize].c_str()));
	flags_->setName(QString::fromUtf8(language[lsFlags].c_str()));
}

void SegmentPropertyManager::setValue(ISection *value)
{
	value_ = value;
	update();
}

void SegmentPropertyManager::update()
{
	lock_ = true;

	excluded_from_packing_->setValue(value_ ? value_->excluded_from_packing() : false);
	excluded_from_memory_protection_->setValue(value_ ? value_->excluded_from_memory_protection() : false);
	name_->setValue(value_ ? QString::fromUtf8(value_->name().c_str()) : QString());
	address_->setValue(value_ ? QString::fromUtf8(DisplayValue(value_->address_size(), value_->address()).c_str()) : QString());
	virtual_size_->setValue(value_ ? QString::fromUtf8(DisplayValue(value_->address_size(), value_->size()).c_str()) : QString());
	physical_offset_->setValue(value_ ? QString::fromUtf8(DisplayValue(osDWord, value_->physical_offset()).c_str()) : QString());
	physical_size_->setValue(value_ ? QString::fromUtf8(DisplayValue(osDWord, value_->physical_size()).c_str()) : QString());
	flags_->setValue(value_ ? QString::fromLatin1(DisplayValue(osDWord, value_->flags()).c_str()) : QString());
	uint32_t memoryType = value_ ? value_->memory_type() : mtNone;
	flag_readable_->setValue(memoryType & mtReadable);
	flag_writable_->setValue(memoryType & mtWritable);
	flag_executable_->setValue(memoryType & mtExecutable);
	flag_shared_->setValue(memoryType & mtShared);
	flag_notpaged_->setValue(memoryType & mtNotPaged);
	flag_discardable_->setValue(memoryType & mtDiscardable);

	lock_ = false;
}

void SegmentPropertyManager::excludedFromPackingChanged(bool value)
{
	if (!value_ || lock_)
		return;

	value_->set_excluded_from_packing(value);
}

void SegmentPropertyManager::excludedFromMemoryProtectionChanged(bool value)
{
	if (!value_ || lock_)
		return;

	value_->set_excluded_from_memory_protection(value);
}

/**
 * LoadCommandPropertyManager
 */

LoadCommandPropertyManager::LoadCommandPropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL)
{
	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));

	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	name_->setReadOnly(true);
	address_ = addStringProperty(details_, QString::fromUtf8(language[lsAddress].c_str()), QString());
	address_->setReadOnly(true);
	size_ = addStringProperty(details_, QString::fromUtf8(language[lsSize].c_str()), QString());
	size_->setReadOnly(true);
	segment_ = addStringProperty(details_, QString::fromUtf8(language[lsSegment].c_str()), QString());
	segment_->setReadOnly(true);
}

void LoadCommandPropertyManager::localize()
{
	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	address_->setName(QString::fromUtf8(language[lsAddress].c_str()));
	size_->setName(QString::fromUtf8(language[lsSize].c_str()));
	segment_->setName(QString::fromUtf8(language[lsSegment].c_str()));
}

void LoadCommandPropertyManager::setValue(ILoadCommand *value)
{
	value_ = value;

	update();
}

void LoadCommandPropertyManager::update()
{
	name_->setValue(value_ ? QString::fromUtf8(value_->name().c_str()) : QString());
	address_->setValue(value_ ? QString::fromUtf8(DisplayValue(value_->address_size(), value_->address()).c_str()) : QString());
	size_->setValue(value_ ? QString::fromUtf8(DisplayValue(osDWord, value_->size()).c_str()) : QString());

	ISection *segment = NULL;
	if (value_) {
		IArchitecture *file = value_->owner()->owner();
		segment = file->segment_list()->GetSectionByAddress(value_->address());
		if (file->owner()->format_name() == "ELF") {
			switch (value_->type()) {
			case DT_NEEDED:
			case DT_PLTRELSZ:
			case DT_RELASZ:
			case DT_RELAENT:
			case DT_STRSZ:
			case DT_SYMENT:
			case DT_SONAME:
			case DT_RPATH:
			case DT_RELSZ:
			case DT_RELENT:
			case DT_PLTREL:
			case DT_DEBUG:
			case DT_BIND_NOW:
			case DT_INIT_ARRAYSZ:
			case DT_FINI_ARRAYSZ:
			case DT_RUNPATH:
			case DT_FLAGS:
			case DT_PREINIT_ARRAYSZ:
			case DT_GNU_HASH:
			case DT_RELACOUNT:
			case DT_RELCOUNT:
			case DT_FLAGS_1:
			case DT_VERDEFNUM:
			case DT_VERNEEDNUM:
				segment = NULL;
				break;
			}
		}
	}
	segment_->setValue(segment ? QString::fromUtf8(segment->name().c_str()) : QString());
}

/**
 * ImportPropertyManager
 */

ImportPropertyManager::ImportPropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL), func_(NULL)
{
	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));

	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	name_->setReadOnly(true);
	address_ = addStringProperty(details_, QString::fromUtf8(language[lsAddress].c_str()), QString());
	address_->setReadOnly(true);
	references_ = addStringProperty(details_, QString::fromUtf8(language[lsReferences].c_str()), QString());
	references_->setReadOnly(true);
}

ImportPropertyManager::~ImportPropertyManager()
{
	delete func_;
}

void ImportPropertyManager::localize()
{
	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	address_->setName(QString::fromUtf8(language[lsAddress].c_str()));
	references_->setName(QString::fromUtf8(language[lsReferences].c_str()));
	references_->setValue(value_ ? QString::number(references_->childCount()) + " " + QString::fromUtf8(language[lsItems].c_str()) : QString());
}

QVariant ImportPropertyManager::data(const QModelIndex &index, int role) const
{
	Property *prop = indexToProperty(index);
	if (role == Qt::FontRole) {
		if (prop && prop->parent() == references_)
			return QFont(MONOSPACE_FONT_FAMILY);
	}

	return PropertyManager::data(index, role);
}

void ImportPropertyManager::setValue(IImportFunction *value)
{
	value_ = value;
	delete func_;
	func_ = NULL;

	if (references_->childCount()) {
		beginRemoveRows(propertyToIndex(references_), 0, references_->childCount());
		references_->clear();
		endRemoveRows();
	}

	if (value_) {
		IArchitecture *file = value_->owner()->owner()->owner();
		func_ = file->function_list()->CreateFunction();
		ReferenceList *reference_list = value_->map_function()->reference_list();
		for (size_t i = 0; i < reference_list->count(); i++) {
			Reference *reference = reference_list->item(i);
			if (file->AddressSeek(reference->address()))
				func_->ParseCommand(*file, reference->address());
		}
		beginInsertRows(propertyToIndex(references_), 0, (int)reference_list->count());
		for (size_t i = 0; i < func_->count(); i++) {
			Property *prop = addCommandProperty(references_, func_->item(i));
			prop->setReadOnly(true);
		}
		endInsertRows();
	}

	update();
}

void ImportPropertyManager::update()
{
	name_->setValue(value_ ? QString::fromUtf8(value_->name().c_str()) : QString());
	address_->setValue(value_ ? QString::fromUtf8(DisplayValue(value_->address_size(), value_->address()).c_str()) : QString());
	references_->setValue(value_ ? QString::number(references_->childCount()) + " " + QString::fromUtf8(language[lsItems].c_str()) : QString());
}

/**
 * ExportPropertyManager
 */

ExportPropertyManager::ExportPropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL)
{
	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));

	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	name_->setReadOnly(true);
	forwarded_ = addStringProperty(details_, "Forwarded", QString());
	forwarded_->setReadOnly(true);
	address_ = addStringProperty(details_, QString::fromUtf8(language[lsAddress].c_str()), QString());
	address_->setReadOnly(true);
}

void ExportPropertyManager::localize()
{
	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	address_->setName(QString::fromUtf8(language[lsAddress].c_str()));
}

void ExportPropertyManager::setValue(IExport *value)
{
	value_ = value;

	update();
}

void ExportPropertyManager::update()
{
	name_->setValue(value_ ? QString::fromUtf8(value_->name().c_str()) : QString());
	forwarded_->setValue(value_ ? QString::fromUtf8(value_->forwarded_name().c_str()) : QString());
	address_->setValue(value_ ? QString::fromUtf8(DisplayValue(value_->address_size(), value_->address()).c_str()) : QString());
}

/**
 * ResourcePropertyManager
 */

ResourcePropertyManager::ResourcePropertyManager(QObject *parent)
	: PropertyManager(parent), value_(NULL), lock_(false)
{
	options_ = addGroupProperty(NULL, QString::fromUtf8(language[lsPacking].c_str()));
	excluded_from_packing_ = addBoolProperty(options_, QString::fromUtf8(language[lsExcludedFromPacking].c_str()), false);

	details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));
	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	name_->setReadOnly(true);
	address_ = addStringProperty(details_, QString::fromUtf8(language[lsAddress].c_str()), QString());
	address_->setReadOnly(true);
	size_ = addStringProperty(details_, QString::fromUtf8(language[lsSize].c_str()), QString());
	size_->setReadOnly(true);

	connect(excluded_from_packing_, SIGNAL(valueChanged(bool)), this, SLOT(excludedFromPackingChanged(bool)));
}

void ResourcePropertyManager::localize()
{
	options_->setName(QString::fromUtf8(language[lsOptions].c_str()));
	excluded_from_packing_->setName(QString::fromUtf8(language[lsExcludedFromPacking].c_str()));

	details_->setName(QString::fromUtf8(language[lsDetails].c_str()));
	name_->setName(QString::fromUtf8(language[lsName].c_str()));
	address_->setName(QString::fromUtf8(language[lsAddress].c_str()));
	size_->setName(QString::fromUtf8(language[lsSize].c_str()));
}

void ResourcePropertyManager::setValue(IResource *value)
{
	value_ = value;

	update();
}

void ResourcePropertyManager::update()
{
	lock_ = true;

	name_->setValue(value_ ? QString::fromUtf8(value_->name().c_str()) : QString());
	address_->setValue(value_ ? QString::fromUtf8(DisplayValue(value_->address_size(), value_->address()).c_str()) : QString());
	size_->setValue(value_ ? QString::fromUtf8(DisplayValue(osDWord, value_->size()).c_str()) : QString());
	excluded_from_packing_->setValue(value_ ? value_->excluded_from_packing() : false);

	lock_ = false;
}

void ResourcePropertyManager::excludedFromPackingChanged(bool value)
{
	if (!value_ || lock_)
		return;

	value_->set_excluded_from_packing(value);
}

/**
 * CorePropertyManager
 */

CorePropertyManager::CorePropertyManager(QObject *parent)
	: PropertyManager(parent), core_(NULL), lock_(false)
{
	file_ = addGroupProperty(NULL, QString::fromUtf8(language[lsFile].c_str()));
	memoryProtection_ = addBoolProperty(file_, QString::fromUtf8(language[lsMemoryProtection].c_str()), false);
	connect(memoryProtection_, SIGNAL(valueChanged(bool)), this, SLOT(memoryProtectionChanged(bool)));
	importProtection_ = addBoolProperty(file_, QString::fromUtf8(language[lsImportProtection].c_str()), false);
	connect(importProtection_, SIGNAL(valueChanged(bool)), this, SLOT(importProtectionChanged(bool)));
	resourceProtection_ = addBoolProperty(file_, QString::fromUtf8(language[lsResourceProtection].c_str()), false);
	connect(resourceProtection_, SIGNAL(valueChanged(bool)), this, SLOT(resourceProtectionChanged(bool)));
	packOutputFile_ = addBoolProperty(file_, QString::fromUtf8(language[lsPackOutputFile].c_str()), false);
	connect(packOutputFile_, SIGNAL(valueChanged(bool)), this, SLOT(packOutputFileChanged(bool)));
	outputFileName_ = addFileNameProperty(file_, QString::fromUtf8(language[lsOutputFile].c_str()), QString("%1 ("
#ifdef VMP_GNU
				"*.dylib *.exe *.dll *.bpl *.ocx *.sys *.scr *.so);;%2 (*)"
#else
				"*.exe *.dll *.bpl *.ocx *.sys *.scr *.dylib *.so);;%2 (*.*)"
#endif		
				).arg(QString::fromUtf8(language[lsExecutableFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())), QString(), true);
	connect(outputFileName_, SIGNAL(valueChanged(const QString &)), this, SLOT(outputFileChanged(const QString &)));

	detection_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetection].c_str()));
	detectionDebugger_ = addEnumProperty(detection_, QString::fromUtf8(language[lsDebugger].c_str()), QStringList() << QString::fromUtf8(language[lsNo].c_str()) << "User-mode" << "User-mode + Kernel-mode", 0);
	connect(detectionDebugger_, SIGNAL(valueChanged(int)), this, SLOT(detectionDebuggerChanged(int)));
	detectionVMTools_ = addBoolProperty(detection_, QString::fromUtf8(language[lsVirtualizationTools].c_str()), false);
	connect(detectionVMTools_, SIGNAL(valueChanged(bool)), this, SLOT(detectioncpVMToolsChanged(bool)));

	additional_ = addGroupProperty(NULL, QString::fromUtf8(language[lsAdditional].c_str()));
	vmSectionName_ = addStringProperty(additional_, QString::fromUtf8(language[lsVMSegments].c_str()), QString());
	connect(vmSectionName_, SIGNAL(valueChanged(const QString &)), this, SLOT(vmSectionNameChanged(const QString &)));
	stripDebugInfo_ = addBoolProperty(additional_, QString::fromUtf8(language[lsStripDebugInfo].c_str()), false);
	connect(stripDebugInfo_, SIGNAL(valueChanged(bool)), this, SLOT(stripDebugInfoChanged(bool)));
	stripRelocations_ = addBoolProperty(additional_, QString::fromUtf8(language[lsStripRelocations].c_str()), false);
	connect(stripRelocations_, SIGNAL(valueChanged(bool)), this, SLOT(stripRelocationsChanged(bool)));
#ifndef LITE
	watermarkName_ = addWatermarkProperty(additional_, QString::fromUtf8(language[lsWatermark].c_str()), QString());
	connect(watermarkName_, SIGNAL(valueChanged(const QString &)), this, SLOT(watermarkNameChanged(const QString &)));
#ifdef ULTIMATE
	hwid_ = addStringProperty(additional_, QString::fromUtf8(language[lsLockToHWID].c_str()), QString());
	connect(hwid_, SIGNAL(valueChanged(const QString &)), this, SLOT(hwidChanged(const QString &)));
#ifdef DEMO
	if (true)
#else
	if (VMProtectGetSerialNumberState() != SERIAL_STATE_SUCCESS)
#endif
	{
		size_t size = VMProtectGetCurrentHWID(NULL, 0);
		char *hwid = new char[size];
		VMProtectGetCurrentHWID(hwid, (int)size);
		hwid_->setValue(QString().fromLatin1(hwid));
		hwid_->setReadOnly(true);
		delete [] hwid;
	}

#endif

	messages_ = addGroupProperty(NULL, QString::fromUtf8(language[lsMessages].c_str()));
	messageDebuggerFound_ = addStringListProperty(messages_, QString::fromUtf8(language[lsDebuggerFound].c_str()), QString());
	connect(messageDebuggerFound_, SIGNAL(valueChanged(const QString &)), this, SLOT(messageDebuggerFoundChanged(const QString &)));
	messageVMToolsFound_ = addStringListProperty(messages_, QString::fromUtf8(language[lsVirtualizationToolsFound].c_str()), QString());
	connect(messageVMToolsFound_, SIGNAL(valueChanged(const QString &)), this, SLOT(messageVMToolsFoundChanged(const QString &)));
	messageFileCorrupted_ = addStringListProperty(messages_, QString::fromUtf8(language[lsFileCorrupted].c_str()), QString());
	connect(messageFileCorrupted_, SIGNAL(valueChanged(const QString &)), this, SLOT(messageFileCorruptedChanged(const QString &)));
	messageSerialNumberRequired_ = addStringListProperty(messages_, QString::fromUtf8(language[lsSerialNumberRequired].c_str()), QString());
	connect(messageSerialNumberRequired_, SIGNAL(valueChanged(const QString &)), this, SLOT(messageSerialNumberRequiredChanged(const QString &)));
	messageHWIDMismatched_ = addStringListProperty(messages_, QString::fromUtf8(language[lsHWIDMismatched].c_str()), QString());
	connect(messageHWIDMismatched_, SIGNAL(valueChanged(const QString &)), this, SLOT(messageHWIDMismatchedChanged(const QString &)));
#endif

#ifdef ULTIMATE
	licensingParameters_ = addGroupProperty(NULL, QString::fromUtf8(language[lsLicensingParameters].c_str()));
	licenseDataFileName_ = addFileNameProperty(licensingParameters_, QString::fromUtf8(language[lsFileName].c_str()), QString("%1 (*.vmp);;"
#ifdef VMP_GNU
		" %2 (*)"
#else
		" %2 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsProjectFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())), QString());
	connect(licenseDataFileName_, SIGNAL(valueChanged(const QString &)), this, SLOT(licenseDataFileNameChanged(const QString &)));
	keyPairAlgorithm_ = addStringProperty(licensingParameters_, QString::fromUtf8(language[lsKeyPairAlgorithm].c_str()), QString());
	keyPairAlgorithm_->setReadOnly(true);
	activationServer_ = addStringProperty(licensingParameters_, QString::fromUtf8(language[lsActivationServer].c_str()), QString());
	connect(activationServer_, SIGNAL(valueChanged(const QString &)), this, SLOT(activationServerChanged(const QString &)));
#endif
}

void CorePropertyManager::localize()
{
	file_->setName(QString::fromUtf8(language[lsFile].c_str()));
	memoryProtection_->setName(QString::fromUtf8(language[lsMemoryProtection].c_str()));
	memoryProtection_->setToolTip(QString::fromUtf8(language[lsMemoryProtectionHelp].c_str()));
	importProtection_->setName(QString::fromUtf8(language[lsImportProtection].c_str()));
	importProtection_->setToolTip(QString::fromUtf8(language[lsImportProtectionHelp].c_str()));
	resourceProtection_->setName(QString::fromUtf8(language[lsResourceProtection].c_str()));
	resourceProtection_->setToolTip(QString::fromUtf8(language[lsResourceProtectionHelp].c_str()));
	packOutputFile_->setName(QString::fromUtf8(language[lsPackOutputFile].c_str()));
	packOutputFile_->setToolTip(QString::fromUtf8(language[lsPackOutputFileHelp].c_str()));
#ifndef LITE
	watermarkName_->setName(QString::fromUtf8(language[lsWatermark].c_str()));
	watermarkName_->setToolTip(QString::fromUtf8(language[lsWatermarkHelp].c_str()));
#endif
	outputFileName_->setName(QString::fromUtf8(language[lsOutputFile].c_str()));
	outputFileName_->setFilter(QString("%1 ("
#ifdef VMP_GNU
				"*.dylib *.exe *.dll *.bpl *.ocx *.sys *.scr *.so);;%2 (*)"
#else
				"*.exe *.dll *.bpl *.ocx *.sys *.scr *.dylib *.so);;%2 (*.*)"
#endif		
				).arg(QString::fromUtf8(language[lsExecutableFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));

	detection_->setName(QString::fromUtf8(language[lsDetection].c_str()));
	detectionDebugger_->setName(QString::fromUtf8(language[lsDebugger].c_str()));
	detectionDebugger_->setToolTip(QString::fromUtf8(language[lsDebuggerHelp].c_str()));
	detectionDebugger_->setText(0, QString::fromUtf8(language[lsNo].c_str()));
	detectionVMTools_->setName(QString::fromUtf8(language[lsVirtualizationTools].c_str()));
	detectionVMTools_->setToolTip(QString::fromUtf8(language[lsVirtualizationToolsHelp].c_str()));

#ifndef LITE
	messages_->setName(QString::fromUtf8(language[lsMessages].c_str()));
	messageDebuggerFound_->setName(QString::fromUtf8(language[lsDebuggerFound].c_str()));
	messageVMToolsFound_->setName(QString::fromUtf8(language[lsVirtualizationToolsFound].c_str()));
	messageFileCorrupted_->setName(QString::fromUtf8(language[lsFileCorrupted].c_str()));
	messageSerialNumberRequired_->setName(QString::fromUtf8(language[lsSerialNumberRequired].c_str()));
	messageHWIDMismatched_->setName(QString::fromUtf8(language[lsHWIDMismatched].c_str()));
#endif

	additional_->setName(QString::fromUtf8(language[lsAdditional].c_str()));
	vmSectionName_->setName(QString::fromUtf8(language[lsVMSegments].c_str()));
	vmSectionName_->setToolTip(QString::fromUtf8(language[lsVMSegmentsHelp].c_str()));
	stripDebugInfo_->setName(QString::fromUtf8(language[lsStripDebugInfo].c_str()));
	stripRelocations_->setName(QString::fromUtf8(language[lsStripRelocations].c_str()));

#ifdef ULTIMATE
	hwid_->setName(QString::fromUtf8(language[lsLockToHWID].c_str()));
	licensingParameters_->setName(QString::fromUtf8(language[lsLicensingParameters].c_str()));
	licensingParameters_->setToolTip(QString::fromUtf8(language[lsLicensingParametersHelp].c_str()));
	licenseDataFileName_->setName(QString::fromUtf8(language[lsFileName].c_str()));
	licenseDataFileName_->setFilter(QString(
#ifdef VMP_GNU
		"%1 (*.vmp);;%2 (*)"
#else
		"%1 (*.vmp);;%2 (*.*)"
#endif
		).arg(QString::fromUtf8(language[lsProjectFiles].c_str())).arg(QString::fromUtf8(language[lsAllFiles].c_str())));
	keyPairAlgorithm_->setName(QString::fromUtf8(language[lsKeyPairAlgorithm].c_str()));
	if (keyPairAlgorithm_->childCount()) {
		QList<Property *> keyProps = keyPairAlgorithm_->children();
		keyProps[0]->setName(QString::fromUtf8(language[lsPublicExp].c_str()));
		keyProps[1]->setName(QString::fromUtf8(language[lsPrivateExp].c_str()));
		keyProps[2]->setName(QString::fromUtf8(language[lsModulus].c_str()));
		keyProps[3]->setName(QString::fromUtf8(language[lsProductCode].c_str()));
	} else {
		keyPairAlgorithm_->setValue(QString::fromUtf8(language[lsNone].c_str()));
	}
	activationServer_->setName(QString::fromUtf8(language[lsActivationServer].c_str()));
#endif
}

void CorePropertyManager::setCore(Core *core)
{
	core_ = core;
	if (core_ && core_->input_file()) {
		uint32_t disable_options = core_->input_file()->disable_options();
		stripRelocations_->setReadOnly(disable_options & cpStripFixups);
		importProtection_->setReadOnly(disable_options & cpImportProtection);
		resourceProtection_->setReadOnly(disable_options & cpResourceProtection);
		packOutputFile_->setReadOnly(disable_options & cpPack);
	}
	outputFileName_->setRelativePath(core ? QString::fromUtf8(core->project_path().c_str()) : QString());
#ifdef ULTIMATE
	licenseDataFileName_->setRelativePath(core ? QString::fromUtf8(core->project_path().c_str()) : QString());
#endif
	update();
}

QVariant CorePropertyManager::data(const QModelIndex &index, int role) const
{
	if (role == Qt::FontRole) {
#ifdef ULTIMATE
		Property *prop = indexToProperty(index);
		if (prop && prop->parent() == keyPairAlgorithm_ && index.column() == 1)
			return QFont(MONOSPACE_FONT_FAMILY);
#endif
	}

	return PropertyManager::data(index, role);
}

void CorePropertyManager::update()
{
	lock_ = true;

	uint32_t options = core_ ? core_->options() : 0;
	if (core_ && core_->input_file())
		options &= ~core_->input_file()->disable_options();
	memoryProtection_->setValue(core_ ? (options & cpMemoryProtection) != 0 : false);
	importProtection_->setValue(core_ ? (options & cpImportProtection) != 0 : false);
	resourceProtection_->setValue(core_ ? (options & cpResourceProtection) != 0 : false);
	packOutputFile_->setValue(core_ ? (options & cpPack) != 0 : false);
#ifndef LITE
	watermarkName_->setValue(core_ ? QString::fromUtf8(core_->watermark_name().c_str()) : QString());
#endif
	outputFileName_->setValue(core_ ? QString::fromUtf8(core_->output_file_name().c_str()) : QString());
	int i;
	switch (options & (cpCheckDebugger | cpCheckKernelDebugger)) {
	case cpCheckDebugger:
		i = 1;
		break;
	case cpCheckDebugger | cpCheckKernelDebugger:
		i = 2;
		break;
	default:
		i = 0;
		break;
	}
	detectionDebugger_->setValue(core_ ? i : 0);
	detectionVMTools_->setValue(core_ ? (options & cpCheckVirtualMachine) != 0 : false);
#ifndef LITE
	messageDebuggerFound_->setValue(core_ ? QString::fromUtf8(core_->message(MESSAGE_DEBUGGER_FOUND).c_str()) : QString());
	messageVMToolsFound_->setValue(core_ ? QString::fromUtf8(core_->message(MESSAGE_VIRTUAL_MACHINE_FOUND).c_str()) : QString());
	messageFileCorrupted_->setValue(core_ ? QString::fromUtf8(core_->message(MESSAGE_FILE_CORRUPTED).c_str()) : QString());
	messageSerialNumberRequired_->setValue(core_ ? QString::fromUtf8(core_->message(MESSAGE_SERIAL_NUMBER_REQUIRED).c_str()) : QString());
	messageHWIDMismatched_->setValue(core_ ? QString::fromUtf8(core_->message(MESSAGE_HWID_MISMATCHED).c_str()) : QString());
#endif

	vmSectionName_->setValue(core_ ? QString::fromUtf8(core_->vm_section_name().c_str()) : QString());
	stripDebugInfo_->setValue(core_ ? (options & cpStripDebugInfo) != 0 : false);
	stripRelocations_->setValue(core_ ? (options & cpStripFixups) != 0 : false);

#ifdef ULTIMATE
	if (!hwid_->readOnly())
		hwid_->setValue(core_ ? QString::fromUtf8(core_->hwid().c_str()) : QString());
	licenseDataFileName_->setValue(core_ ? QString::fromUtf8(core_->license_data_file_name().c_str()) : QString());
	QString str;
	if (core_) {
		LicensingManager *licensingManager = core_->licensing_manager();
		switch (licensingManager->algorithm()) {
		case alNone:
			str = QString::fromUtf8(language[lsNone].c_str());
			if (keyPairAlgorithm_->childCount()) {
				beginRemoveRows(propertyToIndex(keyPairAlgorithm_), 0, keyPairAlgorithm_->childCount());
				keyPairAlgorithm_->clear();
				endRemoveRows();
			}
			break;
		case alRSA:
			str = QString("RSA %1").arg(licensingManager->bits());
			if (!keyPairAlgorithm_->childCount()) {
				beginInsertRows(propertyToIndex(keyPairAlgorithm_), 0, 4);

				StringProperty *prop = addStringProperty(keyPairAlgorithm_, QString::fromUtf8(language[lsPublicExp].c_str()), QString());
				prop->setReadOnly(true);

				prop = addStringProperty(keyPairAlgorithm_, QString::fromUtf8(language[lsPrivateExp].c_str()), QString());
				prop->setReadOnly(true);

				prop = addStringProperty(keyPairAlgorithm_, QString::fromUtf8(language[lsModulus].c_str()), QString());
				prop->setReadOnly(true);

				prop = addStringProperty(keyPairAlgorithm_, QString::fromUtf8(language[lsProductCode].c_str()), QString());
				prop->setReadOnly(true);

				endInsertRows();
			}
			QList<Property *> list = keyPairAlgorithm_->children(); 
			reinterpret_cast<StringProperty *>(list.at(0))->setValue(QString::fromLatin1(formatVector(licensingManager->public_exp()).c_str()));
			reinterpret_cast<StringProperty *>(list.at(1))->setValue(QString::fromLatin1(formatVector(licensingManager->private_exp()).c_str()));
			reinterpret_cast<StringProperty *>(list.at(2))->setValue(QString::fromLatin1(formatVector(licensingManager->modulus()).c_str()));
			uint64_t product_code = licensingManager->product_code();
			reinterpret_cast<StringProperty *>(list.at(3))->setValue(QByteArray(reinterpret_cast<char *>(&product_code), sizeof(product_code)).toBase64());
			break;
		}
	}
	keyPairAlgorithm_->setValue(str);
	activationServer_->setValue(core_ ? QString::fromUtf8(core_->activation_server().c_str()) : QString());
#endif

	lock_ = false;
}

void CorePropertyManager::memoryProtectionChanged(bool value)
{
	if (!core_ || lock_)
		return;

	if (value) {
		core_->include_option(cpMemoryProtection);
	} else {
		core_->exclude_option(cpMemoryProtection);
	}
}

void CorePropertyManager::importProtectionChanged(bool value)
{
	if (!core_ || lock_)
		return;

	if (value) {
		core_->include_option(cpImportProtection);
	} else {
		core_->exclude_option(cpImportProtection);
	}
}

void CorePropertyManager::resourceProtectionChanged(bool value)
{
	if (!core_ || lock_)
		return;

	if (value) {
		core_->include_option(cpResourceProtection);
	} else {
		core_->exclude_option(cpResourceProtection);
	}
}

void CorePropertyManager::packOutputFileChanged(bool value)
{
	if (!core_ || lock_)
		return;

	if (value) {
		core_->include_option(cpPack);
	} else {
		core_->exclude_option(cpPack);
	}
}

void CorePropertyManager::watermarkNameChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_watermark_name(value.toUtf8().constData());
}

void CorePropertyManager::outputFileChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_output_file_name(value.toUtf8().constData());
}

void CorePropertyManager::detectionDebuggerChanged(int value)
{
	if (!core_ || lock_)
		return;

	if (value)
		core_->include_option(cpCheckDebugger);
	else
		core_->exclude_option(cpCheckDebugger);
	if (value == 2)
		core_->include_option(cpCheckKernelDebugger);
	else
		core_->exclude_option(cpCheckKernelDebugger);
}

void CorePropertyManager::detectioncpVMToolsChanged(bool value)
{
	if (!core_ || lock_)
		return;

	if (value) {
		core_->include_option(cpCheckVirtualMachine);
	} else {
		core_->exclude_option(cpCheckVirtualMachine);
	}
}

void CorePropertyManager::vmSectionNameChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_vm_section_name(value.toUtf8().constData());
}

void CorePropertyManager::stripDebugInfoChanged(bool value)
{
	if (!core_ || lock_)
		return;

	if (value) {
		core_->include_option(cpStripDebugInfo);
	} else {
		core_->exclude_option(cpStripDebugInfo);
	}
}

void CorePropertyManager::stripRelocationsChanged(bool value)
{
	if (!core_ || lock_)
		return;

	if (value) {
		core_->include_option(cpStripFixups);
	} else {
		core_->exclude_option(cpStripFixups);
	}
}

void CorePropertyManager::debugModeChanged(bool value)
{
	if (!core_ || lock_)
		return;

	if (value) {
		core_->include_option(cpDebugMode);
	} else {
		core_->exclude_option(cpDebugMode);
	}
}

void CorePropertyManager::messageDebuggerFoundChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_message(MESSAGE_DEBUGGER_FOUND, value.toUtf8().constData());
}

void CorePropertyManager::messageVMToolsFoundChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_message(MESSAGE_VIRTUAL_MACHINE_FOUND, value.toUtf8().constData());
}

void CorePropertyManager::messageFileCorruptedChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_message(MESSAGE_FILE_CORRUPTED, value.toUtf8().constData());
}

void CorePropertyManager::messageSerialNumberRequiredChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_message(MESSAGE_SERIAL_NUMBER_REQUIRED, value.toUtf8().constData());
}

void CorePropertyManager::messageHWIDMismatchedChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_message(MESSAGE_HWID_MISMATCHED, value.toUtf8().constData());
}

#ifdef ULTIMATE
void CorePropertyManager::hwidChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_hwid(value.toUtf8().constData());
}

void CorePropertyManager::licenseDataFileNameChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_license_data_file_name(value.toUtf8().constData());
}

void CorePropertyManager::activationServerChanged(const QString &value)
{
	if (!core_ || lock_)
		return;

	core_->set_activation_server(value.toUtf8().constData());
}
#else
void CorePropertyManager::hwidChanged(const QString &) {}
void CorePropertyManager::licenseDataFileNameChanged(const QString &) {}
void CorePropertyManager::activationServerChanged(const QString &) {}
#endif

/**
 * WatermarkPropertyManager
 */

WatermarkPropertyManager::WatermarkPropertyManager(QObject *parent)
	: PropertyManager(parent), lock_(false), watermark_(NULL)
{
	GroupProperty *details_ = addGroupProperty(NULL, QString::fromUtf8(language[lsDetails].c_str()));
	name_ = addStringProperty(details_, QString::fromUtf8(language[lsName].c_str()), QString());
	blocked_ = addBoolProperty(details_, QString::fromUtf8(language[lsBlocked].c_str()), false);
	useCount_ = addStringProperty(details_, QString::fromUtf8(language[lsUsageCount].c_str()), QString());
	useCount_->setReadOnly(true);

	connect(name_, SIGNAL(valueChanged(const QString &)), this, SLOT(nameChanged(const QString &)));
	connect(blocked_, SIGNAL(valueChanged(bool)), this, SLOT(blockedChanged(bool)));
}

void WatermarkPropertyManager::setWatermark(Watermark *watermark)
{
	watermark_ = watermark;
	update();
}

void WatermarkPropertyManager::update()
{
	lock_ = true;

	name_->setValue(watermark_ ? QString::fromUtf8(watermark_->name().c_str()) : QString());
	blocked_->setValue(watermark_ ? !watermark_->enabled() : false);
	useCount_->setValue(watermark_ ? QString::number(watermark_->use_count()) : QString());

	lock_ = false;
}

void WatermarkPropertyManager::nameChanged(const QString &value)
{
	if (!watermark_ || lock_)
		return;

	watermark_->set_name(value.toUtf8().constData());
}

void WatermarkPropertyManager::blockedChanged(bool value)
{
	if (!watermark_ || lock_)
		return;

	watermark_->set_enabled(!value);
}

/**
 * AddressCalculator
 */

AddressCalculator::AddressCalculator(QObject *parent)
	: PropertyManager(parent), lock_(false), file_(NULL)
{
	address_ = addStringProperty(NULL, QString::fromUtf8(language[lsAddress].c_str()), QString());
	offset_ = addStringProperty(NULL, QString::fromUtf8(language[lsRawAddress].c_str()), QString());
	segment_ = addEnumProperty(NULL, QString::fromUtf8(language[lsSegment].c_str()), QStringList(), -1);

	connect(address_, SIGNAL(valueChanged(const QString &)), this, SLOT(addressChanged(const QString &)));
	connect(offset_, SIGNAL(valueChanged(const QString &)), this, SLOT(offsetChanged(const QString &)));
	connect(segment_, SIGNAL(valueChanged(int)), this, SLOT(segmentChanged(int)));
}

void AddressCalculator::localize()
{
	address_->setName(QString::fromUtf8(language[lsAddress].c_str()));
	offset_->setName(QString::fromUtf8(language[lsRawAddress].c_str()));
	segment_->setName(QString::fromUtf8(language[lsSegment].c_str()));
}

void AddressCalculator::setValue(IArchitecture *file)
{
	if (file_ == file)
		return;

	file_ = file;

	lock_ = true;
	address_->setValue(QString());
	offset_->setValue(QString());
	QStringList segments;
	if (file_) {
		for (size_t i = 0; i < file_->segment_list()->count(); i++) {
			segments.push_back(QString::fromUtf8(file_->segment_list()->item(i)->name().c_str()));
		}
	}
	segment_->setValue(-1);
	segment_->setItems(segments);
	lock_ = false;
}

void AddressCalculator::addressChanged(const QString &value)
{
	if (!file_ || lock_)
		return;

	lock_ = true;
	bool is_valid;
	ISection *segment = NULL;
	uint64_t address = value.toULongLong(&is_valid, 16);
	if (is_valid)
		segment = file_->segment_list()->GetSectionByAddress(address);
	offset_->setValue(segment ? QString::fromUtf8(DisplayValue(osDWord, segment->physical_offset() + address - segment->address()).c_str()) : QString());
	segment_->setValue(segment ? (int)file_->segment_list()->IndexOf(segment) : -1);
	lock_ = false;
}

void AddressCalculator::offsetChanged(const QString &value)
{
	if (!file_ || lock_)
		return;

	lock_ = true;
	bool is_valid;
	ISection *segment = NULL;
	uint64_t offset = value.toULongLong(&is_valid, 16);
	if (is_valid)
		segment = file_->segment_list()->GetSectionByOffset(offset);
	uint64_t address = segment ? segment->address() + offset - segment->physical_offset() : 0;
	address_->setValue(segment ? QString::fromUtf8(DisplayValue(file_->cpu_address_size(), address).c_str()) : QString());
	segment_->setValue(segment ? (int)file_->segment_list()->IndexOf(segment) : -1);
	lock_ = false;
}

void AddressCalculator::segmentChanged(int value)
{
	if (!file_ || lock_)
		return;

	lock_ = true;
	ISection *segment = file_->segment_list()->item(value);
	uint64_t address = segment ? segment->address() : 0;
	address_->setValue(segment ? QString::fromUtf8(DisplayValue(file_->cpu_address_size(), address).c_str()) : QString());
	offset_->setValue(segment ? QString::fromUtf8(DisplayValue(osDWord, segment->physical_offset() + address - segment->address()).c_str()) : QString());
	lock_ = false;
}