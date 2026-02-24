#include "filetreeview.h"


FileTreeView::FileTreeView(QWidget *parent)
    : QTreeView(parent)  // ← вызываем конструктор базового класса
{
}

void FileTreeView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = this->indexAt(event->pos());
    if (index.isValid())
        emit mouseClicked(index, event->button());

    QTreeView::mousePressEvent(event); // стандартное поведение
}