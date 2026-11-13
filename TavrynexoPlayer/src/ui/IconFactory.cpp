#include "ui/IconFactory.hpp"
#include <QLinearGradient>
#include <QPainterPath>
#include <QPixmap>

namespace Tavrynexo::UI {

static void drawVector(QPainter& p, IconKind kind, const QColor& c, qreal s) {
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(c, s * 0.105, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    auto u = [s](qreal v) { return v * s; };

    switch (kind) {
    case IconKind::Play: {
        p.setPen(Qt::NoPen); p.setBrush(c);
        QPainterPath path; path.moveTo(u(0.28), u(0.18)); path.lineTo(u(0.80), u(0.50)); path.lineTo(u(0.28), u(0.82)); path.closeSubpath();
        p.drawPath(path); break;
    }
    case IconKind::Pause:
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawRoundedRect(QRectF(u(0.24), u(0.20), u(0.18), u(0.60)), u(0.05), u(0.05));
        p.drawRoundedRect(QRectF(u(0.58), u(0.20), u(0.18), u(0.60)), u(0.05), u(0.05)); break;
    case IconKind::Stop:
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawRoundedRect(QRectF(u(0.24), u(0.24), u(0.52), u(0.52)), u(0.06), u(0.06)); break;
    case IconKind::Prev: {
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawRect(QRectF(u(0.18), u(0.24), u(0.10), u(0.52)));
        QPainterPath t1, t2;
        t1.moveTo(u(0.54), u(0.24)); t1.lineTo(u(0.28), u(0.50)); t1.lineTo(u(0.54), u(0.76)); t1.closeSubpath();
        t2.moveTo(u(0.82), u(0.24)); t2.lineTo(u(0.56), u(0.50)); t2.lineTo(u(0.82), u(0.76)); t2.closeSubpath();
        p.drawPath(t1); p.drawPath(t2); break;
    }
    case IconKind::Next: {
        p.setPen(Qt::NoPen); p.setBrush(c);
        QPainterPath t1, t2;
        t1.moveTo(u(0.18), u(0.24)); t1.lineTo(u(0.44), u(0.50)); t1.lineTo(u(0.18), u(0.76)); t1.closeSubpath();
        t2.moveTo(u(0.46), u(0.24)); t2.lineTo(u(0.72), u(0.50)); t2.lineTo(u(0.46), u(0.76)); t2.closeSubpath();
        p.drawPath(t1); p.drawPath(t2);
        p.drawRect(QRectF(u(0.72), u(0.24), u(0.10), u(0.52))); break;
    }
    case IconKind::Fullscreen:
        p.drawPolyline(QPolygonF({{u(0.18), u(0.38)}, {u(0.18), u(0.18)}, {u(0.38), u(0.18)}}));
        p.drawPolyline(QPolygonF({{u(0.62), u(0.18)}, {u(0.82), u(0.18)}, {u(0.82), u(0.38)}}));
        p.drawPolyline(QPolygonF({{u(0.82), u(0.62)}, {u(0.82), u(0.82)}, {u(0.62), u(0.82)}}));
        p.drawPolyline(QPolygonF({{u(0.38), u(0.82)}, {u(0.18), u(0.82)}, {u(0.18), u(0.62)}})); break;
    case IconKind::Effects:
        p.drawLine(QPointF(u(0.26), u(0.18)), QPointF(u(0.26), u(0.82)));
        p.drawLine(QPointF(u(0.50), u(0.18)), QPointF(u(0.50), u(0.82)));
        p.drawLine(QPointF(u(0.74), u(0.18)), QPointF(u(0.74), u(0.82)));
        p.setBrush(c);
        p.drawEllipse(QPointF(u(0.26), u(0.36)), u(0.08), u(0.08));
        p.drawEllipse(QPointF(u(0.50), u(0.64)), u(0.08), u(0.08));
        p.drawEllipse(QPointF(u(0.74), u(0.42)), u(0.08), u(0.08)); break;
    case IconKind::Playlist:
        p.drawLine(QPointF(u(0.20), u(0.28)), QPointF(u(0.82), u(0.28)));
        p.drawLine(QPointF(u(0.20), u(0.50)), QPointF(u(0.82), u(0.50)));
        p.drawLine(QPointF(u(0.20), u(0.72)), QPointF(u(0.62), u(0.72))); break;
    case IconKind::RepeatOff:
    case IconKind::RepeatAll:
    case IconKind::RepeatOne:
        p.drawPolyline(QPolygonF({{u(0.20), u(0.45)}, {u(0.20), u(0.28)}, {u(0.78), u(0.28)}}));
        p.drawPolyline(QPolygonF({{u(0.66), u(0.18)}, {u(0.80), u(0.28)}, {u(0.66), u(0.38)}}));
        p.drawPolyline(QPolygonF({{u(0.80), u(0.55)}, {u(0.80), u(0.72)}, {u(0.22), u(0.72)}}));
        p.drawPolyline(QPolygonF({{u(0.34), u(0.62)}, {u(0.20), u(0.72)}, {u(0.34), u(0.82)}}));
        if (kind == IconKind::RepeatOne) {
            QFont f = p.font(); f.setPixelSize(qMax(8, int(s * 0.36))); f.setBold(true); p.setFont(f);
            p.drawText(QRectF(0, 0, s, s), Qt::AlignCenter, "1");
        }
        break;
    case IconKind::Shuffle:
        p.drawLine(QPointF(u(0.18), u(0.30)), QPointF(u(0.42), u(0.30)));
        p.drawLine(QPointF(u(0.42), u(0.30)), QPointF(u(0.62), u(0.70)));
        p.drawLine(QPointF(u(0.62), u(0.70)), QPointF(u(0.82), u(0.70)));
        p.drawLine(QPointF(u(0.18), u(0.70)), QPointF(u(0.42), u(0.70)));
        p.drawLine(QPointF(u(0.42), u(0.70)), QPointF(u(0.62), u(0.30)));
        p.drawLine(QPointF(u(0.62), u(0.30)), QPointF(u(0.82), u(0.30)));
        p.drawPolyline(QPolygonF({{u(0.72), u(0.20)}, {u(0.84), u(0.30)}, {u(0.72), u(0.40)}}));
        p.drawPolyline(QPolygonF({{u(0.72), u(0.60)}, {u(0.84), u(0.70)}, {u(0.72), u(0.80)}})); break;
    case IconKind::VolumeHigh:
    case IconKind::VolumeLow:
    case IconKind::VolumeMute: {
        p.setBrush(c); p.setPen(Qt::NoPen);
        QPainterPath spk;
        spk.moveTo(u(0.16), u(0.38)); spk.lineTo(u(0.32), u(0.38)); spk.lineTo(u(0.50), u(0.22));
        spk.lineTo(u(0.50), u(0.78)); spk.lineTo(u(0.32), u(0.62)); spk.lineTo(u(0.16), u(0.62)); spk.closeSubpath();
        p.drawPath(spk); p.setBrush(Qt::NoBrush); p.setPen(pen);
        if (kind == IconKind::VolumeMute) {
            p.setPen(QPen(QColor("#DC2626"), s * 0.11, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(u(0.60), u(0.36)), QPointF(u(0.84), u(0.64)));
            p.drawLine(QPointF(u(0.84), u(0.36)), QPointF(u(0.60), u(0.64)));
        } else {
            p.drawArc(QRectF(u(0.44), u(0.34), u(0.24), u(0.32)), -50 * 16, 100 * 16);
            if (kind == IconKind::VolumeHigh) p.drawArc(QRectF(u(0.46), u(0.22), u(0.38), u(0.56)), -55 * 16, 110 * 16);
        }
        break;
    }
    case IconKind::OpenFile:
        p.drawRoundedRect(QRectF(u(0.22), u(0.18), u(0.56), u(0.64)), u(0.06), u(0.06));
        p.drawLine(QPointF(u(0.34), u(0.42)), QPointF(u(0.66), u(0.42)));
        p.drawLine(QPointF(u(0.34), u(0.60)), QPointF(u(0.56), u(0.60))); break;
    case IconKind::OpenFolder: {
        QPainterPath path;
        path.moveTo(u(0.16), u(0.30)); path.lineTo(u(0.42), u(0.30)); path.lineTo(u(0.48), u(0.38));
        path.lineTo(u(0.84), u(0.38)); path.lineTo(u(0.84), u(0.76)); path.lineTo(u(0.16), u(0.76)); path.closeSubpath();
        p.drawPath(path); break;
    }
    case IconKind::Subtitle:
        p.drawRoundedRect(QRectF(u(0.14), u(0.24), u(0.72), u(0.52)), u(0.08), u(0.08));
        p.drawLine(QPointF(u(0.26), u(0.46)), QPointF(u(0.54), u(0.46)));
        p.drawLine(QPointF(u(0.62), u(0.46)), QPointF(u(0.74), u(0.46)));
        p.drawLine(QPointF(u(0.26), u(0.60)), QPointF(u(0.74), u(0.60))); break;
    case IconKind::Search:
        p.drawEllipse(QRectF(u(0.18), u(0.18), u(0.46), u(0.46)));
        p.drawLine(QPointF(u(0.58), u(0.58)), QPointF(u(0.82), u(0.82))); break;
    case IconKind::Trash:
        p.drawLine(QPointF(u(0.22), u(0.28)), QPointF(u(0.78), u(0.28)));
        p.drawRect(QRectF(u(0.30), u(0.28), u(0.40), u(0.52)));
        p.drawLine(QPointF(u(0.40), u(0.18)), QPointF(u(0.60), u(0.18))); break;
    }
}

QIcon IconFactory::icon(IconKind kind, const QColor& color, int px) {
    QIcon ic;
    for (int scale : {1, 2}) {
        QPixmap pm(px * scale, px * scale);
        pm.fill(Qt::transparent);
        pm.setDevicePixelRatio(scale);
        QPainter p(&pm);
        drawVector(p, kind, color, px);
        ic.addPixmap(pm);
    }
    return ic;
}

void IconFactory::paintCenterEmblem(QPainter& p, const QRectF& rect) {
    p.save(); p.setRenderHint(QPainter::Antialiasing, true);
    qreal s = qMin(rect.width(), rect.height());
    qreal x0 = rect.center().x() - s * 0.5, y0 = rect.center().y() - s * 0.5;
    auto pt = [&](qreal nx, qreal ny) { return QPointF(x0 + nx * s, y0 + ny * s); };
    QLinearGradient baseG(pt(0.1, 0.78), pt(0.9, 0.90));
    baseG.setColorAt(0.0, QColor("#FF8C1A")); baseG.setColorAt(1.0, QColor("#D95300"));
    QPainterPath base; base.addRoundedRect(QRectF(pt(0.12, 0.76), pt(0.88, 0.89)), s * 0.04, s * 0.04);
    p.setPen(Qt::NoPen); p.setBrush(baseG); p.drawPath(base);

    QLinearGradient coneG(pt(0.30, 0.12), pt(0.72, 0.82));
    coneG.setColorAt(0.0, QColor("#FFA726")); coneG.setColorAt(0.5, QColor("#FF6D00")); coneG.setColorAt(1.0, QColor("#E65100"));
    QPainterPath cone;
    cone.moveTo(pt(0.46, 0.12)); cone.lineTo(pt(0.54, 0.12)); cone.lineTo(pt(0.76, 0.82)); cone.lineTo(pt(0.24, 0.82)); cone.closeSubpath();
    p.setBrush(coneG); p.drawPath(cone);
    p.setClipPath(cone); p.setBrush(QColor("#F8FAFC"));
    p.drawRect(QRectF(pt(0.20, 0.30), pt(0.80, 0.43)));
    p.drawRect(QRectF(pt(0.20, 0.56), pt(0.80, 0.69)));
    p.restore();
}

QIcon IconFactory::appIcon() {
    QIcon ic;
    for (int sz : {16, 32, 48, 64}) {
        QPixmap pm(sz, sz); pm.fill(Qt::transparent);
        QPainter p(&pm); paintCenterEmblem(p, QRectF(0, 0, sz, sz)); ic.addPixmap(pm);
    }
    return ic;
}

} // namespace Tavrynexo::UI
