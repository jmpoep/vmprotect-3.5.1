#include "../core/objects.h"
#include "../core/files.h"
#include "../core/lang.h"
#include "../core/core.h"
#include "models.h"
#include "widgets.h"
#include "help_browser.h"
#include "message_dialog.h"
#include "templates_window.h"
#include "moc/moc_templates_window.cc"

/**
 * TemplatesWindow
 */

TemplatesModel *TemplatesWindow::templates_model_ = NULL;

TemplatesWindow::TemplatesWindow(QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	setWindowTitle(QString::fromUtf8(language[lsTemplates].c_str()));

	renameAct_ = new QAction(QString::fromUtf8(language[lsRename].c_str()), this);
	renameAct_->setShortcut(QString("F2"));
	renameAct_->setEnabled(false);
	connect(renameAct_, SIGNAL(triggered()), this, SLOT(renameClicked()));
	delAct_ = new QAction(QString::fromUtf8(language[lsDelete].c_str()), this);
	delAct_->setShortcut(QString("Del"));
	delAct_->setEnabled(false);
	connect(delAct_, SIGNAL(triggered()), this, SLOT(delClicked()));

	contextMenu_ = new QMenu(this);
	contextMenu_->addAction(delAct_);
	contextMenu_->addAction(renameAct_);

	templateTree_ = new TreeView(this);
	templateTree_->setObjectName("grid");
	templateTree_->setRootIsDecorated(false);
	templateTree_->setUniformRowHeights(true);
	templateTree_->setIconSize(QSize(18, 18));
	templateTree_->setContextMenuPolicy(Qt::CustomContextMenu);
	templateTree_->setItemDelegate(new TemplatesTreeDelegate(this));
	templateTree_->setModel(templates_model_);
	templateTree_->addAction(delAct_);
	connect(templateTree_->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(templateIndexChanged()));
	connect(templateTree_, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenuRequested(const QPoint &)));

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

	closeButton_ = new PushButton(QString::fromUtf8(language[lsClose].c_str()), this);
	connect(closeButton_, SIGNAL(clicked()), this, SLOT(reject()));

    QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	buttonLayout->setSpacing(10);
	buttonLayout->addWidget(helpButton);
	buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton_);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(10);
	mainLayout->addWidget(templateTree_);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	resize(600, 300);
}

void TemplatesWindow::contextMenuRequested(const QPoint &p)
{
	contextMenu_->exec(templateTree_->viewport()->mapToGlobal(p));
}

void TemplatesWindow::helpClicked()
{
	//FIXME HelpBrowser::showTopic("templates");
}

void TemplatesWindow::templateIndexChanged()
{
	ProjectTemplate *pt = selectedTemplate();
	templateTree_->selectionModel()->select(templateTree_->currentIndex(), QItemSelectionModel::ClearAndSelect);

	delAct_->setEnabled(pt != NULL);
	renameAct_->setEnabled(pt != NULL && templateTree_->currentIndex().row() > 0);
}

void TemplatesWindow::delClicked()
{
	ProjectTemplate *pt = selectedTemplate();
	if (!pt)
		return;

	if (pt->is_default()) {
		if (MessageDialog::warning(this, QString(QString::fromUtf8(language[lsDeleteDefaultTemplate].c_str())), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			pt->Reset();
	} else {
		if (MessageDialog::question(this, QString(QString::fromUtf8(language[lsDeleteTemplate].c_str()) +"?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			delete pt;
	}
}

void TemplatesWindow::renameClicked()
{
	templateTree_->edit(templateTree_->currentIndex());
}

ProjectTemplate *TemplatesWindow::selectedTemplate() const
{
	QModelIndex idx = templateTree_->currentIndex();
	ProjectNode *node = templates_model_->indexToNode(idx);
	return (node && node->type() == NODE_TEMPLATE) ? reinterpret_cast<ProjectTemplate *>(node->data()) : NULL;
}

