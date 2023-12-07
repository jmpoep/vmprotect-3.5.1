#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>

class HelpEngineWrapper;
class MainWindow;

class RemoteControl : public QObject
{
	Q_OBJECT

public:
	RemoteControl(QMainWindow *mainWindow);

signals:
	void handleNavigateToKeywordCommand(const QString &arg);

private slots:
	void receivedData();
	void handleCommandString(const QString &cmdString);

private:
	void splitInputString(const QString &input, QString &cmd, QString &arg);
};

#endif
