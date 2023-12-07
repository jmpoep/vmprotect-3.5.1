#ifndef WATERMARKS_WINDOW_H
#define WATERMARKS_WINDOW_H

class WatermarksWindow : public QDialog
{
    Q_OBJECT
public:
	WatermarksWindow(bool selectMode, QWidget *parent = NULL);
	~WatermarksWindow();
	static void setModel(WatermarksModel *model) { watermarks_model = model; }
	QString watermarkName() const;
	void setWatermarkName(const QString &name);
private slots:
	void addClicked();
	void delClicked();
	void renameClicked();
	void blockClicked();
	void watermarkSearchChanged();
	void watermarkIndexChanged();
	void itemDoubleClicked(const QModelIndex &index);
	void okButtonClicked();
	void scanButtonClicked();
	void watermarkNodeUpdated(ProjectNode *node);
	void watermarkNodeRemoved(ProjectNode *node);
	void fileButtonClicked();
	void processButtonClicked();
	void processEditDropDown();
	void processEditChanged();
	void contextMenuRequested(const QPoint &p);
	void helpClicked();
private:
	std::map<Watermark*, size_t> internalScanWatermarks(IFile *file);
	static WatermarksModel *watermarks_model;
	ProjectNode *currentNode() const;
	Watermark *selectedWatermark() const;
	QTreeView *currentTreeView() const;

	QTabWidget *tabBar;
	QAction *addAct;
	QAction *renameAct;
	QAction *delAct;
	QAction *blockAct;
	QTreeView *watermarkTree;
	QPushButton *okButton;
	QToolButton *addButton;
	SearchLineEdit *watermarkFilter;
	QTreeView *searchTree;
	SearchModel *searchModel;
	WatermarkScanModel *scanModel;
	FileNameEdit *fileNameEdit;
	QStackedWidget *pagePanel;
	QRadioButton *fileButton;
	QRadioButton *processButton;
	QLabel *fileNameLabel;
	QLabel *processLabel;
	QLabel *moduleLabel;
	EnumEdit *processEdit;
	EnumEdit *moduleEdit;
	bool processEditLocked;
#ifndef VMP_GNU
	bool debugPrivilegeEnabled;
#endif
	QMenu *contextMenu;
};

#endif
