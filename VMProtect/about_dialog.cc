#include "../core/objects.h"
#include "../core/files.h"
#include "../core/core.h"
#include "../core/lang.h"
#include "widgets.h"
#include "about_dialog.h"
#include "help_browser.h"
#include "moc/moc_about_dialog.cc"

/**
 * AboutDialog
 */

AboutDialog::AboutDialog(QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint)
{
	setWindowTitle(QString::fromUtf8(language[lsAbout].c_str()));

	QAction *helpAction = new QAction(this);
	helpAction->setShortcut(HelpContentsKeySequence());
	connect(helpAction, SIGNAL(triggered()), this, SLOT(help()));

	addAction(helpAction);

	QFrame *top = new QFrame(this);
	top->setObjectName("bootTop");

	QLabel *logo = new QLabel(this);
	logo->setPixmap(QPixmap(":/images/logo.png"));

	QLabel *version = new QLabel(QString::fromLatin1(Core::edition()) + " v " + QString::fromLatin1(Core::version()), this);
	version->setObjectName("version");

	QLabel *build = new QLabel("build " + QString::fromLatin1(Core::build()), this);
	build->setObjectName("build");

	QGridLayout *grid_layout = new QGridLayout();
	grid_layout->setContentsMargins(30, 30, 30, 20);
	grid_layout->setSpacing(0);
	grid_layout->setColumnStretch(1, 1);
	grid_layout->addWidget(logo, 0, 0, 3, 1);
	grid_layout->addWidget(version, 0, 1, Qt::AlignCenter);
	grid_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 1);
	grid_layout->addWidget(build, 2, 1, Qt::AlignRight);
	top->setLayout(grid_layout);

	QFrame *line1 = new QFrame(this);
	line1->setObjectName("bootHSeparator");
	line1->setFixedHeight(1);

	QFrame *center = new QFrame(this);
	center->setObjectName("boot");

	QLabel *registered1 = new QLabel(this);
	registered1->setWordWrap(true);
	QLabel *registered2 = new QLabel(this);
	registered2->setWordWrap(true);

	enum PurchaseMode {
		pmLicense,
		pmSubscriptionPersonal,
		pmSubscriptionCompany
	};

	QPushButton *purchase = NULL;
	PurchaseMode purchaseMode = pmLicense;
#ifdef DEMO
	registered1->setText(QString::fromUtf8(language[lsDemoVersion].c_str()));
	registered1->setObjectName("unregistered");

	purchase = new PushButton(QString::fromUtf8(language[lsPurchaseLicense].c_str()), this);
#else
	VMProtectSerialNumberData data;
	data.nState = SERIAL_STATE_FLAG_INVALID;
	if (VMProtectGetSerialNumberData(&data, sizeof(data)) && data.nState == 0) {
		registered1->setText(QString("%1: <b>%2 [%3], %4</b>").arg(QString::fromUtf8(language[lsRegisteredTo].c_str())).
			arg(QString::fromUtf16((ushort *)data.wUserName)).
			arg(QString::fromUtf16((ushort *)data.wEMail)).
			arg((data.bUserData[0] & 1) ? "Personal License" : "Company License"));
		if (data.dtMaxBuild.wYear) {
			QDate dt = QDate(data.dtMaxBuild.wYear, data.dtMaxBuild.bMonth, data.dtMaxBuild.bDay);
			registered2->setText(QString("%1: <b>%2</b>").arg(QString::fromUtf8(language[lsFreeUpdatesPeriod].c_str())).arg(dt.toString(Qt::SystemLocaleShortDate)));

			if (QDate::currentDate() > dt) {
				purchase = new PushButton(QString::fromUtf8(language[lsPurchaseSubscription].c_str()), this);
				if (data.nUserDataLength)
					purchaseMode = (data.bUserData[0] & 1) ? pmSubscriptionPersonal : pmSubscriptionCompany;
			}
		}
	} else {
		if (data.nState & SERIAL_STATE_FLAG_BLACKLISTED) {
			registered1->setText(QString::fromLatin1(VMProtectDecryptStringA("Your key file is blocked!")));
			// don't show "buy now" button
		} else if (data.nState & SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED) {
			registered1->setText(QString::fromLatin1(VMProtectDecryptStringA("Your key file will not work with this version of VMProtect!\nYou should use older version or buy one more year of updates and technical support.")));
			purchase = new PushButton(QString::fromUtf8(language[lsPurchaseSubscription].c_str()), this);
			if (data.nUserDataLength)
				purchaseMode = (data.bUserData[0] & 1) ? pmSubscriptionPersonal : pmSubscriptionCompany;
		} else {
			registered1->setText(QString::fromUtf8(language[lsUnregisteredVersion].c_str()));
			purchase = new PushButton(QString::fromUtf8(language[lsPurchaseLicense].c_str()), this);
		}
		registered1->setObjectName("unregistered");
	}
#endif

	QBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(5);
	layout->addWidget(registered1);
	layout->addWidget(registered2);
	layout->addStretch(1);
	if (purchase) {
		switch (purchaseMode) {
		case pmSubscriptionPersonal: 
			connect(purchase, SIGNAL(clicked()), this, SLOT(purchaseSubscriptionPersonal()));
			break;
		case pmSubscriptionCompany: 
			connect(purchase, SIGNAL(clicked()), this, SLOT(purchaseSubscriptionCompany()));
			break;
		default:
			connect(purchase, SIGNAL(clicked()), this, SLOT(purchaseLicense()));
			break;
		}
		layout->addWidget(purchase, 0, Qt::AlignCenter);
	}
	center->setLayout(layout);

	QFrame *line2 = new QFrame(this);
	line2->setObjectName("bootHSeparator");
	line2->setFixedHeight(1);

	QLabel *copyright = new QLabel(QString::fromLatin1(Core::copyright()), this);
	copyright->setObjectName("copyright");

	QFrame *bottom = new QFrame(this);
	bottom->setObjectName("bootTop");
	layout = new QHBoxLayout();
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(0);
	layout->addWidget(copyright, 0, Qt::AlignCenter);
	bottom->setLayout(layout);

	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(top);
	layout->addWidget(line1);
	layout->addWidget(center, 1);
	layout->addWidget(line2);
	layout->addWidget(bottom);
	setLayout(layout);

	setMinimumSize(500, 300);
	setMaximumSize(500, 300);
	//auto size: layout->setSizeConstraint(QLayout::SetFixedSize);
}

void AboutDialog::purchaseLicense()
{
	QDesktopServices::openUrl(QUrl(VMProtectDecryptStringA("http://www.vmpsoft.com/purchase/buy-online/")));
}

void AboutDialog::purchaseSubscriptionPersonal()
{
	QDesktopServices::openUrl(QUrl("https://store.payproglobal.com/checkout?products[1][id]=35518"));
}

void AboutDialog::purchaseSubscriptionCompany()
{
	QDesktopServices::openUrl(QUrl("https://store.payproglobal.com/checkout?products[1][id]=35531"));
}

void AboutDialog::help()
{
	HelpBrowser::showTopic("contacts");
}