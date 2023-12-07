#ifndef FUNCTION_DIALOG_H
#define FUNCTION_DIALOG_H

struct AddressInfo {
	uint64_t address;
	IArchitecture *arch;
};

enum ShowMode {
	smAddFunction,
	smGotoAddress
};

class FunctionDialog : public QDialog
{
    Q_OBJECT
public:
	FunctionDialog(IFile *file, IArchitecture *arch, ShowMode mode, QWidget *parent = NULL);
	QList<AddressInfo> addresses() const { return addresses_; }
	CompilationType compilationType() const;
	uint32_t compilationOptions() const;
private slots:
	void okButtonClicked();
	void functionsTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void functionsFilterChanged();
	void helpClicked();
	void sectionClicked(int index);
private:
	bool checkAddress(const AddressInfo &info);

	QTabWidget *tabBar_;
	IFile *file_;
	ShowMode mode_;
	QLineEdit *addressEdit_;
	SearchLineEdit *functionsFilter_;
	QTreeView *functionsTree_;
	QList<AddressInfo> addresses_;
	EnumProperty *compilationType_;
	BoolProperty *lockToKey_;
	std::auto_ptr<MapFunctionBundleListModel> functionBundleListModel_;
	std::auto_ptr<MapFunctionListModel> functionListModel_;
	IArchitecture *defaultArch_;
};

#endif