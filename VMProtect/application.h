#ifndef APPLICATION_H
#define APPLICATION_H

class Application : public QApplication
{
	Q_OBJECT
public:
	Application(int &argc, char **argv);
	bool isHelpMode() { return help_filename_.length() > 0; }
	const QString &helpFileName() const { return help_filename_; }
	static double stylesheetScaleFactor() { return stylesheetScaleFactor_; }
	static double devicePixelRatio() { return devicePixelRatio_; }

	~Application();

signals:
	void fileOpenEvent(const QString &);

protected:
	/*virtual*/ bool event(QEvent *event);

	void installStylesheet();
	QByteArray transformStylesheet(const QByteArray &qssData);
	void initScalingConstants();
	static double stylesheetScaleFactor_, devicePixelRatio_;

	QString help_filename_;
};

#endif