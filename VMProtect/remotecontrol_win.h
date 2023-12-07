#ifndef REMOTECONTROL_WIN_H
#define REMOTECONTROL_WIN_H

class StdInListenerWin : public QThread
{
	Q_OBJECT

public:
	StdInListenerWin(QObject *parent);
	~StdInListenerWin();

signals:
	void receivedCommand(const QString &cmd);

private:
	void run();
};

#endif
