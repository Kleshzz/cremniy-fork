#include "filetab.h"
#include "tooltabwidget.h"
#include <qboxlayout.h>
#include <qdir.h>
#include <qevent.h>

FileTab::FileTab(QWidget* parent, QString path)
    : QWidget(parent),
    filePath(path)
{
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    m_tooltabWidget = new ToolTabWidget(this, path);
    m_tooltabWidget->setObjectName("toolTabWidget");
    vlayout->addWidget(m_tooltabWidget);
    vlayout->setContentsMargins(0,0,0,0);
    this->setLayout(vlayout);

    // - - Connects - -
    connect(m_tooltabWidget, &ToolTabWidget::removeStarSignal, this, &FileTab::removeStar);
    connect(m_tooltabWidget, &ToolTabWidget::setupStarSignal, this, &FileTab::setupStar);

}

void FileTab::removeStar(){
    emit removeStarSignal(this);
}

void FileTab::setupStar(){
    emit setupStarSignal(this);
}