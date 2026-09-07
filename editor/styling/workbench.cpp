#include "editor/styling/workbench.h"
#include "editor/styling/icons.h"

#include <QApplication>
#include <algorithm>
#include <QEvent>
#include <QLayout>
#include <QAbstractSpinBox>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionViewItem>
#include <QVariant>

namespace {

bool prominent(const QAbstractButton *button) {
    return button->objectName() == "primaryAction" ||
           button->objectName() == "workspaceLaunchButton" ||
           (qobject_cast<const QPushButton *>(button) != nullptr &&
            qobject_cast<const QPushButton *>(button)->isDefault());
}

void paintButton(QAbstractButton *button, bool showText, bool vertical,
                 bool hasMenu, bool isTool) {
    QPainter painter(button);
    painter.setRenderHint(QPainter::Antialiasing);
    const bool enabled = button->isEnabled();
    const bool hover = enabled && button->underMouse();
    const bool header = button->objectName() == "inspectorComponentHeader";
    const bool selected = (button->isChecked() && !header) ||
                          button->property("matched").toBool();
    const bool primary = prominent(button);
    const QRectF bounds = QRectF(button->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QColor background(Qt::transparent);
    if (primary)
        background = !enabled ? QColor("#282827")
                     : button->isDown() ? QColor("#C6C6BF")
                     : hover ? QColor("#F4F4EE") : QColor("#E5E5E1");
    else if (button->isDown())
        background = QColor("#333331");
    else if (selected)
        background = QColor("#30302E");
    else if (hover)
        background = QColor("#292928");
    else if (!isTool && !header)
        background = QColor("#242423");
    painter.setPen(Qt::NoPen);
    painter.setBrush(background);
    painter.drawRoundedRect(bounds, 7, 7);
    if (button->hasFocus() && enabled) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor("#777771"), 1));
        painter.drawRoundedRect(bounds.adjusted(1, 1, -1, -1), 6, 6);
    }
    const QColor foreground = !enabled ? QColor("#646460")
                               : primary ? QColor("#191918")
                               : selected || hover || header ? QColor("#ECECE7")
                               : QColor("#B6B6AF");
    QRect content = button->rect().adjusted(10, 0, -10, 0);
    const bool preserveIconColor = button->property("preserveIconColor").toBool() ||
                                   button->objectName() == "materialColorButton" ||
                                   button->objectName() == "inspectorColorSwatch";
    const int iconSize = preserveIconColor ? button->iconSize().width() : 18;
    const int iconHeight = preserveIconColor ? button->iconSize().height() : 18;
    const bool hasIcon = !button->icon().isNull();
    if (hasMenu)
        content.adjust(0, 0, -13, 0);
    if (hasIcon) {
        QRect iconRect;
        if (!showText)
            iconRect = QRect((button->width() - iconSize) / 2,
                             (button->height() - iconHeight) / 2, iconSize, iconHeight);
        else if (vertical)
            iconRect = QRect((button->width() - iconSize) / 2, 8, iconSize, iconHeight);
        else
            iconRect = QRect(content.left(), (button->height() - iconHeight) / 2,
                             iconSize, iconHeight);
        QPixmap glyph = button->icon().pixmap(QSize(iconSize, iconHeight),
                                               button->devicePixelRatioF());
        if (!preserveIconColor) {
            QPainter tint(&glyph);
            tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tint.fillRect(glyph.rect(), foreground);
        }
        painter.drawPixmap(iconRect, glyph);
        if (showText && !vertical)
            content.adjust(iconSize + 7, 0, 0, 0);
    }
    if (showText) {
        QFont font = button->font();
        font.setPixelSize(13);
        font.setWeight(primary || selected || header ? QFont::Medium : QFont::Normal);
        painter.setFont(font);
        painter.setPen(foreground);
        if (vertical)
            content.setTop(28);
        const Qt::Alignment alignment = hasIcon && !vertical
                                            ? Qt::AlignLeft | Qt::AlignVCenter
                                            : Qt::AlignCenter;
        painter.drawText(content, alignment,
                         painter.fontMetrics().elidedText(button->text().remove('&'),
                                                         Qt::ElideRight, content.width()));
    }
    if (hasMenu) {
        painter.setPen(QPen(foreground, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF center(button->width() - 12, button->height() / 2.0);
        QPainterPath path;
        path.moveTo(center + QPointF(-3, -1.5));
        path.lineTo(center + QPointF(0, 1.5));
        path.lineTo(center + QPointF(3, -1.5));
        painter.drawPath(path);
    }
}

class WorkbenchInstaller : public QObject {
public:
    using QObject::QObject;
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::Polish) {
            if (auto *button = qobject_cast<QAbstractButton *>(object)) {
                if (button->accessibleName().isEmpty() && button->text().isEmpty())
                    button->setAccessibleName(button->toolTip());
            }
            if (auto *spin = qobject_cast<QAbstractSpinBox *>(object))
                spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
            auto *view = qobject_cast<QAbstractItemView *>(object);
            if (view != nullptr && !view->property("workbenchStyled").toBool() &&
                !view->inherits("QTableView")) {
                view->setProperty("workbenchStyled", true);
                view->setItemDelegate(new styling::ItemDelegate(view));
                view->setMouseTracking(true);
                view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
                view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            }
        }
        return QObject::eventFilter(object, event);
    }
};

}

QSize styling::Button::sizeHint() const {
    QFont textFont = font();
    textFont.setPixelSize(13);
    return QSize(QFontMetrics(textFont).horizontalAdvance(text().remove('&')) +
                     (icon().isNull() ? 28 : 53) + (menu() ? 14 : 0), 34);
}

void styling::Button::paintEvent(QPaintEvent *) {
    paintButton(this, true, false, menu() != nullptr, false);
}

styling::ToolButton::ToolButton(QWidget *parent) : QToolButton(parent) {
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAutoRaise(true);
    setIconSize(QSize(18, 18));
}

QSize styling::ToolButton::sizeHint() const {
    if (toolButtonStyle() == Qt::ToolButtonIconOnly)
        return QSize(34, 34);
    QFont textFont = font();
    textFont.setPixelSize(13);
    const int textWidth = QFontMetrics(textFont).horizontalAdvance(text().remove('&'));
    if (toolButtonStyle() == Qt::ToolButtonTextUnderIcon)
        return QSize(std::max(44, textWidth + 20), 52);
    return QSize(textWidth + (icon().isNull() ? 24 : 49) + (menu() ? 14 : 0), 34);
}

void styling::ToolButton::paintEvent(QPaintEvent *) {
    paintButton(this, toolButtonStyle() != Qt::ToolButtonIconOnly,
                toolButtonStyle() == Qt::ToolButtonTextUnderIcon, menu() != nullptr, true);
}

styling::ItemDelegate::ItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void styling::ItemDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const {
    QStyleOptionViewItem item(option);
    initStyleOption(&item, index);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    const bool selected = item.state & QStyle::State_Selected;
    const bool hovered = item.state & QStyle::State_MouseOver;
    const bool enabled = item.state & QStyle::State_Enabled;
    const auto *list = qobject_cast<const QListView *>(item.widget);
    const bool grid = list != nullptr && list->viewMode() == QListView::IconMode;
    const QRect bounds = item.rect.adjusted(3, 2, -3, -2);
    painter->setPen(Qt::NoPen);
    painter->setBrush(selected ? QColor("#30302E")
                              : hovered ? QColor("#242423") : QColor(Qt::transparent));
    painter->drawRoundedRect(bounds, 6, 6);
    QRect textRect = bounds.adjusted(10, 0, -10, 0);
    if (!item.icon.isNull()) {
        const int size = grid ? 38 : 18;
        QRect iconRect = grid ? QRect(bounds.center().x() - size / 2, bounds.top() + 14, size, size)
                              : QRect(textRect.left(), bounds.center().y() - size / 2, size, size);
        item.icon.paint(painter, iconRect, Qt::AlignCenter,
                        enabled ? QIcon::Normal : QIcon::Disabled);
        if (grid)
            textRect.setTop(iconRect.bottom() + 9);
        else
            textRect.adjust(size + 9, 0, 0, 0);
    }
    if (item.features & QStyleOptionViewItem::HasCheckIndicator) {
        const QRect check(textRect.left(), bounds.center().y() - 7, 14, 14);
        painter->setPen(QPen(QColor("#666660"), 1));
        painter->setBrush(item.checkState == Qt::Checked ? QColor("#D5D5CF") : QColor("#242423"));
        painter->drawRoundedRect(check, 3, 3);
        if (item.checkState == Qt::Checked)
            styling::icon(styling::Icon::Check, "#181817").paint(painter, check);
        textRect.adjust(22, 0, 0, 0);
    }
    QFont font = item.font;
    font.setPixelSize(13);
    painter->setFont(font);
    painter->setPen(!enabled ? QColor("#666660")
                            : selected ? QColor("#F0F0EA") : QColor("#BDBDB6"));
    const QString secondary = index.data(styling::SecondaryTextRole).toString();
    if (!secondary.isEmpty() && !grid) {
        QRect secondaryRect = textRect;
        secondaryRect.setTop(bounds.top() + 27);
        secondaryRect.setBottom(bounds.bottom() - 3);
        QFont secondaryFont = font;
        secondaryFont.setPixelSize(11);
        painter->setFont(secondaryFont);
        painter->setPen(QColor("#92928B"));
        painter->drawText(secondaryRect, Qt::AlignLeft | Qt::AlignVCenter,
                         painter->fontMetrics().elidedText(secondary, Qt::ElideMiddle,
                                                          secondaryRect.width()));
        textRect.setTop(bounds.top() + 3);
        textRect.setBottom(bounds.top() + 25);
        painter->setFont(font);
        painter->setPen(selected ? QColor("#F0F0EA") : QColor("#BDBDB6"));
    }
    QString text = item.text;
    const int shortcutAt = text.indexOf('\t');
    if (shortcutAt >= 0) {
        const QString shortcut = text.mid(shortcutAt + 1);
        text = text.left(shortcutAt);
        const int shortcutWidth = painter->fontMetrics().horizontalAdvance(shortcut);
        painter->setPen(QColor("#898983"));
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, shortcut);
        textRect.adjust(0, 0, -shortcutWidth - 24, 0);
        painter->setPen(selected ? QColor("#F0F0EA") : QColor("#BDBDB6"));
    }
    painter->drawText(textRect, grid ? Qt::AlignHCenter | Qt::AlignTop : Qt::AlignLeft | Qt::AlignVCenter,
                     painter->fontMetrics().elidedText(text, Qt::ElideMiddle, textRect.width()));
    painter->restore();
}

QSize styling::ItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    const QVariant explicitSize = index.data(Qt::SizeHintRole);
    if (explicitSize.isValid())
        return explicitSize.toSize();
    const auto *list = qobject_cast<const QListView *>(option.widget);
    if (list != nullptr && list->viewMode() == QListView::IconMode)
        return QSize(120, 102);
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(index.data(styling::SecondaryTextRole).toString().isEmpty() ? 34 : 52);
    return size;
}

void styling::TreeView::drawBranches(QPainter *painter, const QRect &rect,
                                     const QModelIndex &index) const {
    if (!model()->hasChildren(index))
        return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(QColor("#94948D"), 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const QPointF center(rect.right() - indentation() / 2.0, rect.center().y());
    QPainterPath path;
    path.moveTo(center + (isExpanded(index) ? QPointF(-3, -1.5) : QPointF(-1.5, -3)));
    path.lineTo(center + (isExpanded(index) ? QPointF(0, 1.5) : QPointF(1.5, 0)));
    path.lineTo(center + (isExpanded(index) ? QPointF(3, -1.5) : QPointF(-1.5, 3)));
    painter->drawPath(path);
    painter->restore();
}

void styling::installWorkbench(QApplication &app) {
    app.installEventFilter(new WorkbenchInstaller(&app));
}

styling::WorkspaceStack::WorkspaceStack(QWidget *parent) : QStackedWidget(parent) {
    layout()->setSizeConstraint(QLayout::SetNoConstraint);
    connect(this, &QStackedWidget::currentChanged, this, [this] { updateGeometry(); });
}

QSize styling::WorkspaceStack::minimumSizeHint() const {
    return currentWidget() != nullptr
               ? currentWidget()->minimumSizeHint().expandedTo(currentWidget()->minimumSize())
               : QSize(0, 0);
}

QSize styling::WorkspaceStack::sizeHint() const {
    return currentWidget() != nullptr ? currentWidget()->sizeHint() : QSize(640, 480);
}

styling::ElidedLabel::ElidedLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent) {
    setToolTip(text);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

QSize styling::ElidedLabel::minimumSizeHint() const {
    return QSize(0, fontMetrics().height());
}

QSize styling::ElidedLabel::sizeHint() const {
    return QSize(std::min(320, fontMetrics().horizontalAdvance(text())), fontMetrics().height());
}

void styling::ElidedLabel::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setFont(font());
    painter.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled,
                                   QPalette::WindowText));
    painter.drawText(contentsRect(), Qt::AlignLeft | Qt::AlignVCenter,
                     fontMetrics().elidedText(text(), Qt::ElideMiddle, contentsRect().width()));
}
