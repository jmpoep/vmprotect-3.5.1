#ifndef MODELS_H
#define MODELS_H

#include "widgets.h"

class Folder;
class IFunction;
class License;
class FileFolder;
class InternalFile;
class PropertyManager;

enum NodeType {
	NODE_ROOT,
	NODE_ARCHITECTURE,
	NODE_OPTIONS,
	NODE_SCRIPT,
	NODE_SCRIPT_BOOKMARK,
	NODE_LOAD_COMMANDS,
	NODE_LOAD_COMMAND,
	NODE_SEGMENTS,
	NODE_SEGMENT,
	NODE_SECTION,
	NODE_IMPORTS,
	NODE_IMPORT,
	NODE_IMPORT_FOLDER,
	NODE_IMPORT_FUNCTION,
	NODE_EXPORTS,
	NODE_EXPORT_FOLDER,
	NODE_EXPORT,
	NODE_MAP_FOLDER,
	NODE_MAP_FUNCTION,
	NODE_FUNCTIONS,
	NODE_FOLDER,
	NODE_FUNCTION,
	NODE_RESOURCES,
	NODE_RESOURCE_FOLDER,
	NODE_RESOURCE,
	NODE_LICENSES,
	NODE_LICENSE,
	NODE_FILES,
	NODE_FILE,
	NODE_FILE_FOLDER,
	NODE_COMMAND,
	NODE_WATERMARK,
	NODE_PROPERTY,
	NODE_DUMP,
	NODE_CALC,
	NODE_MESSAGE,
	NODE_WARNING,
	NODE_ERROR,
	NODE_TEMPLATE,
	NODE_ASSEMBLIES,
};

class ProjectNode
{
public:
	ProjectNode(ProjectNode *parent, NodeType type, void *data = NULL);
	~ProjectNode();
	void clear();
	NodeType type() const { return type_; }
	QString text(int column = 0) const;
	bool contains(const QRegExp &filter) const;
	QString path() const;
	void *data() const { return data_; }
	QIcon icon() const { return icon_; }
	ProjectNode *child(int index) const { return children_[index]; };
	int childCount() const { return children_.size(); }
	void setData(void *data) { data_ = data; }
	void setText(const QString &text) { text_ = text; }
	void setIcon(const QIcon &icon) { icon_ = icon; }
	QList<ProjectNode *> children() const { return children_; };
	ProjectNode *parent() const { return parent_; };
	void addChild(ProjectNode *child);
	void insertChild(int index, ProjectNode *child);
	void removeChild(ProjectNode *child);
	void setPropertyManager(PropertyManager *propertyManager);
	ProjectNode *properties() const { return properties_; };
	void localize();
private:
	ProjectNode *parent_;
	void *data_;
	NodeType type_;
	QString text_;
	QIcon icon_;
	QList<ProjectNode *> children_;
	ProjectNode *properties_;

	// no copy ctr or assignment op
	ProjectNode(const ProjectNode &);
	ProjectNode &operator =(const ProjectNode &);
};

class IProjectNodesModel
{
public:
	virtual ProjectNode *indexToNode(const QModelIndex &index) const = 0;
};

class Core;
class BaseModel : public QAbstractItemModel, public IProjectNodesModel
{
	Q_OBJECT
public:
	BaseModel(QObject *parent = 0);
	~BaseModel();
	virtual void setCore(Core *core);
	bool isEmpty() const { return root_->childCount() == 0; }
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	ProjectNode *indexToNode(const QModelIndex &index) const;
	QModelIndex nodeToIndex(ProjectNode *node) const;
	QModelIndex objectToIndex(void *object) const;
	ProjectNode *objectToNode(void *object) const;
	void setObjectNode(void *object, ProjectNode *node);
	ProjectNode *root() const { return root_; }
	Core *core() const { return core_; }
	void localize();
signals:
	void nodeUpdated(ProjectNode *node);
	void nodeRemoved(ProjectNode *node);
	void objectRemoved(void *object);
protected:
	void clear();
	bool removeNode(void *object);
	void updateNode(ProjectNode *node);
	void removeObject(void *object);
	void createFunctionsTree(ProjectNode *root);
private:
	ProjectNode *root_;
	Core *core_;
	QMap<void *, ProjectNode *> objectToNode_;
};

class ProjectModel : public BaseModel
{
    Q_OBJECT
public:
	ProjectModel(QObject *parent = 0);
	~ProjectModel();
	virtual void setCore(Core *core);
	//virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual Qt::DropActions supportedDragActions() const { return Qt::MoveAction; }
	virtual Qt::DropActions supportedDropActions() const { return Qt::MoveAction; }
	virtual Qt::ItemFlags flags(const QModelIndex & index) const;
	virtual QStringList mimeTypes() const;
	virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
	virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
	void addFolder(Folder *folder);
	void updateFolder(Folder *folder);
	void removeFolder(Folder *folder);
	void addFunction(IFunction *func);
	void updateFunction(IFunction *func);
	void removeFunction(IFunction *func);
	void updateScript();
	QModelIndex optionsIndex() const { return nodeToIndex(nodeOptions_); }
#ifndef LITE
	QModelIndex scriptIndex() const { return nodeToIndex(nodeScript_); }
	uptr_t bookmarkNodeToPos(ProjectNode *node) const;
	void updateScriptBookmarks();
#endif
#ifdef ULTIMATE
	QModelIndex licensesIndex() const { return nodeToIndex(nodeLicenses_); }
	void addLicense(License *license);
	void updateLicense(License *license);
	void removeLicense(License *license);
	void addFileFolder(FileFolder *folder);
	void updateFileFolder(FileFolder *folder);
	void removeFileFolder(FileFolder *folder);
	void addFile(InternalFile *file);
	void updateFile(InternalFile *file);
	void removeFile(InternalFile *file);
	void updateFiles();
#endif
	void sortNode(ProjectNode *node, int field);
signals:
	void modified();
private:
	ProjectNode *internalAddFolder(Folder *folder);
	ProjectNode *internalUpdateFolder(Folder *folder);
	ProjectNode *internalAddFunction(IFunction *func);
	ProjectNode *internalUpdateFunction(IFunction *func);
#ifdef ULTIMATE
	ProjectNode *internalAddLicense(License *license);
	ProjectNode *internalUpdateLicense(License *license);
	ProjectNode *internalAddFileFolder(FileFolder *folder);
	ProjectNode *internalUpdateFileFolder(FileFolder *folder);
	ProjectNode *internalAddFile(InternalFile *file);
	ProjectNode *internalUpdateFile(InternalFile *file);
#endif
	ProjectNode *addObject(ProjectNode *parent, NodeType type, void *object);
	ProjectNode *nodeFunctions_;
#ifdef ULTIMATE
	ProjectNode *nodeLicenses_;
	ProjectNode *nodeFiles_;
#endif
#ifndef LITE
	ProjectNode *nodeScript_;
#endif
	ProjectNode *nodeOptions_;
};

class FunctionsModel : public BaseModel
{
public:
	FunctionsModel(QObject *parent = 0);
	virtual void setCore(Core *core);
	void updateFunction(IFunction *func);
	void removeFunction(IFunction *func);
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

class IResource;
class IArchitecture;
class ISection;
class IImport;

class InfoModel : public BaseModel
{
	Q_OBJECT
public:
	InfoModel(QObject *parent = 0);
	virtual void setCore(Core *core);
	void updateResource(IResource *resource);
	void updateSegment(ISection *segment);
	void updateImport(IImport *import);
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QModelIndex dumpIndex(IArchitecture *file) const { return objectToIndex(file); }
signals:
	void modified();
};

class SearchModel : public QAbstractItemModel
{
public:
	SearchModel(QObject *parent = 0);
	void search(ProjectNode *directory, const QString &text, bool protectedFunctionsOnly = false);
	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());
	QModelIndex nodeToIndex(ProjectNode *node) const;
	void updateNode(ProjectNode *node);
	void removeNode(ProjectNode *node);
	void clear();
	ProjectNode *indexToNode(const QModelIndex &index) const;
private:
	QList<ProjectNode *> items_;
};

class DirectoryModel : public QAbstractItemModel, public IProjectNodesModel
{
public:
	DirectoryModel(QObject *parent = 0);
	void setDirectory(ProjectNode *directory);
	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());
	QModelIndex nodeToIndex(ProjectNode *node) const;
	void updateNode(ProjectNode *node);
	void removeNode(ProjectNode *node);
private:
	ProjectNode *indexToNode(const QModelIndex &index) const;
	QList<ProjectNode *> items_;
	ProjectNode *directory_;
};

class IFile;
class MapFunctionBundle;
class MapFunctionBundleListModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	MapFunctionBundleListModel(IFile &file, bool codeOnly, QObject *parent = 0);
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void search(const QString &text);
	virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
private:
	QList<MapFunctionBundle*> mapFunctionList_;
	QList<MapFunctionBundle*> items_;
};

class MapFunction;
class MapFunctionListModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	MapFunctionListModel(IArchitecture &file, bool codeOnly, QObject *parent = 0);
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void search(const QString &text);
	virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
private:
	QList<MapFunction*> mapFunctionList_;
	QList<MapFunction*> items_;
};

class WatermarkManager;
class Watermark;

class WatermarksModel : public BaseModel
{
    Q_OBJECT
public:
	WatermarksModel(QObject *parent = 0);
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual Qt::ItemFlags flags(const QModelIndex & index) const;
	WatermarkManager *manager() const;
	void setCore(Core *core);
	void addWatermark(Watermark *watermark);
	void updateWatermark(Watermark *watermark);
	void removeWatermark(Watermark *watermark);
	QModelIndex indexByName(const QString &watermarkName) const;
private:
	ProjectNode *internalAddWatermark(Watermark *watermark);
	ProjectNode *internalUpdateWatermark(Watermark *watermark);
};

class WatermarkScanModel : public QAbstractItemModel
{
public:
	WatermarkScanModel(QObject *parent = 0);
	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Watermark *indexToWatermark(const QModelIndex &index) const;
	void setWatermarkData(std::map<Watermark *, size_t> data);
	void removeWatermark(Watermark *watermark);
	void clear();
private:
	QMap<Watermark *, size_t> items_;
};

class ProjectTemplate;
class TemplatesModel : public BaseModel
{
	Q_OBJECT
public:
	TemplatesModel(QObject *parent = 0);
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual Qt::ItemFlags flags(const QModelIndex & index) const;
	void setCore(Core *core);
	void addTemplate(ProjectTemplate * pt);
	void updateTemplate(ProjectTemplate *pt);
	void removeTemplate(ProjectTemplate *pt);
private:
	ProjectNode *internalAddTemplate(ProjectTemplate * pt);
	ProjectNode *internalUpdateTemplate(ProjectTemplate * pt);
};

class LogModel : public BaseModel
{
public:
	LogModel(QObject *parent = 0);
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void clear();
	void addMessage(MessageType type, IObject *sender, const QString &text);
	void removeObject(void *object);
};

class ProjectTreeDelegate : public TreeViewItemDelegate
{
	Q_OBJECT
public:
	ProjectTreeDelegate(QObject *parent = 0);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

class WatermarksTreeDelegate : public TreeViewItemDelegate
{
	Q_OBJECT
public:
	WatermarksTreeDelegate(QObject *parent = 0);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

class TemplatesTreeDelegate : public TreeViewItemDelegate
{
	Q_OBJECT
public:
	TemplatesTreeDelegate(QObject *parent = 0);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

class DumpModel : public QAbstractItemModel
{
public:
	DumpModel(QObject *parent = 0);
	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void setFile(IArchitecture *file);
	QModelIndex addressToIndex(uint64_t address);
private:
	QByteArray read(uint64_t address, int size) const;
	uint64_t indexToAddress(const QModelIndex &index) const;

	IArchitecture *file_;
	QMap<uint64_t, int> addrsToRows_;
	mutable int rowCountCache_;
	mutable uint64_t cacheAddress_;
	mutable QByteArray cache_;
};

class ICommand;
class DisasmModel : public QAbstractItemModel
{
public:
	DisasmModel(QObject *parent = 0);
	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void setFile(IArchitecture *file);
	QModelIndex addressToIndex(uint64_t address);
	ICommand *indexToCommand(const QModelIndex &index) const;
	IArchitecture *file() const { return file_; } 
private:
	int addressOffset(uint64_t address) const;
	uint64_t offsetToAddress(int offset) const;
	void funcClear() const;

	IArchitecture *file_;
	mutable int rowCountCache_;
	QMap<uint64_t, int> addrsToRows_;
	IFunction *func_;
};

class ItemModelSearcher
{
public:
	ItemModelSearcher(QAbstractItemModel *where, QModelIndex from, const QRegExp *term, bool forward, bool incremental) : 
		where_(where), from_(from), term_(term), forward_(forward), incremental_(incremental), current_match_(NULL)
	{
		result_ = find();
	}
	QModelIndex result() { return result_; }

private:
	QModelIndex find();
	QModelIndex result_;

	bool extractMatchingIndexes(const QModelIndex &parent);

	QAbstractItemModel *where_;
	const QRegExp *term_;
	const bool forward_, incremental_;
	const QModelIndex from_;

	QModelIndex match_before_, match_after_;
	QModelIndex *current_match_;
};

#endif