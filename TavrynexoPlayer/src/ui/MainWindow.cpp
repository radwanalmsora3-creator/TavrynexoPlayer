#include "ui/MainWindow.hpp"
#include "ui/Dialogs.hpp"
#include "ui/IconFactory.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace Tavrynexo::UI {

static QString formatClock(double sec) {
    if (sec <= 0.0) return "00:00";
    qint64 s = qRound64(sec);
    qint64 h = s / 3600, m = (s % 3600) / 60, r = s % 60;
    if (h > 0) return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(r, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(r, 2, 10, QChar('0'));
}

MainWindow::MainWindow(const QString& initialFile, QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Tavrynexo Player");
    resize(1120, 680);
    setMinimumSize(640, 420);
    setAcceptDrops(true);

    m_fsHideTimer = new QTimer(this);
    m_fsHideTimer->setSingleShot(true);
    connect(m_fsHideTimer, &QTimer::timeout, this, [this]() {
        if (isFullScreen() && m_autoHideFs && m_stack->currentIndex() == 0) {
            m_controlBar->hide();
            m_video->setCursor(Qt::BlankCursor);
        }
    });

    setupCentralAndControls();
    setupMenuBar();
    setupShortcuts();
    applySettings();

    QSettings s("Tavrynexo", "TavrynexoPlayer");
    if (s.contains("ui/geometry")) restoreGeometry(s.value("ui/geometry").toByteArray());
    int savedVol = s.value("player/volume", 100).toInt();
    m_video->setVolume(savedVol);
    m_volumeSlider->setVolume(savedVol);
    updateVolumeIcon(savedVol, false);

    qApp->installEventFilter(this);

    if (!initialFile.isEmpty()) {
        handleDroppedPaths({initialFile});
    }
}

void MainWindow::setupCentralAndControls() {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_stack = new QStackedWidget(central);
    m_video = new Tavrynexo::Player::MpvVideoWidget(m_stack);
    m_playlist = new PlaylistWidget(m_stack);
    m_stack->addWidget(m_video);
    m_stack->addWidget(m_playlist);
    root->addWidget(m_stack, 1);

    m_controlBar = new QWidget(central);
    m_controlBar->setObjectName("ControlBar");
    auto* barLay = new QVBoxLayout(m_controlBar);
    barLay->setContentsMargins(8, 3, 8, 5);
    barLay->setSpacing(3);

    auto* seekRow = new QHBoxLayout();
    seekRow->setSpacing(8);
    m_timeElapsed = new QLabel("--:--", m_controlBar);
    m_timeElapsed->setMinimumWidth(44);
    m_timeElapsed->setAlignment(Qt::AlignCenter);
    m_seekSlider = new SeekSlider(m_controlBar);
    m_timeTotal = new QLabel("--:--", m_controlBar);
    m_timeTotal->setMinimumWidth(44);
    m_timeTotal->setAlignment(Qt::AlignCenter);
    seekRow->addWidget(m_timeElapsed);
    seekRow->addWidget(m_seekSlider, 1);
    seekRow->addWidget(m_timeTotal);
    barLay->addLayout(seekRow);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(4);
    auto mkBtn = [&](const QString& tip, QSize sz = QSize(28, 26)) {
        auto* b = new QToolButton(m_controlBar);
        b->setToolTip(tip);
        b->setFixedSize(sz);
        b->setAutoRaise(false);
        return b;
    };

    m_playPauseBtn = mkBtn("Play / Pause (Space)", QSize(34, 28));
    m_prevBtn = mkBtn("Previous (P)");
    m_stopBtn = mkBtn("Stop (S)");
    m_nextBtn = mkBtn("Next (N)");
    m_fullscreenBtn = mkBtn("Fullscreen (F / F11)");
    m_effectsBtn = mkBtn("Speed, Sync & Preferences");
    m_playlistBtn = mkBtn("Toggle Playlist (Ctrl+L)");
    m_playlistBtn->setCheckable(true);
    m_repeatBtn = mkBtn("Repeat Mode");
    m_shuffleBtn = mkBtn("Shuffle");
    m_shuffleBtn->setCheckable(true);
    m_speedBtn = mkBtn("Playback Speed (click to reset)", QSize(46, 26));
    m_speedBtn->setText("1.0x");
    m_speedBtn->hide();
    m_muteBtn = mkBtn("Mute (M)", QSize(26, 26));
    m_volumeSlider = new WedgeVolumeSlider(m_controlBar);

    btnRow->addWidget(m_playPauseBtn);
    btnRow->addSpacing(4);
    btnRow->addWidget(m_prevBtn);
    btnRow->addWidget(m_stopBtn);
    btnRow->addWidget(m_nextBtn);
    btnRow->addSpacing(6);
    btnRow->addWidget(m_fullscreenBtn);
    btnRow->addWidget(m_effectsBtn);
    btnRow->addSpacing(6);
    btnRow->addWidget(m_playlistBtn);
    btnRow->addWidget(m_repeatBtn);
    btnRow->addWidget(m_shuffleBtn);
    btnRow->addSpacing(6);
    btnRow->addWidget(m_speedBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(m_muteBtn);
    btnRow->addWidget(m_volumeSlider);
    barLay->addLayout(btnRow);

    root->addWidget(m_controlBar);
    setCentralWidget(central);

    connect(m_playPauseBtn, &QToolButton::clicked, this, [this]() {
        if (!m_video->isMediaLoaded() && m_playlist->count() > 0) m_playlist->playIndex(qMax(0, m_playlist->currentIndex()));
        else if (!m_video->isMediaLoaded()) openFile();
        else m_video->togglePause();
    });
    connect(m_prevBtn, &QToolButton::clicked, m_playlist, &PlaylistWidget::playPrev);
    connect(m_stopBtn, &QToolButton::clicked, m_video, &Tavrynexo::Player::MpvVideoWidget::stopPlayback);
    connect(m_nextBtn, &QToolButton::clicked, this, [this]() { m_playlist->playNext(true); });
    connect(m_fullscreenBtn, &QToolButton::clicked, this, &MainWindow::toggleFullscreen);
    connect(m_playlistBtn, &QToolButton::clicked, this, &MainWindow::togglePlaylistView);
    connect(m_repeatBtn, &QToolButton::clicked, this, &MainWindow::cycleRepeatMode);
    connect(m_shuffleBtn, &QToolButton::toggled, m_playlist, &PlaylistWidget::setShuffle);
    connect(m_speedBtn, &QToolButton::clicked, this, [this]() { m_video->setSpeed(1.0); });
    connect(m_muteBtn, &QToolButton::clicked, this, [this]() { m_video->setMuted(!m_video->isMuted()); });
    connect(m_volumeSlider, &WedgeVolumeSlider::volumeChanged, m_video, &Tavrynexo::Player::MpvVideoWidget::setVolume);
    connect(m_effectsBtn, &QToolButton::clicked, this, [this]() {
        QMenu m(this);
        for (double sp : {0.5, 0.75, 1.0, 1.25, 1.5, 2.0}) {
            QAction* a = m.addAction(QString("Speed %1x").arg(sp));
            connect(a, &QAction::triggered, this, [this, sp]() { m_video->setSpeed(sp); });
        }
        m.addSeparator();
        m.addAction("Audio Delay +100ms", this, [this]() { m_audioDelay += 0.1; m_video->setAudioDelay(m_audioDelay); m_video->showOsd(QString("Audio Delay: %1 ms").arg(qRound(m_audioDelay*1000))); });
        m.addAction("Audio Delay -100ms", this, [this]() { m_audioDelay -= 0.1; m_video->setAudioDelay(m_audioDelay); m_video->showOsd(QString("Audio Delay: %1 ms").arg(qRound(m_audioDelay*1000))); });
        m.addAction("Subtitle Delay +100ms", this, [this]() { m_subDelay += 0.1; m_video->setSubtitleDelay(m_subDelay); m_video->showOsd(QString("Sub Delay: %1 ms").arg(qRound(m_subDelay*1000))); });
        m.addAction("Subtitle Delay -100ms", this, [this]() { m_subDelay -= 0.1; m_video->setSubtitleDelay(m_subDelay); m_video->showOsd(QString("Sub Delay: %1 ms").arg(qRound(m_subDelay*1000))); });
        m.addSeparator();
        m.addAction("Preferences...", this, &MainWindow::openPreferences);
        m.exec(m_effectsBtn->mapToGlobal(QPoint(0, -m.sizeHint().height())));
    });

    connect(m_seekSlider, &SeekSlider::sliderPressed, this, [this]() { m_isSeeking = true; });
    connect(m_seekSlider, &SeekSlider::sliderMoved, this, [this](int val) {
        double dur = m_video->duration();
        if (dur > 0.0) {
            double target = (double(val) / m_seekSlider->maximum()) * dur;
            updateTimeLabels(target, dur);
            m_video->seekAbsolute(target);
        }
    });
    connect(m_seekSlider, &SeekSlider::sliderReleased, this, [this]() { m_isSeeking = false; });

    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::positionChanged, this, [this](double pos) {
        if (!m_isSeeking) {
            double dur = m_video->duration();
            updateTimeLabels(pos, dur);
            if (dur > 0.0) {
                m_seekSlider->blockSignals(true);
                m_seekSlider->setValue(qRound((pos / dur) * m_seekSlider->maximum()));
                m_seekSlider->blockSignals(false);
            }
        }
    });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::durationChanged, this, [this](double dur) {
        m_seekSlider->setDuration(dur);
        updateTimeLabels(m_video->position(), dur);
        m_playlist->updateDuration(m_video->currentFile(), dur);
    });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::pausedChanged, this, [this](bool) { refreshControlIcons(); });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::mediaStateChanged, this, [this](bool loaded) {
        refreshControlIcons();
        if (!loaded) {
            m_seekSlider->setValue(0); m_seekSlider->setDuration(0.0);
            m_timeElapsed->setText("--:--"); m_timeTotal->setText("--:--");
            setWindowTitle("Tavrynexo Player");
        }
    });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::volumeChanged, this, [this](int v) {
        m_volumeSlider->setVolume(v); updateVolumeIcon(v, m_video->isMuted());
    });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::mutedChanged, this, [this](bool m) {
        updateVolumeIcon(m_video->volume(), m);
    });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::speedChanged, this, [this](double sp) {
        m_speedBtn->setText(QString("%1x").arg(sp, 0, 'g', 3));
        m_speedBtn->setVisible(qAbs(sp - 1.0) > 0.01);
    });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::eofReached, this, [this]() {
        if (m_resumeEnabled && !m_video->currentFile().isEmpty()) {
            QSettings s("Tavrynexo", "TavrynexoPlayer");
            s.remove("resume/" + QString::fromLatin1(m_video->currentFile().toUtf8().toBase64()));
        }
        m_playlist->playNext(false);
    });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::tracksChanged, this, &MainWindow::rebuildTrackMenus);
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::playbackError, this, [this](const QString& msg) {
        m_video->showOsd("Error: " + msg);
    });
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::filesDropped, this, &MainWindow::handleDroppedPaths);
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::doubleClicked, this, &MainWindow::toggleFullscreen);
    connect(m_video, &Tavrynexo::Player::MpvVideoWidget::contextMenuRequested, this, &MainWindow::showContextMenu);

    connect(m_playlist, &PlaylistWidget::playRequested, this, &MainWindow::playMediaPath);
    connect(m_playlist, &PlaylistWidget::filesDropped, this, &MainWindow::handleDroppedPaths);
    connect(m_playlist, &PlaylistWidget::addFilesClicked, this, &MainWindow::openMultipleFiles);
    connect(m_playlist, &PlaylistWidget::addFolderClicked, this, &MainWindow::openFolder);
}

void MainWindow::setupMenuBar() {
    QMenu* media = menuBar()->addMenu("&Media");
    media->addAction("Open &File...", QKeySequence("Ctrl+O"), this, &MainWindow::openFile);
    media->addAction("Open &Multiple Files...", QKeySequence("Ctrl+Shift+O"), this, &MainWindow::openMultipleFiles);
    media->addAction("Open F&older...", QKeySequence("Ctrl+F"), this, &MainWindow::openFolder);
    media->addAction("Open &Network Stream...", QKeySequence("Ctrl+N"), this, &MainWindow::openNetworkStream);
    m_recentMenu = media->addMenu("Open &Recent Media");
    rebuildRecentMenu();
    media->addSeparator();
    media->addAction("&Save Playlist to File...", this, &MainWindow::savePlaylist);
    media->addSeparator();
    media->addAction("&Quit", QKeySequence("Ctrl+Q"), this, &QWidget::close);

    QMenu* pb = menuBar()->addMenu("&Playback");
    pb->addAction("Play / Pause", this, [this]() { m_playPauseBtn->click(); });
    pb->addAction("Stop", this, [this]() { m_video->stopPlayback(); });
    pb->addAction("Previous", m_playlist, &PlaylistWidget::playPrev);
    pb->addAction("Next", this, [this]() { m_playlist->playNext(true); });
    QMenu* spMenu = pb->addMenu("Speed");
    for (double sp : {0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0}) {
        spMenu->addAction(sp == 1.0 ? "1.0x (Normal)" : QString("%1x").arg(sp), this, [this, sp]() { m_video->setSpeed(sp); });
    }
    pb->addSeparator();
    pb->addAction("Jump Forward +10s", this, [this]() { m_video->seekRelative(10.0); });
    pb->addAction("Jump Backward -10s", this, [this]() { m_video->seekRelative(-10.0); });
    pb->addAction("Frame by Frame (E)", this, [this]() { m_video->frameStep(); });

    QMenu* aud = menuBar()->addMenu("&Audio");
    m_audioTrackMenu = aud->addMenu("Audio &Track");
    aud->addSeparator();
    aud->addAction("Increase Volume (+5%)", this, [this]() { m_video->setVolume(m_video->volume() + 5); });
    aud->addAction("Decrease Volume (-5%)", this, [this]() { m_video->setVolume(m_video->volume() - 5); });
    aud->addAction("Mute", this, [this]() { m_video->setMuted(!m_video->isMuted()); });

    QMenu* vid = menuBar()->addMenu("&Video");
    vid->addAction("Fullscreen", QKeySequence("F11"), this, &MainWindow::toggleFullscreen);
    m_alwaysOnTopAct = vid->addAction("Always on Top");
    m_alwaysOnTopAct->setCheckable(true);
    connect(m_alwaysOnTopAct, &QAction::toggled, this, [this](bool on) {
        setWindowFlag(Qt::WindowStaysOnTopHint, on); show();
    });
    QMenu* arMenu = vid->addMenu("Aspect Ratio");
    for (const QString& ar : {"Default", "16:9", "4:3", "21:9", "16:10", "1:1"}) {
        arMenu->addAction(ar, this, [this, ar]() { m_video->setAspectRatio(ar == "Default" ? "" : ar); });
    }
    m_deinterlaceAct = vid->addAction("Deinterlace");
    m_deinterlaceAct->setCheckable(true);
    connect(m_deinterlaceAct, &QAction::toggled, m_video, &Tavrynexo::Player::MpvVideoWidget::setDeinterlace);
    vid->addSeparator();
    vid->addAction("Take Snapshot", QKeySequence("Shift+S"), this, [this]() {
        m_video->takeSnapshot(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    });

    QMenu* sub = menuBar()->addMenu("&Subtitle");
    sub->addAction("Add Subtitle File...", QKeySequence("Alt+S"), this, &MainWindow::addSubtitleFile);
    m_subTrackMenu = sub->addMenu("Sub &Track");

    QMenu* tools = menuBar()->addMenu("&Tools");
    tools->addAction("Media &Information...", QKeySequence("Ctrl+I"), this, &MainWindow::openMediaInfo);
    tools->addAction("&Preferences...", QKeySequence("Ctrl+P"), this, &MainWindow::openPreferences);

    QMenu* view = menuBar()->addMenu("&View");
    view->addAction("&Playlist", QKeySequence("Ctrl+L"), this, &MainWindow::togglePlaylistView);
    m_minimalAct = view->addAction("&Minimal Interface", QKeySequence("Ctrl+H"), this, &MainWindow::toggleMinimalView);
    m_minimalAct->setCheckable(true);
    m_darkThemeAct = view->addAction("&Dark Cinema Theme");
    m_darkThemeAct->setCheckable(true);
    connect(m_darkThemeAct, &QAction::toggled, this, [this](bool on) {
        QSettings("Tavrynexo", "TavrynexoPlayer").setValue("ui/theme", on ? "dark" : "light");
        applySettings();
    });

    QMenu* help = menuBar()->addMenu("&Help");
    help->addAction("&Keyboard Shortcuts...", this, [this]() { ShortcutsDialog(this).exec(); });
    help->addAction("&About Tavrynexo Player", this, [this]() {
        QMessageBox::about(this, "About Tavrynexo Player",
            "<h3>Tavrynexo Player 2.0</h3><p>High-performance media player powered by libmpv &amp; Qt 6.</p>");
    });
}

void MainWindow::setupShortcuts() {}

void MainWindow::openFile() {
    QString f = QFileDialog::getOpenFileName(this, "Open Media File", QString(),
        "Media Files (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.webm *.m4v *.ts *.mpg *.mp3 *.flac *.wav *.aac *.m4a *.ogg *.opus);;All Files (*.*)");
    if (!f.isEmpty()) handleDroppedPaths({f});
}

void MainWindow::openMultipleFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Open Media Files", QString(),
        "Media Files (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.webm *.m4v *.ts *.mpg *.mp3 *.flac *.wav *.aac *.m4a *.ogg *.opus);;All Files (*.*)");
    if (!files.isEmpty()) handleDroppedPaths(files);
}

void MainWindow::openFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Open Media Folder");
    if (!dir.isEmpty()) handleDroppedPaths({dir});
}

void MainWindow::openNetworkStream() {
    bool ok = false;
    QString url = QInputDialog::getText(this, "Open Network Stream", "Enter HTTP / HTTPS / RTSP URL:", QLineEdit::Normal, "", &ok);
    if (ok && !url.trimmed().isEmpty()) {
        int idx = m_playlist->count();
        m_playlist->addPaths({url.trimmed()}, false);
        m_stack->setCurrentIndex(0);
        m_playlistBtn->setChecked(false);
        m_playlist->playIndex(idx);
    }
}

void MainWindow::addSubtitleFile() {
    QString f = QFileDialog::getOpenFileName(this, "Load Subtitle File", QString(),
        "Subtitle Files (*.srt *.ass *.ssa *.vtt *.sub);;All Files (*.*)");
    if (!f.isEmpty()) m_video->addSubtitleFile(f);
}

void MainWindow::savePlaylist() {
    QString f = QFileDialog::getSaveFileName(this, "Save Playlist", "playlist.m3u8", "M3U8 Playlist (*.m3u8)");
    if (!f.isEmpty()) m_playlist->exportM3u8(f);
}

void MainWindow::togglePlaylistView() {
    bool showPl = (m_stack->currentIndex() == 0);
    m_stack->setCurrentIndex(showPl ? 1 : 0);
    m_playlistBtn->setChecked(showPl);
}

void MainWindow::toggleFullscreen() {
    if (isFullScreen()) {
        m_fsHideTimer->stop();
        m_video->unsetCursor();
        showNormal();
        if (!m_minimalView) menuBar()->show();
        m_controlBar->show();
    } else {
        menuBar()->hide();
        m_stack->setCurrentIndex(0);
        m_playlistBtn->setChecked(false);
        showFullScreen();
        if (m_autoHideFs) m_fsHideTimer->start(2500);
    }
}

void MainWindow::toggleMinimalView() {
    m_minimalView = m_minimalAct->isChecked();
    menuBar()->setVisible(!m_minimalView && !isFullScreen());
    m_controlBar->setVisible(!m_minimalView);
}

void MainWindow::openPreferences() {
    if (PreferencesDialog(this).exec() == QDialog::Accepted) applySettings();
}

void MainWindow::openMediaInfo() { MediaInfoDialog(m_video, this).exec(); }

void MainWindow::showContextMenu(const QPoint& globalPos) {
    QMenu m(this);
    m.addAction(m_video->isPaused() ? "Play" : "Pause", [this]() { m_playPauseBtn->click(); });
    m.addAction("Stop", m_video, &Tavrynexo::Player::MpvVideoWidget::stopPlayback);
    m.addAction(isFullScreen() ? "Exit Fullscreen" : "Fullscreen", this, &MainWindow::toggleFullscreen);
    m.addSeparator();
    m.addMenu(m_audioTrackMenu);
    m.addMenu(m_subTrackMenu);
    m.addAction("Add Subtitle File...", this, &MainWindow::addSubtitleFile);
    m.addSeparator();
    m.addAction("Open File...", this, &MainWindow::openFile);
    m.addAction("Open Folder...", this, &MainWindow::openFolder);
    m.addAction("Playlist", this, &MainWindow::togglePlaylistView);
    m.addAction("Media Information...", this, &MainWindow::openMediaInfo);
    m.exec(globalPos);
}

void MainWindow::handleDroppedPaths(const QStringList& paths) {
    QStringList media, subs;
    for (const QString& p : paths) {
        if (PlaylistWidget::isSubtitleFile(p)) subs << p;
        else media << p;
    }
    for (const QString& s : subs) {
        if (m_video->isMediaLoaded()) m_video->addSubtitleFile(s);
    }
    if (!media.isEmpty()) {
        int startIdx = m_playlist->count();
        if (m_playlist->addPaths(media, false) > 0) {
            m_stack->setCurrentIndex(0);
            m_playlistBtn->setChecked(false);
            m_playlist->playIndex(startIdx);
        }
    }
}

void MainWindow::playMediaPath(const QString& path) {
    saveCurrentResumePosition();
    double resume = 0.0;
    if (m_resumeEnabled) {
        QSettings s("Tavrynexo", "TavrynexoPlayer");
        resume = s.value("resume/" + QString::fromLatin1(path.toUtf8().toBase64()), 0.0).toDouble();
    }
    m_video->loadFile(path, resume);
    addToRecent(path);
    setWindowTitle(QFileInfo(path).fileName() + " - Tavrynexo Player");
}

void MainWindow::rebuildTrackMenus() {
    m_audioTrackMenu->clear();
    m_subTrackMenu->clear();
    QAction* subOff = m_subTrackMenu->addAction("Disable");
    subOff->setCheckable(true);
    bool anySubSel = false;
    int audioCount = 0;

    for (const auto& t : m_video->tracks()) {
        QString label = QString("Track %1%2%3").arg(t.id)
            .arg(t.title.isEmpty() ? "" : " - " + t.title)
            .arg(t.lang.isEmpty() ? "" : " [" + t.lang + "]");
        if (t.type == "audio") {
            ++audioCount;
            QAction* a = m_audioTrackMenu->addAction(label);
            a->setCheckable(true); a->setChecked(t.selected);
            int tid = t.id;
            connect(a, &QAction::triggered, this, [this, tid]() { m_video->setAudioTrack(tid); });
        } else if (t.type == "sub") {
            if (t.selected) anySubSel = true;
            QAction* a = m_subTrackMenu->addAction(label);
            a->setCheckable(true); a->setChecked(t.selected);
            int tid = t.id;
            connect(a, &QAction::triggered, this, [this, tid]() { m_video->setSubtitleTrack(tid); });
        }
    }
    subOff->setChecked(!anySubSel);
    connect(subOff, &QAction::triggered, this, [this]() { m_video->setSubtitleTrack(0); });
    if (audioCount == 0) m_audioTrackMenu->addAction("(No Audio Tracks)")->setEnabled(false);
}

void MainWindow::rebuildRecentMenu() {
    m_recentMenu->clear();
    QSettings s("Tavrynexo", "TavrynexoPlayer");
    QStringList list = s.value("recent/files").toStringList();
    if (list.isEmpty()) {
        m_recentMenu->addAction("(Empty)")->setEnabled(false);
        return;
    }
    for (const QString& p : list) {
        m_recentMenu->addAction(QFileInfo(p).fileName(), this, [this, p]() { handleDroppedPaths({p}); });
    }
    m_recentMenu->addSeparator();
    m_recentMenu->addAction("Clear Recent", this, [this]() {
        QSettings("Tavrynexo", "TavrynexoPlayer").remove("recent/files");
        rebuildRecentMenu();
    });
}

void MainWindow::addToRecent(const QString& path) {
    QSettings s("Tavrynexo", "TavrynexoPlayer");
    QStringList list = s.value("recent/files").toStringList();
    list.removeAll(path);
    list.prepend(path);
    while (list.size() > 15) list.removeLast();
    s.setValue("recent/files", list);
    rebuildRecentMenu();
}

void MainWindow::saveCurrentResumePosition() {
    if (!m_resumeEnabled || !m_video->isMediaLoaded() || m_video->currentFile().isEmpty()) return;
    double pos = m_video->position(), dur = m_video->duration();
    QString key = "resume/" + QString::fromLatin1(m_video->currentFile().toUtf8().toBase64());
    QSettings s("Tavrynexo", "TavrynexoPlayer");
    if (pos > 5.0 && dur > 10.0 && pos < dur - 5.0) s.setValue(key, pos);
    else s.remove(key);
}

void MainWindow::cycleRepeatMode() {
    auto cur = m_playlist->repeatMode();
    auto next = (cur == RepeatMode::Off) ? RepeatMode::All : (cur == RepeatMode::All ? RepeatMode::One : RepeatMode::Off);
    m_playlist->setRepeatMode(next);
    refreshControlIcons();
}

void MainWindow::applySettings() {
    QSettings s("Tavrynexo", "TavrynexoPlayer");
    m_darkTheme = (s.value("ui/theme", "light").toString() == "dark");
    m_resumeEnabled = s.value("player/resume", true).toBool();
    m_autoHideFs = s.value("player/autoHideFs", true).toBool();
    m_video->setHwdec(s.value("player/hwdec", "auto-safe").toString());
    m_video->setSubtitleFontSize(s.value("sub/fontSize", 48).toInt());
    m_volumeSlider->setMaxVolume(s.value("player/maxVolume", 150).toInt());
    if (m_darkThemeAct) { m_darkThemeAct->blockSignals(true); m_darkThemeAct->setChecked(m_darkTheme); m_darkThemeAct->blockSignals(false); }

    m_seekSlider->setThemeDark(m_darkTheme);
    m_volumeSlider->setThemeDark(m_darkTheme);
    m_playlist->setThemeDark(m_darkTheme);

    if (m_darkTheme) {
        setStyleSheet(
            "QMainWindow, QWidget#ControlBar { background: #141820; color: #E5E7EB; }"
            "QMenuBar { background: #11151C; color: #E5E7EB; border-bottom: 1px solid #262D3B; padding: 2px; }"
            "QMenuBar::item:selected, QMenu::item:selected { background: #2563EB; color: #FFFFFF; }"
            "QMenu { background: #1A202C; color: #E5E7EB; border: 1px solid #2D3748; }"
            "QToolButton { background: #1F2633; color: #E5E7EB; border: 1px solid #2F384A; border-radius: 4px; }"
            "QToolButton:hover { background: #2B3447; border-color: #4B5563; }"
            "QToolButton:checked { background: #2563EB; border-color: #3B82F6; color: #FFFFFF; }"
            "QLabel { color: #CBD5E1; font-size: 11px; }");
    } else {
        setStyleSheet(
            "QMainWindow, QWidget#ControlBar { background: #F4F4F4; color: #1F2328; }"
            "QMenuBar { background: #FFFFFF; color: #1F2328; border-bottom: 1px solid #D8D8D8; padding: 1px 2px; }"
            "QMenuBar::item { padding: 3px 7px; }"
            "QMenuBar::item:selected { background: #E5F1FB; border: 1px solid #CCE4F7; }"
            "QMenu { background: #FFFFFF; color: #1F2328; border: 1px solid #CCCCCC; }"
            "QMenu::item:selected { background: #0078D7; color: #FFFFFF; }"
            "QToolButton { background: #FFFFFF; color: #1F2328; border: 1px solid #C8CDD4; border-radius: 3px; }"
            "QToolButton:hover { background: #E9F2FC; border-color: #90BDE8; }"
            "QToolButton:checked { background: #DCEBFB; border-color: #3B82F6; }"
            "QLabel { color: #24292F; font-size: 11px; }");
    }
    refreshControlIcons();
}

void MainWindow::updateTimeLabels(double pos, double dur) {
    m_timeElapsed->setText(dur > 0.0 || pos > 0.0 ? formatClock(pos) : "--:--");
    m_timeTotal->setText(dur > 0.0 ? formatClock(dur) : "--:--");
}

void MainWindow::updateVolumeIcon(int vol, bool muted) {
    QColor ic = m_darkTheme ? QColor("#E5E7EB") : QColor("#24292F");
    IconKind k = (muted || vol == 0) ? IconKind::VolumeMute : (vol < 50 ? IconKind::VolumeLow : IconKind::VolumeHigh);
    m_muteBtn->setIcon(IconFactory::icon(k, ic, 16));
}

void MainWindow::refreshControlIcons() {
    QColor ic = m_darkTheme ? QColor("#E5E7EB") : QColor("#24292F");
    bool playing = m_video->isMediaLoaded() && !m_video->isPaused();
    m_playPauseBtn->setIcon(IconFactory::icon(playing ? IconKind::Pause : IconKind::Play, ic, 18));
    m_prevBtn->setIcon(IconFactory::icon(IconKind::Prev, ic, 16));
    m_stopBtn->setIcon(IconFactory::icon(IconKind::Stop, ic, 16));
    m_nextBtn->setIcon(IconFactory::icon(IconKind::Next, ic, 16));
    m_fullscreenBtn->setIcon(IconFactory::icon(IconKind::Fullscreen, ic, 16));
    m_effectsBtn->setIcon(IconFactory::icon(IconKind::Effects, ic, 16));
    m_playlistBtn->setIcon(IconFactory::icon(IconKind::Playlist, ic, 16));
    auto rm = m_playlist->repeatMode();
    IconKind rk = (rm == RepeatMode::One) ? IconKind::RepeatOne : (rm == RepeatMode::All ? IconKind::RepeatAll : IconKind::RepeatOff);
    m_repeatBtn->setIcon(IconFactory::icon(rk, rm == RepeatMode::Off ? ic : QColor("#2563EB"), 16));
    m_shuffleBtn->setIcon(IconFactory::icon(IconKind::Shuffle, ic, 16));
    updateVolumeIcon(m_video->volume(), m_video->isMuted());
}

void MainWindow::closeEvent(QCloseEvent* e) {
    saveCurrentResumePosition();
    QSettings s("Tavrynexo", "TavrynexoPlayer");
    s.setValue("ui/geometry", saveGeometry());
    s.setValue("player/volume", m_video->volume());
    QMainWindow::closeEvent(e);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) { if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
void MainWindow::dragMoveEvent(QDragMoveEvent* e) { if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
void MainWindow::dropEvent(QDropEvent* e) {
    QStringList paths;
    for (const QUrl& u : e->mimeData()->urls()) paths << (u.isLocalFile() ? u.toLocalFile() : u.toString());
    if (!paths.isEmpty()) { handleDroppedPaths(paths); e->acceptProposedAction(); }
}
void MainWindow::keyPressEvent(QKeyEvent* e) { QMainWindow::keyPressEvent(e); }
void MainWindow::wheelEvent(QWheelEvent* e) {
    m_video->setVolume(m_video->volume() + (e->angleDelta().y() > 0 ? 5 : -5));
    m_video->showOsd(QString("Volume: %1%").arg(m_video->volume()));
    e->accept();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
    if (ev->type() == QEvent::MouseMove && isFullScreen() && (obj == m_video || obj == m_controlBar)) {
        m_controlBar->show();
        m_video->unsetCursor();
        if (m_autoHideFs) m_fsHideTimer->start(2500);
    } else if (ev->type() == QEvent::KeyPress && isActiveWindow()) {
        auto* fw = QApplication::focusWidget();
        if (!qobject_cast<QLineEdit*>(fw) && !qobject_cast<QSpinBox*>(fw) && !qobject_cast<QComboBox*>(fw)) {
            auto* ke = static_cast<QKeyEvent*>(ev);
            switch (ke->key()) {
            case Qt::Key_Space: m_playPauseBtn->click(); return true;
            case Qt::Key_F: toggleFullscreen(); return true;
            case Qt::Key_Escape: if (isFullScreen()) { toggleFullscreen(); return true; } break;
            case Qt::Key_M: m_video->setMuted(!m_video->isMuted()); return true;
            case Qt::Key_S: if (!(ke->modifiers() & Qt::ShiftModifier)) { m_video->stopPlayback(); return true; } break;
            case Qt::Key_Left: m_video->seekRelative((ke->modifiers() & Qt::ControlModifier) ? -60.0 : -10.0); return true;
            case Qt::Key_Right: m_video->seekRelative((ke->modifiers() & Qt::ControlModifier) ? 60.0 : 10.0); return true;
            case Qt::Key_Up: m_video->setVolume(m_video->volume() + 5); m_video->showOsd(QString("Volume: %1%").arg(m_video->volume())); return true;
            case Qt::Key_Down: m_video->setVolume(m_video->volume() - 5); m_video->showOsd(QString("Volume: %1%").arg(m_video->volume())); return true;
            case Qt::Key_N: m_playlist->playNext(true); return true;
            case Qt::Key_P: if (!(ke->modifiers() & Qt::ControlModifier)) { m_playlist->playPrev(); return true; } break;
            case Qt::Key_E: m_video->frameStep(); return true;
            case Qt::Key_BracketLeft: m_video->setSpeed(m_video->speed() - 0.1); return true;
            case Qt::Key_BracketRight: m_video->setSpeed(m_video->speed() + 0.1); return true;
            default: break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}

} // namespace Tavrynexo::UI
