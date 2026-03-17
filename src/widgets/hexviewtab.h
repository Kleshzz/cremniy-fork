#ifndef HEXVIEWTAB_H
#define HEXVIEWTAB_H

#include "QHexView/qhexview.h"
#include "filecontext.h"
#include "tooltab.h"
#include <QWidget>
#include <qfileinfo.h>

class HexViewTab : public ToolTab
{
    Q_OBJECT

private:
    QHexView* m_hexViewWidget;
    QWidget* createPage();

    void setTabData();

public:
    explicit HexViewTab(QWidget *parent, QString path);

    void saveTabData() override {

        QByteArray data = m_hexViewWidget->getBData();
        uint newDataHash = qHash(data, 0);
        if (newDataHash == m_dataHash) return;
        m_dataHash = newDataHash;

        QFile f(path);
        if (!f.open(QFile::WriteOnly)) return;
        f.write(data);
        f.close();

    };

    void setTabData(QByteArray &data) override {
        qDebug() << "HexViewTab: setTabData()";
        m_dataHash = qHash(data, 0);
        m_hexViewWidget->setBData(data);
        emit dataEqual();
    };

signals:
    void modifyData(bool modified);
    void dataEqual();
};

#endif // HEXVIEWTAB_H
