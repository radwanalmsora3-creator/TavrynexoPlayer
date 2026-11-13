#include "player/MpvVideoWidget.hpp"
#include "ui/IconFactory.hpp"
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMetaObject>
#include <QMimeData>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPainter>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>
#include <cmath>

namespace Tavrynexo::Player {

namespace {
class IdleOverlayWidget : public QWidget {
public:
    explicit IdleOverlayWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);
        qreal size = qBound(76.0, qMin(width(), height()) * 0.22, 130.0);
        QRectF r((width() - size) * 0.5, (height() - size) * 0.5 - 10.0, size, size);
        Tavrynexo::UI::IconFactory::paintCenterEmblem(p, r);
        p.setPen(QColor("#6B7280"));
        QFont f = p.font(); f.setPointSize(9); p.setFont(f);
        p.drawText(QRectF(20, r.bottom() + 14, width() - 40, 24),
                   Qt::AlignHCenter | Qt::AlignTop,
                   "Drop video or audio files here, or open via Media > Open File");
    }
};
} // namespace

MpvVideoWidget::MpvVideoWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_idleOverlay = new IdleOverlayWidget(this);
    m_idleOverlay->show();

    m_osdLabel = new QLabel(this);
    m_osdLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_osdLabel->setStyleSheet(
        "QLabel { background: rgba(15,17,23,205); color: #F9FAFB; "
        "border: 1px solid rgba(255,255,255,45); border-radius: 6px; "
        "padding: 6px 12px; font-size: 13px; font-weight: 600; }");
    m_osdLabel->hide();

    m_osdTimer = new QTimer(this);
    m_osdTimer->setSingleShot(true);
    connect(m_osdTimer, &QTimer::timeout, m_osdLabel, &QWidget::hide);

    m_mpv = mpv_create();
    if (!m_mpv) return;

    mpv_set_option_string(m_mpv, "terminal", "no");
    mpv_set_option_string(m_mpv, "msg-level", "all=warn");
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(m_mpv, "ytdl", "no");
    mpv_set_option_string(m_mpv, "volume-max", "150");
    mpv_set_option_string(m_mpv, "sub-auto", "fuzzy");

    QString pics = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (!pics.isEmpty()) {
        QByteArray ba = QDir::toNativeSeparators(pics).toUtf8();
        mpv_set_option_string(m_mpv, "screenshot-directory", ba.constData());
    }

    if (mpv_initialize(m_mpv) < 0) {
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return;
    }

    mpv_observe_property(m_mpv, 1, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 2, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 3, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 4, "eof-reached", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 5, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 6, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 7, "speed", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 8, "media-title", MPV_FORMAT_STRING);
    mpv_observe_property(m_mpv, 9, "video-params/w", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpv, 10, "video-params/h", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpv, 11, "hwdec-current", MPV_FORMAT_STRING);
    mpv_observe_property(m_mpv, 12, "track-list/count", MPV_FORMAT_INT64);
    mpv_set_wakeup_callback(m_mpv, &MpvVideoWidget::onMpvWakeup, this);
}

MpvVideoWidget::~MpvVideoWidget() { cleanupAll(); }

void MpvVideoWidget::cleanupAll() {
    m_shuttingDown.store(true);
    if (m_mpvGl) mpv_render_context_set_update_callback(m_mpvGl, nullptr, nullptr);
    if (m_mpv) mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);
    if (m_mpvGl && context()) {
        makeCurrent(); mpv_render_context_free(m_mpvGl); m_mpvGl = nullptr; doneCurrent();
    } else if (m_mpvGl) {
        mpv_render_context_free(m_mpvGl); m_mpvGl = nullptr;
    }
    if (m_mpv) { mpv_terminate_destroy(m_mpv); m_mpv = nullptr; }
}

void MpvVideoWidget::cleanupRenderContext() {
    if (m_mpvGl) {
        mpv_render_context_set_update_callback(m_mpvGl, nullptr, nullptr);
        mpv_render_context_free(m_mpvGl); m_mpvGl = nullptr;
    }
}

void MpvVideoWidget::initializeGL() {
    initializeOpenGLFunctions();
    if (!m_mpv || m_mpvGl) return;
    if (context()) {
        connect(context(), &QOpenGLContext::aboutToBeDestroyed,
                this, &MpvVideoWidget::cleanupRenderContext, Qt::DirectConnection);
    }
    mpv_opengl_init_params glInit{};
    glInit.get_proc_address = &MpvVideoWidget::getProcAddress;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    if (mpv_render_context_create(&m_mpvGl, m_mpv, params) < 0) {
        emit playbackError("Failed to initialize OpenGL video renderer.");
        return;
    }
    mpv_render_context_set_update_callback(m_mpvGl, &MpvVideoWidget::onMpvRenderUpdate, this);
}

void MpvVideoWidget::paintGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_mpvGl || !m_hasMedia || m_shuttingDown.load()) return;

    const qreal dpr = devicePixelRatioF();
    mpv_opengl_fbo fbo{};
    fbo.fbo = static_cast<int>(defaultFramebufferObject());
    fbo.w = qMax(1, static_cast<int>(std::lround(width() * dpr)));
    fbo.h = qMax(1, static_cast<int>(std::lround(height() * dpr)));
    int flipY = 1;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    mpv_render_context_render(m_mpvGl, params);
}

void MpvVideoWidget::resizeEvent(QResizeEvent* event) {
    QOpenGLWidget::resizeEvent(event);
    layoutOverlays();
}

void MpvVideoWidget::layoutOverlays() {
    if (m_idleOverlay) m_idleOverlay->setGeometry(rect());
    if (m_osdLabel && m_osdLabel->isVisible()) {
        m_osdLabel->adjustSize();
        m_osdLabel->move(width() - m_osdLabel->width() - 18, 18);
    }
}

void MpvVideoWidget::showOsd(const QString& text) {
    if (text.isEmpty()) return;
    m_osdLabel->setText(text);
    m_osdLabel->adjustSize();
    m_osdLabel->move(width() - m_osdLabel->width() - 18, 18);
    m_osdLabel->show(); m_osdLabel->raise();
    m_osdTimer->start(1800);
}

void MpvVideoWidget::onMpvWakeup(void* ctx) {
    auto* self = static_cast<MpvVideoWidget*>(ctx);
    if (!self || self->m_shuttingDown.load()) return;
    QPointer<MpvVideoWidget> guard(self);
    QMetaObject::invokeMethod(self, [guard]() {
        if (guard && !guard->m_shuttingDown.load()) guard->processMpvEvents();
    }, Qt::QueuedConnection);
}

void MpvVideoWidget::onMpvRenderUpdate(void* ctx) {
    auto* self = static_cast<MpvVideoWidget*>(ctx);
    if (!self || self->m_shuttingDown.load()) return;
    QPointer<MpvVideoWidget> guard(self);
    QMetaObject::invokeMethod(self, [guard]() {
        if (guard && !guard->m_shuttingDown.load()) guard->maybeUpdate();
    }, Qt::QueuedConnection);
}

void MpvVideoWidget::maybeUpdate() { if (!m_shuttingDown.load()) update(); }

void* MpvVideoWidget::getProcAddress(void*, const char* name) {
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    return ctx ? reinterpret_cast<void*>(ctx->getProcAddress(name)) : nullptr;
}

void MpvVideoWidget::processMpvEvents() {
    while (m_mpv && !m_shuttingDown.load()) {
        mpv_event* ev = mpv_wait_event(m_mpv, 0);
        if (!ev || ev->event_id == MPV_EVENT_NONE) break;
        switch (ev->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE:
            handlePropertyChange(static_cast<mpv_event_property*>(ev->data), ev->reply_userdata);
            break;
        case MPV_EVENT_FILE_LOADED:
            m_hasMedia = true; m_eofSignaled = false;
            emit mediaStateChanged(true); emit tracksChanged();
            if (m_pendingResumeSec > 1.0) { seekAbsolute(m_pendingResumeSec); m_pendingResumeSec = 0.0; }
            update(); break;
        case MPV_EVENT_END_FILE: {
            auto* ef = static_cast<mpv_event_end_file*>(ev->data);
            if (ef && ef->reason == MPV_END_FILE_REASON_ERROR)
                emit playbackError(QString::fromUtf8(mpv_error_string(ef->error)));
            if (ef && (ef->reason == MPV_END_FILE_REASON_STOP || ef->reason == MPV_END_FILE_REASON_ERROR)) {
                m_hasMedia = false; m_position = 0.0; m_duration = 0.0; m_videoW = 0; m_videoH = 0;
                if (m_idleOverlay) m_idleOverlay->show();
                emit mediaStateChanged(false); emit positionChanged(0.0); emit durationChanged(0.0); update();
            }
            break;
        }
        default: break;
        }
    }
}

void MpvVideoWidget::handlePropertyChange(mpv_event_property* prop, uint64_t id) {
    if (!prop || !prop->data) return;
    switch (id) {
    case 1: if (prop->format == MPV_FORMAT_DOUBLE) { m_position = *static_cast<double*>(prop->data); emit positionChanged(m_position); } break;
    case 2: if (prop->format == MPV_FORMAT_DOUBLE) { m_duration = *static_cast<double*>(prop->data); emit durationChanged(m_duration); } break;
    case 3: if (prop->format == MPV_FORMAT_FLAG) { m_paused = (*static_cast<int*>(prop->data) != 0); emit pausedChanged(m_paused); } break;
    case 4: if (prop->format == MPV_FORMAT_FLAG) {
        bool eof = (*static_cast<int*>(prop->data) != 0);
        if (eof && !m_eofSignaled && m_hasMedia) { m_eofSignaled = true; emit eofReached(); }
        else if (!eof) m_eofSignaled = false;
    } break;
    case 5: if (prop->format == MPV_FORMAT_DOUBLE) { m_volume = qRound(*static_cast<double*>(prop->data)); emit volumeChanged(m_volume); } break;
    case 6: if (prop->format == MPV_FORMAT_FLAG) { m_muted = (*static_cast<int*>(prop->data) != 0); emit mutedChanged(m_muted); } break;
    case 7: if (prop->format == MPV_FORMAT_DOUBLE) { m_speed = *static_cast<double*>(prop->data); emit speedChanged(m_speed); } break;
    case 8: if (prop->format == MPV_FORMAT_STRING) { m_mediaTitle = QString::fromUtf8(*static_cast<char**>(prop->data)); emit mediaTitleChanged(m_mediaTitle); } break;
    case 9: if (prop->format == MPV_FORMAT_INT64) {
        m_videoW = static_cast<int>(*static_cast<int64_t*>(prop->data));
        if (m_idleOverlay) m_idleOverlay->setVisible(!m_hasMedia || m_videoW <= 0);
        emit videoInfoChanged();
    } break;
    case 10: if (prop->format == MPV_FORMAT_INT64) { m_videoH = static_cast<int>(*static_cast<int64_t*>(prop->data)); emit videoInfoChanged(); } break;
    case 11: if (prop->format == MPV_FORMAT_STRING) { m_hwdecCurrent = QString::fromUtf8(*static_cast<char**>(prop->data)); emit videoInfoChanged(); } break;
    case 12: emit tracksChanged(); break;
    }
}

void MpvVideoWidget::loadFile(const QString& pathOrUrl, double resumeSeconds) {
    if (!m_mpv || pathOrUrl.isEmpty()) return;
    m_currentFile = pathOrUrl; m_pendingResumeSec = resumeSeconds; m_eofSignaled = false; m_videoW = 0; m_videoH = 0;
    int pauseFlag = 0; mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &pauseFlag);
    QByteArray ba = pathOrUrl.toUtf8();
    const char* args[] = {"loadfile", ba.constData(), "replace", nullptr};
    mpv_command_async(m_mpv, 0, args);
}

void MpvVideoWidget::stopPlayback() {
    if (!m_mpv) return;
    const char* args[] = {"stop", nullptr}; mpv_command_async(m_mpv, 0, args);
    m_hasMedia = false; m_currentFile.clear(); m_position = 0.0; m_duration = 0.0; m_videoW = 0; m_videoH = 0;
    if (m_idleOverlay) m_idleOverlay->show();
    emit mediaStateChanged(false); emit positionChanged(0.0); emit durationChanged(0.0); update();
}

void MpvVideoWidget::togglePause() { if (m_mpv && m_hasMedia) setPaused(!m_paused); }
void MpvVideoWidget::setPaused(bool p) { if (m_mpv) { int f = p ? 1 : 0; mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &f); } }
void MpvVideoWidget::seekAbsolute(double sec) {
    if (!m_mpv || !m_hasMedia) return;
    QByteArray v = QByteArray::number(qMax(0.0, sec), 'f', 3);
    const char* args[] = {"seek", v.constData(), "absolute", nullptr}; mpv_command_async(m_mpv, 0, args);
}
void MpvVideoWidget::seekRelative(double d) {
    if (!m_mpv || !m_hasMedia) return;
    QByteArray v = QByteArray::number(d, 'f', 2);
    const char* args[] = {"seek", v.constData(), "relative", nullptr}; mpv_command_async(m_mpv, 0, args);
}
void MpvVideoWidget::frameStep() { if (m_mpv && m_hasMedia) { const char* args[] = {"frame-step", nullptr}; mpv_command_async(m_mpv, 0, args); } }
void MpvVideoWidget::setVolume(int p) { if (m_mpv) { double v = qBound(0, p, 150); mpv_set_property_async(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE, &v); } }
void MpvVideoWidget::setMuted(bool m) { if (m_mpv) { int f = m ? 1 : 0; mpv_set_property_async(m_mpv, 0, "mute", MPV_FORMAT_FLAG, &f); } }
void MpvVideoWidget::setSpeed(double s) { if (m_mpv) { double v = qBound(0.25, s, 4.0); mpv_set_property_async(m_mpv, 0, "speed", MPV_FORMAT_DOUBLE, &v); } }
void MpvVideoWidget::setAspectRatio(const QString& r) { if (m_mpv) { QByteArray b = r.toUtf8(); const char* p = b.constData(); mpv_set_property_async(m_mpv, 0, "video-aspect-override", MPV_FORMAT_STRING, &p); } }
void MpvVideoWidget::setDeinterlace(bool e) { if (m_mpv) { int f = e ? 1 : 0; mpv_set_property_async(m_mpv, 0, "deinterlace", MPV_FORMAT_FLAG, &f); } }
void MpvVideoWidget::setHwdec(const QString& m) { if (m_mpv) { QByteArray b = m.toUtf8(); const char* p = b.constData(); mpv_set_property_async(m_mpv, 0, "hwdec", MPV_FORMAT_STRING, &p); } }
void MpvVideoWidget::addSubtitleFile(const QString& path) {
    if (!m_mpv || path.isEmpty()) return;
    QByteArray b = path.toUtf8(); const char* args[] = {"sub-add", b.constData(), "select", nullptr};
    mpv_command_async(m_mpv, 0, args); showOsd("Subtitle: " + QFileInfo(path).fileName());
}
void MpvVideoWidget::setAudioTrack(int id) {
    if (m_mpv) { QByteArray b = id <= 0 ? QByteArray("no") : QByteArray::number(id); const char* p = b.constData(); mpv_set_property_async(m_mpv, 0, "aid", MPV_FORMAT_STRING, &p); }
}
void MpvVideoWidget::setSubtitleTrack(int id) {
    if (m_mpv) { QByteArray b = id <= 0 ? QByteArray("no") : QByteArray::number(id); const char* p = b.constData(); mpv_set_property_async(m_mpv, 0, "sid", MPV_FORMAT_STRING, &p); }
}
void MpvVideoWidget::setAudioDelay(double s) { if (m_mpv) mpv_set_property_async(m_mpv, 0, "audio-delay", MPV_FORMAT_DOUBLE, &s); }
void MpvVideoWidget::setSubtitleDelay(double s) { if (m_mpv) mpv_set_property_async(m_mpv, 0, "sub-delay", MPV_FORMAT_DOUBLE, &s); }
void MpvVideoWidget::setSubtitleFontSize(int sz) { if (m_mpv) { int64_t v = qBound(16, sz, 96); mpv_set_property_async(m_mpv, 0, "sub-font-size", MPV_FORMAT_INT64, &v); } }
void MpvVideoWidget::takeSnapshot(const QString& dir) {
    if (!m_mpv || !m_hasMedia) return;
    if (!dir.isEmpty()) { QByteArray d = QDir::toNativeSeparators(dir).toUtf8(); const char* dp = d.constData(); mpv_set_property(m_mpv, "screenshot-directory", MPV_FORMAT_STRING, &dp); }
    const char* args[] = {"screenshot", "video", nullptr}; mpv_command_async(m_mpv, 0, args); showOsd("Snapshot saved to Pictures");
}

QList<MediaTrack> MpvVideoWidget::tracks() const {
    QList<MediaTrack> out; if (!m_mpv) return out;
    mpv_node node{}; if (mpv_get_property(m_mpv, "track-list", MPV_FORMAT_NODE, &node) < 0) return out;
    if (node.format == MPV_FORMAT_NODE_ARRAY && node.u.list) {
        for (int i = 0; i < node.u.list->num; ++i) {
            const mpv_node& item = node.u.list->values[i];
            if (item.format != MPV_FORMAT_NODE_MAP || !item.u.list) continue;
            MediaTrack t;
            for (int k = 0; k < item.u.list->num; ++k) {
                QString key = QString::fromUtf8(item.u.list->keys[k]);
                const mpv_node& val = item.u.list->values[k];
                if (key == "id" && val.format == MPV_FORMAT_INT64) t.id = static_cast<int>(val.u.int64);
                else if (key == "type" && val.format == MPV_FORMAT_STRING) t.type = QString::fromUtf8(val.u.string);
                else if (key == "title" && val.format == MPV_FORMAT_STRING) t.title = QString::fromUtf8(val.u.string);
                else if (key == "lang" && val.format == MPV_FORMAT_STRING) t.lang = QString::fromUtf8(val.u.string);
                else if (key == "codec" && val.format == MPV_FORMAT_STRING) t.codec = QString::fromUtf8(val.u.string);
                else if (key == "selected" && val.format == MPV_FORMAT_FLAG) t.selected = (val.u.flag != 0);
                else if (key == "external" && val.format == MPV_FORMAT_FLAG) t.external = (val.u.flag != 0);
            }
            out.append(t);
        }
    }
    mpv_free_node_contents(&node); return out;
}

void MpvVideoWidget::mouseDoubleClickEvent(QMouseEvent* e) { if (e->button() == Qt::LeftButton) emit doubleClicked(); QOpenGLWidget::mouseDoubleClickEvent(e); }
void MpvVideoWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::RightButton) { emit contextMenuRequested(e->globalPosition().toPoint()); e->accept(); return; }
    QOpenGLWidget::mousePressEvent(e);
}
void MpvVideoWidget::dragEnterEvent(QDragEnterEvent* e) { if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
void MpvVideoWidget::dragMoveEvent(QDragMoveEvent* e) { if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
void MpvVideoWidget::dropEvent(QDropEvent* e) {
    QStringList paths;
    for (const QUrl& u : e->mimeData()->urls()) paths << (u.isLocalFile() ? u.toLocalFile() : u.toString());
    if (!paths.isEmpty()) { emit filesDropped(paths); e->acceptProposedAction(); }
}

} // namespace Tavrynexo::Player
