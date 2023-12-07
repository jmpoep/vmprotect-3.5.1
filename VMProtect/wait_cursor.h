#ifndef WAITCURSOR_H
#define WAITCURSOR_H

class WaitCursor
{
public:
	WaitCursor() { QApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); }
	~WaitCursor() { QApplication::restoreOverrideCursor(); }
protected:
private:
};

#endif