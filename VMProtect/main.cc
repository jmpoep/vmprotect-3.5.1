#include "../core/objects.h"
#include "../core/osutils.h"
#include "application.h"
#include "moc/moc_application.cc"
#include "mainwindow.h"
#include "help_browser.h"

#ifndef VMP_GNU
#ifndef _DEBUG
#include <QtCore\QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#endif
#endif

class PatchedProxyStyle : public QProxyStyle
{
public:
	PatchedProxyStyle() : QProxyStyle("windows") {}

	int styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *widget,
		QStyleHintReturn *returnData) const
	{
		int ret = 0;
		switch (hint) 
		{
		case QStyle::SH_MainWindow_SpaceBelowMenuBar:
			ret = 0;
			break;
		default:
			ret = QProxyStyle::styleHint(hint, opt, widget, returnData);
			break;
		}
		return ret;
	}

	void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
	{
		if(element == QStyle::PE_FrameFocusRect) 
		{
			if (dynamic_cast<const QTabBar *>(widget))
			{
				// compensate 'const int OFFSET = 1 + pixelMetric(PM_DefaultFrameWidth)' at qcommonstyle.cpp:1891
				const_cast<QStyleOption *>(option)->rect.adjust(-8, -3, 8, 3);
			}
		}
		QProxyStyle::drawPrimitive(element, option, painter, widget);
	}
};

/*
 Application
 */

Application::Application(int &argc, char **argv) 
	: QApplication(argc, argv) 
{
	installStylesheet();

	QStringList arguments = QCoreApplication::arguments();
	if (arguments.length() >= 3 && 0 == arguments.at(1).compare("--help"))
	{
		help_filename_ = arguments.at(2);
	}
#ifdef VMP_GNU
	if (isHelpMode()) {
		QIcon icon(":/images/help_icon.png");
#ifdef __APPLE__
		void qt_mac_set_app_icon(const QIcon &);
		qt_mac_set_app_icon(icon);
#endif
		setWindowIcon(icon);
	}
#ifdef __unix__
	else
	{
		QIcon icon(":/images/logo.png");
		setWindowIcon(icon);
	}
#endif
#else
	HICON icon = static_cast<HICON>(LoadImageA(GetModuleHandle(NULL), 
		MAKEINTRESOURCEA(isHelpMode() ? 3 : 1),
		IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT));
	setWindowIcon(QtWin::fromHICON(icon));
	::DestroyIcon(icon);
#endif
}

double Application::stylesheetScaleFactor_ = 96.0 / 72.0, Application::devicePixelRatio_ = 1.0;
void Application::initScalingConstants()
{
	double dpi = 96.0;
	QScreen *srn = QApplication::primaryScreen();
	if (srn)
		dpi = (double)srn->logicalDotsPerInch();

	devicePixelRatio_ = 
#ifdef __APPLE__
		1.4 *  // проверял на виртуалке (RENDER_DPI для мака, говорят, примерно во столько раз меньше), но подозреваю, что на макбуке с ретиной получим сюрприз
#endif
		devicePixelRatio();

	double dprOverride = QString(qgetenv("VMP_PIXEL_RATIO")).toDouble();
	if (dprOverride != 0.0)
	{
		devicePixelRatio_ = dprOverride;
	}
	stylesheetScaleFactor_ = dpi * devicePixelRatio_ / 72.0; // 'point' to pixel
}

Application::~Application()
{
	QFile t(os::CombineThisAppDataDirectory("styles.qss.tmp").c_str());
	if (t.exists())
		t.remove();
}

bool Application::event(QEvent *event)
{
	switch (event->type()) {
	case QEvent::FileOpen:
		if (isHelpMode() == false) 
		{
			emit fileOpenEvent(static_cast<QFileOpenEvent *>(event)->file());
			return true;
		}
	default:
		return QApplication::event(event);
	}

}

void Application::installStylesheet()
{
	initScalingConstants();
	std::string qssFileName = os::CombineThisAppDataDirectory("styles.qss");
	if (!QFile::exists(qssFileName.c_str()))
		qssFileName = ":/styles.qss";

	QFile f(qssFileName.c_str());
	if (f.open(QIODevice::ReadOnly))
	{
		QByteArray qssData = f.readAll();
		QByteArray qssDataTransformed = transformStylesheet(qssData);
		QFile t(os::CombineThisAppDataDirectory("styles.qss.tmp").c_str());
		if (t.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			t.write(qssDataTransformed);
		}
		setStyleSheet(qssDataTransformed);
	}
}

QByteArray Application::transformStylesheet(const QByteArray &qssData)
{
	enum {
		OUTSIDE, INSIDE
	} state = OUTSIDE;
	QByteArray ret;
	ret.reserve(qssData.length() * 2);
	QString current;
	for(int idx = 0; idx < qssData.length(); ++idx)
	{
		char ch = qssData[idx];
		switch(state)
		{
		case OUTSIDE:
			if (ch == '<')
				state = INSIDE;
			else
				ret.append(ch);
			break;
		case INSIDE:
			if (ch == '>')
			{
				double val = stylesheetScaleFactor_ * current.toDouble();
				char buf[30];
				sprintf_s(buf, "%dpx", int(val + 0.5));
				ret.append(buf);
				current.clear();
				state = OUTSIDE;
			} else
			{
				current.append(ch);
			}
			break;
		}
	}
	return ret;
}

int main(int argc, char *argv[])
{
	QString file_name;

#ifdef VMP_GNU
	file_name = QString::fromUtf8(argv[0]);
#else
	wchar_t **argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc)
		file_name = QString::fromWCharArray(argv_w[0]);
	LocalFree(argv_w);
#endif

	QDir dir = QFileInfo(file_name).absoluteDir();
#ifdef __APPLE__
	dir.cdUp();
#endif
	dir.cd("PlugIns");

	//QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setStyle(new PatchedProxyStyle());

	Application app(argc, argv);
	std::auto_ptr<QMainWindow> win;
	if(app.isHelpMode())
	{
		win.reset(new HelpBrowser(app.helpFileName()));
		win->show();
	} else
	{
		win.reset(new MainWindow);
		win->connect(&app, SIGNAL(fileOpenEvent(const QString &)), win.get(), SLOT(loadFile(const QString &)));
		win->showMaximized();
	}
	return app.exec();
}
