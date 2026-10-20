#include "editor/styling/icons.h"

#include <QPainter>
#include <QHash>
#include <QPainterPath>
#include <QPixmap>
#include <QString>

namespace {

QString iconName(styling::Icon icon) {
    switch (icon) {
    case styling::Icon::Aperture:
        return QStringLiteral("aperture");
    case styling::Icon::ArrowClockwise:
        return QStringLiteral("rotate-cw");
    case styling::Icon::ArrowCounterClockwise:
        return QStringLiteral("rotate-ccw");
    case styling::Icon::ArrowLeft:
        return QStringLiteral("arrow-left");
    case styling::Icon::ArrowRight:
        return QStringLiteral("arrow-right");
    case styling::Icon::ArrowUp:
        return QStringLiteral("arrow-up");
    case styling::Icon::ArrowsOutCardinal:
        return QStringLiteral("move");
    case styling::Icon::Assign:
        return QStringLiteral("arrow-right");
    case styling::Icon::BoundingBox:
        return QStringLiteral("scan");
    case styling::Icon::Camera:
        return QStringLiteral("camera");
    case styling::Icon::CaretDown:
        return QStringLiteral("chevron-down");
    case styling::Icon::CaretLeft:
        return QStringLiteral("chevron-left");
    case styling::Icon::CaretRight:
        return QStringLiteral("chevron-right");
    case styling::Icon::CaretUp:
        return QStringLiteral("chevron-up");
    case styling::Icon::Check:
        return QStringLiteral("check");
    case styling::Icon::Close:
        return QStringLiteral("x");
    case styling::Icon::Cloud:
        return QStringLiteral("cloud");
    case styling::Icon::Code:
        return QStringLiteral("code");
    case styling::Icon::Component:
        return QStringLiteral("component");
    case styling::Icon::PanelRight:
        return QStringLiteral("panel-right");
    case styling::Icon::Crosshair:
        return QStringLiteral("crosshair");
    case styling::Icon::Cube:
        return QStringLiteral("box");
    case styling::Icon::CubeFocus:
        return QStringLiteral("scan-line");
    case styling::Icon::CubeTransparent:
        return QStringLiteral("boxes");
    case styling::Icon::CursorClick:
        return QStringLiteral("mouse-pointer-click");
    case styling::Icon::Database:
        return QStringLiteral("database");
    case styling::Icon::DotsVertical:
        return QStringLiteral("ellipsis-vertical");
    case styling::Icon::DotsNine:
        return QStringLiteral("grip");
    case styling::Icon::Export:
        return QStringLiteral("upload");
    case styling::Icon::Eye:
        return QStringLiteral("eye");
    case styling::Icon::EyeSlash:
        return QStringLiteral("eye-off");
    case styling::Icon::File:
        return QStringLiteral("file");
    case styling::Icon::FileCode:
        return QStringLiteral("file-code");
    case styling::Icon::FilmStrip:
        return QStringLiteral("film");
    case styling::Icon::FloppyDisk:
        return QStringLiteral("save");
    case styling::Icon::Folder:
        return QStringLiteral("folder");
    case styling::Icon::FolderOpen:
        return QStringLiteral("folder-open");
    case styling::Icon::GameController:
        return QStringLiteral("gamepad-2");
    case styling::Icon::Gear:
        return QStringLiteral("settings");
    case styling::Icon::Globe:
        return QStringLiteral("globe");
    case styling::Icon::Hand:
        return QStringLiteral("hand");
    case styling::Icon::HardDrives:
        return QStringLiteral("hard-drive");
    case styling::Icon::Image:
        return QStringLiteral("image");
    case styling::Icon::Info:
        return QStringLiteral("info");
    case styling::Icon::Layout:
        return QStringLiteral("panels-top-left");
    case styling::Icon::Lightbulb:
        return QStringLiteral("lightbulb");
    case styling::Icon::MagnifyingGlass:
        return QStringLiteral("search");
    case styling::Icon::Material:
        return QStringLiteral("swatch-book");
    case styling::Icon::Monitor:
        return QStringLiteral("monitor");
    case styling::Icon::MonitorPlay:
        return QStringLiteral("monitor-play");
    case styling::Icon::Mountains:
        return QStringLiteral("mountain");
    case styling::Icon::MusicNote:
        return QStringLiteral("music");
    case styling::Icon::Package:
        return QStringLiteral("package");
    case styling::Icon::PaintBrush:
        return QStringLiteral("paintbrush");
    case styling::Icon::Palette:
        return QStringLiteral("palette");
    case styling::Icon::Pause:
        return QStringLiteral("pause");
    case styling::Icon::Play:
        return QStringLiteral("play");
    case styling::Icon::Plus:
        return QStringLiteral("plus");
    case styling::Icon::RocketLaunch:
        return QStringLiteral("rocket");
    case styling::Icon::Rows:
        return QStringLiteral("rows-3");
    case styling::Icon::Sidebar:
        return QStringLiteral("panel-left");
    case styling::Icon::SkipForward:
        return QStringLiteral("skip-forward");
    case styling::Icon::SlidersHorizontal:
        return QStringLiteral("sliders-horizontal");
    case styling::Icon::Sparkle:
        return QStringLiteral("sparkles");
    case styling::Icon::SpeakerHigh:
        return QStringLiteral("volume-2");
    case styling::Icon::Sphere:
        return QStringLiteral("circle");
    case styling::Icon::SquaresFour:
        return QStringLiteral("grid-2x2");
    case styling::Icon::Stack:
        return QStringLiteral("layers");
    case styling::Icon::Stop:
        return QStringLiteral("square");
    case styling::Icon::Sun:
        return QStringLiteral("sun");
    case styling::Icon::TerminalWindow:
        return QStringLiteral("square-terminal");
    case styling::Icon::Trash:
        return QStringLiteral("trash-2");
    case styling::Icon::TreeStructure:
        return QStringLiteral("network");
    case styling::Icon::Warning:
        return QStringLiteral("triangle-alert");
    case styling::Icon::Waveform:
        return QStringLiteral("audio-lines");
    case styling::Icon::Wrench:
        return QStringLiteral("wrench");
    }
    return QStringLiteral("file");
}

QPixmap renderIcon(styling::Icon icon, const QColor &color, int size) {
    const QIcon source(QStringLiteral(":/editor/assets/lucide/%1.svg").arg(iconName(icon)));
    QPixmap pixmap = source.pixmap(size * 2, size * 2);
    if (pixmap.isNull())
        return pixmap;
    pixmap.setDevicePixelRatio(2.0);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(QRectF(0, 0, pixmap.width() / 2.0, pixmap.height() / 2.0), color);
    return pixmap;
}

}

QIcon styling::icon(Icon icon, const QColor &color) {
    static QHash<quint64, QIcon> cache;
    const quint64 key = (quint64(color.rgba()) << 32) | quint64(icon);
    const auto cached = cache.constFind(key);
    if (cached != cache.cend())
        return cached.value();
    QIcon result;
    const QColor disabled("#707070");
    const QColor active = color.lighter(118);
    for (const int size : {16, 20, 24, 32, 48}) {
        result.addPixmap(renderIcon(icon, color, size), QIcon::Normal,
                         QIcon::Off);
        result.addPixmap(renderIcon(icon, active, size), QIcon::Active,
                         QIcon::Off);
        result.addPixmap(renderIcon(icon, disabled, size), QIcon::Disabled,
                         QIcon::Off);
    }
    cache.insert(key, result);
    return result;
}

QIcon styling::colorSwatch(const QColor &color, const QSize &size) {
    const QSize swatchSize = size.expandedTo(QSize(8, 8));
    QPixmap pixmap(swatchSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF bounds = QRectF(pixmap.rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#D0D0D0"));
    painter.drawRoundedRect(bounds, 4.0, 4.0);
    painter.save();
    QPainterPath clip;
    clip.addRoundedRect(bounds, 4.0, 4.0);
    painter.setClipPath(clip);
    painter.fillRect(QRectF(bounds.left(), bounds.top(), bounds.width() / 2.0,
                            bounds.height() / 2.0),
                     QColor("#8A8A8A"));
    painter.fillRect(QRectF(bounds.center().x(), bounds.center().y(),
                            bounds.width() / 2.0, bounds.height() / 2.0),
                     QColor("#8A8A8A"));
    painter.fillRect(bounds, color);
    painter.restore();
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QColor("#5B5D60"));
    painter.drawRoundedRect(bounds, 4.0, 4.0);
    return QIcon(pixmap);
}

QPixmap styling::brandMark(const QSize &size) {
    QPixmap mark(":/editor/assets/atlas-star.png");
    mark = mark.scaled(size * 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    mark.setDevicePixelRatio(2.0);
    return mark;
}
