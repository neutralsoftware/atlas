#pragma once

#include <QPushButton>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeView>

class QApplication;

namespace styling {

inline constexpr int SecondaryTextRole = Qt::UserRole + 42;

class Button : public QPushButton {
public:
    using QPushButton::QPushButton;
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *) override;
};

class ToolButton : public QToolButton {
public:
    explicit ToolButton(QWidget *parent = nullptr);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *) override;
};

class ItemDelegate : public QStyledItemDelegate {
public:
    explicit ItemDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

class TreeView : public QTreeView {
public:
    using QTreeView::QTreeView;
protected:
    void drawBranches(QPainter *painter, const QRect &rect,
                      const QModelIndex &index) const override;
};

class ElidedLabel : public QLabel {
public:
    explicit ElidedLabel(const QString &text, QWidget *parent = nullptr);
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent *) override;
};

class WorkspaceStack : public QStackedWidget {
public:
    explicit WorkspaceStack(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
};

void installWorkbench(QApplication &app);

}
