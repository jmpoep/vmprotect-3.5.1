#include "../core/objects.h"
#include "../core/files.h"
#include "../core/core.h"
#include "../core/processors.h"
#include "../core/script.h"
#include "../core/lang.h"

#include "models.h"
#include "moc/moc_models.cc"
#include "widgets.h"
#include "property_editor.h"
#include "message_dialog.h"

static QIcon nodeIcon(NodeType type, bool is_disabled = false)
{
	QString iconName;
	switch (type) {
	case NODE_FUNCTIONS:
		iconName = ":/images/star.png";
		break;
	case NODE_LICENSES:
		iconName = ":/images/key.png";
		break;
	case NODE_SCRIPT:
		iconName = ":/images/script.png";
		break;
	case NODE_OPTIONS:
		iconName = ":/images/gear.png";
		break;
	case NODE_FILES:
	case NODE_ASSEMBLIES:
		iconName = ":/images/box.png";
		break;
	case NODE_ARCHITECTURE:
		iconName = ":/images/processor.png";
		break;
	case NODE_LOAD_COMMANDS:
		iconName = ":/images/lightning.png";
		break;
	case NODE_SEGMENTS:
		iconName = ":/images/code.png";
		break;
	case NODE_EXPORTS:
		iconName = ":/images/export.png";
		break;
	case NODE_IMPORTS:
		iconName = ":/images/import.png";
		break;
	case NODE_RESOURCES:
		iconName = ":/images/database.png";
		break;
	case NODE_CALC:
		iconName = ":/images/calc.png";
		break;
	case NODE_DUMP:
		iconName = ":/images/dump.png";
		break;
	case NODE_LICENSE:
		iconName = ":/images/item_key.png";
		break;
	case NODE_FILE:
		iconName = ":/images/item_file.png";
		break;
	case NODE_PROPERTY:
		iconName = ":/images/item_property.png";
		break;
	case NODE_LOAD_COMMAND:
		iconName = ":/images/item_load_command.png";
		break;
	case NODE_SEGMENT:
	case NODE_SECTION:
		iconName = ":/images/item_code.png";
		break;
	case NODE_IMPORT_FUNCTION:
		iconName = ":/images/item_import.png";
		break;
	case NODE_EXPORT:
		iconName = ":/images/item_export.png";
		break;
	case NODE_RESOURCE:
		iconName = ":/images/item_resource.png";
		break;
	case NODE_WATERMARK:
	case NODE_FUNCTION:
		iconName = ":/images/item.png";
		break;
	case NODE_SCRIPT_BOOKMARK:
		iconName = ":/images/item_code.png";
		break;
	case NODE_FOLDER:
	case NODE_RESOURCE_FOLDER:
	case NODE_MAP_FOLDER:
	case NODE_IMPORT_FOLDER:
	case NODE_EXPORT_FOLDER:
	case NODE_IMPORT:
	case NODE_FILE_FOLDER:
		if (is_disabled) {
			QIcon res;
			QPixmap firstImage(":/images/folder_close.png");
			QPixmap secondImage(":/images/bullet_delete.png");
			{
				QPainter painter(&firstImage);
				painter.drawPixmap(firstImage.width() - secondImage.width(), firstImage.height() - secondImage.height(), secondImage);
			}
			res.addPixmap(firstImage);

			firstImage = QPixmap(":/images/folder.png");
			{
				QPainter painter(&firstImage);
				painter.drawPixmap(firstImage.width() - secondImage.width(), firstImage.height() - secondImage.height(), secondImage);
			}
			res.addPixmap(firstImage, QIcon::Normal, QIcon::On);

			return res;
		} else {
			QIcon res = QIcon(":/images/folder_close.png");
			res.addPixmap(QPixmap(":/images/folder.png"), QIcon::Normal, QIcon::On);
			return res;
		}
		break;
	}

	if (is_disabled) {
		QPixmap firstImage(iconName);
		QPixmap secondImage(":/images/bullet_delete.png");
		QPainter painter(&firstImage);
		painter.drawPixmap(firstImage.width() - secondImage.width(), firstImage.height() - secondImage.height(), secondImage);

		return QIcon(firstImage);
	} else {
		return QIcon(iconName);
	}
}

template<typename F>
static QIcon functionIcon(F *func)
{
	if (!func || !func->need_compile())
		return nodeIcon(NODE_FUNCTION, true);

	QString iconName;
	switch (func->compilation_type()) {
	case ctMutation:
		iconName = ":/images/item_m.png";
		break;
	case ctUltra:
		iconName = ":/images/item_u.png";
		break;
	default:
		iconName = ":/images/item_v.png";
		break;
	}

	QString bullet_name;
	if (func->type() == otUnknown)
		bullet_name = ":/images/bullet_warning.png";
	else if (func->compilation_type() != ctMutation && (func->compilation_options() & coLockToKey))
		bullet_name = ":/images/bullet_key.png";

	if (!bullet_name.isEmpty()) {
		QPixmap firstImage(iconName);
		QPixmap secondImage(bullet_name);
		QPainter painter(&firstImage);
		painter.drawPixmap(firstImage.width() - secondImage.width(), firstImage.height() - secondImage.height(), secondImage);

		QIcon res;
		res.addPixmap(firstImage);
		return res;
	}
	return QIcon(iconName);
}

/**
 * ProjectNode
 */

ProjectNode::ProjectNode(ProjectNode *parent, NodeType type, void *data)
	: parent_(NULL), data_(data), type_(type), properties_(NULL)
{
	if (parent)
		parent->addChild(this);
}

void ProjectNode::clear()
{
	for (int i = 0; i < children_.size(); i++) {
		ProjectNode *node = children_[i];
		node->parent_ = NULL;
		delete node;
	}
	children_.clear();

	if (properties_) {
		delete properties_;
		properties_ = NULL;
	}
}

ProjectNode::~ProjectNode()
{
	clear();
	if (parent_)
		parent_->removeChild(this);
}

QString functionProtection(const FunctionBundle *func)
{
	return func ? QString::fromUtf8(func->display_protection().c_str()) : QString::fromUtf8(language[lsNone].c_str());
}

bool ProjectNode::contains(const QRegExp &filter) const
{
	for (int i = 0; i < 2; i++) {
		if (text(i).contains(filter))
			return true;
	}

	return false;
}

QString ProjectNode::text(int column) const
{
	if (column == 0)
		return text_;

	switch (type_) {
	case NODE_SCRIPT:
	case NODE_SCRIPT_BOOKMARK:
		{
			Script *object = static_cast<Script *>(data_);
			if (column == 1) {
				return QString::fromUtf8(object->text().c_str());
			}
		}
		break;
	case NODE_FUNCTION:
		{
			FunctionBundle *object = static_cast<FunctionBundle *>(data_);
			if (column == 1) {
				return QString::fromUtf8(object->display_address().c_str());
			} else if (column == 2) {
				return QString::fromUtf8(object->display_protection().c_str());
			}
		}
		break;
	case NODE_MAP_FUNCTION:
		{
			MapFunctionBundle *object = static_cast<MapFunctionBundle *>(data_);
			if (column == 1) {
				return QString::fromUtf8(object->display_address().c_str());
			} else if (column == 2) {
				IFile *file = object->owner()->owner();
				return functionProtection(file->function_list()->GetFunctionByName(object->name()));
			}
		}
		break;
	case NODE_LOAD_COMMAND:
		{
			ILoadCommand *object = static_cast<ILoadCommand *>(data_);
			if (column == 1) {
				return QString::fromUtf8(DisplayValue(object->address_size(), object->address()).c_str());
			} else if (column == 2) {
				return QString::fromUtf8(DisplayValue(osDWord, object->size()).c_str());
			}
		}
		break;
	case NODE_SEGMENT:
	case NODE_SECTION:
		{
			ISection *object = static_cast<ISection *>(data_);
			if (column == 1) {
				return QString::fromUtf8(DisplayValue(object->address_size(), object->address()).c_str());
			} else if (column == 2) {
				return QString::fromUtf8(DisplayValue(osDWord, object->size()).c_str());
			} else if (column == 3) {
				return QString::fromUtf8(DisplayValue(osDWord, object->physical_offset()).c_str());
			} else if (column == 4) {
				return QString::fromUtf8(DisplayValue(osDWord, object->physical_size()).c_str());
			} else if (column == 5) {
				return QString::fromUtf8(DisplayValue(osDWord, object->flags()).c_str());
			}
		}
		break;
	case NODE_RESOURCE:
		{
			IResource *object = static_cast<IResource *>(data_);
			if (column == 1) {
				return QString::fromUtf8(DisplayValue(object->address_size(), object->address()).c_str());
			} else 	if (column == 2) {
				return QString::fromUtf8(DisplayValue(osDWord, object->size()).c_str());
			}
		}
		break;
	case NODE_IMPORT_FUNCTION:
		{
			IImportFunction *object = static_cast<IImportFunction *>(data_);
			if (column == 1) {
				return QString::fromUtf8(DisplayValue(object->address_size(), object->address()).c_str());
			}
		}
		break;
	case NODE_EXPORT:
		{
			IExport *object = static_cast<IExport *>(data_);
			if (column == 1) {
				return QString::fromUtf8(DisplayValue(object->address_size(), object->address()).c_str());
			}
		}
		break;
	case NODE_PROPERTY:
		{
			Property *object = static_cast<Property *>(data_);
			if (column == 1) {
				return object->valueText();
			}
		}
		break;
#ifdef ULTIMATE
	case NODE_LICENSE:
		{
			License *object = static_cast<License *>(data_);
			if (column == 1) {
				return QString::fromUtf8(object->customer_email().c_str());
			} else if (column == 2) {
				LicenseDate date = object->date();
				return QDate(date.Year, date.Month, date.Day).toString(Qt::SystemLocaleShortDate);
			}
		}
		break;
	case NODE_FILE:
		{
			InternalFile *object = static_cast<InternalFile *>(data_);
			if (column == 1) {
				return QString::fromUtf8(object->file_name().c_str());
			}
		}
		break;
#endif
	}
	return QString();
}

QString ProjectNode::path() const
{
	QString res;
	for (ProjectNode *p = parent(); p && p->type() != NODE_ROOT; p = p->parent()) {
		if (!res.isEmpty())
			res = " > " + res;
		res = p->text() + res;
	}
	return res;
}

void ProjectNode::addChild(ProjectNode *child)
{
	insertChild(childCount(), child);
}

void ProjectNode::insertChild(int index, ProjectNode *child)
{ 
	if (child->parent())
		return;

	child->parent_ = this;
	children_.insert(index, child);
}

void ProjectNode::removeChild(ProjectNode *child)
{
	if (children_.removeOne(child))
		child->parent_ = NULL;
}

void ProjectNode::setPropertyManager(PropertyManager *propertyManager)
{
	if (properties_) {
		delete properties_;
		properties_ = NULL;
	}

	if (!propertyManager)
		return;

	properties_ = new ProjectNode(NULL, type_, data_); 
	properties_->setText(text_);
	properties_->setIcon(nodeIcon(type_));

	QList<Property *> propertyList;
	QMap<Property *, ProjectNode *> propertyMap;
	propertyMap[propertyManager->root()] = properties_;
	propertyList.append(propertyManager->root()->children());
	for (int i = 0; i < propertyList.size(); i++) {
		Property *prop = propertyList[i];
		propertyList.append(prop->children());

		ProjectNode *node = new ProjectNode(propertyMap[prop->parent()], NODE_PROPERTY, prop); 
		node->setText(prop->name());
		node->setIcon(nodeIcon(node->type()));
		propertyMap[prop] = node;
	}
}

void ProjectNode::localize()
{
	switch (type_) {
	case NODE_FUNCTIONS:
		setText(QString::fromUtf8(language[lsFunctionsForProtection].c_str()));
		break;
	case NODE_LICENSES:
		setText(QString::fromUtf8(language[lsLicenses].c_str()));
		break;
	case NODE_FILES:
		setText(QString::fromUtf8(language[lsFiles].c_str()));
		break;
	case NODE_ASSEMBLIES:
		setText(QString::fromUtf8(language[lsAssemblies].c_str()));
		break;
	case NODE_SCRIPT:
		setText(QString::fromUtf8(language[lsScript].c_str()));
		break;
	case NODE_OPTIONS:
		setText(QString::fromUtf8(language[lsOptions].c_str()));
		break;
	case NODE_PROPERTY:
		setText(static_cast<Property*>(data_)->name());
		break;
	case NODE_LOAD_COMMANDS:
		setText(QString::fromUtf8(language[lsDirectories].c_str()));
		break;
	case NODE_SEGMENTS:
		setText(QString::fromUtf8(language[lsSegments].c_str()));
		break;
	case NODE_IMPORTS:
		setText(QString::fromUtf8(language[lsImports].c_str()));
		break;
	case NODE_EXPORTS:
		setText(QString::fromUtf8(language[lsExports].c_str()));
		break;
	case NODE_RESOURCES:
		setText(QString::fromUtf8(language[lsResources].c_str()));
		break;
	case NODE_CALC:
		setText(QString::fromUtf8(language[lsCalculator].c_str()));
		break;
	case NODE_DUMP:
		setText(QString::fromUtf8(language[lsDump].c_str()));
		break;
	case NODE_ARCHITECTURE:
		for (int i = 0; i < childCount(); i++) {
			child(i)->localize();
		}
	}

	if (properties_) {
		QList<ProjectNode *> list;
		list.append(properties_);
		for (int i = 0; i < list.size(); i++) {
			ProjectNode *node = list[i];
			node->localize();

			list.append(node->children());
		}
	}
}

/**
 * BaseModel
 */

BaseModel::BaseModel(QObject *parent)
	: QAbstractItemModel(parent), root_(NULL), core_(NULL)
{
	root_ = new ProjectNode(NULL, NODE_ROOT);
}

BaseModel::~BaseModel()
{
	delete root_;
}

ProjectNode *BaseModel::indexToNode(const QModelIndex &index) const 
{
	if (index.isValid())
		return static_cast<ProjectNode *>(index.internalPointer());
	return root_;
}

QModelIndex BaseModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	ProjectNode *childNode = indexToNode(index);
	ProjectNode *parentNode = childNode->parent();
	if (parentNode == root_)
		return QModelIndex();

	return createIndex(parentNode->parent()->children().indexOf(parentNode), 0, parentNode);
}

int BaseModel::columnCount(const QModelIndex & /* parent */) const
{
	return 1;
}

int BaseModel::rowCount(const QModelIndex &parent) const
{
	ProjectNode *parentNode = indexToNode(parent);
	return parentNode->childCount();
}

QVariant BaseModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	ProjectNode *node = indexToNode(index);
	switch (role) {
	case Qt::DisplayRole:
		return node->text();
	case Qt::DecorationRole:
		return node->icon();
	case Qt::FontRole:
		{
			switch (node->type()) {
			case NODE_MAP_FUNCTION:
			case NODE_WATERMARK:
			case NODE_MESSAGE:
			case NODE_WARNING:
			case NODE_ERROR:
			case NODE_TEMPLATE:
				break;
			default:
				if (node->parent()->type() == NODE_ROOT) {
					QFont font;
					font.setBold(true);
					return font;
				}
			}
		}
		break;
	case Qt::BackgroundColorRole:
		if (node->type() == NODE_WARNING)
			return QColor(0xff, 0xf8, 0xe3);
		else if (node->type() == NODE_ERROR)
			return QColor(0xff, 0xf6, 0xf4);
		break;
	case Qt::TextColorRole:
		if (node->type() == NODE_ERROR)
			return QColor(0xff, 0x3c, 0x08);
		break;
	case Qt::Vmp::StaticTextRole:
		{
			int size = 0;
			switch (node->type())
			{
			case NODE_FUNCTIONS:
				size = (int)reinterpret_cast<FunctionBundleList*>(node->data())->count();
				break;
			case NODE_MAP_FOLDER:
				if (node->parent()->type() == NODE_ROOT) {
					QList<ProjectNode *> nodeList = node->children();
					for (int i = 0; i < nodeList.size(); i++) {
						ProjectNode *child = nodeList[i];
						if (child->type() == NODE_MAP_FOLDER)
							nodeList.append(child->children());
						else
							size++;
					}
					break;
				}
			case NODE_FOLDER:
			case NODE_IMPORT_FOLDER:
			case NODE_EXPORT_FOLDER:
			case NODE_FILE_FOLDER:
				{
					QListIterator<ProjectNode *> i(node->children());
					while (i.hasNext()) {
						NodeType childType = i.next()->type();
						if (childType != NODE_FOLDER && childType != NODE_MAP_FOLDER && childType != NODE_FILE_FOLDER)
							size++;
					}
				}
				break;
			case NODE_RESOURCE_FOLDER:
			case NODE_IMPORT:
			case NODE_IMPORTS:
			case NODE_EXPORTS:
			case NODE_SEGMENTS:
			case NODE_RESOURCES:
			case NODE_LICENSES:
			case NODE_LOAD_COMMANDS:
			case NODE_SEGMENT:
				size = node->childCount();
				break;
#ifdef ULTIMATE
			case NODE_FILES:
			case NODE_ASSEMBLIES:
				size = (int)reinterpret_cast<FileManager*>(node->data())->count();
				break;
#endif
			case NODE_WATERMARK:
				size = (int)reinterpret_cast<Watermark*>(node->data())->use_count();
				break;
			}
			if (size > 0) 
				return QString("(%1)").arg(size);
		}
		break;
	case Qt::Vmp::StaticColorRole: 
		return QColor(0x9b, 0xa2, 0xaa);
	}
	return QVariant();
}

QModelIndex BaseModel::nodeToIndex(ProjectNode *node) const
{
	if (!node || node == root_)
		return QModelIndex();

	return createIndex(node->parent()->children().indexOf(node), 0, node);
}

QModelIndex BaseModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	ProjectNode *parentNode = indexToNode(parent);
	ProjectNode *childNode = parentNode->children().value(row);
	if (!childNode)
		return QModelIndex();

	return createIndex(row, column, childNode);
}

void BaseModel::setCore(Core *core)
{
	clear();
	core_ = core;
}

void BaseModel::clear()
{
	root_->clear();
	objectToNode_.clear();
}

void BaseModel::setObjectNode(void *object, ProjectNode *node)
{
	objectToNode_[object] = node;
}

QModelIndex BaseModel::objectToIndex(void *object) const
{
	return nodeToIndex(objectToNode(object));
}

ProjectNode *BaseModel::objectToNode(void *object) const
{
	return objectToNode_[object];
}

bool BaseModel::removeNode(void *object)
{
	ProjectNode *node = objectToNode(object);
	if (!node)
		return false;

	removeObject(object);

	ProjectNode *parent = node->parent();
	int i = parent->children().indexOf(node);
	beginRemoveRows(nodeToIndex(parent), i, i);
	emit nodeRemoved(node);
	delete node;
	endRemoveRows();
	return true;
}

void BaseModel::updateNode(ProjectNode *node)
{
	QModelIndex index = nodeToIndex(node);
	dataChanged(index, index);
	emit nodeUpdated(node);
}

void BaseModel::removeObject(void *object)
{
	objectToNode_.remove(object);
	emit objectRemoved(object);
}

void BaseModel::createFunctionsTree(ProjectNode *root)
{
	size_t i, count;
	int k, j;
	QList<QString> classList;
	QList<QString> delimList;
	QList<ProjectNode *> folderList;
	QMap<ProjectNode *, QString> delimMap;
	QMap<QString, ProjectNode *> classMap;
	NodeType folder_type;
	QString delim;
	ushort u;
	ProjectNode *folder, *node, *child;
	MapFunctionBundleList *map_function_list = NULL;
	IImport *import = NULL;
	IExportList *export_list = NULL;

	switch (root->type()) {
#ifdef LITE
	case NODE_FUNCTIONS:
#else
	case NODE_ROOT:
#endif
		if (!core()->input_file())
			return;
		map_function_list = core()->input_file()->map_function_list();
		count = map_function_list->count();
		folder_type = NODE_MAP_FOLDER;
		break;
	case NODE_IMPORT:
		import = static_cast<IImport*>(root->data());
		count = import->count();
		folder_type = NODE_IMPORT_FOLDER;
		break;
	case NODE_EXPORTS:
		export_list = static_cast<IExportList*>(root->data());
		count = export_list->count();
		folder_type = NODE_EXPORT_FOLDER;
		break;
	default:
		return;
	}

	for (i = 0; i < count; i++) {
		MapFunctionBundle *func = NULL;
		IImportFunction *import_func = NULL;
		IExport *export_func = NULL;
		QString funcName;

		switch (root->type()) {
#ifdef LITE
		case NODE_FUNCTIONS:
#else
		case NODE_ROOT:
#endif
			func = map_function_list->item(i);
			if (!func->is_code())
				continue;
			funcName = QString::fromUtf8(func->display_name(false).c_str());
			break;
		case NODE_IMPORT:
			import_func = import->item(i);
			funcName = QString::fromUtf8(import_func->display_name(false).c_str());
			break;
		case NODE_EXPORTS:
			export_func = export_list->item(i);
			funcName = QString::fromUtf8(export_func->display_name(false).c_str());
			break;
		}

		size_t c = 0;
		int p = 0;
		classList.clear();
		delimList.clear();
		delim.clear();
		for (k = 0; k < funcName.size(); k++) {
			u = funcName[k].unicode();
			switch (u) {
			case '<':
				c++;
				break;
			case '>':
				c--;
				break;
			case ':':
				if (c == 0 && k > 0 && funcName[k - 1].unicode() == ':') {
					if (funcName[p] == '`')
						p++;
					classList.push_back(funcName.mid(p, k - p - 1));
					delimList.push_back(delim);
					delim = "::";
					p = k + 1;
				}
				break;
			case '.':
				if (c == 0 && k > 0 && funcName[k - 1].unicode() != ':') {
					classList.push_back(funcName.mid(p, k - p));
					delimList.push_back(delim);
					delim = QChar(u);
					p = k + 1;
				}
				break;
			case '/':
				if (c == 0 && k > 0) {
					classList.push_back(funcName.mid(p, k - p));
					delimList.push_back(delim);
					delim = QChar(u);
					p = k + 1;
				}
				break;
			case ' ':
				if (c == 0)
					p = k + 1;
				break;
			case '(':
			case '"':
				k = funcName.size();
				break;
			}
		}

		folder = root;
		QString fullClassName;
		for (k = 0; k < classList.size(); k++) {
			QString className = classList[k];
			delim = delimList[k];
			fullClassName = fullClassName + delim + className;

			ProjectNode *classFolder;
			auto it = classMap.upperBound(fullClassName);
			if (it != classMap.begin() && (it - 1).key() == fullClassName)
				classFolder = (it - 1).value();
			else {
				classFolder = new ProjectNode(NULL, folder_type);
				if (it == classMap.end()) {
					for (j = 0; j < folder->childCount(); j++) {
						if (folder->child(j)->type() != folder_type)
							break;
					}
				}
				else
					j = folder->children().indexOf(*it);
				folder->insertChild(j, classFolder);
				classFolder->setText(className);
				classFolder->setIcon(nodeIcon(classFolder->type()));

				delimMap[classFolder] = delim;
				classMap[fullClassName] = classFolder;
			}
			folder = classFolder;
		}

		switch (root->type()) {
#ifdef LITE
		case NODE_FUNCTIONS:
#else
		case NODE_ROOT:
#endif
			{
				node = new ProjectNode(folder, NODE_MAP_FUNCTION, func);
				node->setText(QString::fromUtf8(func->display_name().c_str()));
				node->setIcon(functionIcon(core()->input_file()->function_list()->GetFunctionByName(func->name())));
				setObjectNode(func, node);
			}
			break;
		case NODE_IMPORT:
			{
				node = new ProjectNode(folder, NODE_IMPORT_FUNCTION, import_func);
				node->setText(QString::fromUtf8(import_func->display_name().c_str()));
				node->setIcon(nodeIcon(node->type()));
				setObjectNode(import_func, node);
			}
			break;
		case NODE_EXPORTS:
			{
				node = new ProjectNode(folder, NODE_EXPORT, export_func);
				node->setText(QString::fromUtf8(export_func->display_name().c_str()));
				node->setIcon(nodeIcon(node->type()));
			}
			break;
		}
	}

	// optimize
	folderList.push_back(root);
	for (k = 0; k < folderList.size(); k++) {
		folder = folderList[k];
		if (folder->childCount() == 1 && folder->child(0)->type() == folder_type) {
			child = folder->child(0);
			while (child->childCount()) {
				node = child->child(0);
				child->removeChild(node);
				folder->addChild(node);
			}
			folder->setText(folder->text() + delimMap[child] + child->text());

			delete child;
			j = folderList.indexOf(child);
			if (j != -1)
				folderList[j] = folder;
		}
	}
}

void BaseModel::localize()
{
	if (isEmpty())
		return;

	QList<ProjectNode*> nodes = root()->children();
	if (nodes.isEmpty())
		nodes.append(root());

	for (int i = 0; i < nodes.count(); i++) {
		ProjectNode *node = nodes[i];
		node->localize();
		updateNode(node);
	}
}

/**
 * ProjectModel
 */

ProjectModel::ProjectModel(QObject *parent)
	: BaseModel(parent), nodeFunctions_(NULL), 
#ifdef ULTIMATE
	nodeLicenses_(NULL), nodeFiles_(NULL),
#endif
#ifndef LITE
	nodeScript_(NULL),
#endif
	nodeOptions_(NULL)
{

}

ProjectModel::~ProjectModel()
{

}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags res = QAbstractItemModel::flags(index);
	if (res & Qt::ItemIsSelectable) {
		ProjectNode *node = indexToNode(index);
		if (node) {
			if (node->type() == NODE_FOLDER || node->type() == NODE_FUNCTION || node->type() == NODE_FILE_FOLDER || node->type() == NODE_FILE)
				res |= Qt::ItemIsDragEnabled;
			if (node->type() == NODE_FUNCTIONS || node->type() == NODE_FOLDER || node->type() == NODE_FILES || node->type() == NODE_FILE_FOLDER || node->type() == NODE_ASSEMBLIES)
				res |= Qt::ItemIsDropEnabled;
			if ((node->type() == NODE_FOLDER && !static_cast<Folder *>(node->data())->read_only()) || node->type() == NODE_LICENSE || node->type() == NODE_FILE_FOLDER || node->type() == NODE_FILE)
				res |= Qt::ItemIsEditable;
		}
	}
	return res;
}

QStringList ProjectModel::mimeTypes() const
{
    return QStringList() << QLatin1String("application/x-projectmodeldatalist");
}

QMimeData *ProjectModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.count() <= 0)
        return 0;
    QStringList types = mimeTypes();
    if (types.isEmpty())
        return 0;
    QMimeData *data = new QMimeData();
    QString format = types.at(0);
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    for (QModelIndexList::ConstIterator it = indexes.begin(); it != indexes.end(); ++it) {
		ProjectNode *node = indexToNode(*it);
		if (!node)
			continue;

		stream << node->type();
		switch (node->type()) {
		case NODE_FUNCTION:
			{
				FunctionBundle *func = reinterpret_cast<FunctionBundle *>(node->data());
				stream << QString::fromUtf8(func->id().c_str());
			}
			break;
		case NODE_FOLDER:
			{
				Folder *folder = reinterpret_cast<Folder *>(node->data());
				stream << QString::fromUtf8(folder->id().c_str());
			}
			break;
#ifdef ULTIMATE
		case NODE_FILE_FOLDER:
			{
				FileFolder *folder = reinterpret_cast<FileFolder *>(node->data());
				stream << QString::fromUtf8(folder->id().c_str());
			}
			break;
		case NODE_FILE:
			{
				InternalFile *file = reinterpret_cast<InternalFile *>(node->data());
				stream << (int)file->id();
			}
			break;
#endif
		}
	}
    data->setData(format, encoded);
    return data;
}

bool ProjectModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent)
{
    if (!data || !(action == Qt::CopyAction || action == Qt::MoveAction))
        return false;
    QStringList types = mimeTypes();
    if (types.isEmpty())
        return false;
    QString format = types.at(0);
    if (!data->hasFormat(format))
        return false;
	ProjectNode *dst = indexToNode(parent);
	if (!dst)
		return false;

	if (dst->type()== NODE_FILES || dst->type() == NODE_ASSEMBLIES || dst->type()== NODE_FILE_FOLDER || dst->type()== NODE_FILE) {
#ifdef ULTIMATE
		FileFolder *dst_folder;
		switch (dst->type()) {
		case NODE_FILE:
			dst_folder = reinterpret_cast<InternalFile *>(dst->data())->folder();
			break;
		case NODE_FILE_FOLDER:
			dst_folder = reinterpret_cast<FileFolder *>(dst->data());
			break;
		case NODE_FILES:
		case NODE_ASSEMBLIES:
			dst_folder = NULL;
			break;
		default:
			return false;
		}
		QByteArray encoded = data->data(format);
		QDataStream stream(&encoded, QIODevice::ReadOnly);
		while (!stream.atEnd()) {
			int t;
			stream >> t;
			switch (t) {
			case NODE_FILE:
				{
					int id;
					stream >> id;
					InternalFile *file = core()->file_manager()->item((size_t)id);
					file->set_folder(dst_folder);
				}
				break;
			case NODE_FILE_FOLDER:
				{
					QString str_id;
					stream >> str_id;
					FileFolder *folder = core()->file_manager()->folder_list()->GetFolderById(str_id.toUtf8().constData());
					if (folder)
						folder->set_owner(dst_folder ? dst_folder : core()->file_manager()->folder_list());
				}
				break;
			}
		}
#endif
	} else {
		Folder *dst_folder;
		switch (dst->type()) {
		case NODE_FUNCTION:
			dst_folder = reinterpret_cast<IFunction *>(dst->data())->folder();
			break;
		case NODE_FOLDER:
			dst_folder = reinterpret_cast<Folder *>(dst->data());
			break;
		case NODE_FUNCTIONS:
			dst_folder = NULL;
			break;
		default:
			return false;
		}
		QByteArray encoded = data->data(format);
		QDataStream stream(&encoded, QIODevice::ReadOnly);
		while (!stream.atEnd()) {
			int t;
			stream >> t;
			switch (t) {
			case NODE_FUNCTION:
				{
					QString str_id;
					stream >> str_id;
					FunctionBundle *func = core()->input_file()->function_list()->GetFunctionById(str_id.toUtf8().constData());
					if (func)
						func->set_folder(dst_folder);
				}
				break;
			case NODE_FOLDER:
				{
					QString str_id;
					stream >> str_id;
					Folder *folder = core()->input_file()->folder_list()->GetFolderById(str_id.toUtf8().constData());
					if (folder)
						folder->set_owner(dst_folder ? dst_folder : core()->input_file()->folder_list());
				}
				break;
			}
		}
	}
	return true;
}

QVariant ProjectModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0)
			return QString::fromUtf8(language[lsProject].c_str());
	}
	return QVariant();
}

void ProjectModel::addFolder(Folder *folder)
{
	ProjectNode *node = internalAddFolder(folder);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::updateFolder(Folder *folder)
{
	ProjectNode *node = internalUpdateFolder(folder);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::removeFolder(Folder *folder)
{
	if (removeNode(folder))
		emit modified();
}

ProjectNode *ProjectModel::internalAddFolder(Folder *folder)
{
#ifdef LITE
	return NULL;
#else
	ProjectNode *nodeFolder = objectToNode(folder->owner());
	if (!nodeFolder)
		nodeFolder = nodeFunctions_;
	ProjectNode *node = new ProjectNode(NULL, NODE_FOLDER, (void *)folder);
	node->setText(QString::fromUtf8(folder->name().c_str()));
	node->setIcon(nodeIcon(node->type()));
	QList<ProjectNode *> folderList = nodeFolder->children();
	int index = folderList.size();
	for (int i = 0; i < folderList.size(); i++) {
		if (folderList[i]->type() != NODE_FOLDER) {
			index = i;
			break;
		}
	}
	beginInsertRows(nodeToIndex(nodeFolder), index, index);
	nodeFolder->insertChild(index, node);
	endInsertRows();

	setObjectNode(folder, node);
	return node;
#endif
}

ProjectNode *ProjectModel::internalUpdateFolder(Folder *folder)
{
#ifdef LITE
	return NULL;
#else
	ProjectNode *node = objectToNode(folder);
	if (!node)
		return NULL;

	node->setText(QString::fromUtf8(folder->name().c_str()));

	ProjectNode *folder_node = folder->owner() ? objectToNode(folder->owner()) : nodeFunctions_;
	ProjectNode *parent_node = node->parent();
	if (folder_node && parent_node && parent_node != folder_node) {
		int index_from = parent_node->children().indexOf(node);
		int index_to = folder_node->childCount();
		for (int i = 0; i < folder_node->childCount(); i++) {
			if (folder_node->child(i)->type() != NODE_FOLDER) {
				index_to = i;
				break;
			}
		}
		beginMoveRows(nodeToIndex(parent_node), index_from, index_from, nodeToIndex(folder_node), index_to);
		parent_node->removeChild(node);
		folder_node->insertChild(index_to, node);
		endMoveRows();
	}

	return node;
#endif
}

void ProjectModel::addFunction(IFunction *func)
{
	ProjectNode *node = internalAddFunction(func);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::updateFunction(IFunction *func)
{
	ProjectNode *node = internalUpdateFunction(func);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::removeFunction(IFunction *func)
{
#ifdef LITE
	FunctionBundle *bundle = core()->input_file()->function_list()->GetFunctionByFunc(func);
	if (!bundle)
		return;

	removeObject(func);

	FunctionArch *func_arch = bundle->GetArchByFunction(func);
	if (func_arch)
		delete func_arch;
	if (bundle->count() == 0)
		delete bundle;
	ProjectNode *node = internalUpdateFunction(func);
	if (node)
		updateNode(node);
#else
	ProjectNode *node = objectToNode(func);
	if (!node)
		return;
	removeObject(func);

	FunctionBundle *bundle = reinterpret_cast<FunctionBundle *>(node->data());
	FunctionArch *func_arch = bundle->GetArchByFunction(func);
	if (func_arch)
		delete func_arch;
	if (bundle->count() == 0) {
		removeNode(bundle);
		delete bundle;
	}
#endif
	emit modified();
}

ProjectNode *ProjectModel::internalAddFunction(IFunction *func)
{
#ifndef LITE
	FunctionBundle *bundle = 
#endif
		core()->input_file()->function_list()->Add(func->owner()->owner(), func);
#ifndef LITE
	ProjectNode *nodeFolder = objectToNode(func->folder());
	if (!nodeFolder)
		nodeFolder = nodeFunctions_;

	ProjectNode *node = objectToNode(bundle);
	if (!node) {
		beginInsertRows(nodeToIndex(nodeFolder), nodeFolder->childCount(), nodeFolder->childCount());
		node = new ProjectNode(nodeFolder, NODE_FUNCTION, bundle);
		endInsertRows();
		setObjectNode(bundle, node);
	}
	setObjectNode(func, node);

#endif
	return internalUpdateFunction(func);
}

ProjectNode *ProjectModel::internalUpdateFunction(IFunction *func)
{
#ifdef LITE
	IArchitecture *arch = func->owner()->owner();
	MapFunctionBundle *map_func = core()->input_file()->map_function_list()->GetFunctionByAddress(arch, func->address());
	if (!map_func)
		return NULL;
	
	ProjectNode *node = objectToNode(map_func);
	if (!node)
		return NULL;

	node->setIcon(functionIcon(core()->input_file()->function_list()->GetFunctionByAddress(arch, func->address())));
#else
	ProjectNode *node = objectToNode(func);
	if (!node)
		return NULL;

	FunctionBundle *bundle = reinterpret_cast<FunctionBundle *>(node->data());
	std::string func_name = bundle->display_name();
	node->setText(func_name.empty() ? QString::fromUtf8(bundle->display_address().c_str()) : QString::fromUtf8(func_name.c_str()));
	node->setIcon(functionIcon(bundle));

	ProjectNode *folder_node = bundle->folder() ? objectToNode(bundle->folder()) : nodeFunctions_;
	ProjectNode *parent_node = node->parent();
	if (folder_node && parent_node && parent_node != folder_node) {
		int i = parent_node->children().indexOf(node);
		beginMoveRows(nodeToIndex(parent_node), i, i, nodeToIndex(folder_node), folder_node->childCount());
		parent_node->removeChild(node);
		folder_node->addChild(node);
		endMoveRows();
	}
#endif

	return node;
}

void ProjectModel::updateScript()
{
#ifndef LITE
	ProjectNode *node = nodeScript_;
	if (!node)
		return;

	node->setIcon(nodeIcon(node->type(), !core()->script()->need_compile()));
#endif
}

#ifdef ULTIMATE
void ProjectModel::addLicense(License *license)
{
	ProjectNode *node = internalAddLicense(license);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::updateLicense(License *license)
{
	ProjectNode *node = internalUpdateLicense(license);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::removeLicense(License *license)
{
	if (removeNode(license))
		emit modified();
}

void ProjectModel::addFileFolder(FileFolder *folder)
{
	ProjectNode *node = internalAddFileFolder(folder);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::updateFileFolder(FileFolder *folder)
{
	ProjectNode *node = internalUpdateFileFolder(folder);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::removeFileFolder(FileFolder *folder)
{
	if (removeNode(folder))
		emit modified();
}

void ProjectModel::addFile(InternalFile *file)
{
	ProjectNode *node = internalAddFile(file);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::updateFile(InternalFile *file)
{
	ProjectNode *node = internalUpdateFile(file);
	if (node) {
		updateNode(node);
		emit modified();
	}
}

void ProjectModel::removeFile(InternalFile *file)
{
	if (removeNode(file))
		emit modified();
}

ProjectNode *ProjectModel::internalAddLicense(License *license)
{
	beginInsertRows(nodeToIndex(nodeLicenses_), nodeLicenses_->childCount(), nodeLicenses_->childCount());
	ProjectNode *node = new ProjectNode(nodeLicenses_, NODE_LICENSE, license);
	endInsertRows();

	setObjectNode(license, node);
	internalUpdateLicense(license);
	return node;
}

ProjectNode *ProjectModel::internalUpdateLicense(License *license)
{
	ProjectNode *node = objectToNode(license);
	if (!node)
		return NULL;

	node->setText(QString::fromUtf8(license->customer_name().c_str()));
	node->setIcon(nodeIcon(node->type(), license->blocked()));
	return node;
}

ProjectNode *ProjectModel::internalAddFileFolder(FileFolder *folder)
{
	ProjectNode *nodeFolder = objectToNode(folder->owner());
	if (!nodeFolder)
		nodeFolder = nodeFiles_;
	ProjectNode *node = new ProjectNode(NULL, NODE_FILE_FOLDER, (void *)folder);
	node->setText(QString::fromUtf8(folder->name().c_str()));
	node->setIcon(nodeIcon(node->type()));
	QList<ProjectNode *> folderList = nodeFolder->children();
	int index = folderList.size();
	for (int i = 0; i < folderList.size(); i++) {
		if (folderList[i]->type() != NODE_FILE_FOLDER) {
			index = i;
			break;
		}
	}
	beginInsertRows(nodeToIndex(nodeFolder), index, index);
	nodeFolder->insertChild(index, node);
	endInsertRows();

	setObjectNode(folder, node);
	return node;
}

ProjectNode *ProjectModel::internalUpdateFileFolder(FileFolder *folder)
{
	ProjectNode *node = objectToNode(folder);
	if (!node)
		return NULL;

	node->setText(QString::fromUtf8(folder->name().c_str()));

	ProjectNode *folder_node = folder->owner() ? objectToNode(folder->owner()) : nodeFiles_;
	ProjectNode *parent_node = node->parent();
	if (folder_node && parent_node && parent_node != folder_node) {
		int index_from = parent_node->children().indexOf(node);
		int index_to = folder_node->childCount();
		for (int i = 0; i < folder_node->childCount(); i++) {
			if (folder_node->child(i)->type() != NODE_FILE_FOLDER) {
				index_to = i;
				break;
			}
		}
		beginMoveRows(nodeToIndex(parent_node), index_from, index_from, nodeToIndex(folder_node), index_to);
		parent_node->removeChild(node);
		folder_node->insertChild(index_to, node);
		endMoveRows();
	}

	return node;
}

ProjectNode *ProjectModel::internalAddFile(InternalFile *file)
{
	if (!nodeFiles_)
		return NULL;

	ProjectNode *nodeFolder = objectToNode(file->folder());
	if (!nodeFolder)
		nodeFolder = nodeFiles_;

	beginInsertRows(nodeToIndex(nodeFiles_), nodeFiles_->childCount(), nodeFiles_->childCount());
	ProjectNode *node = new ProjectNode(nodeFolder, NODE_FILE, file);
	endInsertRows();

	setObjectNode(file, node);
	internalUpdateFile(file);
	return node;
}

ProjectNode *ProjectModel::internalUpdateFile(InternalFile *file)
{
	ProjectNode *node = objectToNode(file);
	if (!node)
		return NULL;

	node->setText(QString::fromUtf8(file->name().c_str()));
	node->setIcon(nodeIcon(node->type()));
	ProjectNode *folder_node = file->folder() ? objectToNode(file->folder()) : nodeFiles_;
	ProjectNode *parent_node = node->parent();
	if (folder_node && parent_node && parent_node != folder_node) {
		int i = parent_node->children().indexOf(node);
		beginMoveRows(nodeToIndex(parent_node), i, i, nodeToIndex(folder_node), folder_node->childCount());
		parent_node->removeChild(node);
		folder_node->addChild(node);
		endMoveRows();
	}

	return node;
}

void ProjectModel::updateFiles()
{
	ProjectNode *node = nodeFiles_;
	if (!node)
		return;

	node->setIcon(nodeIcon(node->type(), !core()->file_manager()->need_compile()));
	updateNode(node);
}
#endif
#ifndef LITE
QRegularExpression FuncRegex("function\\s+[_a-zA-Z]+[_a-zA-Z0-9]*\\s*\\([^\\)]*\\)");
void ProjectModel::updateScriptBookmarks()
{
	ProjectNode *parent = nodeScript_;
	if (!parent)
		return;

	QString script = QString::fromUtf8(core()->script()->text().c_str());
	QRegularExpression multi_space("\\s+");
	int nChild = 0;
	auto it = FuncRegex.globalMatch(script);
	while(it.hasNext()) {
		QString newText = it.next().captured().replace(multi_space, " ");

		if (nChild >= parent->childCount()) {
			beginInsertRows(nodeToIndex(nodeScript_), nodeScript_->childCount(), nodeScript_->childCount());
			ProjectNode *node = new ProjectNode(nodeScript_, NODE_SCRIPT_BOOKMARK, core()->script());
			node->setText(newText);
			node->setIcon(nodeIcon(node->type()));
			endInsertRows();
		} else {
			ProjectNode *node = parent->child(nChild);
			if(node->text(0).compare(newText)) {
				node->setText(newText);
				updateNode(node);
			}
		}
		nChild++;
	}
	if(nChild != parent->childCount()) {
		beginRemoveRows(nodeToIndex(parent), nChild, parent->childCount() - 1);
		while (nChild != parent->childCount()) {
			ProjectNode *node = parent->child(nChild);
			emit nodeRemoved(node);
			delete node;
		}
		endRemoveRows();
	}
}
uptr_t ProjectModel::bookmarkNodeToPos(ProjectNode *node) const
{
	uptr_t ret = 0;
	int bookIdx = nodeToIndex(node).row();
	QString script = QString::fromUtf8(core()->script()->text().c_str());
	auto it = FuncRegex.globalMatch(script);
	int nChild = 0;
	while (it.hasNext()) {
		auto m = it.next();
		if (nChild == bookIdx)
			return m.capturedStart();
		nChild++;
	}
	return ret;
}
#endif

void ProjectModel::setCore(Core *core)
{
	BaseModel::setCore(core);

	if (!core) {
		beginResetModel();
		nodeFunctions_ = NULL;
#ifdef ULTIMATE
		nodeLicenses_ = NULL;
		nodeFiles_ = NULL;
#endif
#ifndef LITE
		nodeScript_ = NULL;
#endif
		nodeOptions_ = NULL;
		endResetModel();
		return;
	}

	size_t i, j;

	IFile *file = core->input_file();
	if (file) {
		nodeFunctions_ = new ProjectNode(root(), NODE_FUNCTIONS, file->function_list());
		nodeFunctions_->setText(QString::fromUtf8(language[lsFunctionsForProtection].c_str()));
		nodeFunctions_->setIcon(nodeIcon(nodeFunctions_->type()));

#ifdef LITE
		setObjectNode(file->map_function_list(), nodeFunctions_);
		createFunctionsTree(nodeFunctions_);
#else
		setObjectNode(file->folder_list(), nodeFunctions_);
		std::vector<Folder*> folderList = file->folder_list()->GetFolderList();
		for (i = 0; i < folderList.size(); i++) {
			internalAddFolder(folderList[i]);
		}
#endif

		for (i = 0; i < file->count(); i++) {
			IArchitecture *arch = file->item(i);
			if (!arch->visible())
				continue;

			for (j = 0; j < arch->function_list()->count(); j++) {
				internalAddFunction(arch->function_list()->item(j));
			}
		}
	}

#ifdef ULTIMATE
	LicensingManager *licensing_manager = core->licensing_manager();
	nodeLicenses_ = new ProjectNode(root(), NODE_LICENSES, licensing_manager);
	nodeLicenses_->setText(QString::fromUtf8(language[lsLicenses].c_str()));
	nodeLicenses_->setIcon(nodeIcon(nodeLicenses_->type()));
	for (i = 0; i < licensing_manager->count(); i++) {
		internalAddLicense(licensing_manager->item(i));
	}

	if (file && (file->disable_options() & cpVirtualFiles) == 0) {
		FileManager *file_manager = core->file_manager();
		if (file->format_name() == "PE" && file->count() > 1) {
			nodeFiles_ = new ProjectNode(root(), NODE_ASSEMBLIES, file_manager);
			nodeFiles_->setText(QString::fromUtf8(language[lsAssemblies].c_str()));
		}
		else {
			nodeFiles_ = new ProjectNode(root(), NODE_FILES, file_manager);
			nodeFiles_->setText(QString::fromUtf8(language[lsFiles].c_str()));
		}
		nodeFiles_->setIcon(nodeIcon(nodeFiles_->type(), !file_manager->need_compile()));
		setObjectNode(file_manager->folder_list(), nodeFiles_);

		std::vector<FileFolder*> folderList = file_manager->folder_list()->GetFolderList();
		for (i = 0; i < folderList.size(); i++) {
			internalAddFileFolder(folderList[i]);
		}
		for (i = 0; i < file_manager->count(); i++) {
			internalAddFile(file_manager->item(i));
		}
	}
#endif

	if (file) {
#ifndef LITE
		nodeScript_ = new ProjectNode(root(), NODE_SCRIPT, core->script());
		nodeScript_->setText(QString::fromUtf8(language[lsScript].c_str()));
		nodeScript_->setIcon(nodeIcon(nodeScript_->type(), !core->script()->need_compile()));
		updateScriptBookmarks();
#endif

		nodeOptions_ = new ProjectNode(root(), NODE_OPTIONS);
		nodeOptions_->setText(QString::fromUtf8(language[lsOptions].c_str()));
		nodeOptions_->setIcon(nodeIcon(nodeOptions_->type()));
	}
}

struct FunctionBundleListCompareHelper {
	Folder *folder;
	int field;
	FunctionBundleListCompareHelper(Folder *_folder, int _field) : folder(_folder), field(_field) {}
	bool operator () (const FunctionBundle *func1, const FunctionBundle *func2) const
	{
		if (func1->folder() == func2->folder() && func1->folder() == folder) {
			switch (field) {
			case 0:
				return QString::compare(QString::fromUtf8(func1->display_name().c_str()), QString::fromUtf8(func2->display_name().c_str()), Qt::CaseInsensitive) < 0;
			case 1:
				return func1->display_address() < func2->display_address();
			case 2:
				{
					CompilationType ct1 = func1->need_compile() ? func1->compilation_type() : ctNone;
					CompilationType ct2 = func2->need_compile() ? func2->compilation_type() : ctNone;
					return (ct1 == ct2) ? (func1->compilation_options() < func2->compilation_options()) : (ct1 < ct2);
				}
			}
		}
		return func1->folder() < func2->folder();
	}
};

#ifdef ULTIMATE
struct LicensingManagerCompareHelper {
	int field;
	LicensingManagerCompareHelper(int _field) : field(_field) {}
	bool operator () (const License *license1, const License *license2) const
	{
		switch (field) {
		case 0:
			return QString::compare(QString::fromUtf8(license1->customer_name().c_str()), QString::fromUtf8(license2->customer_name().c_str()), Qt::CaseInsensitive) < 0;
		case 1:
			return QString::compare(QString::fromUtf8(license1->customer_email().c_str()), QString::fromUtf8(license2->customer_email().c_str()), Qt::CaseInsensitive) < 0;
		case 2:
			return license1->date().value() < license2->date().value();
		}
		return false;
	}
};

struct FileManagerCompareHelper {
	FileFolder *folder;
	int field;
	FileManagerCompareHelper(FileFolder *_folder, int _field) : folder(_folder), field(_field) {}
	bool operator () (const InternalFile *file1, const InternalFile *file2) const
	{
		if (file1->folder() == file2->folder() && file1->folder() == folder) {
			switch (field) {
			case 0:
				return QString::compare(QString::fromUtf8(file1->name().c_str()), QString::fromUtf8(file2->name().c_str()), Qt::CaseInsensitive) < 0;
			case 1:
				return QString::compare(QString::fromUtf8(file1->file_name().c_str()), QString::fromUtf8(file2->file_name().c_str()), Qt::CaseInsensitive) < 0;
			}
		}
		return file1->folder() < file2->folder();
	}
};

struct FileFolderCompareHelper {
	bool operator () (const FileFolder *folder1, const FileFolder *folder2) const
	{
		return QString::compare(QString::fromUtf8(folder1->name().c_str()), QString::fromUtf8(folder2->name().c_str()), Qt::CaseInsensitive) < 0;
	}
};
#endif

struct FolderCompareHelper {
	bool operator () (const Folder *folder1, const Folder *folder2) const
	{
		return QString::compare(QString::fromUtf8(folder1->name().c_str()), QString::fromUtf8(folder2->name().c_str()), Qt::CaseInsensitive) < 0;
	}
};

void ProjectModel::sortNode(ProjectNode *node, int field)
{
	bool isModified = false;
	QModelIndex parent = nodeToIndex(node);
	QList<ProjectNode*> nodeList = node->children();

	switch (node->type()) {
	case NODE_FUNCTIONS:
	case NODE_FOLDER:
		{
			IFile *file = core()->input_file();
			Folder *folder = (node->type() == NODE_FUNCTIONS) ? core()->input_file()->folder_list() : reinterpret_cast<Folder *>(node->data());

			if (field == 0) {
				std::sort(folder->_begin(),folder->_end(), FolderCompareHelper());

				QList<int> posList;
				for (int i = 0; i < nodeList.size(); i++) {
					ProjectNode *child = nodeList[i];
					if (child->type() != NODE_FOLDER) 
						break;

					int j = (int)folder->IndexOf(reinterpret_cast<Folder *>(child->data()));
					if (j == -1)
						continue;

					int p = posList.size();
					for (int c = 0; c < posList.size(); c++) {
						if (posList[c] > j) {
							p = c;
							break;
						}
					}

					if (p < i) {
						beginMoveRows(parent, i, i, parent, p);
						node->removeChild(child);
						node->insertChild(p, child);
						endMoveRows();
						isModified = true;
					}
					posList.insert(p, j);
				}
			}

			if (node->type() == NODE_FUNCTIONS)
				folder = NULL;

			std::sort(file->function_list()->_begin(), file->function_list()->_end(), FunctionBundleListCompareHelper(folder, field));

			QList<FunctionBundle *> funcList;
			for (size_t i = 0; i < file->function_list()->count(); i++) {
				FunctionBundle *func = file->function_list()->item(i);
				if (func->folder() == folder)
					funcList.append(func);
			}

			int k = 0;
			QList<int> posList;
			for (int i = 0; i < nodeList.size(); i++) {
				ProjectNode *child = nodeList[i];
				if (child->type() == NODE_FOLDER) {
					k++;
					continue;
				}

				int j = funcList.indexOf(reinterpret_cast<FunctionBundle *>(child->data()));
				if (j == -1)
					continue;

				int p = posList.size();
				for (int c = 0; c < posList.size(); c++) {
					if (posList[c] > j) {
						p = c;
						break;
					}
				}

				if (k + p < i) {
					beginMoveRows(parent, i, i, parent, k + p);
					node->removeChild(child);
					node->insertChild(k + p, child);
					endMoveRows();
					isModified = true;
				}
				posList.insert(p, j);
			}
		}
		break;
#ifdef ULTIMATE
	case NODE_LICENSES:
		{
			std::sort(core()->licensing_manager()->_begin(), core()->licensing_manager()->_end(), LicensingManagerCompareHelper(field));

			QList<int> posList;
			for (int i = 0; i < node->childCount(); i++) {
				ProjectNode *child = node->child(i);

				int j = (int)core()->licensing_manager()->IndexOf(reinterpret_cast<License *>(child->data()));
				if (j == -1)
					continue;

				int p = posList.size();
				for (int c = 0; c < posList.size(); c++) {
					if (posList[c] > j) {
						p = c;
						break;
					}
				}

				if (p < i) {
					beginMoveRows(parent, i, i, parent, p);
					node->removeChild(child);
					node->insertChild(p, child);
					endMoveRows();
					isModified = true;
				}
				posList.insert(p, j);
			}
		}
		break;
	case NODE_FILES:
	case NODE_FILE_FOLDER:
	case NODE_ASSEMBLIES:
		{
			FileFolder *folder = (node->type() == NODE_FILES || node->type() == NODE_ASSEMBLIES) ? core()->file_manager()->folder_list() : reinterpret_cast<FileFolder *>(node->data());

			if (field == 0) {
				std::sort(folder->_begin(),folder->_end(), FileFolderCompareHelper());

				QList<int> posList;
				for (int i = 0; i < node->childCount(); i++) {
					ProjectNode *child = node->child(i);
					if (child->type() != NODE_FILE_FOLDER) 
						break;

					int j = (int)folder->IndexOf(reinterpret_cast<FileFolder *>(child->data()));
					if (j == -1)
						continue;

					int p = posList.size();
					for (int c = 0; c < posList.size(); c++) {
						if (posList[c] > j) {
							p = c;
							break;
						}
					}

					if (p < i) {
						beginMoveRows(parent, i, i, parent, p);
						node->removeChild(child);
						node->insertChild(p, child);
						endMoveRows();
						isModified = true;
					}
					posList.insert(p, j);
				}
			}

			if (node->type() == NODE_FILES || node->type() == NODE_ASSEMBLIES)
				folder = NULL;

			std::sort(core()->file_manager()->_begin(), core()->file_manager()->_end(), FileManagerCompareHelper(folder, field));

			QList<InternalFile *> fileList;
			for (size_t i = 0; i < core()->file_manager()->count(); i++) {
				InternalFile *file = core()->file_manager()->item(i);
				if (file->folder() == folder)
					fileList.append(file);
			}

			int k = 0;
			QList<int> posList;
			for (int i = 0; i < node->childCount(); i++) {
				ProjectNode *child = node->child(i);
				if (child->type() == NODE_FILE_FOLDER) {
					k++;
					continue;
				}

				int j = fileList.indexOf(reinterpret_cast<InternalFile *>(child->data()));
				if (j == -1)
					continue;

				int p = posList.size();
				for (int c = 0; c < posList.size(); c++) {
					if (posList[c] > j) {
						p = c;
						break;
					}
				}

				if (k + p < i) {
					beginMoveRows(parent, i, i, parent, k + p);
					node->removeChild(child);
					node->insertChild(k + p, child);
					endMoveRows();
					isModified = true;
				}
				posList.insert(p, j);
			}
		}
		break;

#endif
	}

	if (isModified)
		emit modified();
}

/**
 * FunctionsModel
 */

FunctionsModel::FunctionsModel(QObject *parent)
	: BaseModel(parent)
{

}

void FunctionsModel::setCore(Core *core)
{
	beginResetModel();
	BaseModel::setCore(core);

	if (core)
		createFunctionsTree(root());

	endResetModel();
}

void FunctionsModel::updateFunction(IFunction *func)
{
	IArchitecture *arch = func->owner()->owner();
	MapFunctionBundle *map_func = core()->input_file()->map_function_list()->GetFunctionByAddress(arch, func->address());
	if (!map_func)
		return;
	
	ProjectNode *node = objectToNode(map_func);
	if (!node)
		return;

	node->setIcon(functionIcon(core()->input_file()->function_list()->GetFunctionByAddress(arch, func->address())));
	updateNode(node);
}

void FunctionsModel::removeFunction(IFunction *func)
{
	updateFunction(func);
}

QVariant FunctionsModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0)
			return QString::fromUtf8(language[lsFunctions].c_str());
	}
	return QVariant();
}

/**
 * InfoModel
 */

InfoModel::InfoModel(QObject *parent)
	: BaseModel(parent)
{

}

void InfoModel::setCore(Core *core)
{
	BaseModel::setCore(core);

	if (!core || !core->input_file()) {
		beginResetModel();
		endResetModel();
		return;
	}

	size_t i, j, k;
	QList<ProjectNode*> nodes;
	for (i = 0; i < core->input_file()->count(); i++) {
		IArchitecture *arch = core->input_file()->item(i);
		if (!arch->visible())
			continue;

		ProjectNode *node = new ProjectNode(root(), NODE_ARCHITECTURE, arch);
		node->setText(arch->name().c_str());
		node->setIcon(nodeIcon(node->type()));
		nodes.append(node);
	}

	IArchitecture *default_arch = NULL;
	if (nodes.size() == 1) {
		default_arch = reinterpret_cast<IArchitecture*>(nodes[0]->data());
		delete nodes[0];
		nodes[0] = root();
	}

	for (int f = 0; f < nodes.size(); f++) {
		ProjectNode *nodeFile = nodes[f];
		IArchitecture *file = (nodeFile->type() == NODE_ROOT) ? default_arch : reinterpret_cast<IArchitecture*>(nodeFile->data());
		assert(file);
		if (file == NULL) continue;

		if (file->command_list()->count()) {
			ProjectNode *nodeCommands = new ProjectNode(nodeFile, NODE_LOAD_COMMANDS);
			nodeCommands->setText(QString::fromUtf8(language[lsDirectories].c_str()));
			nodeCommands->setIcon(nodeIcon(nodeCommands->type()));
			for (i = 0; i < file->command_list()->count(); i++) {
				ILoadCommand *command = file->command_list()->item(i);
				if (!command->visible())
					continue;

				ProjectNode *node = new ProjectNode(nodeCommands, NODE_LOAD_COMMAND, command);
				node->setText(QString::fromUtf8(command->name().c_str()));
				node->setIcon(nodeIcon(node->type()));
				setObjectNode(command, node);
			}
		}

		ProjectNode *nodeSegments = new ProjectNode(nodeFile, NODE_SEGMENTS);
		nodeSegments->setText(QString::fromUtf8(language[lsSegments].c_str()));
		nodeSegments->setIcon(nodeIcon(nodeSegments->type()));
		for (i = 0; i < file->segment_list()->count(); i++) {
			ISection *segment = file->segment_list()->item(i);

			ProjectNode *node = new ProjectNode(nodeSegments, NODE_SEGMENT, segment);
			node->setText(QString::fromUtf8(segment->name().c_str()));
			bool has_children = false;
			for (j = 0; j < file->section_list()->count(); j++) {
				if (file->section_list()->item(j)->parent() == segment) {
					has_children = true;
					break;
				}
			}

			node->setIcon(nodeIcon(has_children ? NODE_FOLDER : node->type(), segment->excluded_from_packing() || segment->excluded_from_memory_protection()));
			setObjectNode(segment, node);
		}

		for (i = 0; i < file->section_list()->count(); i++) {
			ISection *section = file->section_list()->item(i);
			j =  file->segment_list()->IndexOf(section->parent());
			if (j == NOT_ID)
				continue;

			ProjectNode *node = new ProjectNode(nodeSegments->child((int)j), NODE_SECTION, section);
			node->setText(QString::fromUtf8(section->name().c_str()));
			node->setIcon(nodeIcon(node->type()));
		}

		if (file->import_list()->count()) {
			ProjectNode *nodeImports = new ProjectNode(nodeFile, NODE_IMPORTS, file->import_list());
			nodeImports->setText(QString::fromUtf8(language[lsImports].c_str()));
			nodeImports->setIcon(nodeIcon(nodeImports->type()));
			for (i = 0; i < file->import_list()->count(); i++) {
				IImport *import = file->import_list()->item(i);

				ProjectNode *nodeImport = new ProjectNode(nodeImports, NODE_IMPORT, import);
				nodeImport->setText(QString::fromUtf8(import->name().c_str()));
				nodeImport->setIcon(nodeIcon(nodeImport->type(), import->excluded_from_import_protection()));
				setObjectNode(import, nodeImport);

				createFunctionsTree(nodeImport);

				if (import->name().empty()) {
					QList<ProjectNode *> nodeList = nodeImport->children();
					for (int k = 0; k < nodeList.count(); k++) {
						ProjectNode *child = nodeList[k];
						nodeImport->removeChild(child);
						nodeImports->addChild(child);
					}
					delete nodeImport;
				}
			}
		}

		if (file->export_list()->count()) {
			ProjectNode *nodeExports = new ProjectNode(nodeFile, NODE_EXPORTS, file->export_list());
			nodeExports->setText(QString::fromUtf8(language[lsExports].c_str()));
			nodeExports->setIcon(nodeIcon(nodeExports->type()));

			createFunctionsTree(nodeExports);
		}

		if (file->resource_list() && file->resource_list()->count()) {
			ProjectNode *nodeResources = new ProjectNode(nodeFile, NODE_RESOURCES, file->resource_list());
			nodeResources->setText(QString::fromUtf8(language[lsResources].c_str()));
			nodeResources->setIcon(nodeIcon(nodeResources->type()));
			std::vector<IResource *> resourceList;
			for (i = 0; i < file->resource_list()->count(); i++) {
				resourceList.push_back(file->resource_list()->item(i));
			}
			for (k = 0; k < resourceList.size(); k++) {
				IResource *resource = resourceList[k];
				for (j = 0; j < resource->count(); j++) {
					resourceList.push_back(resource->item(j));
				}
			}
			for (k = 0; k < resourceList.size(); k++) {
				IResource *resource = resourceList[k];

				ProjectNode *nodeParent = objectToNode(resource->owner());
				if (!nodeParent)
					nodeParent = nodeResources;
				ProjectNode *node = new ProjectNode(nodeParent, resource->is_directory() ? NODE_RESOURCE_FOLDER : NODE_RESOURCE, resource);
				node->setText(QString::fromUtf8(resource->name().c_str()));
				node->setIcon(nodeIcon(node->type(), node->type() == NODE_RESOURCE && resource->excluded_from_packing()));
				setObjectNode(resource, node);
			}
		}

		ProjectNode *nodeCalc = new ProjectNode(nodeFile, NODE_CALC, file);
		nodeCalc->setText(QString::fromUtf8(language[lsCalculator].c_str()));
		nodeCalc->setIcon(nodeIcon(nodeCalc->type()));
		
		ProjectNode *nodeDump = new ProjectNode(nodeFile, NODE_DUMP, file);
		nodeDump->setText(QString::fromUtf8(language[lsDump].c_str()));
		nodeDump->setIcon(nodeIcon(nodeDump->type()));
		setObjectNode(file, nodeDump);
	}
}

void InfoModel::updateResource(IResource *resource)
{
	ProjectNode *node = objectToNode(resource);
	if (!node)
		return;

	node->setIcon(nodeIcon(node->type(), resource->excluded_from_packing()));
	updateNode(node);
	emit modified();
}

void InfoModel::updateSegment(ISection *segment)
{
	ProjectNode *node = objectToNode(segment);
	if (!node)
		return;

	node->setIcon(nodeIcon(node->childCount() ? NODE_FOLDER : node->type(), segment->excluded_from_packing() || segment->excluded_from_memory_protection()));
	updateNode(node);
	emit modified();
}

void InfoModel::updateImport(IImport *import)
{
	ProjectNode *node = objectToNode(import);
	if (!node)
		return;

	node->setIcon(nodeIcon(node->type(), import->excluded_from_import_protection()));
	updateNode(node);
	emit modified();
}

QVariant InfoModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0)
			return QString::fromUtf8(language[lsDetails].c_str());
	}
	return QVariant();
}

/**
 * SearchModel
 */

SearchModel::SearchModel(QObject *parent)
	: QAbstractItemModel(parent)
{

}

void SearchModel::clear()
{
	if (items_.count())
		removeRows(0, items_.count());
}

void SearchModel::search(ProjectNode *directory, const QString &text, bool protectedFunctionsOnly)
{
	beginResetModel();
	QList<ProjectNode *> list;
	QRegExp filter(text, Qt::CaseInsensitive, QRegExp::Wildcard);

	items_.clear();
	list.append(directory);
	for (int i = 0; i < list.count(); i++) {
		ProjectNode *node = list[i];
		for (int j = 0; j < node->childCount(); j++) {
			list.insert(i + 1 + j, node->child(j));
		}

		if (node->properties())
			list.insert(i + 1, node->properties());

		if (node->parent() && node->contains(filter)) {
			if (protectedFunctionsOnly) {
				switch (node->type()) {
				case NODE_MAP_FUNCTION:
					{
						MapFunctionBundle *map = reinterpret_cast<MapFunctionBundle*>(node->data());
						FunctionBundle *func = map->owner()->owner()->function_list()->GetFunctionByName(map->name());
						if (!func || !func->need_compile())
							continue;
					}
					break;
				default:
					continue;
				}
			}
			items_.append(node);
		}
	}

	endResetModel();
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	ProjectNode *node = items_.value(row);
	if (!node)
		return QModelIndex();

	return createIndex(row, column, node);
}

QModelIndex SearchModel::parent(const QModelIndex & /*index*/) const
{
	return QModelIndex();
}

int SearchModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return items_.size();
}

int SearchModel::columnCount(const QModelIndex & /*parent*/) const
{
	return 1;
}

ProjectNode *SearchModel::indexToNode(const QModelIndex &index) const 
{
	if (index.isValid())
		return static_cast<ProjectNode *>(index.internalPointer());
	return NULL;
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	ProjectNode *node = indexToNode(index);
	assert(node);
	if (node != NULL) {
		switch (role) {
		case Qt::DisplayRole:
			return node->text(index.column());
		case Qt::DecorationRole:
			if (index.column() == 0)
				return node->icon();
			break;
		case Qt::ToolTipRole:
			{
				QString text = node->path();
				if (!text.isEmpty())
					text += " > " + node->text(index.column());
				return text;
			}
			break;
		}
	}
	return QVariant();
}

QVariant SearchModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole)
		return QString::fromUtf8(language[lsSearchResult].c_str());
	return QVariant();
}

QModelIndex SearchModel::nodeToIndex(ProjectNode *node) const
{
	int i = items_.indexOf(node);
	if (i == -1)
		return QModelIndex();

	return createIndex(i, 0, node);
}

void SearchModel::updateNode(ProjectNode *node)
{
	int i = items_.indexOf(node);
	if (i == -1)
		return;

	QModelIndex index = createIndex(i, 0, node);
	dataChanged(index, index);
}

bool SearchModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    bool res;

    beginRemoveRows(parent, position, position + rows - 1);
    if (position < 0 || position + rows > items_.size()) {
		res = false;
	} else {
		for (int row = position; row < rows; ++row)
			items_.removeAt(position);
		res = true;
	}
	endRemoveRows();

    return res;
}

void SearchModel::removeNode(ProjectNode *node)
{
	int i = items_.indexOf(node);
	if (i == -1)
		return;

	removeRows(i, 1);
}

/**
 * DirectoryModel
 */

DirectoryModel::DirectoryModel(QObject *parent)
	: QAbstractItemModel(parent), directory_(NULL)
{

}

Qt::ItemFlags DirectoryModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags res = QAbstractItemModel::flags(index);
	if (res & Qt::ItemIsSelectable) {
		ProjectNode *node = indexToNode(index);
		if (node) {
			if (node->type() == NODE_FILE_FOLDER || node->type() == NODE_FOLDER || node->type() == NODE_FILE || node->type() == NODE_LICENSE)
				res |= Qt::ItemIsEditable;
		}
	}
	return res;
}

void DirectoryModel::setDirectory(ProjectNode *directory)
{
	beginResetModel();
	directory_ = directory;
	if (directory_) {
		items_ = directory->children();
	} else {
		items_.clear();
	}

	endResetModel();
}

QModelIndex DirectoryModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	ProjectNode *node = items_.value(row);
	if (!node)
		return QModelIndex();

	return createIndex(row, column, node);
}

QModelIndex DirectoryModel::parent(const QModelIndex & /*index*/) const
{
	return QModelIndex();
}

int DirectoryModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return items_.size();
}

QStringList headerLabels(NodeType type)
{
	QStringList res;
	switch (type) {
	case NODE_FUNCTIONS:
	case NODE_FOLDER:
	case NODE_MAP_FOLDER:
		res << QString::fromUtf8(language[lsName].c_str()) << QString::fromUtf8(language[lsAddress].c_str()) <<  QString::fromUtf8(language[lsProtection].c_str());
		break;
	case NODE_RESOURCES:
	case NODE_RESOURCE_FOLDER:
	case NODE_LOAD_COMMANDS:
		res << QString::fromUtf8(language[lsName].c_str()) << QString::fromUtf8(language[lsAddress].c_str()) << QString::fromUtf8(language[lsSize].c_str());
		break;
	case NODE_SEGMENTS:
	case NODE_SEGMENT:
		res << QString::fromUtf8(language[lsName].c_str()) << QString::fromUtf8(language[lsAddress].c_str()) << QString::fromUtf8(language[lsSize].c_str()) << QString::fromUtf8(language[lsRawAddress].c_str()) << QString::fromUtf8(language[lsRawSize].c_str()) << QString::fromUtf8(language[lsFlags].c_str());
		break;
	case NODE_IMPORT:
	case NODE_IMPORT_FOLDER:
	case NODE_EXPORTS:
	case NODE_EXPORT_FOLDER:
		res << QString::fromUtf8(language[lsName].c_str()) << QString::fromUtf8(language[lsAddress].c_str());
		break;
	case NODE_LICENSES:
		res << QString::fromUtf8(language[lsCustomerName].c_str()) << QString::fromUtf8(language[lsEmail].c_str()) << QString::fromUtf8(language[lsDate].c_str());
		break;
	case NODE_FILES:
	case NODE_FILE_FOLDER:
	case NODE_ASSEMBLIES:
		res << QString::fromUtf8(language[lsName].c_str()) << QString::fromUtf8(language[lsFileName].c_str());
		break;
	default:
		res << QString::fromUtf8(language[lsName].c_str());
		break;
	}
	return res;
};

int DirectoryModel::columnCount(const QModelIndex & /*parent*/) const
{
	if (!directory_)
		return 0;

	return headerLabels(directory_->type()).size();
}

ProjectNode *DirectoryModel::indexToNode(const QModelIndex &index) const 
{
	if (index.isValid())
		return static_cast<ProjectNode *>(index.internalPointer());
	return NULL;
}

QVariant DirectoryModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	ProjectNode *node = indexToNode(index);
	if (node != NULL) {
		if (role == Qt::DisplayRole) {
			return node->text(index.column());
		} else if (role == Qt::DecorationRole) {
			if (index.column() == 0)
				return node->icon();
		}
	}
	return QVariant();
}

QVariant DirectoryModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (!directory_)
		return QVariant();

	if (role == Qt::DisplayRole) {
		return headerLabels(directory_->type()).value(section);
	}
	return QVariant();
}

QModelIndex DirectoryModel::nodeToIndex(ProjectNode *node) const
{
	int i = items_.indexOf(node);
	if (i == -1)
		return QModelIndex();

	return createIndex(i, 0, node);
}

void DirectoryModel::updateNode(ProjectNode *node)
{
	int i = items_.indexOf(node);
	if (i == -1)
		return;

	QModelIndex index = createIndex(i, 0, node);
	dataChanged(index, index);
}

bool DirectoryModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    bool res;

    beginRemoveRows(parent, position, position + rows - 1);
    if (position < 0 || position + rows > items_.size()) {
		res = false;
	} else {
		for (int row = 0; row < rows; ++row)
			items_.removeAt(position);
		res = true;
	}
	endRemoveRows();

    return res;
}

void DirectoryModel::removeNode(ProjectNode *node)
{
	ProjectNode *parent = directory_;
	while (parent) {
		if (parent == node) {
			setDirectory(NULL);
			return;
		}
		parent = parent->parent();
	}

	int i = items_.indexOf(node);
	if (i == -1)
		return;

	removeRows(i, 1);
}

/**
 * MapFunctionBundleListModel
 */

MapFunctionBundleListModel::MapFunctionBundleListModel(IFile &file, bool codeOnly, QObject *parent)
	: QAbstractItemModel(parent)
{
	MapFunctionBundleList *map_function_list = file.map_function_list();
	for (size_t i = 0; i < map_function_list->count(); i++) {
		MapFunctionBundle *func = map_function_list->item(i);
		switch (func->type()) {
		case otCode:
		case otExport:
		case otMarker:
		case otAPIMarker:
		case otString:
			break;
		case otUnknown:
			continue;
		default:
			if (codeOnly)
				continue;
			break;
		}

		mapFunctionList_.append(func);
	}

	items_ = mapFunctionList_;
}

int MapFunctionBundleListModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid())
		return items_.size();

	return 0;
}

int MapFunctionBundleListModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 2;
}

QModelIndex MapFunctionBundleListModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	MapFunctionBundle *bundle = items_.value(row);
	if (!bundle)
		return QModelIndex();

	return createIndex(row, column, bundle);
}

QModelIndex MapFunctionBundleListModel::parent(const QModelIndex & /*index*/) const
{
	return QModelIndex();
}

QVariant MapFunctionBundleListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

	MapFunctionBundle *func = items_.at(index.row());

	if (role == Qt::DisplayRole) {
		if (index.column() == 0) {
			return QString::fromUtf8(func->display_name().c_str());
		} else if (index.column() == 1) {
			return QString::fromLatin1(func->display_address().c_str());
		}
	} else if (role == Qt::DecorationRole)	{
		if (index.column() == 0) {
			switch (func->type()) {
			case otImport:
				return nodeIcon(NODE_IMPORT_FUNCTION);
			default:
				return functionIcon(func->owner()->owner()->function_list()->GetFunctionByName(func->name()));
			}
		}
	}
	return QVariant();
}

QVariant MapFunctionBundleListModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0:
        return QString::fromUtf8(language[lsName].c_str());
    case 1:
        return QString::fromUtf8(language[lsAddress].c_str());
    }
    return QVariant();
}

void MapFunctionBundleListModel::search(const QString &text)
{
	beginResetModel();
	if (text.isEmpty()) {
		items_ = mapFunctionList_;
	} else {
		items_.clear();
		QRegExp filter(text, Qt::CaseInsensitive, QRegExp::Wildcard);

		for (int i = 0; i < mapFunctionList_.size(); i++) {
			MapFunctionBundle *func = mapFunctionList_[i];
			QString name = QString::fromUtf8(func->name().c_str());
			if (name.contains(filter))
				items_.append(func);
		}
	}

	endResetModel();
}

struct MapFunctionBundleListCompareHelper {
	int column;
	MapFunctionBundleListCompareHelper(int _column) : column(_column) {}
	bool operator () (const MapFunctionBundle *func1, const MapFunctionBundle *func2) const
	{
		switch (column) {
		case 0:
			return QString::fromUtf8(func1->display_name().c_str()).compare(QString::fromUtf8(func2->display_name().c_str()), Qt::CaseInsensitive) < 0;
		case 1:
			return func1->display_address() < func2->display_address();
		}
		return false;
	}
};

void MapFunctionBundleListModel::sort(int column, Qt::SortOrder order)
{
	beginResetModel();
	qSort(mapFunctionList_.begin(), mapFunctionList_.end(), MapFunctionBundleListCompareHelper(column));
	if (items_.size() == mapFunctionList_.size())
		items_ = mapFunctionList_;
	else
		qSort(items_.begin(), items_.end(), MapFunctionBundleListCompareHelper(column));
	endResetModel();
}

/**
 * MapFunctionListModel
 */

MapFunctionListModel::MapFunctionListModel(IArchitecture &file, bool codeOnly, QObject *parent)
	: QAbstractItemModel(parent)
{
	MapFunctionList *map_function_list = file.map_function_list();
	for (size_t i = 0; i < map_function_list->count(); i++) {
		MapFunction *func = map_function_list->item(i);
		switch (func->type()) {
		case otCode:
		case otExport:
		case otMarker:
		case otAPIMarker:
		case otString:
			break;
		case otUnknown:
			continue;
		default:
			if (codeOnly)
				continue;
			break;
		}

		mapFunctionList_.append(func);
	}

	items_ = mapFunctionList_;
}

int MapFunctionListModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid())
		return items_.size();

	return 0;
}

int MapFunctionListModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 2;
}

QModelIndex MapFunctionListModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	MapFunction *func = items_.value(row);
	if (!func)
		return QModelIndex();

	return createIndex(row, column, func);
}

QModelIndex MapFunctionListModel::parent(const QModelIndex & /*index*/) const
{
	return QModelIndex();
}

QVariant MapFunctionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

	MapFunction *func = items_.at(index.row());

	if (role == Qt::DisplayRole) {
		if (index.column() == 0) {
			return QString::fromUtf8(func->display_name().c_str());
		} else if (index.column() == 1) {
			return QString::fromLatin1(func->display_address("").c_str());
		}
	} else if (role == Qt::DecorationRole)	{
		if (index.column() == 0) {
			switch (func->type()) {
			case otImport:
				return nodeIcon(NODE_IMPORT_FUNCTION);
			default:
				return functionIcon(func->owner()->owner()->function_list()->GetFunctionByAddress(func->address()));
			}
		}
	}
	return QVariant();
}

QVariant MapFunctionListModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0:
        return QString::fromUtf8(language[lsName].c_str());
    case 1:
        return QString::fromUtf8(language[lsAddress].c_str());
    }
    return QVariant();
}

void MapFunctionListModel::search(const QString &text)
{
	beginResetModel();
	if (text.isEmpty()) {
		items_ = mapFunctionList_;
	} else {
		items_.clear();
		QRegExp filter(text, Qt::CaseInsensitive, QRegExp::Wildcard);

		for (int i = 0; i < mapFunctionList_.size(); i++) {
			MapFunction *func = mapFunctionList_[i];
			QString name = QString::fromUtf8(func->name().c_str());
			if (name.contains(filter))
				items_.append(func);
		}
	}

	endResetModel();
}

struct MapFunctionListCompareHelper {
	int column;
	MapFunctionListCompareHelper(int _column) : column(_column) {}
	bool operator () (const MapFunction *func1, const MapFunction *func2) const
	{
		switch (column) {
		case 0:
			return QString::fromUtf8(func1->display_name().c_str()).compare(QString::fromUtf8(func2->display_name().c_str()), Qt::CaseInsensitive) < 0;
		case 1:
			return func1->display_address("") < func2->display_address("");
		}
		return false;
	}
};

void MapFunctionListModel::sort(int column, Qt::SortOrder order)
{
	beginResetModel();
	qSort(mapFunctionList_.begin(), mapFunctionList_.end(), MapFunctionListCompareHelper(column));
	if (items_.size() == mapFunctionList_.size())
		items_ = mapFunctionList_;
	else
		qSort(items_.begin(), items_.end(), MapFunctionListCompareHelper(column));
	endResetModel();
}

/**
 * WatermarksModel
 */

WatermarksModel::WatermarksModel(QObject *parent)
	: BaseModel(parent)
{

}

QVariant WatermarksModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0)
			return QString::fromUtf8(language[lsName].c_str());
	}
	return QVariant();
}

Qt::ItemFlags WatermarksModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags res = QAbstractItemModel::flags(index);
	if (res & Qt::ItemIsSelectable) {
		ProjectNode *node = indexToNode(index);
		if (node) {
			if (node->type() == NODE_WATERMARK)
				res |= Qt::ItemIsEditable;
		}
	}
	return res;
}

ProjectNode *WatermarksModel::internalAddWatermark(Watermark *watermark)
{
	beginInsertRows(nodeToIndex(NULL), root()->childCount(), root()->childCount());
	ProjectNode *node = new ProjectNode(root(), NODE_WATERMARK, (void *)watermark);
	endInsertRows();

	setObjectNode(watermark, node);
	internalUpdateWatermark(watermark);
	return node;
}

ProjectNode *WatermarksModel::internalUpdateWatermark(Watermark *watermark)
{
	ProjectNode *node = objectToNode(watermark);
	if (!node)
		return NULL;

	node->setText(QString::fromUtf8(watermark->name().c_str()));
	node->setIcon(nodeIcon(node->type(), !watermark->enabled()));
	return node;
}

void WatermarksModel::setCore(Core *core)
{
	beginResetModel();
	BaseModel::setCore(core);

	if (core) {
		WatermarkManager *manager = core->watermark_manager();
		for (size_t i = 0; i < manager->count(); i++) {
			internalAddWatermark(manager->item(i));
		}
	}
	endResetModel();
}

void WatermarksModel::addWatermark(Watermark *watermark)
{
	updateNode(internalAddWatermark(watermark));
}

void WatermarksModel::updateWatermark(Watermark *watermark)
{
	updateNode(internalUpdateWatermark(watermark));
}

void WatermarksModel::removeWatermark(Watermark *watermark)
{
	removeNode(watermark);
}

QModelIndex WatermarksModel::indexByName(const QString &watermarkName) const
{
	for (int i = 0; i < root()->childCount(); i++) {
		ProjectNode *node = root()->child(i);
		if (node->text() == watermarkName)
			return nodeToIndex(node);
	}

	return QModelIndex();
}

WatermarkManager * WatermarksModel::manager() const
{
	return core() ? core()->watermark_manager() : NULL;
}

/**
 * WatermarkScanModel
 */

WatermarkScanModel::WatermarkScanModel(QObject *parent)
	: QAbstractItemModel(parent)
{

}

QModelIndex WatermarkScanModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	Watermark *watermark = items_.keys().value(row);
	if (!watermark)
		return QModelIndex();

	return createIndex(row, column, watermark);
}

QModelIndex WatermarkScanModel::parent(const QModelIndex & /*index*/) const
{
	return QModelIndex();
}

int WatermarkScanModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return items_.size();
}

int WatermarkScanModel::columnCount(const QModelIndex & /*parent*/) const
{
	return 2;
}

Watermark *WatermarkScanModel::indexToWatermark(const QModelIndex &index) const 
{
	if (index.isValid())
		return static_cast<Watermark *>(index.internalPointer());
	return NULL;
}

QVariant WatermarkScanModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	Watermark *watermark = indexToWatermark(index);
	assert(watermark);
	if (watermark != NULL ) {
		if (role == Qt::DisplayRole) {
			if (index.column() == 0) {
				return QString::fromUtf8(watermark->name().c_str());
			} else if (index.column() == 1) {
				return QString::number(items_[watermark]);
			}
		} else if (role == Qt::DecorationRole) {
			if (index.column() == 0)
				return watermark->enabled() ? QIcon(":/images/item_watermark.png") : QIcon(":/images/item_watermark_black.png");
		}
	}
	return QVariant();
}

QVariant WatermarkScanModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0) {
			return QString::fromUtf8(language[lsName].c_str());
		} else if (section == 1) {
			return QString::fromUtf8(language[lsCount].c_str());
		}
	}
	return QVariant();
}

void WatermarkScanModel::setWatermarkData(std::map<Watermark *, size_t> data)
{
	beginResetModel();
	items_.clear();
	for (std::map<Watermark *, size_t>::const_iterator it = data.begin(); it != data.end(); it++) {
		items_[it->first] = it->second;
	}

	endResetModel();
}

void WatermarkScanModel::removeWatermark(Watermark *watermark)
{
	int position = items_.keys().indexOf(watermark);
	if (position == -1)
		return;

	beginRemoveRows(QModelIndex(), position, position);
	items_.remove(watermark);
	endRemoveRows();
}

void WatermarkScanModel::clear()
{
	if (items_.count())
		removeRows(0, items_.count());
}

/**
 * TemplatesModel
 */

TemplatesModel::TemplatesModel(QObject *parent)
	: BaseModel(parent)
{

}

QVariant TemplatesModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section == 0)
			return QString::fromUtf8(language[lsName].c_str());
	}
	return QVariant();
}

Qt::ItemFlags TemplatesModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags res = QAbstractItemModel::flags(index);
	if (res & Qt::ItemIsSelectable) {
		if (index.row() != 0) 
			res |= Qt::ItemIsEditable;
	}
	return res;
}

QVariant TemplatesModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole && index.row() == 0)
		return "(" + QString::fromUtf8(language[lsDefault].c_str()) + ")";
	return BaseModel::data(index, role);
}

void TemplatesModel::setCore(Core *core)
{
	beginResetModel();
	BaseModel::setCore(core);

	if (core) {
		ProjectTemplateManager *manager = core->template_manager();
		for (size_t i = 0; i < manager->count(); i++) {
			ProjectTemplate *pt = manager->item(i);
			internalAddTemplate(pt);
		}
	}
	endResetModel();
}

void TemplatesModel::updateTemplate(ProjectTemplate *pt)
{
	updateNode(internalUpdateTemplate(pt));
}

void TemplatesModel::removeTemplate(ProjectTemplate *pt)
{
	removeNode(pt);
}

ProjectNode * TemplatesModel::internalUpdateTemplate(ProjectTemplate *pt)
{
	ProjectNode *node = objectToNode(pt);
	if (!node)
		return NULL;

	node->setText(QString::fromUtf8(pt->name().c_str()));
	node->setIcon(nodeIcon(node->type()));
	return node;
}

ProjectNode * TemplatesModel::internalAddTemplate(ProjectTemplate *pt)
{
	beginInsertRows(nodeToIndex(NULL), root()->childCount(), root()->childCount());
	ProjectNode *node = new ProjectNode(root(), NODE_TEMPLATE, (void *)pt);
	endInsertRows();

	setObjectNode(pt, node);
	internalUpdateTemplate(pt);
	return node;
}

void TemplatesModel::addTemplate(ProjectTemplate * pt)
{
	updateNode(internalAddTemplate(pt));
}

/**
 * LogModel
 */

LogModel::LogModel(QObject *parent)
	: BaseModel(parent)
{

}

void LogModel::addMessage(MessageType type, IObject *sender, const QString &text)
{
	NodeType node_type;
	QString icon_name;
	if (type == mtWarning) {
		node_type = NODE_WARNING;
		icon_name = ":/images/warning.png";
	} else if (type == mtError) {
		node_type = NODE_ERROR;
		icon_name = ":/images/error.png";
	} else
		node_type = NODE_MESSAGE;

	ProjectNode *node;
	beginInsertRows(QModelIndex(), root()->childCount(), root()->childCount());
	node = new ProjectNode(root(), node_type, sender);
	endInsertRows();

	if (sender)
		setObjectNode(sender, node);

	node->setText(text);
	if (!icon_name.isEmpty())
		node->setIcon(QIcon(icon_name));
}

QVariant LogModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		return QString::fromUtf8(language[lsCompilationLog].c_str());
	}
	return QVariant();
}

void LogModel::clear()
{
	beginRemoveRows(QModelIndex(), 0, root()->childCount());
	BaseModel::clear();
	endRemoveRows();
}

void LogModel::removeObject(void *object)
{
	removeNode(object);
}

/**
 * ProjectTreeDelegate
 */

ProjectTreeDelegate::ProjectTreeDelegate(QObject *parent)
	: TreeViewItemDelegate(parent)
{

}

QWidget *ProjectTreeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
	QLineEdit *editor = new LineEdit(parent);
	editor->setObjectName("editor");
	editor->setFrame(false);

	return editor;
}

void ProjectTreeDelegate::setModelData(QWidget *editor, QAbstractItemModel * /*model*/, const QModelIndex &index) const
{
	ProjectNode *node = static_cast<ProjectNode *>(index.internalPointer());
	if (!node)
		return;

	auto le = qobject_cast<QLineEdit *>(editor);
	assert(le);
	if (!le)
		return;

	QString text = le->text();

	switch (node->type()) {
	case NODE_FOLDER:
		static_cast<Folder *>(node->data())->set_name(text.toUtf8().constData());
		break;
#ifdef ULTIMATE
	case NODE_LICENSE:
		static_cast<License *>(node->data())->set_customer_name(text.toUtf8().constData());
		break;
	case NODE_FILE_FOLDER:
		static_cast<FileFolder *>(node->data())->set_name(text.toUtf8().constData());
		break;
	case NODE_FILE:
		static_cast<InternalFile *>(node->data())->set_name(text.toUtf8().constData());
		break;
#endif
	case NODE_WATERMARK:
		static_cast<Watermark *>(node->data())->set_name(text.toUtf8().constData());
		break;
	}
}

/**
 * WatermarksTreeDelegate
 */

WatermarksTreeDelegate::WatermarksTreeDelegate(QObject *parent)
	: TreeViewItemDelegate(parent)
{

}

QWidget *WatermarksTreeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
	QLineEdit *editor = new LineEdit(parent);
	editor->setObjectName("editor");
	editor->setFrame(false);

	return editor;
}

void WatermarksTreeDelegate::setModelData(QWidget *editor, QAbstractItemModel * /*model*/, const QModelIndex &index) const
{
	ProjectNode *node = static_cast<ProjectNode *>(index.internalPointer());
	if (!node)
		return;

	QString text = qobject_cast<QLineEdit *>(editor)->text();

	switch (node->type()) {
	case NODE_WATERMARK:
		static_cast<Watermark *>(node->data())->set_name(text.toUtf8().constData());
		break;
	}
}

/**
 * TemplatesTreeDelegate
 */

TemplatesTreeDelegate::TemplatesTreeDelegate(QObject *parent)
	: TreeViewItemDelegate(parent)
{

}

QWidget *TemplatesTreeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
	QLineEdit *editor = new LineEdit(parent);
	editor->setObjectName("editor");
	editor->setFrame(false);

	return editor;
}

void TemplatesTreeDelegate::setModelData(QWidget *editor, QAbstractItemModel * /*model*/, const QModelIndex &index) const
{
	ProjectNode *node = static_cast<ProjectNode *>(index.internalPointer());
	if (!node)
		return;

	QString text = qobject_cast<QLineEdit *>(editor)->text();
	if (text.isEmpty())
	{
		//FIXME i18n
		MessageDialog::warning(editor->parentWidget(), "Please provide meaningful name", QMessageBox::Ok);
		return;
	}
	switch (node->type()) {
	case NODE_TEMPLATE:
		static_cast<ProjectTemplate *>(node->data())->set_name(text.toUtf8().constData());
		break;
	}
}

/**
 * DumpModel
 */

DumpModel::DumpModel(QObject *parent)
	: QAbstractItemModel(parent), file_(NULL), rowCountCache_(0), cacheAddress_(0)
{

}

QModelIndex DumpModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	return createIndex(row, column);
}

QModelIndex DumpModel::parent(const QModelIndex & /*index*/) const
{
	return QModelIndex();
}

int DumpModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	if(rowCountCache_ == 0) {
		QMapIterator<uint64_t, int> i(addrsToRows_);
		while (i.hasNext()) {
			i.next();
			rowCountCache_ += i.value();
		}
	}
	return rowCountCache_;
}

int DumpModel::columnCount(const QModelIndex & /*parent*/) const
{
	return 3;
}

void DumpModel::setFile(IArchitecture *file) 
{
	if (file_ == file)
		return;

	beginResetModel();
	file_ = file;
	rowCountCache_ = 0;
	addrsToRows_.clear();
	cacheAddress_ = 0;
	cache_.clear();

	if (file_) {
		size_t i;
		ISection *segment;
		std::vector<ISection *> segmentList;
		std::string formatName = file->owner()->format_name();

		if (formatName == "PE") {
			segment = file->segment_list()->GetSectionByAddress(file->image_base());
			if (segment)
				segmentList.push_back(segment);
		}

		for (i = 0; i < file->segment_list()->count(); i++) {
			segment = file->segment_list()->item(i);
			if (segment->memory_type() == mtNone || segment->size() == 0)
				continue;

			if (formatName == "ELF" && segment->name() != "PT_LOAD")
				continue;

			segmentList.push_back(segment);
		}

		uint64_t curAddr;
		int curRows = 0;
		for (i = 0; i < segmentList.size(); i++) {
			segment = segment = segmentList[i];
			uint64_t address = segment->address();
			uint32_t rows = (uint32_t)(segment->size() + 15) / 16;

			if(curRows == 0) {
				curAddr = address;
				curRows = rows;
				continue;
			} 
			uint64_t curEndAddr = curAddr + 16 * curRows;
			if(curEndAddr < address) { //gap
				addrsToRows_.insert(curAddr, curRows);
				curAddr = address;
				curRows = rows;
				continue;
			}
			uint64_t nextEndAddr = address + 16 * rows;
			if(nextEndAddr > curEndAddr) curRows = (nextEndAddr - curAddr) / 16;
		}
		if (curRows) addrsToRows_.insert(curAddr, curRows);
	}
	endResetModel();
}

QByteArray DumpModel::read(uint64_t address, int size) const
{
	if (!cacheAddress_ || address < cacheAddress_ || address + size > cacheAddress_ + cache_.size()) {
		if (file_->AddressSeek(address)) {
			ISection *segment = file_->selected_segment();
			cacheAddress_ = address;
			int cacheSize = (int)qMin(segment->physical_size(), (uint32_t)segment->size()) - (address - segment->address());
			if (cacheSize > 0x100000 && cacheSize > size) // 1Mbyte read ahead limit
				cacheSize = 0x100000;
			if (cacheSize) {
				cache_.resize(cacheSize);
				file_->Read(cache_.data(), cacheSize);
			} else {
				cache_.clear();
			}
		} else {
			return QByteArray();
		}
	}

	uint64_t offset = address - cacheAddress_;
	return cache_.mid(offset, qMin(cache_.size() - (int)offset, size));
}

QVariant DumpModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role == Qt::DisplayRole) {
		uint64_t address = indexToAddress(index);
		switch (index.column()) {
		case 0:
			{
				QString res;
				ISection *segment = file_->segment_list()->GetSectionByAddress(address);
				if (segment)
					res = QString::fromLatin1(segment->name().c_str()).append(':');
				res.append(QString::fromUtf8(DisplayValue(file_->cpu_address_size(), address).c_str()));
				return res;
			}
			
		case 1:
			{
				QByteArray data = read(address, 0x10);
				unsigned char res[16*3] = "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ??";
				static char hex[] = "0123456789ABCDEF";
				for (int i = 0; i < 16; i++) {
					int value = -1;
					if (i < data.size()) {
						value = (unsigned char)data[i];
					} else {
						data = read(address + i, 1);
						if(data.size()) {
							value = (unsigned char)data[0];
						}
					}
					if (value >= 0) {
						res[3 * i] = hex[value >> 4];
						res[3 * i + 1] = hex[value & 0x0f];
					}
				}
				return QString((char *)res);
		}

		case 2:
			{
				QByteArray data = read(address, 0x10);
				char res[17], *ptr = res;
				for (int i = 0; i < 16; i++) {
					if (i < data.size()) {
						unsigned char value = data[i];
						if (value < 32 || value > 127) value = 0xB7;
						*ptr++ = value;
					} else {
						data = read(address + i, 1);
						if (data.size()) {
							unsigned char value = data[0];
							if (value < 32 || value > 127) value = 0xB7;
							*ptr++ = value;
						}
						else {
							*ptr++ = '?';
						}
					}
				}
				*ptr = 0;
				return QString::fromLatin1((char *)res);
			}
		}
	} else if (role == Qt::FontRole) {
		QFont font(MONOSPACE_FONT_FAMILY);
		return font;
	}

	return QVariant();
}

QVariant DumpModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return QString::fromUtf8(language[lsAddress].c_str());
		case 1:
			return QString::fromUtf8(language[lsDump].c_str());
		case 2:
			return QString::fromUtf8(language[lsValue].c_str());
		}
	}
	return QVariant();
}

QModelIndex DumpModel::addressToIndex(uint64_t address)
{
	QMapIterator<uint64_t, int> i(addrsToRows_);
	int rows = 0;
	while (i.hasNext()) {
		i.next();
		if(address >= i.key() && i.value() > (int)((address - i.key()) / 16))
			return createIndex(rows + (int)((address - i.key()) / 16), 0);
		rows += i.value();
	}
	return QModelIndex();
}

uint64_t DumpModel::indexToAddress(const QModelIndex &index) const
{
	QMapIterator<uint64_t, int> i(addrsToRows_);
	int rows = 0;
	while (i.hasNext()) {
		i.next();
		if (rows + i.value() > index.row())
			return i.key() + (index.row() - rows) * 16;
		rows += i.value();
	}
	assert(0);
	return 0;
}

/**
 * DisasmModel
 */

DisasmModel::DisasmModel(QObject *parent)
	: QAbstractItemModel(parent), file_(NULL), rowCountCache_(0), func_(NULL)
{

}

QModelIndex DisasmModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	return createIndex(row, column);
}

QModelIndex DisasmModel::parent(const QModelIndex & /*index*/) const
{
	return QModelIndex();
}

int DisasmModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	if (rowCountCache_ == 0) {
		QMapIterator<uint64_t, int> i(addrsToRows_);
		while (i.hasNext()) {
			i.next();
			rowCountCache_ += i.value();
		}
	}
	return rowCountCache_;
}

int DisasmModel::columnCount(const QModelIndex & /*parent*/) const
{
	return 3;
}

QModelIndex DisasmModel::addressToIndex(uint64_t address)
{
	int row = addressOffset(address);
	if (row >= 0)
	{
		QModelIndex index = createIndex(row, 0);
		if (func_->count()) {
			QModelIndex end = createIndex(index.row() + (int)func_->count(), 2);
			func_->clear();
			dataChanged(index, end);
		}
		return index;
	}
	return QModelIndex();
}

int DisasmModel::addressOffset(uint64_t address) const
{
	QMapIterator<uint64_t, int> i(addrsToRows_);
	int ret = 0;
	while (i.hasNext()) {
		i.next();
		if (address >= i.key() && i.value() > (int)(address - i.key()))
		{
			return ret + (int)(address - i.key());
		}
		ret += i.value();
	}
	return -1;
}

uint64_t DisasmModel::offsetToAddress(int offset) const
{
	QMapIterator<uint64_t, int> i(addrsToRows_);
	while (i.hasNext()) {
		i.next();
		if (i.value() > offset)
		{
			return i.key() + offset;
		}
		offset -= i.value();
	}
	return 0;
}

ICommand *DisasmModel::indexToCommand(const QModelIndex &index) const
{
	if (func_->count()) {
		uint64_t address = func_->item(0)->address();
		int row = addressOffset(address);
		if (row >= 0 && row <= index.row()) {
			size_t commandIndex = index.row() - row;
			if (commandIndex < func_->count())
				return func_->item(commandIndex);
		}
	}
	return NULL;
}

void DisasmModel::setFile(IArchitecture *file) 
{
	if (file_ == file)
		return;

	beginResetModel();
	file_ = file;
	rowCountCache_ = 0;
	addrsToRows_.clear();
	delete func_;
	func_ = NULL;

	if (file_) {
		size_t i;
		ISection *segment;
		std::vector<ISection *> segmentList;
		std::string formatName = file->owner()->format_name();

		if (formatName == "PE" && file->AddressSeek(file->image_base()))
			segmentList.push_back(file->selected_segment());

		for (i = 0; i < file->segment_list()->count(); i++) {
			segment = file->segment_list()->item(i);
			if (segment->memory_type() == mtNone || segment->size() == 0)
				continue;

			if (formatName == "ELF" && segment->name() != "PT_LOAD")
				continue;

			segmentList.push_back(segment);
		}

		uint64_t curAddr;
		int curRows = 0;
		for (i = 0; i < segmentList.size(); i++) {
			segment = segment = segmentList[i];
			uint64_t address = segment->address();
			uint32_t rows = (uint32_t)segment->size();

			if (curRows == 0) {
				curAddr = address;
				curRows = rows;
				continue;
			}
			uint64_t curEndAddr = curAddr + curRows;
			if (curEndAddr < address) { //gap
				addrsToRows_.insert(curAddr, curRows);
				curAddr = address;
				curRows = rows;
				continue;
			}
			uint64_t nextEndAddr = address + rows;
			if (nextEndAddr > curEndAddr) curRows = nextEndAddr - curAddr;
		}
		if (curRows) addrsToRows_.insert(curAddr, curRows);
		func_ = file->function_list()->CreateFunction(file->cpu_address_size());
	}
	endResetModel();
}

QVariant DisasmModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	switch (role) {
	case Qt::DisplayRole:
	case Qt::Vmp::StaticTextRole:
	case Qt::Vmp::StaticColorRole: 
		{
			if (func_->count() > 100000)
				func_->clear();

			uint64_t address = offsetToAddress(index.row());
			ICommand *command = NULL;
			if (func_->count()) {
				int startIndex = addressOffset(func_->item(0)->address());
				if (startIndex >= 0 && startIndex <= index.row()) {
					size_t commandIndex = index.row() - startIndex;
					if (commandIndex < func_->count())
						command = func_->item(commandIndex);
					else if (commandIndex == func_->count())
						address = func_->item(func_->count() - 1)->next_address();
					else
						func_->clear();
				} else {
					func_->clear();
				}
			}

			if (!command)
				command = func_->ParseCommand(*file_, address, true);

			if (role == Qt::DisplayRole) {
				switch (index.column()) {
				case 0: 
					{
						QString res;
						ISection *segment = file_->segment_list()->GetSectionByAddress(address - address % file_->segment_alignment());
						if (segment)
							res = QString::fromLatin1(segment->name().c_str()).append(':');
						return res.append(QString::fromLatin1(command->display_address().c_str()));
					}
				case 1:
					return QString::fromLatin1(command->dump_str().c_str());
				case 2:
					return QString::fromUtf8(command->text().c_str());
				}
			} else if (role == Qt::Vmp::StaticTextRole) {
				if (index.column() == 2 && !command->comment().value.empty())
					return QString::fromUtf8(command->comment().display_value().c_str());
			} else if (role == Qt::Vmp::StaticColorRole) {
				if (index.column() == 2 && !command->comment().value.empty()) {
					switch (command->comment().type) {
					case ttFunction: case ttJmp:
						return QColor(Qt::blue);
					case ttImport:
						return QColor(Qt::darkRed);
					case ttExport:
						return QColor(Qt::red);
					case ttString:
						return QColor(Qt::darkGreen);
					case ttVariable:
						return QColor(Qt::magenta);
					case ttComment:
						return QColor(Qt::gray);
					default:
						return QColor();
					}
				}
			}
		}
		break;
	case Qt::FontRole: {
			return QFont(MONOSPACE_FONT_FAMILY);
		}
	}

	return QVariant();
}

QVariant DisasmModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return QString::fromUtf8(language[lsAddress].c_str());
		case 1:
			return QString::fromUtf8(language[lsDump].c_str());
		case 2:
			return QString::fromUtf8(language[lsCode].c_str());
		}
	}
	return QVariant();
}

/**
 * ProjectNode
 */

bool ItemModelSearcher::extractMatchingIndexes(const QModelIndex &parent)
{
	//TODO: возможны дальнейшие оптимизации
	if (current_match_ == NULL)
		current_match_ = &match_before_;

	int rows = where_->rowCount(parent);
	for(int i = 0; i < rows; ++i)
	{
		QModelIndex idx0;
		for(int col = 0; col < where_->columnCount(parent); col++)
		{
			QModelIndex idx = where_->index(i, col, parent);
			if(idx.isValid())
			{
				if(col == 0)
					idx0 = idx;
				if(from_ == idx)
				{
					assert(current_match_ == &match_before_);
					if (match_before_.isValid() && !forward_)
						return true;
					if (!forward_ && !incremental_ && !match_before_.isValid())
						return false;
					current_match_ = &match_after_;
				} else
				{
					bool matched = (idx.data(Qt::DisplayRole).toString() + idx.data(Qt::Vmp::StaticTextRole).toString()).contains(*term_);
					if (matched)
					{
						*current_match_ = idx;
						if (incremental_ && current_match_ == &match_before_ && from_.isValid() && !match_after_.isValid())
							match_after_ = idx; //store wrapped result
						if (from_.isValid() && current_match_ == &match_after_ && forward_)
							return true;
						if (!from_.isValid() && forward_)
							return true;
					}
				}
			}
		}
		if (extractMatchingIndexes(idx0))
			return true;
	}
	return false;
}

QModelIndex ItemModelSearcher::find()
{
	assert(incremental_ == false || forward_ == true); //other modes unsupported
	if(incremental_ == false || forward_ == true)
	{
		bool from_matched = from_.isValid() && (from_.data(Qt::DisplayRole).toString() + from_.data(Qt::Vmp::StaticTextRole).toString()).contains(*term_);
		if(from_matched && incremental_)
			return from_;

		if (extractMatchingIndexes(QModelIndex()))
		{
			assert(current_match_ && current_match_->isValid());
			return *current_match_;
		}
		// 1. from_ invalid backward
		if (!from_.isValid() && !forward_ && match_before_.isValid())
		{
			return match_before_;
		}

		// 2. from_ valid incremental_ wrapped
		if (from_.isValid() && incremental_ && match_after_.isValid())
		{
			return match_after_;
		}
	}
	return QModelIndex();
}
