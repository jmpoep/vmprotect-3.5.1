//#include "../core/objects.h"
#include "remotecontrol.h"
#include "moc/moc_remotecontrol.cc"

#ifndef VMP_GNU
#include "remotecontrol_win.h"
#include "moc/moc_remotecontrol_win.cc"

StdInListenerWin::StdInListenerWin(QObject *parent)
	: QThread(parent)
{
}

StdInListenerWin::~StdInListenerWin()
{
	terminate();
	wait();
}

void StdInListenerWin::run()
{
	bool ok = true;
	char chBuf[4096];
	DWORD dwRead;

	HANDLE hStdin, hStdinDup;

	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE)
		return;

	DuplicateHandle(GetCurrentProcess(), hStdin,
		GetCurrentProcess(), &hStdinDup,
		0, false, DUPLICATE_SAME_ACCESS);

	CloseHandle(hStdin);

	while (ok) {
		ok = ReadFile(hStdinDup, chBuf, sizeof(chBuf), &dwRead, NULL);
		if (ok && dwRead != 0)
			emit receivedCommand(QString::fromLocal8Bit(chBuf, dwRead));
	}
	CloseHandle(hStdinDup);
}
#endif

RemoteControl::RemoteControl(QMainWindow *mainWindow)
	: QObject(mainWindow)
{
#ifndef VMP_GNU
	StdInListenerWin *l = new StdInListenerWin(this);
	connect(l, SIGNAL(receivedCommand(QString)),
		this, SLOT(handleCommandString(QString)));
	l->start();
#else
	QSocketNotifier *notifier = new QSocketNotifier(fileno(stdin),
		QSocketNotifier::Read, this);
	connect(notifier, SIGNAL(activated(int)), this, SLOT(receivedData()));
	notifier->setEnabled(true);
#endif
}

void RemoteControl::receivedData()
{
	QByteArray ba;
	while (true) {
		int c = getc(stdin);
		if (c == EOF || c == 0)
			break;
		ba.append(char(c));
		if (c == '\n')
			break;
	}
	handleCommandString(QString::fromLocal8Bit(ba));
}

void RemoteControl::handleCommandString(const QString &cmdString)
{
	QStringList cmds = cmdString.split(QLatin1Char(';'));
	QStringList::const_iterator it = cmds.constBegin();
	while (it != cmds.constEnd()) {
		QString cmd, arg;
		splitInputString(*it, cmd, arg);

		if (cmd == QLatin1String("navigatetokeyword"))
			emit handleNavigateToKeywordCommand(arg);
		else
			break;

		++it;
	}
}

void RemoteControl::splitInputString(const QString &input, QString &cmd,
									 QString &arg)
{
	QString cmdLine = input.trimmed();
	int i = cmdLine.indexOf(QLatin1Char('#'));
	cmd = cmdLine.left(i);
	arg = cmdLine.mid(i+1);
	cmd = cmd.toLower();
}
