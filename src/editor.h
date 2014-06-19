#ifndef EDITOR_H
#define EDITOR_H

//#include <QsciScintilla>
#include <qwidget.h>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qsciapis.h>
#include <QFileInfo>
#include <QTextStream>
#include <QApplication>
#include <QColor>

class Editor : public QsciScintilla
{
	Q_OBJECT

public:
	Editor(QWidget *parent);
	~Editor();
	void SetFileName(QString filename);
	QString GetFileName(void);
	void setEditorStyle (void);

public slots:
	void onCursorPositionChanged(int line, int index);
	
private:
	QsciAPIs *api;
    QsciLexerCPP *lexer;
	QString file;

	void setLexerStyle (int style, QColor foreground, QColor background, bool bold = false, bool italic = false, bool underline = false);
};

#endif // EDITOR_H
