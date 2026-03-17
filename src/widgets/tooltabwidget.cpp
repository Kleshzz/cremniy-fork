#include "tooltabwidget.h"
#include <QCodeEditor.hpp>
#include <QFile>
#include <QSyntaxStyle.hpp>

#include <QCodeEditor.hpp>
#include <QCECompleter.hpp>
#include <QSyntaxStyle.hpp>
#include <QCXXHighlighter.hpp>
#include <QJSONHighlighter.hpp>
#include <qboxlayout.h>
#include <qfileinfo.h>
#include "globalwidgetsmanager.h"

ToolTabWidget::ToolTabWidget(QWidget *parent, QString path)
    {

    // Tabs
    m_codeEditorTab = new CodeEditorTab(this, path);
    m_hexViewTab = new HexViewTab(this, path);
    m_hexViewTab->setObjectName("hexViewTab");
    m_disassemblerTab = new DisassemblerTab(this, path);

    // Tab Icons
    QIcon codeIcon(":/icons/code.png");
    QIcon hexIcon(":/icons/hex.png");
    QIcon disasmIcon(":/icons/dasm.png");

    // Add Tabs
    this->addTab(m_codeEditorTab, codeIcon, "Code");
    this->addTab(m_hexViewTab, hexIcon, "Hex");
    this->addTab(m_disassemblerTab, disasmIcon, "Disassembler");

    // - - Connects - -

    // Trigger: Menu Bar: File->SaveFile or CTRL+S - saveTabData
    connect(GlobalWidgetsManager::instance().get_IDEWindow_menuBar_file_saveFile(),
            &QAction::triggered, this, &ToolTabWidget::saveCurrentTabData);

    // Code Editor Tab
    connect(m_codeEditorTab, &CodeEditorTab::modifyData, this, &ToolTabWidget::setupStar);
    connect(m_codeEditorTab, &CodeEditorTab::dataEqual, this, &ToolTabWidget::removeStar);
    connect(m_codeEditorTab, &CodeEditorTab::setHexViewTab, this, &ToolTabWidget::setHexViewTab); // when "Open in HexView" button clicked

    // Hex View Tab
    connect(m_hexViewTab, &HexViewTab::modifyData, this, &ToolTabWidget::setupStar);
    connect(m_hexViewTab, &HexViewTab::dataEqual, this, &ToolTabWidget::removeStar);

    // Disassembler Tab
    connect(m_disassemblerTab, &DisassemblerTab::modifyData, this, &ToolTabWidget::setupStar);
}

void ToolTabWidget::saveCurrentTabData(){
    ToolTab* tab = dynamic_cast<ToolTab*>(currentWidget());
    if (tab) tab->saveTabData();
}

void ToolTabWidget::removeStar(){

    qDebug() << "ToolTabWidget: removeStar()";

    // remove star at sender
    QObject* obj = sender();
    QWidget* widget = qobject_cast<QWidget*>(obj);

    if (!widget) return;

    int index = indexOf(widget);
    if (index < 0) return;

    QString text = tabText(index);
    text.replace("*", "");
    setTabText(index, text);

    // check other tabs
    if (!tabText(this->indexOf(m_codeEditorTab)).contains("*") &&
        !tabText(this->indexOf(m_hexViewTab)).contains("*") &&
        !tabText(this->indexOf(m_disassemblerTab)).contains("*") ){
        emit removeStarSignal();
    }

}

void ToolTabWidget::setupStar(bool modified){

    qDebug() << "ToolTabWidget: setupStar()";

    // setup star on tab
    QObject* obj = sender();
    QWidget* widget = qobject_cast<QWidget*>(obj);

    if (!widget) return;

    int index = indexOf(widget);
    if (index < 0) return;

    QString text = tabText(index);
    if (!text.endsWith("*")){
        setTabText(index, text + "*");
    }

    // signal "setup star" to up
    emit setupStarSignal();

}

void ToolTabWidget::setHexViewTab(){
    int index = indexOf(m_hexViewTab);
    setCurrentIndex(index);
}