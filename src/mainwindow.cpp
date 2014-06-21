#include "mainwindow.h"

//-----------------------------------------------------------------------------
#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);		
	
	//connect ( this, SIGNAL ( ShowErrorSignal ( QString ) ), SLOT ( ShowError ( QString  ) ) );
	
	int res = config.Load();
	
	if (res != 0) {
		QString msg;
		switch (res) {
			case 101: 
			case 102: 
			case 103: 
			case 104: 
			case 201: 
			case 202:
			case 203:
			case 204: msg = "Error reading configuration file 'hardware.xml'"; break;
			default: msg = "Generic error while loadinh the configuration file";
		}
		QMessageBox msgBox;
		msgBox.setText(msg);
		msgBox.exec();
	}

	//ui.outputWindow->setFontFamily("Inconsolata");
	//ui.outputWindow->setFontWeight(18);

	ui.messageTabs->setCurrentIndex(0);
	//SetWorkspace();

	QTimer *timer = new QTimer();
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(100);

    QPalette palette = ui.menuBar->palette();
	palette.setColor(QPalette::Button,  ui.mainToolBar->palette().color(QPalette::Window));
	ui.menuBar->setPalette(palette);

    buildWindow = new BuildWindow();
	
	ui.buildStatus->setCurrentIndex(0);

	setupActions();

	CreateTreeContextMenu();

	// Prepare editor UI
	//ui.editorTabs->clear();  //removes all the previous tabs
	tabsEditor = new EditorTab(this);
	connect(tabsEditor, SIGNAL(codeChanged()), this, SLOT(OnProjectModified()));
	//ui.tabParent->addWidget(tabsEditor);
	ui.splitter->addWidget(tabsEditor);

	// Load stylesheet
	QString cssFileName =  qApp->applicationDirPath() + "/config/style_main.css";
	QFile cssFile(cssFileName);
	cssFile.open(QFile::ReadOnly | QFile::Text);
    QTextStream css(&cssFile);
	QString styleText = css.readAll();
	ui.messageTabs->setStyleSheet(styleText);
	ui.actionSave_Workspace->setData(0);
	if (QDir(config.workspace).exists()){
		OpenWorkspace();	
	}
}

//-----------------------------------------------------------------------------

void MainWindow::CreateTreeContextMenu(void)
{
	projectContext = new QMenu(this);
	ui.tree->setContextMenuPolicy(Qt::CustomContextMenu);
	projectContext->setStyleSheet(ui.menuBar->styleSheet());
	bool ok = connect(ui.tree, SIGNAL(customContextMenuRequested(const QPoint &)),
		this,SLOT(ShowTreeMenu(const QPoint )));

	QAction * action = projectContext->addAction("Set default project");
	connect(action, SIGNAL(triggered()), this, SLOT(SetDefaultProject()));
	projectContext->addAction(action);

	//action = projectContext->addAction("Import library");
	//connect(action, SIGNAL(triggered()), this, SLOT(ImportLibrary()));
	projectContext->addAction(ui.actionAdd_file);	

	action = projectContext->addAction("Properties");
	connect(action, SIGNAL(triggered()), this, SLOT(EditProjectProperties()));
	projectContext->addAction(action);
}

//-----------------------------------------------------------------------------

void MainWindow::ShowTreeMenu(const QPoint point)
{
	QTreeWidgetItem * item = ui.tree->itemAt(point);
	if (item == NULL) {
		return;
	}

	if (item->data(0,255) == WorskspaceTree::Project) {
		projectContext->popup(ui.tree->viewport()->mapToGlobal(point));
	}
}

//-----------------------------------------------------------------------------

void MainWindow::SetDefaultProject(void)
{
	if (ui.tree->selectedItems().count() != 1) {
		return;
	}

	QTreeWidgetItem * item = ui.tree->selectedItems().at(0);

	if (item->data(0,255) != WorskspaceTree::Project) {
		return;
	}

	workspace.SetCurrentProject(item->text(0));
	AdjustWorkspaceTree();
}

//-----------------------------------------------------------------------------

void MainWindow::EditProjectProperties(void)
{
	if (ui.tree->selectedItems().count() != 1) {
		return;
	}

	QTreeWidgetItem * item = ui.tree->selectedItems().at(0);

	//QString text = item->text(0);

	if (item->data(0,255) != WorskspaceTree::Project) {
		return;
	}

	workspace.SetCurrentProject(item->text(0));

	if (workspace.GetCurrentProject() == NULL) {
		return;
	}

	if (workspace.GetCurrentProject()->name == item->text(0)) {
		ProjectProperties * prop = new ProjectProperties();
		prop->Edit(workspace.GetCurrentProject());
		delete prop;
	}
	AdjustWorkspaceTree();
}

//-----------------------------------------------------------------------------

void MainWindow::update(void)
{
	// update the output messages window with all the last builder messages
	if (msg.HasOutput()) {
		QStringList list = msg.GetOutput().split("\n");
		for (int i=0; i < list.count(); i++) {
			QString txt = list.at(i);		
			if (txt == "<<<CLEAR>>>") {
				ui.outputWindow->setText("");
			} else {
				ui.outputWindow->append(txt);
			}
		}
	}

	// update the builder messages window with the new builder error/warning messages
	while (msg.BuildMessagesCount() > 0) {
		//QString text;
		BuildMessageItem * item = new BuildMessageItem;//("", ui.buildMessages);	
		ui.buildMessages->addItem(item);
		item->bm = msg.GetBuildMessage(); 
		item->setToolTip(item->bm.file);
		
		//MMBuildMessage bm = msg.GetBuildMessage(); 
		QString text;
		if (item->bm.type == mtRegular) {
			text = item->bm.text;
		} else {
			if (item->bm.line >= 0) {
				text = QFileInfo(item->bm.file).fileName() + " (" + 
					QString::number(item->bm.line);
				if (item->bm.column >= 0) {
					text += "," + QString::number(item->bm.column) + "): ";
				} else {
					text += "): ";
				}
			}
			text += item->bm.text;
		}
		item->setText(text);
				
		//QListWidgetItem * item = new QListWidgetItem(text, ui.buildMessages);		
		QFont font = item->font();		
		if (item->bm.type == mtError) {
			item->setIcon(QIcon(":/MainWindow/resources/build_messages/build_messages_icon_build_error.png"));
			//item->setForeground(Qt::red);
		} else if (item->bm.type == mtWarning) {
			item->setIcon(QIcon(":/MainWindow/resources/build_messages/build_messages_icon_warning.png"));
			//item->setForeground(Qt::white);
		} else if (item->bm.type == mtSuccess) {
			item->setIcon(QIcon(":/MainWindow/resources/build_messages/build_messages_icon_build_check.png"));		
			font.setBold(true);
			
			//// Turn serialports open again
			if (item->bm.text.indexOf("successfully uploaded to board") > 0) {
				tabsEditor->EnableAllSerialPorts(true);
			}
		} else {
			item->setIcon(QIcon(":/MainWindow/resources/build_messages/build_messages_icon_build_info.png"));
		}
		item->setFont(font);
	}

	qApp->processEvents();
}

//-----------------------------------------------------------------------------

void MainWindow::setupActions()
{
    //Open Action
    ui.actionOpen_file->setShortcut(tr("Ctrl+O"));
    ui.actionOpen_file->setStatusTip(tr("Open an existing file"));
    connect(ui.actionOpen_file, SIGNAL(triggered()), this, SLOT(open()));

    //Save Action

	//Build project Action
	ui.actionBuild_project->setShortcut(tr("Ctrl+B"));
    ui.actionBuild_project->setStatusTip(tr("Build the current project"));
	connect(ui.actionBuild_project, SIGNAL(triggered()), this, SLOT(BuildProject()));

	//Clean current project
	ui.actionClean_current_project->setShortcut(tr("Ctrl+Q"));
    ui.actionClean_current_project->setStatusTip(tr("Erase all files generated on previous builds"));
	connect(ui.actionClean_current_project, SIGNAL(triggered()), this, SLOT(CleanProject()));

	//Upload program Action	
	ui.actionBuild_and_upload_project->setShortcut(tr("Ctrl+U"));
    ui.actionBuild_and_upload_project->setStatusTip(tr("Upload the current project to the board, building it if necessary"));
	connect(ui.actionBuild_and_upload_project, SIGNAL(triggered()), this, SLOT(UploadProgram()));

	// set workspace
	ui.actionSelect_workspace->setShortcut(tr("Ctrl+W"));
    ui.actionSelect_workspace->setStatusTip(tr("Select the workspace path"));
	connect (ui.actionSelect_workspace, SIGNAL(triggered()), this, SLOT(SetWorkspacePath()));

	// open workspace folder
	ui.actionOpen_workspace_folder->setStatusTip(tr("Open the workspace folder"));
	connect (ui.actionOpen_workspace_folder, SIGNAL(triggered()), this, SLOT(OpenWorkspaceFolder()));

	// Add new project
	//ui.actionAdd_a_new_project->setShortcut(tr("Ctrl+W"));
    ui.actionAdd_a_new_project->setStatusTip(tr("Create a new project"));
	connect (ui.actionAdd_a_new_project, SIGNAL(triggered()), this, SLOT(AddNewProject()));

	// Add new file
	//ui.actionAdd_a_new_file->setShortcut(tr("Ctrl+N"));
    ui.actionImport_arduino_library->setStatusTip(tr("Import Arduino library"));
	connect (ui.actionImport_arduino_library, SIGNAL(triggered()), this, SLOT(ImportLibrary()));

	//action = projectContext->addAction("Import library");
	connect(ui.actionAdd_file, SIGNAL(triggered()), this, SLOT(AddNewFileToProject()));
	//projectContext->addAction(ui.actionAdd_a_new_file);	

	// edit properties
	//ui.actionEdit_configuration->setShortcut();
	ui.actionProject_configurations->setStatusTip(tr("Configure the current project"));
	connect (ui.actionProject_configurations, SIGNAL(triggered()), this, SLOT(ConfigureCurrentProject()));

	// Save workspace
	ui.actionSave_Workspace->setShortcut(tr("Ctrl+S"));
    ui.actionSave_Workspace->setStatusTip(tr("Save the current workspace, including all projects and files"));
	connect (ui.actionSave_Workspace, SIGNAL(triggered()), this, SLOT(SaveWorkspace()));

	// build complete
	connect (buildWindow, SIGNAL(buildComplete()), this, SLOT(OnBuildComplete()));

	//mouse click on workspace tree	
	connect (ui.tree, SIGNAL(itemDoubleClicked (QTreeWidgetItem *, int)), 
			this, SLOT (OnTreeDoubleClick(QTreeWidgetItem *, int)));	

	// open serial port
	ui.actionOpen_serial_port->setShortcut(tr("Ctrl+I"));
    ui.actionOpen_serial_port->setStatusTip(tr("Open the serial port for the current project"));
	connect (ui.actionOpen_serial_port, SIGNAL(triggered()), this, SLOT(OpenSerialPort()));


	// double click on build error/warning messages
	connect(ui.buildMessages, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(OnBuildMessagesDoubleClick(QListWidgetItem*)));
	//connect (ui.tree, SIGNAL(itemDoubleClicked (QTreeWidgetItem *, int)), 
			//this, SLOT (OnTreeDoubleClick(QTreeWidgetItem *, int)));	

	
	
}

//-----------------------------------------------------------------------------

void MainWindow::OnTreeDoubleClick (QTreeWidgetItem * item, int column)
{
	// if it's a file, open it
	if ( (item->data(0, 255) != WorskspaceTree::File) && (item->data(0, 255) != WorskspaceTree::ExternalFile) ) {
		return;
	}

	QString filename = item->text(0);
	QString projectName = item->parent()->text(0);
	if (item->data(0, 255) == WorskspaceTree::ExternalFile) {
		projectName = item->parent()->parent()->text(0);
	}

	filename = workspace.GetFullFilePath(projectName, filename);
	
	tabsEditor->openFile(filename);
}


//-----------------------------------------------------------------------------

void MainWindow::OnBuildMessagesDoubleClick(QListWidgetItem* item)
{
	BuildMessageItem * bmItem = (BuildMessageItem *)item;

	QString filename = bmItem->bm.file;

	if (QFileInfo(filename).exists()) {
		tabsEditor->openFile(filename, bmItem->bm.line);
	}
}

//-----------------------------------------------------------------------------
void MainWindow::OpenWorkspaceFolder(void)
{
	// Open the equivalent of "Windows Explorer" on the current operating
	// system, but always pointing to the workspace folder

	QString path = QDir::toNativeSeparators(config.workspace);
	QDesktopServices::openUrl(QUrl("file:///" + path));
}

//-----------------------------------------------------------------------------

void MainWindow::ImportLibrary(void)
{	
	if (workspace.GetCurrentProject() == NULL) {
		return;
	}

	Wizard * wizard = new Wizard();
	bool ok = wizard->ImportLibrary();	
	
	if (ok) {
		QString libName = wizard->GetLibraryName();
		if (libName != "") {
			workspace.ImportLibrary(workspace.GetCurrentProject(), libName);
		}
		AdjustWorkspaceTree();
	}

	delete wizard;	
}


//-----------------------------------------------------------------------------

void MainWindow::AddNewFileToProject(void)
{
	Wizard * wizard = new Wizard();
	bool ok = wizard->NewFile();	
	
	if (ok) {
		QString file = wizard->GetNewFileName();
		tabsEditor->openFile(file);		
		AdjustWorkspaceTree();
	}

	delete wizard;	
}

//-----------------------------------------------------------------------------
	
void MainWindow::AddNewProject(void)
{
	Wizard * wizard = new Wizard();
	bool ok = wizard->NewProject();
	delete wizard;
	if (ok) {
		ProjectProperties * prop = new ProjectProperties();
		prop->Edit(workspace.GetCurrentProject());
		delete prop;
		AdjustWorkspaceTree();
	}	
}

//-----------------------------------------------------------------------------

void MainWindow::ConfigureCurrentProject(void)
{
	ProjectProperties * prop = new ProjectProperties();
	prop->Edit(workspace.GetCurrentProject());
	delete prop;
	AdjustWorkspaceTree();
}

//-----------------------------------------------------------------------------

void MainWindow::OnBuildComplete(void)
{
	ui.buildStatus->setCurrentIndex(builder.GetLastBuildStatus());
	if (builder.GetLastBuildStatus() == 2) {
		int ps, ds, es;
		float pp, dp, ep;
		msg.GetLastSucessfullBuildInfo(ps, pp, ds, dp, es, ep);
		if (ps >= 0) {
			ui.lbProgramSize->setText("Program size: " + QString::number(ps) + " bytes");
			ui.pbProgramSize->setValue(pp);
			ui.pbProgramSize->setVisible(true);
		} else {
			ui.lbProgramSize->setText("Program size: N/A");
			ui.pbProgramSize->setVisible(false);
		}
		if (ds >= 0) {
			ui.lbMemorySize->setText("Memory size: " + QString::number(ds) + " bytes");
			ui.pbMemorySize->setValue(dp);
			ui.pbMemorySize->setVisible(true);
		} else {
			ui.lbMemorySize->setText("Memory size: N/A");
			ui.pbMemorySize->setVisible(false);
		}
		if (es >= 0) {
			ui.lbEEPROMSize->setText("EEPROM size: " + QString::number(es) + " bytes");
			ui.pbEEPROMSize->setValue(ep);
			ui.pbEEPROMSize->setVisible(true);
		} else {
			ui.lbEEPROMSize->setText("EEPROM size: N/A");
			ui.pbEEPROMSize->setVisible(false);
		}
	}
}

//-----------------------------------------------------------------------------

void MainWindow::open()
{
	QString fileName = QFileDialog::getOpenFileName(this);

    if (!fileName.isEmpty())
		loadFile(fileName);
}

//-----------------------------------------------------------------------------

void MainWindow::BuildProject()
{
	SaveWorkspace();
	ui.buildMessages->clear();
	//tabsEditor->EnableAllSerialPorts(false);
	buildWindow->Build(false);
	//tabsEditor->EnableAllSerialPorts(true);
}

//-----------------------------------------------------------------------------

void MainWindow::CleanProject()
{
	ui.buildMessages->clear();
	builder.Clean();
	ui.buildStatus->setCurrentIndex(builder.GetLastBuildStatus());
}

//-----------------------------------------------------------------------------

void MainWindow::UploadProgram()
{
	ui.buildMessages->clear();
	tabsEditor->EnableAllSerialPorts(false);
	buildWindow->Build(true);	
}

//-----------------------------------------------------------------------------

void MainWindow::loadFile(const QString &fileName)
{
    /*QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }*/

	tabsEditor->openFile(fileName);
//	ui.editorTabs->LoadFile(file);

    /*QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QString txt(in.readAll());

    //textEdit = new Editor(txt); // Alex: comentei aqui

    ui.editorTabs->addTab(textEdit, QFileInfo(fileName).fileName());

    cout << ui.editorTabs->count() << endl;

    ui.editorTabs->setCurrentIndex(ui.editorTabs->count() - 1);



    QApplication::restoreOverrideCursor();*/
}

//-----------------------------------------------------------------------------

MainWindow::~MainWindow()
{
	delete buildWindow;
}

//-----------------------------------------------------------------------------

void MainWindow::resizeEvent(QResizeEvent *event)
{
	float w = width();
	float h = height();
	//ui.tree->resize(0.1 * w, ui.tree->height());
	ui.messageTabs->resize(w, 0.1 * h);
	//ui.editorTabs->resize(0.90 * w, h * 9);
	tabsEditor->resize(0.90 * w, h * 9);
	ui.tree->resize(0.20 * w, h * 0.9);
	
}

//-----------------------------------------------------------------------------

void MainWindow::SetWorkspacePath(void)
{
	SetWorkspace * setWorkspace = new SetWorkspace();
	if (setWorkspace->Select()) {
		OpenWorkspace();
	}	
	delete setWorkspace;	
}

//-----------------------------------------------------------------------------

void MainWindow::OpenWorkspace(void)
{
	ui.tree->clear();
	QTreeWidgetItem *root = new QTreeWidgetItem(ui.tree);
	root->setText(0, tr("Workspace"));	
	root->setIcon (0, QIcon(":/MainWindow/resources/workspace_tree/workspace.png"));

	workspace.Open(config.workspace);

	// open all previously open files
	tabsEditor->closeAll();
	for (int p=0; p < workspace.projects.size(); p++) {
		for (int f=0; f < workspace.projects.at(p).files.size(); f++) {
			QString filename = workspace.projects.at(p).files.at(f).name;
			filename = workspace.GetFullFilePath(workspace.projects.at(p).name, filename);
			if (workspace.projects.at(p).files.at(f).open) {
				tabsEditor->openFile(filename);
			}
		}
	}
	AdjustWorkspaceTree();
}

//-----------------------------------------------------------------------------

void MainWindow::AdjustWorkspaceTree(void)
{	
	QTreeWidgetItem *root = ui.tree->topLevelItem(0);
	//root->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	root->setIcon(0, QIcon(":/MainWindow/resources/workspace_tree/workspace.png"));
	root->setData(0, 255, WorskspaceTree::Workspace);
	root->setText(0, "workspace");
	// First, find projects on workspace that are not yet on the tree, and add them.	

	if (workspace.GetCurrentProject() == NULL) {
		return;
	}

        
	for (int pwi=0; pwi < workspace.projects.size(); pwi++) {
		bool foundAtTree = false;
		for (int pti=0; pti < root->childCount(); pti++) {
			QTreeWidgetItem *projNode = root->child(pti);
			if (workspace.projects.at(pwi).name == projNode->text(0)) {
				foundAtTree = true;
				break;
			}
		}
		if (foundAtTree == false) {
			QTreeWidgetItem *projNode = new QTreeWidgetItem(root);
			projNode->setText(0, workspace.projects.at(pwi).name);			
			//QString txt = projNode->text(0);
			//QString txt2 = ui.tree->topLevelItem(0)->child(0)->text(0);
			//int kkk= root->childCount();
			projNode->setIcon(0, QIcon(":/MainWindow/resources/workspace_tree/project.png"));
			projNode->setData(0, 255, WorskspaceTree::Project);
	
			QTreeWidgetItem *extNode = new QTreeWidgetItem(projNode);
			extNode->setText(0, "External files");			
			//txt = extNode->text(0);
			extNode->setIcon(0, QIcon(":/MainWindow/resources/workspace_tree/external.png"));
			extNode->setData(0, 255, WorskspaceTree::ExternalTree);
			//txt = ui.tree->topLevelItem(0)->child(0)->text(0);
	
		}
	}

	//QString txt = ui.tree->topLevelItem(0)->child(0)->text(0);
	
	// Then Update the tree with all projects on workspace
	int pti = 0;
	while (pti < root->childCount()) {
		QTreeWidgetItem *projNode = root->child(pti);		
	//for (int pti=0; pti < root->childCount(); pti++) {		
		bool foundAtWorkspace = false;
		for (int pwi=0; pwi < workspace.projects.size(); pwi++) {
			QString projectName = projNode->text(0);
			//int kkk= root->childCount();
			if (workspace.projects.at(pwi).name == projectName) {
				foundAtWorkspace = true;
				AdjustProjectFilesOnTree(pwi, projNode);				
			}
		}
		// If project no longer exists on workspace, remove it from the tree
		if (foundAtWorkspace == false) {
			delete projNode;
			//ui.tree->removeItemWidget(projNode, 0);
		} else {
			pti++;
		}
	}	

	ui.tree->expandItem(root);
	ui.tree->setRootIsDecorated(false);
	//root->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
	for (int i=0; i < root->childCount(); i++) {
		QTreeWidgetItem *projNode = root->child(i);
		//QFont f = projNode->font(0);
		if (projNode->text(0) == workspace.GetCurrentProject()->name) {			
			//f.setBold(true);
			//f.setWeight(30);
			projNode->setForeground(0, Qt::white);
			//projNode->setCheckState(0, Qt::Checked);
			//projNode->setstyle			
		} else {			
			projNode->setForeground(0, QColor(200,200,200));
			//f.setBold(false);
		}
		//projNode->setFont(0, f);
	}
	int i0 = ui.tree->children().count();
	int i = root->childCount();
	//int ii = root->child(0)->childCount();
	
}

//-----------------------------------------------------------------------------

void MainWindow::AdjustProjectFilesOnTree(int pwi, QTreeWidgetItem * projNode)
{
		
	vector <Project>::iterator project = workspace.projects.begin() + pwi;
	
	// Get the node with the label "External files"
	QTreeWidgetItem * externNode = NULL;
	for (int fti=0; fti < projNode->childCount(); fti++) {			
		if (projNode->child(fti)->text(0) == "External files") {
			externNode = projNode->child(fti);
		}		
	}

	// First, check if all files in this project at workspace exist at the tree. If not, add them to the tree
	for (int fwi=0; fwi < project->files.size(); fwi++) {
		bool foundAtTree = false;
		if (project->files.at(fwi).type == ptSource) {
			// source files
			for (int fti=0; fti < projNode->childCount(); fti++) {
				QTreeWidgetItem *fileNode = projNode->child(fti);
				QString nodeName = fileNode->text(0);
				if (project->files.at(fwi).name == nodeName) {
					foundAtTree = true;
					break;
				}
			}
			if (foundAtTree == false) {			
				QTreeWidgetItem *fileNode = new QTreeWidgetItem(projNode);
				QString text = project->files.at(fwi).name;
				fileNode->setText(0, text);
				fileNode->setIcon(0, QIcon(":/MainWindow/resources/workspace_tree/file1.png"));
				fileNode->setData(0, 255, WorskspaceTree::File);
			}
		} else { 
			// external files
			for (int fti=0; fti < externNode->childCount(); fti++) {
				QTreeWidgetItem *fileNode = externNode->child(fti);
				if (project->files.at(fwi).name == fileNode->text(0)) {
					foundAtTree = true;
					break;
				}
			}
			if (foundAtTree == false) {			
				QTreeWidgetItem *fileNode = new QTreeWidgetItem(externNode);
				QString text = project->files.at(fwi).name;
				fileNode->setIcon(0, QIcon(":/MainWindow/resources/workspace_tree/file2.png"));
				fileNode->setText(0, text);
				fileNode->setData(0, 255, WorskspaceTree::ExternalFile);
	
			}
		}		
	}

	// Now check if any file at the tree no longer exist on the workspace
	// In these cases, remove the file from the tree

	// source files
	int fti = 0;
	while (fti < projNode->childCount()) {
		QTreeWidgetItem *fileNode = projNode->child(fti);
		if (fileNode->text(0) == "External files") {
			fti++;
			continue;
		}

		bool foundAtWorkspace = false;
		for (int fwi=0; fwi < project->files.size(); fwi++) {
			if (project->files.at(fwi).type != ptSource) {
				continue;
			}		
			if (project->files.at(fwi).name == fileNode->text(0)) {
				foundAtWorkspace = true;
				break;
			}
		}
		if (foundAtWorkspace == false) {
			delete fileNode;
			//ui.tree->removeItemWidget(fileNode, 0);
		} else {
			fti++;
		}
	}

	// extern files
	fti = 0;
	while (fti < externNode->childCount()) {
		QTreeWidgetItem *fileNode = externNode->child(fti);
		QString nodeText = fileNode->text(0);
		
		bool foundAtWorkspace = false;
		for (int fwi=0; fwi < project->files.size(); fwi++) {
			if (project->files.at(fwi).type == ptSource) {
				continue;
			}		
			if (project->files.at(fwi).name == nodeText) {
				foundAtWorkspace = true;
				break;
			}
		}
		if (foundAtWorkspace == false) {
			delete fileNode;
			//ui.tree->removeItemWidget(fileNode, 0);
		} else {
			fti++;
		}
	}
}

//-----------------------------------------------------------------------------

void MainWindow::SaveWorkspace(void)
{
	// atualize workspace projects with file currently open
	// Also saves all open files
	if (tabsEditor->saveAllFiles() == false) {
		return;
	}
	for (int p=0; p < workspace.projects.size(); p++) {
		for (int f=0; f < workspace.projects.at(p).files.size(); f++) {
			QString filename = workspace.projects.at(p).files.at(f).name;
			filename = workspace.GetFullFilePath(workspace.projects.at(p).name, filename);
			int index = tabsEditor->fileIndex(filename);
			workspace.projects.at(p).files.at(f).open = (index >= 0);
		}
	}

	// Then save all projects on workspaces
	workspace.Save();
	if (workspace.IsModified() == false) {
		ui.actionSave_Workspace->setData(0);
		ui.actionSave_Workspace->setIcon(QIcon(":/MainWindow/resources/toolbar/save.png"));
	}
}

//-----------------------------------------------------------------------------

void MainWindow::OpenSerialPort(void)
{
	if (workspace.GetCurrentProject() == NULL) {
		return;
	}
	QString port = workspace.GetCurrentProject()->serialPort;
	if ( (port != "") && (port != "N/A")) {
		tabsEditor->openSerialPort(port, workspace.GetCurrentProject()->serialPortSpeed);
	} else {
		ErrorMessage("There's no serial port associated with the current project");
	}
}


//-----------------------------------------------------------------------------

void MainWindow::OnProjectModified(void)
{
	if (ui.actionSave_Workspace->data() == 0) {
		ui.actionSave_Workspace->setData(1);
		ui.actionSave_Workspace->setIcon(QIcon(":/MainWindow/resources/toolbar/save2.png"));
	}
}