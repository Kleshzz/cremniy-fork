#include "filetab.h"
#include "QCodeEditor.hpp"
#include "tooltab.h"
#include <qdir.h>
#include <qevent.h>

FileTab::FileTab(FilesTabWidget *fwparent, QString path)
    : QWidget{fwparent}
{
    filePath = path;
    parrentTabWidget = fwparent;
}


void FileTab::saveFile(){
    ToolTab* tooltabInner = this->findChild<ToolTab*>("toolTabWidget");
    if (!tooltabInner) return;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    QCodeEditor* ce = tooltabInner->get_codeEditor();
    QString plainText = ce->toPlainText();
    QByteArray data =  plainText.toUtf8();
    file.write(data);
    file.close();
    tooltabInner->get_codeEditor()->document()->setModified(false);
    parrentTabWidget->setTabText(parrentTabWidget->currentIndex(), QFileInfo(filePath).fileName());
}

void FileTab::fileModifyEvent(bool modified){
    qDebug() << "mod";
    file_is_modify = true;
    parrentTabWidget->setTabText(parrentTabWidget->currentIndex(), QFileInfo(filePath).fileName()+"*");
}