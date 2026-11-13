#pragma once
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QString>
#include <QTimer>
#include <atomic>
#include <mpv/client.h>
#include <mpv/render_gl.h>

namespace Tavrynexo::Player {

struct MediaTrack {
    int id{0};
    QString type, title, lang, codec;
    bool selected{false};
    bool external{false};
};

class MpvVideoWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit MpvVideoWidget(QWidget* parent = nullptr);
    ~MpvVideoWidget() override;

    bool isMediaLoaded() const { return m_hasMedia; }
    bool isPaused() const { return m_paused; }
    double position() const { return m_position; }
    double duration() const { return m_duration; }
    int volume() const { return m_volume; }
    bool isMuted() const { return m_muted; }
    double speed() const { return m_speed; }
    QString currentFile() const { return m_currentFile; }
    QString mediaTitle() const { return m_mediaTitle; }
    int videoWidth() const { return m_videoW; }
    int videoHeight() const { return m_videoH; }
    QString hwdecCurrent() const { return m_hwdecCurrent; }
    QList<MediaTrack> tracks() const;

public slots:
    void loadFile(const QString& pathOrUrl, double resumeSeconds = 0.0);
    void stopPlayback();
    void togglePause();
    void setPaused(bool paused);
    void seekAbsolute(double seconds);
    void seekRelative(double deltaSeconds);
    void frameStep();
    void setVolume(int percent);
    void setMuted(bool muted);
    void setSpeed(double speed);
    void setAspectRatio(const QString& ratio);
    void setDeinterlace(bool enabled);
    void setHwdec(const QString& mode);
    void addSubtitleFile(const QString& path);
    void setAudioTrack(int id);
    void setSubtitleTrack(int id);
    void setAudioDelay(double seconds);
    void setSubtitleDelay(double seconds);
    void setSubtitleFontSize(int size);
    void takeSnapshot(const QString& directory);
    void showOsd(const QString& text);

signals:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void pausedChanged(bool paused);
    void volumeChanged(int percent);
    void mutedChanged(bool muted);
    void speedChanged(double speed);
    void mediaStateChanged(bool loaded);
    void mediaTitleChanged(const QString& title);
    void eofReached();
    void tracksChanged();
    void videoInfoChanged();
    void playbackError(const QString& message);
    void filesDropped(const QStringList& paths);
    void doubleClicked();
    void contextMenuRequested(const QPoint& globalPos);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void processMpvEvents();
    void maybeUpdate();
    void cleanupRenderContext();

private:
    static void onMpvWakeup(void* ctx);
    static void onMpvRenderUpdate(void* ctx);
    static void* getProcAddress(void* ctx, const char* name);
    void handlePropertyChange(mpv_event_property* prop, uint64_t id);
    void cleanupAll();
    void layoutOverlays();

    mpv_handle* m_mpv{nullptr};
    mpv_render_context* m_mpvGl{nullptr};
    std::atomic<bool> m_shuttingDown{false};
    bool m_hasMedia{false}, m_paused{false}, m_muted{false}, m_eofSignaled{false};
    double m_position{0.0}, m_duration{0.0}, m_speed{1.0}, m_pendingResumeSec{0.0};
    int m_volume{100}, m_videoW{0}, m_videoH{0};
    QString m_currentFile, m_mediaTitle, m_hwdecCurrent;
    QWidget* m_idleOverlay{nullptr};
    QLabel* m_osdLabel{nullptr};
    QTimer* m_osdTimer{nullptr};
};

} // namespace Tavrynexo::Player
