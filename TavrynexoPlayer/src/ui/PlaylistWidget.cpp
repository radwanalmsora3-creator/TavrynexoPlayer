#include "ui/PlaylistWidget.hpp"
#include "ui/IconFactory.hpp"
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMimeData>
#include <QRandomGenerator>
#include <QSet>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

namespace Tavrynexo::UI {

bool PlaylistWidget::isSubtitleFile(const QString& path) {
    static const QSet<QString> exts = {"srt", "ass", "ssa", "vtt", "sub", "idx"};
    return exts.contains(QFileInfo(path).suffix().toLower());
}

bool PlaylistWidget::isMediaFile(const QString& path) {
    if (path.startsWith("http://", Qt::CaseInsensitive) ||
        path.startsWith("https://", Qt::CaseInsensitive) ||
        path.startsWith("rtsp://", Qt::CaseInsensitive)) return true;
    static const QSet<QString> exts = {
        "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "m4v", "ts", "m2ts",
        "mpg", "mpeg", "3gp", "vob", "ogv", "mp3", "flac", "wav", "aac", "m4a",
        "ogg", "opus", "wma", "mka", "alac"
    };
    return exts.contains(QFileInfo(path).suffix().toLower());
}

PlaylistWidget::PlaylistWidget(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto* bar = new QHBoxLayout();
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("Search playlist...");
    m_search->setClearButtonEnabled(true);

    m_addFileBtn = new QPushButton("Add Files", this);
    m_addDirBtn = new QPushButton("Add Folder", this);
    m_removeBtn = new QPushButton("Remove", this);
    m_clearBtn = new QPushButton("Clear", this);

    bar->addWidget(m_search, 1);
    bar->addWidget(m_addFileBtn);
    bar->addWidget(m_addDirBtn);
    bar->addWidget(m_removeBtn);
    bar->addWidget(m_clearBtn);
    root->addLayout(bar);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"#", "Title", "Folder", "Duration"});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    root->addWidget(m_tree, 1);

    connect(m_search, &QLineEdit::textChanged, this, &PlaylistWidget::filterItems);
    connect(m_addFileBtn, &QPushButton::clicked, this, &PlaylistWidget::addFilesClicked);
    connect(m_addDirBtn, &QPushButton::clicked, this, &PlaylistWidget::addFolderClicked);
    connect(m_removeBtn, &QPushButton::clicked, this, &PlaylistWidget::removeSelected);
    connect(m_clearBtn, &QPushButton::clicked, this, &PlaylistWidget::clearAll);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        int idx = m_tree->indexOfTopLevelItem(item);
        if (idx >= 0) playIndex(idx);
    });
}

int PlaylistWidget::addPaths(const QStringList& paths, bool clearFirst) {
    if (clearFirst) clearAll();
    QStringList media;
    for (const QString& raw : paths) {
        QFileInfo fi(raw);
        if (fi.isDir()) {
            QStringList dirFiles;
            QDirIterator it(raw, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString f = it.next();
                if (isMediaFile(f)) dirFiles << f;
            }
            dirFiles.sort(Qt::CaseInsensitive);
            media.append(dirFiles);
        } else if (isMediaFile(raw)) {
            media << raw;
        }
    }
    int added = 0;
    for (const QString& m : media) {
        QFileInfo fi(m);
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, QString::number(m_tree->topLevelItemCount()));
        item->setText(1, fi.fileName().isEmpty() ? m : fi.fileName());
        item->setText(2, fi.dir().dirName());
        item->setText(3, "--:--");
        item->setData(0, Qt::UserRole, m);
        ++added;
    }
    refreshRowNumbers();
    return added;
}

void PlaylistWidget::playIndex(int idx) {
    if (idx < 0 || idx >= m_tree->topLevelItemCount()) return;
    m_currentIndex = idx;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* it = m_tree->topLevelItem(i);
        QFont f = it->font(1); f.setBold(i == idx);
        for (int c = 0; c < 4; ++c) it->setFont(c, f);
    }
    m_tree->setCurrentItem(m_tree->topLevelItem(idx));
    emit playRequested(m_tree->topLevelItem(idx)->data(0, Qt::UserRole).toString());
}

void PlaylistWidget::playNext(bool manual) {
    int n = count();
    if (n == 0) return;
    if (m_repeat == RepeatMode::One && !manual) { playIndex(qMax(0, m_currentIndex)); return; }
    if (m_shuffle) {
        int next = n > 1 ? QRandomGenerator::global()->bounded(n) : 0;
        if (n > 1 && next == m_currentIndex) next = (next + 1) % n;
        playIndex(next); return;
    }
    int next = m_currentIndex + 1;
    if (next >= n) {
        if (m_repeat == RepeatMode::All || manual) next = 0;
        else return;
    }
    playIndex(next);
}

void PlaylistWidget::playPrev() {
    int n = count();
    if (n == 0) return;
    int prev = m_currentIndex - 1;
    if (prev < 0) prev = (m_repeat == RepeatMode::All) ? (n - 1) : 0;
    playIndex(prev);
}

void PlaylistWidget::updateDuration(const QString& path, double seconds) {
    if (seconds <= 0.0) return;
    qint64 s = qRound64(seconds);
    qint64 h = s / 3600, m = (s % 3600) / 60, r = s % 60;
    QString str = h > 0 ? QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(r, 2, 10, QChar('0'))
                        : QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(r, 2, 10, QChar('0'));
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* it = m_tree->topLevelItem(i);
        if (it->data(0, Qt::UserRole).toString() == path) { it->setText(3, str); break; }
    }
}

bool PlaylistWidget::exportM3u8(const QString& outPath) const {
    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out << "#EXTM3U\n";
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
        out << m_tree->topLevelItem(i)->data(0, Qt::UserRole).toString() << "\n";
    return true;
}

void PlaylistWidget::setRepeatMode(RepeatMode mode) { m_repeat = mode; emit repeatModeChanged(mode); }
void PlaylistWidget::setShuffle(bool enabled) { m_shuffle = enabled; emit shuffleChanged(enabled); }
void PlaylistWidget::setThemeDark(bool dark) {
    Q_UNUSED(dark);
}

void PlaylistWidget::filterItems(const QString& text) {
    QString q = text.trimmed();
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* it = m_tree->topLevelItem(i);
        bool match = q.isEmpty() || it->text(1).contains(q, Qt::CaseInsensitive) || it->text(2).contains(q, Qt::CaseInsensitive);
        it->setHidden(!match);
    }
}

void PlaylistWidget::removeSelected() {
    auto sel = m_tree->selectedItems();
    for (auto* it : sel) {
        int idx = m_tree->indexOfTopLevelItem(it);
        if (idx == m_currentIndex) m_currentIndex = -1;
        else if (idx < m_currentIndex) --m_currentIndex;
        delete m_tree->takeTopLevelItem(idx);
    }
    refreshRowNumbers();
}

void PlaylistWidget::clearAll() { m_tree->clear(); m_currentIndex = -1; }

void PlaylistWidget::refreshRowNumbers() {
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
        m_tree->topLevelItem(i)->setText(0, QString::number(i + 1));
}

void PlaylistWidget::dragEnterEvent(QDragEnterEvent* e) { if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
void PlaylistWidget::dragMoveEvent(QDragMoveEvent* e) { if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
void PlaylistWidget::dropEvent(QDropEvent* e) {
    QStringList paths;
    for (const QUrl& u : e->mimeData()->urls()) paths << (u.isLocalFile() ? u.toLocalFile() : u.toString());
    if (!paths.isEmpty()) { emit filesDropped(paths); e->acceptProposedAction(); }
}

} // namespace Tavrynexo::UI
