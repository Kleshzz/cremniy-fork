#ifndef GLOBALWIDGETSMANAGER_H
#define GLOBALWIDGETSMANAGER_H

#include <QObject>
#include <qmenubar.h>

class GlobalWidgetsManager : public QObject {
    Q_OBJECT
public:
    static GlobalWidgetsManager& instance() {
        static GlobalWidgetsManager inst;
        return inst;
    }

    void set_IDEWindow_menuBar_view_wordWrap(QAction* act) { m_IDEWindow_menuBar_view_wordWrap = act; }
    QAction* get_IDEWindow_menuBar_view_wordWrap() const { return m_IDEWindow_menuBar_view_wordWrap; }

    void set_IDEWindow_menuBar_file_saveFile(QAction* act) { m_IDEWindow_menuBar_file_saveFile = act; }
    QAction* get_IDEWindow_menuBar_file_saveFile() const { return m_IDEWindow_menuBar_file_saveFile; }

signals:
    void actionTriggered(const QString& actionName);

private:
    GlobalWidgetsManager() = default;
    QAction* m_IDEWindow_menuBar_view_wordWrap = nullptr;
    QAction* m_IDEWindow_menuBar_file_saveFile = nullptr;
};

#endif // GLOBALWIDGETSMANAGER_H
