#include "idewindow.h"
#include "dialogs/filecreatedialog.h"
#include "./ui_idewindow.h"
#include "QFileSystemModel"
#include "QMessageBox"
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <QStandardPaths>

IDEWindow::IDEWindow(QString ProjectPath, QJsonObject ProjectInfo, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    this->setWindowState(Qt::WindowMaximized);
    SaveProjectInCache(ProjectPath);

    QFile file(":/styles/style.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);

    ui->splitter->setSizes({200, 1000});
    ui->splitter->setCollapsible(0, false);
    ui->splitter->setCollapsible(1, false);
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);

    ui->treeView->setMinimumWidth(180);
    ui->treeView->setTextElideMode(Qt::ElideNone);
    ui->treeView->setIndentation(12);

    QFileSystemModel *model = new QFileSystemModel(this);

    model->setRootPath(ProjectPath);

    model->setReadOnly(false);
    ui->treeView->setModel(model);

    // ограничиваем отображение только этой директории
    ui->treeView->setRootIndex(model->index(ProjectPath));
    // model->setIconProvider(new IconProvider());

    ui->treeView->setColumnHidden(1, true);
    ui->treeView->setColumnHidden(2, true);
    ui->treeView->setColumnHidden(3, true);
    ui->treeView->header()->hide();
    ui->treeView->setAnimated(true);

    ui->horizontalLayout_2->setContentsMargins(0,0,0,0);
    ui->horizontalLayout->setContentsMargins(0,0,0,0);

    while (ui->filesTabWidget->count() > 0) {
        ui->filesTabWidget->removeTab(0);
    }

    ui->filesTabWidget->setTabsClosable(true);
    ui->filesTabWidget->setMovable(true);

    connect(ui->actionSave_File, &QAction::triggered, this, &IDEWindow::onSaveFile);

    connect(ui->filesTabWidget, &QTabWidget::tabCloseRequested,
            this, [=](int index){
                ui->filesTabWidget->removeTab(index);
            });

    ui->actionSave_File->setShortcut(QKeySequence::Save);

    ui->treeView->setEditTriggers(QAbstractItemView::EditKeyPressed);

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested,
            this, &IDEWindow::onTreeContextMenu);


}

IDEWindow::~IDEWindow()
{
    delete ui;
}

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
        if (!QFile::exists(l+"/"+"project.cremniy")) continue;
        out << l << "\n";
    }

    history_file.close();
}

void IDEWindow::onSaveFile()
{
    ui->filesTabWidget->saveCurrentFile();
}

void IDEWindow::on_treeView_doubleClicked(const QModelIndex &index)
{
    auto *model = static_cast<QFileSystemModel*>(ui->treeView->model());
    if (model->isDir(index)) return;
    QString fileName = model->fileName(index);
    QString filePath = model->filePath(index);

    ui->filesTabWidget->openFile(filePath, fileName);

}


void IDEWindow::on_treeView_clicked(const QModelIndex &index)
{

}

void IDEWindow::onTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->treeView->indexAt(pos); // индекс под курсором

    QFileSystemModel *model = qobject_cast<QFileSystemModel*>(ui->treeView->model());
    if (!model)
        return;

    QMenu menu(this);

    if (index.isValid()){

        QString path = model->filePath(index);
        QString fileName = model->fileName(index);
        bool isDir = model->isDir(index);  // <-- проверяем, директория ли

        if (isDir){
            menu.addAction("Open", [this, path]() {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(ui->treeView->model());
                if (!model)
                    return;

                QModelIndex index = model->index(path);
                if (!index.isValid())
                    return;

                // Разворачиваем саму директорию
                ui->treeView->expand(index);

                // Прокручиваем и выделяем
                //ui->treeView->scrollTo(index);
                //ui->treeView->setCurrentIndex(index);
                //ui->treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

            });

            menu.addAction("Rename", [this, path]() {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(ui->treeView->model());
                if (!model)
                    return;

                QModelIndex index = model->index(path);
                if (!index.isValid())
                    return;

                // Включаем редактирование индекса
                ui->treeView->edit(index);
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
                ui->filesTabWidget->openFile(path, fileName);
            });
            menu.addAction("Rename", [this, path]() {
                QFileSystemModel *model = qobject_cast<QFileSystemModel*>(ui->treeView->model());
                if (!model)
                    return;

                QModelIndex index = model->index(path);
                if (!index.isValid())
                    return;

                // Включаем редактирование индекса
                ui->treeView->edit(index);
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
    menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
}

