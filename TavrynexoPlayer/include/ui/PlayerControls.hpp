#pragma once
#include <QSlider>
#include <QWidget>

namespace Tavrynexo::UI {

class SeekSlider : public QSlider {
    Q_OBJECT
public:
    explicit SeekSlider(QWidget* parent = nullptr);
    void setDuration(double seconds) { m_duration = seconds; update(); }
    void setThemeDark(bool dark) { m_dark = dark; update(); }

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int valueFromPos(int x) const;
    double m_duration{0.0};
    bool m_dark{false};
};

class WedgeVolumeSlider : public QWidget {
    Q_OBJECT
public:
    explicit WedgeVolumeSlider(QWidget* parent = nullptr);
    int volume() const { return m_volume; }
    void setVolume(int vol);
    void setMaxVolume(int maxVol);
    void setThemeDark(bool dark) { m_dark = dark; update(); }

signals:
    void volumeChanged(int vol);

protected:
    QSize sizeHint() const override { return QSize(112, 26); }
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateFromX(int x);
    int m_volume{100};
    int m_maxVolume{150};
    bool m_dark{false};
};

} // namespace Tavrynexo::UI
