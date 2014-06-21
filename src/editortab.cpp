#include "editortab.h"

EditorTab::EditorTab(QWidget *parent)
	: QTabWidget(parent)
{
	this->setTabsClosable(true);
    this->setTabShape(Triangular);

	// Load stylesheet
	QString cssFileName =  qApp->applicationDirPath() + "/config/style_code_editor.css";
	QFile cssFile(cssFileName);
	cssFile.open(QFile::ReadOnly | QFile::Text);
    QTextStream css(&cssFile);
	QString styleText = css.readAll();
	setStyleSheet(styleText);

	connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
}

EditorTab::~EditorTab()
{


}

MM::TabType EditorTab::tabType(int index)
{
	if ( (index < 0) || (index >= count()) ) {
		return MM::undefinedTab;
	}

	QWidget * w = widget(index);
	
	if (widget(index)->windowIconText() == "editor") {
		return MM::codeTab;
	} else {
		return MM::serialTab;
	}
}

int EditorTab::fileIndex(QString filename)
// returns the tab index that holds the requested file
// if no tab holds this file, returns -1
{
	for (int i=0;  i < count(); i++) {
		QWidget * w = widget(i);
		// Check if the widget is a code editor
		if (tabType(i) == MM::codeTab) {		
			Editor * editor = (Editor *)(widget(i));
			if (editor->GetFileName() == filename) {
				return i;
			}
		}		
	}

	return -1;
}

int EditorTab::portIndex(QString port)
// returns the tab index that holds the requested serila port
{
	for (int i=0;  i < count(); i++) {
		QWidget * w = widget(i);
		// Check if the widget is a code editor
		if (tabType(i) == MM::serialTab) {		
			SerialMonitor * monitor = (SerialMonitor *)(widget(i));
			if (monitor->GetPort() == port) {
				return i;
			}
		}		
	}

	return -1;
}

void EditorTab::EnableAllSerialPorts(bool enable)
{
	for (int i=0; i < count(); i++) {
		if (tabType(i) == MM::serialTab) {		
			SerialMonitor * serial = (SerialMonitor *)widget(i);
			if (enable) {
				serial->OpenPort();
			} else {
				serial->ClosePort();				
			}
		}
	}
}

bool EditorTab::openSerialPort(QString port, QString speed)
{	
	int index = portIndex(port);
	if (index >= 0) {
		setCurrentIndex(index);		
	} else {		
		SerialMonitor * monitor = new SerialMonitor(this);
		addTab(monitor, port);
		monitor->OpenPort(port, speed);		
		setCurrentIndex(count() - 1);		
		QApplication::restoreOverrideCursor();
	}

	return true;
}

bool EditorTab::openFile(QString filename, int highlightLine)
{
	// First, check if the file isn't already loaded
	Editor * textEdit = NULL;
	int index = fileIndex(filename);
	if (index >= 0) {
		setCurrentIndex(index);
		textEdit = (Editor *)widget(index);
	} else {		
		// File wasn't already open, So, let's create a new editor and load the file on it
		QFile file(filename);
		if (!file.open(QFile::ReadOnly)) {
			ErrorMessage(tr("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString()));			
			return false;
		}
		QTextStream in(&file);
		QApplication::setOverrideCursor(Qt::WaitCursor);

		QString txt(in.readAll());

		textEdit = new Editor(this);
		textEdit->setText(txt);
		textEdit->SetFileName(filename);
		textEdit->setModified(false);

		connect(textEdit, SIGNAL(textChanged()), this, SLOT(onEditorTextChanged()));
	
		addTab(textEdit, QFileInfo(file).fileName());
		setCurrentIndex(count() - 1);		
		QApplication::restoreOverrideCursor();
	}

	if (highlightLine >= 0) {		
		//textEdit->setCursorPosition(highlightLine, 0);
		textEdit->ensureLineVisible(highlightLine-1);		
		textEdit->markerAdd (highlightLine-1, 0);
		//textEdit->setCaretLineBackgroundColor(QColor(255, 100, 100));
	}

	return true;
}

void EditorTab::onEditorTextChanged(void)
{
	emit codeChanged();
}

void EditorTab::closeTab(int index)
{
	if (tabType(index) == MM::codeTab) {
		Editor * editor = (Editor *)widget(index);
		if (editor->isModified()) {
			if (GetUserConfirmation("File was modified. Do you want to save it?\n" + editor->GetFileName())) {
			/*QMessageBox::StandardButton reply;
			reply = QMessageBox::question(this, "File modified", "File was modified. Do you want to save it?\n" + editor->GetFileName(),
                                QMessageBox::Yes|QMessageBox::No);
			if (reply == QMessageBox::Yes) {*/
				saveFile(index);
			}
		}
	}
	
	// removeTab doesnt delete the widget
	QWidget * w = widget(index);
	this->removeTab(index);
	delete w;

	//delete widget(index);
   // cout << "Index to remove == "  << index << endl;
    //QWidget* tabItem = this->widget(index);
    // Removes the tab at position index from this stack of widgets.
    // The page widget itself is not deleted.
    //this->removeTab(index);
    //delete this->widget(index);
    //delete tabItem; //It does not work, still do not know why...
}

bool EditorTab::saveFile(int index) 
{
	Editor * editor = (Editor *)widget(index);
	QString filename = editor->GetFileName();
	
	// text open in editor is always associated to a filename
	// so wer don't need to ask for filenames before
	// saving those files

	QFile file(filename);
	file.open(QIODevice::WriteOnly);// | QIODevice::Text);
	if (file.error() != 0) {		
		QMessageBox::warning(NULL, tr("Application"),
								tr("Error while saving file %1:\n%2.")
								.arg(filename)
								.arg(file.errorString()));
		return false;
	} else {    		
		QTextStream out(&file); // we will serialize the data into the file
		QString text = editor->text();		
		out << text; 		
		file.close();

		editor->setModified(true);
		return true;
	}
}

bool EditorTab::saveAllFiles(void)
{
	bool ok = true;
	for (int i=0; i < count(); i++) {
		if (tabType(i) != MM::codeTab) {
			continue;
		}
		ok = ok && saveFile(i);		
	}

	return ok;
}

void EditorTab::closeAll(void)
{
	while (count() > 0) {
		closeTab(count()-1);
	}
}