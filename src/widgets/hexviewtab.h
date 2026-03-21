#ifndef HEXVIEWTAB_H
#define HEXVIEWTAB_H

#include "QHexView/qhexview.h"
#include "tooltab.h"
#include <QWidget>
#include <qfileinfo.h>
#include <QByteArray>

class HexViewTab : public ToolTab
{
    Q_OBJECT

private:

    /**
    * @brief Виджет HexView
   */

    QHexView* m_hexViewWidget;

    // Новые члены для поддержки вкладок форматов

    QByteArray m_rawData;
    QWidget* m_pageELF = nullptr;
    QWidget* m_pagePE = nullptr;
    QWidget* m_pageMBR = nullptr;


    /**
   * @brief Создаёт страницу для вкладки формата
  */


    QWidget* createPage();

    //Функция обновления содержимого вкладок
    void updateMBRPage();
    void updateELFPage();
    void updatePEPage();

public:
    explicit HexViewTab(QWidget* parent, QString path);

public slots:
    void setTabData() override;
    void saveTabData() override;
};

#endif // HEXVIEWTAB_H