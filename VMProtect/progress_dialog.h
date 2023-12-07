#ifndef PROGRESS_DIALOG_H
#define PROGRESS_DIALOG_H

class WaitCursor;

class ProgressDialog : public QDialog
{
    Q_OBJECT
public:
	ProgressDialog(QWidget *parent = NULL);
public slots:
	void startProgress(const QString &caption, unsigned long long max);
	void stepProgress(unsigned long long value);
	void notify(MessageType type, IObject *sender, const QString &message);
	void cancelClicked();
signals:
	void cancel();
protected:
	void closeEvent(QCloseEvent *event);
private:
	std::auto_ptr<WaitCursor> wait_cursor_;
	QProgressBar *progressBar;
	QLabel *label;
};

Q_DECLARE_INTERFACE(ILog, "ILog")

class GUILog : public QObject, ILog
{
	Q_OBJECT
	Q_INTERFACES(ILog)
public:
	GUILog(QObject *parent = 0);
	virtual void Notify(MessageType type, IObject *sender, const std::string &message);
	virtual void StartProgress(const std::string &caption, unsigned long long max);
	virtual void StepProgress(unsigned long long value, bool is_project);
	virtual void EndProgress();
	virtual void set_warnings_as_errors(bool /*value*/) { }
	virtual void set_arch_name(const std::string & /*arch_name*/) { }
	void reset() { is_canceled_ = false; }
public slots:
	void cancel();
signals:
	void notify(MessageType type, IObject *sender, const QString &message);
	void startProgress(const QString &, unsigned long long);
	void stepProgress(unsigned long long);
	void endProgress();
private:
	void checkCanceled();
	bool is_canceled_;
};

#endif