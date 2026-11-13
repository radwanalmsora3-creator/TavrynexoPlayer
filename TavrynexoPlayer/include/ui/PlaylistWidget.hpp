#pragma once
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QWidget>

namespace Tavrynexo::UI {

enum class RepeatMode { Off, All, One };

class PlaylistWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistWidget(QWidget* parent = nullptr);

    static bool isMediaFile(const QString& path);
    static bool isSubtitleFile(const QString& path);

    int addPaths(const QStringList& paths, bool clearFirst = false);
    void playIndex(int index);
    void playNext(bool manual = false);
    void playPrev();
    void updateDuration(const QString& path, double seconds);
    bool exportM3u8(const QString& outPath) const;

    int currentIndex() const { return m_currentIndex; }
    int count() const { return m_tree->topLevelItemCount(); }
    RepeatMode repeatMode() const { return m_repeat; }
    bool isShuffle() const { return m_shuffle; }
    void setRepeatMode(RepeatMode mode);
    void setShuffle(bool enabled);
    void setThemeDark(bool dark);

signals:
    void playRequested(const QString& path);
    void repeatModeChanged(RepeatMode mode);
    void shuffleChanged(bool enabled);
    void filesDropped(const QStringList& paths);
    void addFilesClicked();
    void addFolderClicked();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void filterItems(const QString& text);
    void removeSelected();
    void clearAll();

private:
    void refreshRowNumbers();
    QLineEdit* m_search{nullptr};
    QTreeWidget* m_tree{nullptr};
    QPushButton* m_addFileBtn{nullptr};
    QPushButton* m_addDirBtn{nullptr};
    QPushButton* m_removeBtn{nullptr};
    QPushButton* m_clearBtn{nullptr};
    int m_currentIndex{-1};
    RepeatMode m_repeat{RepeatMode::Off};
    bool m_shuffle{false};
};

} // namespace Tavrynexo::UI
