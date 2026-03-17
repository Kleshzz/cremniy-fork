#include "idewindow.h"
#include "dialogs/filecreatedialog.h"
#include "QFileSystemModel"
#include "QMessageBox"
#include <qheaderview.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <QStandardPaths>
#include <QApplication>
#include "globalwidgetsmanager.h"
#include "app/WelcomeWindow/welcomeform.h"

IDEWindow::IDEWindow(QString ProjectPath, QWidget *parent)
    : QMainWindow(parent)
{
    this->setWindowState(Qt::WindowMaximized);
    this->setWindowTitle("Cremniy");
    SaveProjectInCache(ProjectPath);

    // Create Widgets/Layouts
    m_menuBar = menuBar();
    m_fileMenu = m_menuBar->addMenu("File");
    m_editMenu = m_menuBar->addMenu("Edit");
    m_viewMenu = m_menuBar->addMenu("View");
    m_gitMenu = m_menuBar->addMenu("Git");

    m_file_openProject = new QAction("New Project", this);
    m_file_newProject = new QAction("Open Project", this);
    m_file_saveFile = new QAction("Save File", this);
    GlobalWidgetsManager::instance().set_IDEWindow_menuBar_file_saveFile(m_file_saveFile);
    m_file_closeProject = new QAction("Close Project", this);

    m_view_wordWrap = new QAction("Word Wrap", this);
    m_view_wordWrap->setCheckable(true);
    m_view_wordWrap->setChecked(true);
    GlobalWidgetsManager::instance().set_IDEWindow_menuBar_view_wordWrap(m_view_wordWrap);

    m_git_commit = new QAction("Commit", this);
    m_git_commitAndPush = new QAction("Commit And Push", this);
    m_git_setBranch = new QAction("Set Branch", this);

    m_statusBar = statusBar();

    m_mainWidget = new QWidget(this);
    m_mainLayout = new QHBoxLayout(m_mainWidget);
    m_mainSplitter = new QSplitter(m_mainWidget);

    m_filesTabWidget = new FilesTabWidget();
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(m_mainWidget);
    leftLayout->setContentsMargins(0,0,0,0);
    m_filesTreeView = new FileTreeView();

    QHBoxLayout* RunBuildBtns_layout = new QHBoxLayout();
    QPushButton* RunButton = new QPushButton();
    QPushButton* DebugButton = new QPushButton();
    QPushButton* BuildButton = new QPushButton();

    RunButton->setFixedWidth(30);
    RunButton->setFixedHeight(30);
    RunButton->setProperty("state", "green");

    DebugButton->setFixedWidth(30);
    DebugButton->setFixedHeight(30);
    DebugButton->setProperty("state", "green");

    BuildButton->setFixedWidth(30);
    BuildButton->setFixedHeight(30);

    RunBuildBtns_layout->setContentsMargins(0,0,0,0);

    RunBuildBtns_layout->addStretch();
    RunBuildBtns_layout->addWidget(RunButton);
    RunBuildBtns_layout->addWidget(DebugButton);
    RunBuildBtns_layout->addWidget(BuildButton);
    RunBuildBtns_layout->addStretch();

    leftWidget->setLayout(leftLayout);
    leftLayout->addWidget(m_filesTreeView);
    leftLayout->addLayout(RunBuildBtns_layout);

    // Tunning Widgets/Layouts
    m_fileMenu->addAction(m_file_openProject);
    m_fileMenu->addAction(m_file_newProject);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_file_saveFile);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_file_closeProject);

    m_viewMenu->addAction(m_view_wordWrap);

    m_gitMenu->addAction(m_git_commit);
    m_gitMenu->addAction(m_git_commitAndPush);
    m_gitMenu->addSeparator();
    m_gitMenu->addAction(m_git_setBranch);

    setCentralWidget(m_mainWidget);

    m_mainLayout->addWidget(m_mainSplitter);

    m_filesTabWidget->setObjectName("filesTabWidget");

    m_mainSplitter->addWidget(leftWidget);
    m_mainSplitter->addWidget(m_filesTabWidget);

    m_mainSplitter->setSizes({200, 1000});
    m_mainSplitter->setCollapsible(0, false);
    m_mainSplitter->setCollapsible(1, false);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);

    m_filesTreeView->setMinimumWidth(180);
    m_filesTreeView->setTextElideMode(Qt::ElideNone);
    m_filesTreeView->setIndentation(12);

    QFileSystemModel *model = new QFileSystemModel(this);

    model->setRootPath(ProjectPath);

    model->setReadOnly(false);
    m_filesTreeView->setModel(model);

    // ограничиваем отображение только этой директории
    m_filesTreeView->setRootIndex(model->index(ProjectPath));
    // model->setIconProvider(new IconProvider());

    m_filesTreeView->setColumnHidden(1, true);
    m_filesTreeView->setColumnHidden(2, true);
    m_filesTreeView->setColumnHidden(3, true);
    m_filesTreeView->header()->hide();
    m_filesTreeView->setAnimated(true);

    m_mainLayout->setContentsMargins(0,0,0,0);

    while (m_filesTabWidget->count() > 0) {
        m_filesTabWidget->removeTab(0);
    }

    m_filesTabWidget->setTabsClosable(true);
    m_filesTabWidget->setMovable(true);

    connect(m_file_closeProject, &QAction::triggered, this, &IDEWindow::onClosingProject);

    connect(m_filesTabWidget, &QTabWidget::tabCloseRequested,
            this, [=](int index){
                m_filesTabWidget->removeTab(index);
            });

    m_file_saveFile->setShortcut(QKeySequence::Save);

    m_filesTreeView->setEditTriggers(QAbstractItemView::EditKeyPressed);

    m_filesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_filesTreeView, &QTreeView::customContextMenuRequested,
            this, &IDEWindow::onTreeContextMenu);

    connect(m_filesTreeView, &QTreeView::doubleClicked, this, &IDEWindow::on_treeView_doubleClicked);
}

IDEWindow::~IDEWindow()
{}

void IDEWindow::SaveProjectInCache(const QString project_path){
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(dataDir).mkpath(".");
    QFile history_file(dataDir+"/"+"history_open_projects.dat");
    QStringList lines;
    if (history_file.open(QIODevice::ReadOnly)) {
        QByteArray data = history_file.readAll();
        QString text = QString::fromUtf8(data);
        lines = text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        history_file.close();
    }
    lines.removeAll(project_path);
    lines.prepend(project_path);
    while (lines.size() > 15)
        lines.removeLast();
    if (!history_file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&history_file);
    for (const QString& l : lines){
        if (!QDir(l).exists()) continue;
        out << l << "\n";
    }

    history_file.close();
}

void IDEWindow::onClosingProject() {
    WelcomeForm* wForm = new WelcomeForm();
    wForm->show();

    this->close();
    this->deleteLater();
}

void IDEWindow::on_treeView_doubleClicked(const QModelIndex &index)
{
    auto *model = static_cast<QFileSystemModel*>(m_filesTreeView->model());
    if (model->isDir(index)) return;
    QString fileName = model->fileName(index);
    QString filePath = model->filePath(index);

    m_filesTabWidget->openFile(filePath, fileName);

}


void IDEWindow::on_treeView_clicked(const QModelIndex &index)
{

}

void IDEWindow::onTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = m_filesTreeView->indexAt(pos); // индекс под курсором

    QFileSystemModel *model = qobject_cast<QFileSystemModel*>(m_filesTreeView->model());
    if (!model)
        return;

    QMenu menu(this);

    if (index.isValid()){

        QString path = model->filePath(index);
        QString fileName = model->fileName(index);
        bool isDir = model->isDir(index);  // <-- проверяем, директория ли

        if (isDir){
            menu.addAction("Open", [this, path]() {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(m_filesTreeView->model());
                if (!model)
                    return;

                QModelIndex index = model->index(path);
                if (!index.isValid())
                    return;

                // Разворачиваем саму директорию
                m_filesTreeView->expand(index);

                // Прокручиваем и выделяем
                //m_filesTreeView->scrollTo(index);
                //m_filesTreeView->setCurrentIndex(index);
                //m_filesTreeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

            });

            menu.addAction("Rename", [this, path]() {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(m_filesTreeView->model());
                if (!model)
                    return;

                QModelIndex index = model->index(path);
                if (!index.isValid())
                    return;

                // Включаем редактирование индекса
                m_filesTreeView->edit(index);
            });
            menu.addAction("Delete", [path, this]() {
                QDir dir(path);
                QString dialogTitle = QString("Are you sure you want to delete the folder \"%1\"?").arg(dir.dirName());
                auto res = QMessageBox::question(this, "Delete", dialogTitle, QMessageBox::Ok | QMessageBox::Cancel);
                if (res == QMessageBox::Ok) dir.removeRecursively();
            });
            menu.addSeparator();
            menu.addAction("Create File", [path,this]() {
                FileCreateDialog fcd(this,path,false);
                fcd.exec();

            });
            menu.addAction("Create Folder", [path,this]() {
                FileCreateDialog fcd(this,path,true);
                fcd.exec();
            });
        }
        else{
            menu.addAction("Open", [this, path, fileName]() {
                m_filesTabWidget->openFile(path, fileName);
            });
            menu.addAction("Rename", [this, path]() {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(m_filesTreeView->model());
                if (!model)
                    return;

                QModelIndex index = model->index(path);
                if (!index.isValid())
                    return;

                // Включаем редактирование индекса
                m_filesTreeView->edit(index);
            });
            menu.addAction("Delete", [path,this]() {
                QString dialogTitle = QString("Are you sure you want to delete the file \"%1\"?").arg(QFileInfo(path).fileName());
                auto res = QMessageBox::question(this, "Delete", dialogTitle, QMessageBox::Ok | QMessageBox::Cancel);
                if (res == QMessageBox::Ok) QFile(path).remove();
            });
        }

        // Показать меню в глобальных координатах

    }

    else{
        QString path = model->rootPath();
        menu.addAction("Create File", [path,this]() {
            FileCreateDialog fcd(this,path,false);
            fcd.exec();
        });
        menu.addAction("Create Folder", [path,this]() {
            FileCreateDialog fcd(this,path,true);
            fcd.exec();
        });
    }
    menu.exec(m_filesTreeView->viewport()->mapToGlobal(pos));
}

