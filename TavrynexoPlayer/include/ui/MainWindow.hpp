#pragma once
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>

#include "player/MpvVideoWidget.hpp"
#include "ui/PlayerControls.hpp"
#include "ui/PlaylistWidget.hpp"

namespace Tavrynexo::UI {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& initialFile = QString(), QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void openFile();
    void openMultipleFiles();
    void openFolder();
    void openNetworkStream();
    void addSubtitleFile();
    void savePlaylist();
    void togglePlaylistView();
    void toggleFullscreen();
    void toggleMinimalView();
    void openPreferences();
    void openMediaInfo();
    void showContextMenu(const QPoint& globalPos);
    void handleDroppedPaths(const QStringList& paths);
    void playMediaPath(const QString& path);
    void rebuildTrackMenus();
    void rebuildRecentMenu();
    void cycleRepeatMode();
    void applySettings();

private:
    void setupMenuBar();
    void setupCentralAndControls();
    void setupShortcuts();
    void saveCurrentResumePosition();
    void addToRecent(const QString& path);
    void updateTimeLabels(double pos, double dur);
    void updateVolumeIcon(int vol, bool muted);
    void refreshControlIcons();

    Tavrynexo::Player::MpvVideoWidget* m_video{nullptr};
    PlaylistWidget* m_playlist{nullptr};
    QStackedWidget* m_stack{nullptr};
    QWidget* m_controlBar{nullptr};

    SeekSlider* m_seekSlider{nullptr};
    WedgeVolumeSlider* m_volumeSlider{nullptr};
    QLabel* m_timeElapsed{nullptr};
    QLabel* m_timeTotal{nullptr};
    QLabel* m_volumeLabel{nullptr};

    QToolButton* m_playPauseBtn{nullptr};
    QToolButton* m_prevBtn{nullptr};
    QToolButton* m_stopBtn{nullptr};
    QToolButton* m_nextBtn{nullptr};
    QToolButton* m_fullscreenBtn{nullptr};
    QToolButton* m_effectsBtn{nullptr};
    QToolButton* m_playlistBtn{nullptr};
    QToolButton* m_repeatBtn{nullptr};
    QToolButton* m_shuffleBtn{nullptr};
    QToolButton* m_speedBtn{nullptr};
    QToolButton* m_muteBtn{nullptr};

    QMenu* m_recentMenu{nullptr};
    QMenu* m_audioTrackMenu{nullptr};
    QMenu* m_subTrackMenu{nullptr};
    QAction* m_alwaysOnTopAct{nullptr};
    QAction* m_deinterlaceAct{nullptr};
    QAction* m_minimalAct{nullptr};
    QAction* m_darkThemeAct{nullptr};

    QTimer* m_fsHideTimer{nullptr};
    bool m_isSeeking{false};
    bool m_darkTheme{false};
    bool m_resumeEnabled{true};
    bool m_autoHideFs{true};
    bool m_minimalView{false};
    double m_audioDelay{0.0};
    double m_subDelay{0.0};
};

} // namespace Tavrynexo::UI
