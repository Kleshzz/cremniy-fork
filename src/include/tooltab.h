#ifndef TOOLTAB_H
#define TOOLTAB_H

#include "filecontext.h"
#include <qstringview.h>
#include <qwidget.h>

class ToolTab : public QWidget {
    Q_OBJECT

protected:
    uint m_dataHash = 0;
    FileContext* m_fileContext;

public:
    explicit ToolTab(QWidget* parent = nullptr) : QWidget(parent) {}
public slots:
    virtual void saveTabData() = 0;
};

#endif // TOOLTAB_H
