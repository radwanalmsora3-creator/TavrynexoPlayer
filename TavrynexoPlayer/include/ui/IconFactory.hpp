#pragma once
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QRectF>

namespace Tavrynexo::UI {

enum class IconKind {
    Play, Pause, Prev, Stop, Next, Fullscreen, Effects, Playlist,
    RepeatOff, RepeatAll, RepeatOne, Shuffle, VolumeHigh, VolumeLow,
    VolumeMute, OpenFile, OpenFolder, Subtitle, Search, Trash
};

class IconFactory {
public:
    static QIcon icon(IconKind kind, const QColor& color = QColor("#24292F"), int px = 18);
    static QIcon appIcon();
    static void paintCenterEmblem(QPainter& p, const QRectF& rect);
};

} // namespace Tavrynexo::UI
