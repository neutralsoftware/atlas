#include "editor/styling/icons.h"

#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QPixmap>
#include <QStringList>

namespace {

QString iconFamily;
bool iconFontAttempted = false;

ushort codepoint(styling::Icon icon) {
    switch (icon) {
    case styling::Icon::Aperture:
        return 0xE00A;
    case styling::Icon::ArrowClockwise:
        return 0xE036;
    case styling::Icon::ArrowCounterClockwise:
        return 0xE038;
    case styling::Icon::ArrowLeft:
        return 0xE058;
    case styling::Icon::ArrowRight:
    case styling::Icon::Assign:
        return 0xE06C;
    case styling::Icon::ArrowUp:
        return 0xE08E;
    case styling::Icon::ArrowsOutCardinal:
        return 0xE0A4;
    case styling::Icon::BoundingBox:
        return 0xE6CE;
    case styling::Icon::Camera:
        return 0xE10E;
    case styling::Icon::CaretDown:
        return 0xE136;
    case styling::Icon::CaretLeft:
        return 0xE138;
    case styling::Icon::CaretRight:
        return 0xE13A;
    case styling::Icon::CaretUp:
        return 0xE13C;
    case styling::Icon::Check:
        return 0xE182;
    case styling::Icon::Close:
        return 0xE4F6;
    case styling::Icon::Cloud:
        return 0xE1AA;
    case styling::Icon::Code:
        return 0xE1BC;
    case styling::Icon::Crosshair:
        return 0xE1D6;
    case styling::Icon::Cube:
        return 0xE1DA;
    case styling::Icon::CubeFocus:
        return 0xED0A;
    case styling::Icon::CursorClick:
        return 0xE7C8;
    case styling::Icon::Database:
        return 0xE1DE;
    case styling::Icon::DotsVertical:
        return 0xE208;
    case styling::Icon::Export:
        return 0xEAF0;
    case styling::Icon::Eye:
        return 0xE220;
    case styling::Icon::EyeSlash:
        return 0xE224;
    case styling::Icon::File:
        return 0xE230;
    case styling::Icon::FileCode:
        return 0xE914;
    case styling::Icon::FilmStrip:
        return 0xE792;
    case styling::Icon::FloppyDisk:
        return 0xE248;
    case styling::Icon::Folder:
        return 0xE24A;
    case styling::Icon::FolderOpen:
        return 0xE256;
    case styling::Icon::GameController:
        return 0xE26E;
    case styling::Icon::Gear:
        return 0xE270;
    case styling::Icon::Globe:
        return 0xE288;
    case styling::Icon::Hand:
        return 0xE298;
    case styling::Icon::HardDrives:
        return 0xE2A0;
    case styling::Icon::Image:
        return 0xE2CA;
    case styling::Icon::Layout:
        return 0xE6D6;
    case styling::Icon::Lightbulb:
        return 0xE2DC;
    case styling::Icon::MagnifyingGlass:
        return 0xE30C;
    case styling::Icon::Material:
        return 0xE6F0;
    case styling::Icon::Monitor:
        return 0xE32E;
    case styling::Icon::MonitorPlay:
        return 0xE58C;
    case styling::Icon::Mountains:
        return 0xE7AE;
    case styling::Icon::MusicNote:
        return 0xE33C;
    case styling::Icon::Package:
        return 0xE390;
    case styling::Icon::PaintBrush:
        return 0xE6F0;
    case styling::Icon::Palette:
        return 0xE6C8;
    case styling::Icon::Pause:
        return 0xE39E;
    case styling::Icon::Play:
        return 0xE3D0;
    case styling::Icon::Plus:
        return 0xE3D4;
    case styling::Icon::RocketLaunch:
        return 0xE3FE;
    case styling::Icon::Rows:
        return 0xE5A2;
    case styling::Icon::Sidebar:
        return 0xEAB6;
    case styling::Icon::SkipForward:
        return 0xE5A6;
    case styling::Icon::SlidersHorizontal:
        return 0xE434;
    case styling::Icon::Sparkle:
        return 0xE6A2;
    case styling::Icon::SpeakerHigh:
        return 0xE44A;
    case styling::Icon::Sphere:
        return 0xEE66;
    case styling::Icon::SquaresFour:
        return 0xE464;
    case styling::Icon::Stack:
        return 0xE466;
    case styling::Icon::Stop:
        return 0xE46C;
    case styling::Icon::Sun:
        return 0xE472;
    case styling::Icon::TerminalWindow:
        return 0xEAE8;
    case styling::Icon::Trash:
        return 0xE4A6;
    case styling::Icon::TreeStructure:
        return 0xE67C;
    case styling::Icon::Warning:
        return 0xE4E0;
    case styling::Icon::Waveform:
        return 0xE802;
    case styling::Icon::Wrench:
        return 0xE5D4;
    }
    return 0xE230;
}

QPixmap renderIcon(styling::Icon icon, const QColor &color, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setPen(color);
    QFont font(iconFamily);
    font.setPixelSize(qRound(size * 0.82));
    font.setStyleStrategy(QFont::PreferAntialias);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter,
                     QString(QChar(codepoint(icon))));
    return pixmap;
}

}

bool styling::loadIconFont() {
    if (iconFontAttempted)
        return !iconFamily.isEmpty();
    iconFontAttempted = true;
    const int id = QFontDatabase::addApplicationFont(
        ":/editor/assets/Phosphor.ttf");
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (!families.isEmpty())
        iconFamily = families.first();
    return !iconFamily.isEmpty();
}

QIcon styling::icon(Icon icon, const QColor &color) {
    loadIconFont();
    if (iconFamily.isEmpty())
        return {};
    QIcon result;
    const QColor disabled("#566174");
    const QColor active = color.lighter(118);
    for (const int size : {16, 20, 24, 32, 48}) {
        result.addPixmap(renderIcon(icon, color, size), QIcon::Normal,
                         QIcon::Off);
        result.addPixmap(renderIcon(icon, active, size), QIcon::Active,
                         QIcon::Off);
        result.addPixmap(renderIcon(icon, disabled, size), QIcon::Disabled,
                         QIcon::Off);
    }
    return result;
}
