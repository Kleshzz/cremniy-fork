#ifndef CODEEDITORTAB_H
#define CODEEDITORTAB_H

#include "QCodeEditor.hpp"
#include "filecontext.h"
#include "tooltab.h"
#include <QWidget>
#include <qfileinfo.h>
#include <qlabel.h>
#include "filemanager.h"

class CodeEditorTab : public ToolTab
{
    Q_OBJECT

private:
    QCodeEditor* m_codeEditorWidget;
    QWidget* m_overlayWidget;
    bool forceSetData = false;

    void setTabData();

public:
    explicit CodeEditorTab(QWidget *parent, QString path);

signals:
    void modifyData(bool modified);
    void dataEqual();
    void askData();
    void setHexViewTab();

public slots:
    void saveTabData() override {
        qDebug() << "CodeEditorTab: saveTabData";

        QByteArray data = m_codeEditorWidget->getBData();
        uint newDataHash = qHash(data, 0);
        if (newDataHash == m_dataHash) return;
        m_dataHash = newDataHash;

        FileManager::saveFile(m_fileContext, &data);

        m_codeEditorWidget->document()->setModified(false);

        emit dataEqual();
    }

};

#endif // CODEEDITORTAB_H
