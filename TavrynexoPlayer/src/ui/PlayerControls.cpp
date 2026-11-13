#include "ui/PlayerControls.hpp"
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <QWheelEvent>

namespace Tavrynexo::UI {

static QString fmtTime(double sec) {
    if (sec < 0.0) sec = 0.0;
    qint64 s = qRound64(sec);
    qint64 h = s / 3600, m = (s % 3600) / 60, r = s % 60;
    if (h > 0) return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(r, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(r, 2, 10, QChar('0'));
}

SeekSlider::SeekSlider(QWidget* parent) : QSlider(Qt::Horizontal, parent) {
    setRange(0, 10000);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(20);
}

int SeekSlider::valueFromPos(int x) const {
    const int margin = 7;
    const int span = qMax(1, width() - 2 * margin);
    double ratio = qBound(0.0, double(x - margin) / span, 1.0);
    return qRound(ratio * maximum());
}

void SeekSlider::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        int v = valueFromPos(e->pos().x());
        setValue(v);
        emit sliderPressed();
        emit sliderMoved(v);
        e->accept();
        return;
    }
    QSlider::mousePressEvent(e);
}

void SeekSlider::mouseMoveEvent(QMouseEvent* e) {
    int v = valueFromPos(e->pos().x());
    if (m_duration > 0.0) {
        double targetSec = (double(v) / maximum()) * m_duration;
        QToolTip::showText(e->globalPosition().toPoint(), fmtTime(targetSec), this);
    }
    if (e->buttons() & Qt::LeftButton) {
        setValue(v);
        emit sliderMoved(v);
        e->accept();
        return;
    }
    QSlider::mouseMoveEvent(e);
}

void SeekSlider::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const int margin = 7;
    const int trackH = 5;
    const int y = (height() - trackH) / 2;
    const int span = qMax(1, width() - 2 * margin);
    double ratio = maximum() > 0 ? qBound(0.0, double(value()) / maximum(), 1.0) : 0.0;
    int fillW = qRound(span * ratio);

    QColor bg = m_dark ? QColor("#334155") : QColor("#D5D8DE");
    QColor border = m_dark ? QColor("#475569") : QColor("#B9BFC9");
    QColor fill = m_dark ? QColor("#FF7A00") : QColor("#257AFD");

    p.setPen(QPen(border, 1));
    p.setBrush(bg);
    p.drawRoundedRect(QRectF(margin, y, span, trackH), 2.5, 2.5);

    if (fillW > 0) {
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawRoundedRect(QRectF(margin, y, fillW, trackH), 2.5, 2.5);
    }

    if (m_duration > 0.0 || underMouse()) {
        qreal cx = margin + fillW;
        qreal cy = height() * 0.5;
        p.setPen(QPen(fill, 1.5));
        p.setBrush(Qt::white);
        p.drawEllipse(QPointF(cx, cy), 5.5, 5.5);
    }
}

WedgeVolumeSlider::WedgeVolumeSlider(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(112, 26);
}

void WedgeVolumeSlider::setVolume(int vol) {
    int clamped = qBound(0, vol, m_maxVolume);
    if (m_volume != clamped) { m_volume = clamped; update(); }
}

void WedgeVolumeSlider::setMaxVolume(int maxVol) {
    m_maxVolume = qBound(100, maxVol, 150);
    m_volume = qMin(m_volume, m_maxVolume);
    update();
}

void WedgeVolumeSlider::updateFromX(int x) {
    const int pad = 5;
    const int span = qMax(1, width() - 2 * pad);
    double ratio = qBound(0.0, double(x - pad) / span, 1.0);
    int v = qRound(ratio * m_maxVolume);
    if (v != m_volume) { m_volume = v; update(); emit volumeChanged(m_volume); }
}

void WedgeVolumeSlider::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) { updateFromX(e->pos().x()); e->accept(); }
}

void WedgeVolumeSlider::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) { updateFromX(e->pos().x()); e->accept(); }
}

void WedgeVolumeSlider::wheelEvent(QWheelEvent* e) {
    int delta = e->angleDelta().y() > 0 ? 5 : -5;
    int nv = qBound(0, m_volume + delta, m_maxVolume);
    if (nv != m_volume) { m_volume = nv; update(); emit volumeChanged(m_volume); }
    e->accept();
}

void WedgeVolumeSlider::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal l = 5.0, r = width() - 5.0, b = height() - 3.0, t = 5.0;
    QPainterPath wedge;
    wedge.moveTo(l, b - 2.0);
    wedge.lineTo(r, t);
    wedge.lineTo(r, b);
    wedge.lineTo(l, b);
    wedge.closeSubpath();

    p.setPen(QPen(m_dark ? QColor("#475569") : QColor("#B4BAC4"), 1.0));
    p.setBrush(m_dark ? QColor("#1E293B") : QColor("#E2E5EA"));
    p.drawPath(wedge);

    double ratio = m_maxVolume > 0 ? qBound(0.0, double(m_volume) / m_maxVolume, 1.0) : 0.0;
    qreal fillX = l + (r - l) * ratio;

    p.save();
    p.setClipPath(wedge);
    QLinearGradient grad(l, 0, r, 0);
    grad.setColorAt(0.0, QColor("#22C55E"));
    grad.setColorAt(0.66, QColor("#84CC16"));
    grad.setColorAt(0.82, QColor("#F59E0B"));
    grad.setColorAt(1.0, QColor("#EF4444"));
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawRect(QRectF(l, 0, fillX - l, height()));
    p.restore();

    p.setPen(m_dark ? QColor("#E2E8F0") : QColor("#374151"));
    QFont f = p.font(); f.setPixelSize(10); f.setBold(true); p.setFont(f);
    p.drawText(QRectF(l, 0, 42, 13), Qt::AlignLeft | Qt::AlignVCenter, QString("%1%").arg(m_volume));
}

} // namespace Tavrynexo::UI
