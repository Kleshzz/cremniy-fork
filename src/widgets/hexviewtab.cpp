#include "hexviewtab.h"
#include "verticaltabstyle.h"
#include <QApplication>
#include <QBoxLayout>
#include <QStackedWidget>
#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QtEndian>             
#include "filemanager.h"

HexViewTab::HexViewTab(QWidget* parent, QString path)
    : ToolTab{ parent }
{
    // - - Init variables - -
    m_fileContext = new FileContext(path);

    // - - Tab Widgets - -
    auto mainHexTabLayout = new QHBoxLayout(this);
    mainHexTabLayout->setSpacing(0);
    mainHexTabLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(mainHexTabLayout);

    QListWidget* tabsList = new QListWidget();
    tabsList->setObjectName("hexTabsList");
    tabsList->setFocusPolicy(Qt::NoFocus);
    QStackedWidget* tabView = new QStackedWidget();

    mainHexTabLayout->addWidget(tabsList);
    mainHexTabLayout->addWidget(tabView, 1);

    tabsList->addItem("Raw");
    tabsList->addItem("ELF");
    tabsList->addItem("PE");
    tabsList->addItem("MBR");

    // - - Create Pages - -
    m_hexViewWidget = new QHexView(this);
    auto pageRaw = createPage();
    pageRaw->layout()->addWidget(m_hexViewWidget);

    m_pageELF = createPage();
    m_pagePE = createPage();
    m_pageMBR = createPage();          

    tabView->addWidget(pageRaw);
    tabView->addWidget(m_pageELF);
    tabView->addWidget(m_pagePE);
    tabView->addWidget(m_pageMBR);

    tabsList->setCurrentRow(0);

    // - - Connects - -
    connect(tabsList, &QListWidget::currentRowChanged,
        tabView, &QStackedWidget::setCurrentIndex);

    connect(m_hexViewWidget->hexDocument(),
        &QHexDocument::changed,
        this,
        [this]() {
            QByteArray data = m_hexViewWidget->getBData();
            uint newDataHash = qHash(data, 0);
            if (m_dataHash == newDataHash) {
                emit dataEqual();
            }
            else {
                if (!m_hexViewWidget->m_ignoreModification)
                    emit modifyData(true);
            }
        });

    // Добавлено: обновление MBR при смене вкладки
    connect(tabsList, &QListWidget::currentRowChanged,
        this, [this](int index) {
            if (index == 1) updateELFPage();
            if (index == 2) updatePEPage();
            if (index == 3) updateMBRPage();
        });

    this->setTabData();
}

QWidget* HexViewTab::createPage()
{
    QWidget* pageWidget = new QWidget();
    QVBoxLayout* pageWidgetLayout = new QVBoxLayout(pageWidget);
    pageWidgetLayout->setContentsMargins(0, 0, 0, 0);
    pageWidget->setLayout(pageWidgetLayout);
    return pageWidget;
}

void HexViewTab::setTabData()
{
    qDebug() << "HexViewTab: setTabData()";

    QByteArray data = FileManager::openFile(m_fileContext);

    m_dataHash = qHash(data, 0);
    m_hexViewWidget->setBData(data);

    m_rawData = data;               // сохраняем для парсеров

    emit dataEqual();

    updateELFPage();
    updatePEPage();
    updateMBRPage();
}

void HexViewTab::saveTabData()
{
    qDebug() << "HexViewTab: saveTabData";

    QByteArray data = m_hexViewWidget->getBData();
    uint newDataHash = qHash(data, 0);
    if (newDataHash == m_dataHash) return;
    m_dataHash = newDataHash;

    FileManager::saveFile(m_fileContext, &data);

    emit dataEqual();
    emit refreshDataAllTabsSignal();
}

// ────────────────────────────────────────────────
// Реализация парсинга MBR (таблица разделов)
// ────────────────────────────────────────────────
void HexViewTab::updateMBRPage()
{
    if (!m_pageMBR || m_rawData.size() < 512) {
        return;
    }

    // Очищаем предыдущее содержимое
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(m_pageMBR->layout());
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    const uchar* bytes = reinterpret_cast<const uchar*>(m_rawData.constData());

    // Заголовок
    QLabel* title = new QLabel("Master Boot Record (MBR)", m_pageMBR);
    title->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    // Проверка сигнатуры
    bool valid = (bytes[510] == 0x55 && bytes[511] == 0xAA);
    if (!valid) {
        QLabel* warn = new QLabel("Нет сигнатуры 55 AA на позициях 510–511 → не похоже на MBR", m_pageMBR);
        warn->setStyleSheet("color: #ff8800; font-size: 14px; margin: 10px;");
        layout->addWidget(warn);
    }

    // Таблица разделов
    QTableWidget* table = new QTableWidget(4, 6, m_pageMBR);
    table->setHorizontalHeaderLabels({ "№", "Active", "Type (hex)", "Start LBA", "Size (sectors)", "Тип" });
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);

    for (int i = 0; i < 4; ++i) {
        int off = 0x1BE + i * 16;

        uchar boot = bytes[off + 0];
        uchar type = bytes[off + 4];
        uint32_t lba = qFromLittleEndian<uint32_t>(bytes + off + 8);
        uint32_t size = qFromLittleEndian<uint32_t>(bytes + off + 12);

        table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));

        QString active = (boot == 0x80) ? "Yes" : (boot == 0 ? "No" : QString("0x%1").arg(boot, 2, 16, QChar('0')));
        table->setItem(i, 1, new QTableWidgetItem(active));

        table->setItem(i, 2, new QTableWidgetItem(QString("0x%1").arg(type, 2, 16, QChar('0'))));

        table->setItem(i, 3, new QTableWidgetItem(QString::number(lba)));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(size)));

        QString desc = "Unknown";
        if (type == 0x00) desc = "Empty";
        if (type == 0x01) desc = "FAT12";
        if (type == 0x04 || type == 0x06 || type == 0x0E) desc = "FAT16";
        if (type == 0x0B || type == 0x0C) desc = "FAT32";
        if (type == 0x07) desc = "NTFS / exFAT";
        if (type == 0x83) desc = "Linux";
        if (type == 0xEE) desc = "GPT protective";
        table->setItem(i, 5, new QTableWidgetItem(desc));

        if (boot == 0x80) {
            for (int c = 0; c < 6; ++c) {
                table->item(i, c)->setBackground(QColor(0, 100, 0, 60));
            }
        }
    }

    layout->addWidget(table);
    layout->addStretch(1);
}

// ────────────────────────────────────────────────
// Реализация парсинга ELF
// ────────────────────────────────────────────────
void HexViewTab::updateELFPage()
{
    if (!m_pageELF || m_rawData.size() < 64) {
        return;
    }

    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(m_pageELF->layout());
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    const uchar* bytes = reinterpret_cast<const uchar*>(m_rawData.constData());

    QLabel* title = new QLabel("ELF Header", m_pageELF);
    title->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    // Проверка магии ELF: 0x7F 'E' 'L' 'F'
    bool valid = (bytes[0] == 0x7F && bytes[1] == 'E' &&
        bytes[2] == 'L' && bytes[3] == 'F');
    if (!valid) {
        QLabel* warn = new QLabel("Нет ELF-сигнатуры (7F 45 4C 46) → не является ELF файлом", m_pageELF);
        warn->setStyleSheet("color: #ff8800; font-size: 14px; margin: 10px;");
        layout->addWidget(warn);
        layout->addStretch(1);
        return;
    }

    QTableWidget* table = new QTableWidget(0, 2, m_pageELF);
    table->setHorizontalHeaderLabels({ "Поле", "Значение" });
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);

    auto addRow = [&](const QString& field, const QString& value) {
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(field));
        table->setItem(row, 1, new QTableWidgetItem(value));
        };

    // Класс (32/64 бит)
    QString cls = bytes[4] == 1 ? "32-bit" : bytes[4] == 2 ? "64-bit" : "Unknown";
    addRow("Class", cls);

    // Порядок байт
    QString endian = bytes[5] == 1 ? "Little Endian" : bytes[5] == 2 ? "Big Endian" : "Unknown";
    addRow("Data", endian);

    // Тип файла
    uint16_t type = qFromLittleEndian<uint16_t>(bytes + 16);
    QString typeStr;
    switch (type) {
    case 1: typeStr = "Relocatable (ET_REL)"; break;
    case 2: typeStr = "Executable (ET_EXEC)"; break;
    case 3: typeStr = "Shared object (ET_DYN)"; break;
    case 4: typeStr = "Core (ET_CORE)"; break;
    default: typeStr = QString("Unknown (0x%1)").arg(type, 4, 16, QChar('0'));
    }
    addRow("Type", typeStr);

    // Архитектура
    uint16_t machine = qFromLittleEndian<uint16_t>(bytes + 18);
    QString machineStr;
    switch (machine) {
    case 0x03: machineStr = "x86"; break;
    case 0x3E: machineStr = "x86-64"; break;
    case 0x28: machineStr = "ARM"; break;
    case 0xB7: machineStr = "AArch64"; break;
    default: machineStr = QString("0x%1").arg(machine, 4, 16, QChar('0'));
    }
    addRow("Machine", machineStr);

    layout->addWidget(table);
    layout->addStretch(1);
}

// ────────────────────────────────────────────────
// Реализация парсинга PE (Windows EXE/DLL)
// ────────────────────────────────────────────────
void HexViewTab::updatePEPage()
{
    if (!m_pagePE || m_rawData.size() < 64) {
        return;
    }

    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(m_pagePE->layout());
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    const uchar* bytes = reinterpret_cast<const uchar*>(m_rawData.constData());

    QLabel* title = new QLabel("PE Header", m_pagePE);
    title->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    // Проверка DOS сигнатуры MZ
    bool valid = (bytes[0] == 'M' && bytes[1] == 'Z');
    if (!valid) {
        QLabel* warn = new QLabel("Нет DOS-сигнатуры MZ → не является PE файлом", m_pagePE);
        warn->setStyleSheet("color: #ff8800; font-size: 14px; margin: 10px;");
        layout->addWidget(warn);
        layout->addStretch(1);
        return;
    }

    // Смещение PE заголовка (по адресу 0x3C)
    uint32_t peOffset = qFromLittleEndian<uint32_t>(bytes + 0x3C);
    if ((int)(peOffset + 6) >= m_rawData.size()) {
        QLabel* warn = new QLabel("Некорректное смещение PE заголовка", m_pagePE);
        warn->setStyleSheet("color: #ff8800; font-size: 14px; margin: 10px;");
        layout->addWidget(warn);
        layout->addStretch(1);
        return;
    }

    // Проверка сигнатуры PE\0\0
    bool peValid = (bytes[peOffset] == 'P' && bytes[peOffset + 1] == 'E' &&
        bytes[peOffset + 2] == 0 && bytes[peOffset + 3] == 0);
    if (!peValid) {
        QLabel* warn = new QLabel("Нет PE-сигнатуры → повреждённый файл", m_pagePE);
        warn->setStyleSheet("color: #ff8800; font-size: 14px; margin: 10px;");
        layout->addWidget(warn);
        layout->addStretch(1);
        return;
    }

    QTableWidget* table = new QTableWidget(0, 2, m_pagePE);
    table->setHorizontalHeaderLabels({ "Поле", "Значение" });
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);

    auto addRow = [&](const QString& field, const QString& value) {
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(field));
        table->setItem(row, 1, new QTableWidgetItem(value));
        };

    // Машина
    uint16_t machine = qFromLittleEndian<uint16_t>(bytes + peOffset + 4);
    QString machineStr;
    switch (machine) {
    case 0x014C: machineStr = "x86 (i386)"; break;
    case 0x8664: machineStr = "x86-64 (AMD64)"; break;
    case 0xAA64: machineStr = "AArch64"; break;
    default: machineStr = QString("0x%1").arg(machine, 4, 16, QChar('0'));
    }
    addRow("Machine", machineStr);

    // Количество секций
    uint16_t sections = qFromLittleEndian<uint16_t>(bytes + peOffset + 6);
    addRow("Number of Sections", QString::number(sections));

    // Характеристики
    uint16_t chars = qFromLittleEndian<uint16_t>(bytes + peOffset + 22);
    QStringList charList;
    if (chars & 0x0002) charList << "Executable";
    if (chars & 0x0020) charList << "Large address aware";
    if (chars & 0x2000) charList << "DLL";
    addRow("Characteristics", charList.isEmpty() ? QString("0x%1").arg(chars, 4, 16, QChar('0')) : charList.join(", "));

    layout->addWidget(table);
    layout->addStretch(1);
}