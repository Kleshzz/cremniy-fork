#include "welcomeform.h"
#include "app/IDEWindow/idewindow.h"
#include "widgets/tooltab.h"
#include "ui_welcomeform.h"
#include <qboxlayout.h>
#include <qdir.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <QListView>
#include <QFileDialog>
#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <qstandardpaths.h>
#include <qstringlistmodel.h>

WelcomeForm::WelcomeForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WelcomeForm)
{
    ui->setupUi(this);


    QFile file(":/styles/style.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);

    // Base
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    this->setLayout(mainLayout);

    stack = new QStackedWidget(this);
    mainLayout->addWidget(stack);

    // Page "Welcome"
    QWidget *pageWelcome = new QWidget();
    QVBoxLayout *l1 = new QVBoxLayout(pageWelcome);

    history_project_list = new QListView(pageWelcome);
    l1->addWidget(history_project_list);
    history_project_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    history_project_list->setSelectionMode(QAbstractItemView::SingleSelection);
    SetProjectHistoryList();

    QHBoxLayout *btnLayout = new QHBoxLayout();

    open_recent_proj_btn = new QPushButton("Open", pageWelcome);
    btnLayout->addWidget(open_recent_proj_btn);
    open_recent_proj_btn->setEnabled(false);

    QPushButton *open_browse_proj_btn = new QPushButton("Open...", pageWelcome);
    btnLayout->addWidget(open_browse_proj_btn);

    QPushButton *create_proj_btn = new QPushButton("Create", pageWelcome);
    btnLayout->addWidget(create_proj_btn);

    l1->addLayout(btnLayout);

    stack->addWidget(pageWelcome);

    // Page "Create Project"
    QWidget *pageCreate = new QWidget();

    QVBoxLayout *l2 = new QVBoxLayout(pageCreate);

    // --- Grid layout для текста и полей ---
    QGridLayout *grid = new QGridLayout();

    // Первая строка: "Текст" + QLineEdit
    proj_name_label = new QLabel("Project Name:");
    proj_name_lineEdit = new QLineEdit();
    QRegularExpression re("^[A-Za-z0-9_-]+$");
    QValidator *validator = new QRegularExpressionValidator(re, this);
    proj_name_lineEdit->setValidator(validator);
    grid->addWidget(proj_name_label, 0, 0);
    grid->addWidget(proj_name_lineEdit, 0, 1);

    // Вторая строка: "Текст" + QComboBox
    language_label = new QLabel("Language:");
    language_comboBox = new QComboBox();
    language_comboBox->addItems({"C", "C++", "ASM", "C + ASM", "Custom"});
    grid->addWidget(language_label, 1, 0);
    grid->addWidget(language_comboBox, 1, 1);

    path_label = new QLabel("Path:");
    path_lineEdit = new ClickableLineEdit();
    path_lineEdit->setReadOnly(true);
    grid->addWidget(path_label, 2, 0);
    grid->addWidget(path_lineEdit, 2, 1);

    // Добавляем grid в основной вертикальный layout
    l2->addLayout(grid);
    l2->addStretch(1);

    info_label = new QLabel();
    info_label->setVisible(false);
    info_label->setStyleSheet("color: #bf3131; font-weight: bold;");
    info_label->setAlignment(Qt::AlignCenter);
    l2->addWidget(info_label);

    // --- Горизонтальный layout для кнопок ---
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *createButton = new QPushButton("Create");
    QPushButton *backButton = new QPushButton("Back");
    // buttonLayout->addStretch(1);       // пустое пространство слева
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(backButton);
    // buttonLayout->addStretch(1);       // пустое пространство справа

    // Добавляем кнопки в основной вертикальный layout
    l2->addLayout(buttonLayout);

    stack->addWidget(pageCreate);

    // Set Default Page (Welcome)
    stack->setCurrentIndex(0);

    // Events
    connect(history_project_list->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WelcomeForm::SelectProjectInList);

    connect(open_recent_proj_btn, &QPushButton::clicked, this, &WelcomeForm::OpenRecentProjectHandler);
    connect(open_browse_proj_btn, &QPushButton::clicked, this, &WelcomeForm::OpenProjectHandler);
    connect(create_proj_btn, &QPushButton::clicked, this, &WelcomeForm::CreateProjectHandler);

    connect(backButton, &QPushButton::clicked, this, &WelcomeForm::L2BackButton);
    connect(createButton, &QPushButton::clicked, this, &WelcomeForm::L2CreateButton);
}

WelcomeForm::~WelcomeForm()
{
    delete ui;
}

void WelcomeForm::SelectProjectInList(){
    open_recent_proj_btn->setEnabled(true);
    open_recent_proj_btn->setStyleSheet(""
                                        "QPushButton { border: 1px solid #2c7c32; } "
                                        "QPushButton:hover { background-color: #163318; border: 1px solid #2c7c32; } "
                                        "QPushButton:pressed { background-color: #163318; border: 2px solid #2c7c32; font-weight: bold; } "
                                        );
}

void WelcomeForm::OpenRecentProjectHandler(){
    QModelIndex index = history_project_list->currentIndex();

    if (index.isValid()) {
        OpenProject(index.data().toString());
    }
}

void WelcomeForm::OpenProjectHandler()
{
    QFileDialog dlg(this);
    dlg.setWindowTitle("Open project");
    dlg.setNameFilter("Cremniy Project (*.cremniy)");
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setDirectory(QDir::homePath());

    if (dlg.exec() == QDialog::Accepted) {
        QFileInfo f(dlg.selectedFiles().first());
        OpenProject(f.dir().path());
    }
}

void WelcomeForm::OpenProject(QString path){
    if (!QDir(path).exists()) return;
    if (!QFile::exists(path+"/"+"project.cremniy")) return;
    QFile project_file(path+"/"+"project.cremniy");
    if (!project_file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    // Читаем весь файл
    QByteArray data = project_file.readAll();
    project_file.close();

    // Парсим JSON
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject project_info = doc.object();

    IDEWindow *mw = new IDEWindow(path, project_info, nullptr);
    mw->setWindowState(Qt::WindowMaximized);
    mw->show();
    this->destroy();
}

void WelcomeForm::CreateProjectHandler()
{
    stack->setCurrentIndex(1);
}

void WelcomeForm::L2BackButton()
{
    stack->setCurrentIndex(0);
}

void WelcomeForm::L2CreateButton()
{
    info_label->setVisible(false);
    proj_name_label->setStyleSheet("color: #ffffff;");
    language_label->setStyleSheet("color: #ffffff;");
    path_label->setStyleSheet("color: #ffffff;");

    QString project_name = proj_name_lineEdit->text().trimmed();
    // Check Project Name
    if (project_name.isEmpty()) {
        proj_name_label->setStyleSheet("color: #bf3131;");
        info_label->setText("Please enter project name");
        info_label->setVisible(true);
        return;
    }

    // Check Directory Path
    QFileInfo dirinfo(path_lineEdit->text());
    if (!dirinfo.exists() || !dirinfo.isDir()) {
        path_label->setStyleSheet("color: #bf3131;");
        info_label->setText("Directory is invalid");
        info_label->setVisible(true);
        return;
    }

    QString new_project_path = path_lineEdit->text() + "/" + project_name;
    qDebug() << new_project_path;

    QDir dir;
    if (dir.exists(new_project_path)){
        path_label->setStyleSheet("color: #bf3131;");
        proj_name_label->setStyleSheet("color: #bf3131;");
        info_label->setText("Directory is exists!");
        info_label->setVisible(true);
        return;
    }

    if (!dir.mkdir(new_project_path)) {
        info_label->setText("Failed to create project directory!");
        info_label->setVisible(true);
        return;
    }

    QFile project_file(new_project_path+"/"+"project.cremniy");
    if (!project_file.open(QIODevice::WriteOnly)) {
        info_label->setText("Failed to create project file!");
        info_label->setVisible(true);
        return;
    }

    QJsonObject project_info;
    project_info["project_name"] = project_name;
    project_info["project_language"] = language_comboBox->currentText();

    // QJsonArray files;
    // files.append("main.cpp");
    // files.append("mainwindow.cpp");
    // project["files"] = files;

    QJsonDocument doc_proj_info(project_info);
    project_file.write(doc_proj_info.toJson(QJsonDocument::Indented));
    project_file.close();

    IDEWindow *mw = new IDEWindow(new_project_path, project_info, nullptr);
    mw->show();
    this->destroy();
}

void WelcomeForm::L2CreateProject(QString name, QString path, QString language){

}


void WelcomeForm::SetProjectHistoryList(){
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
    QStringListModel *model = new QStringListModel(this);
    model->setStringList(lines);
    history_project_list->setModel(model);
}