//-----------------------------------------------------------------------------
#ifndef LAUNCHER_H
#define LAUNCHER_H
//-----------------------------------------------------------------------------

#include <QProcess>
#include "message_handler.h"
#include "buildwindow.h"

//-----------------------------------------------------------------------------

class Launcher : public QObject//QProcess
{
	Q_OBJECT

public:
	Launcher(QObject *parent);
	~Launcher();
	bool RunCommand(QString cmd, QStringList args, unsigned int timeOut = 0, BuildWindow * const progress = NULL);
	void SetCancel(bool value);
	//bool GetCancel(void);
	//void SetPercentage(int value);
	//int GetPercentage(void);

public slots:
    void OnReadStandardOutput(void);
	
private:
	int percentage;
	//bool cancel;
	QProcess * proc;
	bool enable;
};

extern Launcher * launcher;

//-----------------------------------------------------------------------------
#endif // LAUNCHER_H
//-----------------------------------------------------------------------------
