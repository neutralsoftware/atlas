#ifndef ATLAS_EDITOR_ICONS_H
#define ATLAS_EDITOR_ICONS_H

#include <QColor>
#include <QIcon>

namespace styling {

enum class Icon {
    Aperture,
    ArrowClockwise,
    ArrowCounterClockwise,
    ArrowLeft,
    ArrowRight,
    ArrowUp,
    ArrowsOutCardinal,
    Assign,
    BoundingBox,
    Camera,
    CaretDown,
    CaretLeft,
    CaretRight,
    CaretUp,
    Check,
    Close,
    Cloud,
    Code,
    Crosshair,
    Cube,
    CubeFocus,
    CursorClick,
    Database,
    DotsVertical,
    Export,
    Eye,
    EyeSlash,
    File,
    FileCode,
    FilmStrip,
    FloppyDisk,
    Folder,
    FolderOpen,
    GameController,
    Gear,
    Globe,
    Hand,
    HardDrives,
    Image,
    Info,
    Layout,
    Lightbulb,
    MagnifyingGlass,
    Material,
    Monitor,
    MonitorPlay,
    Mountains,
    MusicNote,
    Package,
    PaintBrush,
    Palette,
    Pause,
    Play,
    Plus,
    RocketLaunch,
    Rows,
    Sidebar,
    SkipForward,
    SlidersHorizontal,
    Sparkle,
    SpeakerHigh,
    Sphere,
    SquaresFour,
    Stack,
    Stop,
    Sun,
    TerminalWindow,
    Trash,
    TreeStructure,
    Warning,
    Waveform,
    Wrench
};

bool loadIconFont();
QIcon icon(Icon icon, const QColor &color = QColor("#AAB4C4"));

}

#endif
