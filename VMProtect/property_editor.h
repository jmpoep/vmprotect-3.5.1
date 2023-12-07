#ifndef PROPERTY_EDITOR_H
#define PROPERTY_EDITOR_H

class Property : public QObject
{
    Q_OBJECT
public:
	Property(Property *parent, const QString &name);
	~Property();
	QString name() const { return name_; }
	void setName(const QString &name);
	virtual QString valueText() const;
	virtual QWidget *createEditor(QWidget *parent);
	Property *parent() const { return parent_; }
	virtual bool hasValue() const { return true; }
	bool readOnly() const { return readOnly_; }
	void setReadOnly(bool value);
	QList<Property *> children() const { return children_; }
	void addChild(Property *child);
	void insertChild(int index, Property *child);
	void removeChild(Property *child);
	void clear();
	int childCount() const { return children_.size(); }
	virtual bool hasStaticText(int column) const { return false; }
	virtual QString staticText(int column) const { return QString(); }
	virtual QColor staticColor(int column) const { return QColor(); }
	QString toolTip() const { return toolTip_; }
	void setToolTip(const QString &value) { toolTip_ = value; }
signals:
	void destroyed(Property *prop);
	void changed(Property *prop);
private:
	Property *parent_;
	QString name_;
	bool readOnly_;
	QList<Property *> children_;
	QString toolTip_;
};

class StringProperty : public Property
{
	Q_OBJECT
public:
	StringProperty(Property *parent, const QString &name, const QString &value);
	virtual QString valueText() const;
	virtual QWidget *createEditor(QWidget *parent);
	QString value() const { return value_; }
	void setValue(const QString &value);
signals:
	void valueChanged(const QString &value);
private slots:
	void editorChanged(const QString &value);
private:
	QString value_;
};

class BoolProperty : public Property
{
	Q_OBJECT
public:
	BoolProperty(Property *parent, const QString &name, bool value);
	virtual QString valueText() const;
	virtual QWidget *createEditor(QWidget *parent);
	bool value() const { return value_; }
	void setValue(bool value);
signals:
	void valueChanged(bool value);
private slots:
	void editorChanged(bool value);
private:
	bool value_;
};

class DateProperty : public Property
{
	Q_OBJECT
public:
	DateProperty(Property *parent, const QString &name, const QDate &value);
	virtual QString valueText() const;
	virtual QWidget *createEditor(QWidget *parent);
	QDate value() const { return value_; }
	void setValue(const QDate &value);
signals:
	void valueChanged(const QDate &value);
private slots:
	void editorChanged(const QDate &value);
private:
	QDate value_;
};

class StringListProperty : public StringProperty
{
public:
	StringListProperty(Property *parent, const QString &name, const QString &value);
	virtual QWidget *createEditor(QWidget *parent);
};

class GroupProperty : public Property
{
	Q_OBJECT
public:
	GroupProperty(Property *parent, const QString &name);
	virtual bool hasValue() const { return false; }
};

class CommandProperty : public Property
{
	Q_OBJECT
public:
	CommandProperty(Property *parent, ICommand *value);
	virtual QString valueText() const;
	ICommand *value() const { return value_; }
	virtual bool hasStaticText(int column) const;
	virtual QString staticText(int column) const;
	virtual QColor staticColor(int column) const;
private:
	ICommand *value_;
	QString text_;
};

class EnumProperty : public Property
{
	Q_OBJECT
public:
	EnumProperty(Property *parent, const QString &name, const QStringList &items, int value);
	int value() const { return value_; }
	void setValue(int value);
	void setItems(const QStringList &items) { items_ = items; } 
	virtual QString valueText() const;
	virtual QWidget *createEditor(QWidget *parent);
	void setText(int index, const QString &text) { items_[index] = text; } 
signals:
	void valueChanged(int value);
private slots:
	void editorChanged(int value);
private:
	QStringList items_;
	int value_;
};

class CompilationTypeProperty : public Property
{
	Q_OBJECT
public:
	CompilationTypeProperty(Property *parent, const QString &name, CompilationType value);
	CompilationType value() const { return value_; }
	void setValue(CompilationType value);
	virtual QString valueText() const;
	virtual QWidget *createEditor(QWidget *parent);
	void setDefaultValue(CompilationType value) { defaultValue_ = value; } 
	void setText(int index, const QString &text) { items_[index] = text; } 
signals:
	void valueChanged(CompilationType value);
private slots:
	void editorChanged(int value);
private:
	QStringList items_;
	CompilationType value_;
	CompilationType defaultValue_;
};

class FileNameProperty : public Property
{
	Q_OBJECT
public:
	FileNameProperty(Property *parent, const QString &name, const QString &filter, const QString &value, bool saveMode = false);
	QString value() const { return value_; }
	void setValue(const QString &value);
	void setRelativePath(const QString &relativePath) { relativePath_ = relativePath; }
	virtual QString valueText() const;
	virtual QWidget *createEditor(QWidget *parent);
	void setFilter(const QString &filter) { filter_ = filter; }
signals:
	void valueChanged(const QString &value);
private slots:
	void editorChanged(const QString &value);
private:
	QString filter_;
	QString value_;
	QString relativePath_;
	bool saveMode_;
};

class WatermarkProperty : public Property
{
	Q_OBJECT
public:
	WatermarkProperty(Property *parent, const QString &name, const QString &value);
	QString value() const { return value_; }
	void setValue(const QString &value);
	virtual QString valueText() const;
	virtual QWidget *createEditor(QWidget *parent);
signals:
	void valueChanged(const QString &value);
private slots:
	void editorChanged(const QString &value);
private:
	QString value_;
};

class PropertyManager : public QAbstractItemModel
{
	Q_OBJECT
public:
	PropertyManager(QObject *parent = 0);
	~PropertyManager();
	void clear();
	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	BoolProperty *addBoolProperty(Property *parent, const QString &name, bool value);
	StringProperty *addStringProperty(Property *parent, const QString &name, const QString &value);
	GroupProperty *addGroupProperty(Property *parent, const QString &name);
	DateProperty *addDateProperty(Property *parent, const QString &name, const QDate &value);
	StringListProperty *addStringListProperty(Property *parent, const QString &name, const QString &value);
	EnumProperty *addEnumProperty(Property *parent, const QString &name, const QStringList &items, int value);
	FileNameProperty *addFileNameProperty(Property *parent, const QString &name, const QString &filter, const QString &value, bool saveMode = false);
	CommandProperty *addCommandProperty(Property *parent, ICommand *value);
	WatermarkProperty *addWatermarkProperty(Property *parent, const QString &name, const QString &value);
	CompilationTypeProperty *addCompilationTypeProperty(Property *parent, const QString &name, CompilationType value);
	QModelIndex propertyToIndex(Property *prop) const;
	Property *indexToProperty(const QModelIndex &index) const;
	GroupProperty *root() const { return root_; }
private slots:
    void slotPropertyChanged(Property *prop);
    void slotPropertyDestroyed(Property *prop);
private:
	void addProperty(Property *prop);
	GroupProperty *root_;
};

class PropertyEditorDelegate : public TreeViewItemDelegate
{
    Q_OBJECT
public:
	PropertyEditorDelegate(QObject *parent = 0);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const;
private:
	void setEditorData(QWidget * /*widget*/, const QModelIndex &) const;
	Property *indexToProperty(const QModelIndex &index) const;
};

class TreePropertyEditor : public TreeView
{
    Q_OBJECT
public:
	TreePropertyEditor(QWidget *parent = NULL);
protected:
	void keyPressEvent(QKeyEvent *event);
	void mousePressEvent(QMouseEvent *event);
	Property *indexToProperty(const QModelIndex &index) const;
protected slots:
	virtual void dataChanged (const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> &roles = QVector<int>());
};

class License;
class InternalFile;

class LicensePropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	LicensePropertyManager(QObject *parent = 0);
	License *value() const { return value_; }
	void setValue(License *value);
	void update();
	virtual QVariant data(const QModelIndex &index, int role) const;
	void localize();
	StringProperty *serialNumber() const { return licenseSerialNumber_; }
private slots:
	void nameChanged(const QString &value);
	void emailChanged(const QString &value);
	void orderChanged(const QString &value);
	void dateChanged(const QDate &value);
	void commentsChanged(const QString &value);
	void blockedChanged(bool value);
private:
	License *value_;
	bool lock_;

	GroupProperty *details_;
	StringProperty *licenseName_;
	StringProperty *licenseEmail_;
	DateProperty *licenseDate_;
	StringProperty *licenseOrder_;
	StringListProperty *licenseComments_;
	StringProperty *licenseSerialNumber_;
	BoolProperty *licenseBlocked_;

	GroupProperty *contents_;
	StringProperty *serialName_;
	StringProperty *serialEmail_;
	StringProperty *serialHWID_;
	StringProperty *serialTimeLimit_;
	DateProperty *serialExpirationDate_;
	DateProperty *serialMaxBuildDate_;
	StringProperty *serialUserData_;
};

class InternalFilePropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	InternalFilePropertyManager(QObject *parent = 0);
	InternalFile *value() const { return value_; }
	void setValue(InternalFile *value);
	void update();
	void localize();
	QModelIndex fileNameIndex() const { return propertyToIndex(fileName_); }
private slots:
	void nameChanged(const QString &value);
	void fileNameChanged(const QString &value);
	void actionChanged(int value);
private:
	InternalFile *value_;
	bool lock_;

	GroupProperty *details_;
	StringProperty *name_;
	FileNameProperty *fileName_;
	EnumProperty *action_;
};

class AssemblyPropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	AssemblyPropertyManager(QObject *parent = 0);
	InternalFile *value() const { return value_; }
	void setValue(InternalFile *value);
	void update();
	void localize();
	QModelIndex fileNameIndex() const { return propertyToIndex(fileName_); }
	private slots:
	void nameChanged(const QString &value);
	void fileNameChanged(const QString &value);
private:
	InternalFile *value_;
	bool lock_;

	GroupProperty *details_;
	StringProperty *name_;
	FileNameProperty *fileName_;
};

class FunctionPropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	FunctionPropertyManager(QObject *parent = 0);
	FunctionBundle *value() const { return value_; }
	void setValue(FunctionBundle *value);
	void update();
	void localize();
	void addFunction(IFunction *func);
	void updateFunction(IFunction *func);
	bool removeFunction(IFunction *func);
	virtual QVariant data(const QModelIndex &index, int role) const;
	QModelIndex commandToIndex(ICommand *command) const;
	ICommand *indexToCommand(const QModelIndex &index) const;
	IFunction *indexToFunction(const QModelIndex &index) const;
private slots:
	void compilationTypeChanged(CompilationType value);
	void lockToKeyChanged(bool value);
private:
	void createCommands(IFunction *func, Property *code);
	bool internalAddFunction(IFunction *func);
	bool internalRemoveFunction(IFunction *func);
	struct ArchInfo {
		StringProperty *code;
		QMap<IFunction *, StringProperty *> map;
		ArchInfo() : code(NULL) {}
	};
	FunctionBundle *value_;
	bool lock_;

	GroupProperty *protection_;
	CompilationTypeProperty *compilationType_;
	BoolProperty *lockToKey_;
	GroupProperty *details_;
	StringProperty *name_;
	StringProperty *address_;
	StringProperty *type_;
	QMap<IArchitecture *, ArchInfo> map_;
};

class SectionPropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	SectionPropertyManager(QObject *parent = 0);
	ISection *value() const { return value_; }
	void setValue(ISection *value);
	void update();
	void localize();
private:
	ISection *value_;

	GroupProperty *details_;
	StringProperty *name_;
	StringProperty *address_;
	StringProperty *virtual_size_;
	StringProperty *physical_offset_;
	StringProperty *physical_size_;
	StringProperty *flags_;
};

class SegmentPropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	SegmentPropertyManager(QObject *parent = 0);
	ISection *value() const { return value_; }
	void setValue(ISection *value);
	void update();
	void localize();
private slots:
	void excludedFromPackingChanged(bool value);
	void excludedFromMemoryProtectionChanged(bool value);
private:
	ISection *value_;
	bool lock_;

	GroupProperty *options_;
	GroupProperty *details_;
	BoolProperty *excluded_from_packing_;
	BoolProperty *excluded_from_memory_protection_;
	StringProperty *name_;
	StringProperty *address_;
	StringProperty *virtual_size_;
	StringProperty *physical_offset_;
	StringProperty *physical_size_;
	StringProperty *flags_;
	BoolProperty *flag_readable_;
	BoolProperty *flag_writable_;
	BoolProperty *flag_executable_;
	BoolProperty *flag_shared_;
	BoolProperty *flag_discardable_;
	BoolProperty *flag_notpaged_;
};

class ImportPropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	ImportPropertyManager(QObject *parent = 0);
	~ImportPropertyManager();
	virtual QVariant data(const QModelIndex &index, int role) const;
	IImportFunction *value() const { return value_; }
	void setValue(IImportFunction *value);
	void update();
	void localize();
private:
	IImportFunction *value_;

	GroupProperty *details_;
	StringProperty *name_;
	StringProperty *address_;
	StringProperty *references_;
	IFunction *func_;
};

class ExportPropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	ExportPropertyManager(QObject *parent = 0);
	IExport *value() const { return value_; }
	void setValue(IExport *value);
	void update();
	void localize();
private:
	IExport *value_;

	GroupProperty *details_;
	StringProperty *name_;
	StringProperty *forwarded_;
	StringProperty *address_;
};

class ResourcePropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	ResourcePropertyManager(QObject *parent = 0);
	IResource *value() const { return value_; }
	void setValue(IResource *value);
	void update();
	void localize();
private slots:
	void excludedFromPackingChanged(bool value);
private:
	IResource *value_;
	bool lock_;

	GroupProperty *options_;
	BoolProperty *excluded_from_packing_;
	GroupProperty *details_;
	StringProperty *name_;
	StringProperty *address_;
	StringProperty *size_;
};

class LoadCommandPropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	LoadCommandPropertyManager(QObject *parent = 0);
	ILoadCommand *value() const { return value_; }
	void setValue(ILoadCommand *value);
	void update();
	void localize();
private:
	ILoadCommand *value_;

	GroupProperty *details_;
	StringProperty *name_;
	StringProperty *address_;
	StringProperty *size_;
	StringProperty *segment_;
};

class CorePropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	CorePropertyManager(QObject *parent = 0);
	Core *core() const { return core_; }
	void setCore(Core *core);
	void update();
	virtual QVariant data(const QModelIndex &index, int role) const;
	void localize();
#ifndef LITE
	QModelIndex watermarkNameIndex() const { return propertyToIndex(watermarkName_); }
#endif
private slots:
	void memoryProtectionChanged(bool value);
	void importProtectionChanged(bool value);
	void resourceProtectionChanged(bool value);
	void packOutputFileChanged(bool value);
	void watermarkNameChanged(const QString &value);
	void hwidChanged(const QString &value);
	void outputFileChanged(const QString &value);
	void detectionDebuggerChanged(int value);
	void detectioncpVMToolsChanged(bool value);
	void vmSectionNameChanged(const QString &value);
	void stripDebugInfoChanged(bool value);
	void stripRelocationsChanged(bool value);
	void debugModeChanged(bool value);
	void messageDebuggerFoundChanged(const QString &value);
	void messageVMToolsFoundChanged(const QString &value);
	void messageFileCorruptedChanged(const QString &value);
	void messageSerialNumberRequiredChanged(const QString &value);
	void messageHWIDMismatchedChanged(const QString &value);
	void licenseDataFileNameChanged(const QString &value);
	void activationServerChanged(const QString &value);
private:
	Core *core_;
	bool lock_;

	GroupProperty *file_;
	BoolProperty *memoryProtection_;
	BoolProperty *importProtection_;
	BoolProperty *resourceProtection_;
	BoolProperty *packOutputFile_;
#ifndef LITE
	WatermarkProperty *watermarkName_;
#endif
#ifdef ULTIMATE
	StringProperty *hwid_;
#endif
	FileNameProperty *outputFileName_;

	GroupProperty *detection_;
	EnumProperty *detectionDebugger_;
	BoolProperty *detectionVMTools_;

#ifndef LITE
	GroupProperty *messages_;
	StringListProperty *messageDebuggerFound_;
	StringListProperty *messageVMToolsFound_;
	StringListProperty *messageFileCorrupted_;
	StringListProperty *messageSerialNumberRequired_;
	StringListProperty *messageHWIDMismatched_;
#endif

	GroupProperty *additional_;
	StringProperty *vmSectionName_;
	BoolProperty *stripDebugInfo_;
	BoolProperty *stripRelocations_;

#ifdef ULTIMATE
	GroupProperty *licensingParameters_;
	FileNameProperty *licenseDataFileName_;
	StringProperty *keyPairAlgorithm_;
	StringProperty *activationServer_;
#endif
};

class WatermarkPropertyManager : public PropertyManager
{
	Q_OBJECT
public:
	WatermarkPropertyManager(QObject *parent = 0);
	void setWatermark(Watermark *watermark);
	Watermark *value() const { return watermark_; }
	void update();
private slots:
	void nameChanged(const QString &value);
	void blockedChanged(bool value);
private:
	bool lock_;
	StringProperty *name_;
	BoolProperty *blocked_;
	StringProperty *useCount_;
	Watermark *watermark_;
};

class AddressCalculator : public PropertyManager
{
	Q_OBJECT
public:
	AddressCalculator(QObject *parent = 0);
	void setValue(IArchitecture *file);
	void localize();
private slots:
	void addressChanged(const QString &value);
	void offsetChanged(const QString &value);
	void segmentChanged(int value);
private:
	IArchitecture *file_;
	bool lock_;
	StringProperty *address_;
	StringProperty *offset_;
	EnumProperty *segment_;
};

#endif