#include "ui/Dialogs.hpp"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSettings>
#include <QTableWidget>
#include <QVBoxLayout>

namespace Tavrynexo::UI {

PreferencesDialog::PreferencesDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Preferences - Tavrynexo Player");
    setMinimumWidth(420);
    QSettings s("Tavrynexo", "TavrynexoPlayer");

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem("Classic VLC (Light Controls)", "light");
    m_themeCombo->addItem("Dark Cinema", "dark");
    m_themeCombo->setCurrentIndex(s.value("ui/theme", "light").toString() == "dark" ? 1 : 0);

    m_hwdecCombo = new QComboBox(this);
    m_hwdecCombo->addItem("Auto-Safe (Hardware DXVA2/D3D11VA)", "auto-safe");
    m_hwdecCombo->addItem("Auto-Copy (Hardware Copy)", "auto-copy");
    m_hwdecCombo->addItem("Off (CPU Software Decode)", "no");
    int hwIdx = m_hwdecCombo->findData(s.value("player/hwdec", "auto-safe").toString());
    m_hwdecCombo->setCurrentIndex(hwIdx >= 0 ? hwIdx : 0);

    m_resumeCheck = new QCheckBox("Resume playback from last saved position", this);
    m_resumeCheck->setChecked(s.value("player/resume", true).toBool());

    m_autoHideCheck = new QCheckBox("Auto-hide controls in fullscreen mode", this);
    m_autoHideCheck->setChecked(s.value("player/autoHideFs", true).toBool());

    m_maxVolCombo = new QComboBox(this);
    m_maxVolCombo->addItem("100% (Standard)", 100);
    m_maxVolCombo->addItem("150% (Boosted)", 150);
    m_maxVolCombo->setCurrentIndex(s.value("player/maxVolume", 150).toInt() == 100 ? 0 : 1);

    m_subSizeSpin = new QSpinBox(this);
    m_subSizeSpin->setRange(16, 96);
    m_subSizeSpin->setValue(s.value("sub/fontSize", 48).toInt());

    form->addRow("Interface Theme:", m_themeCombo);
    form->addRow("Hardware Decoding:", m_hwdecCombo);
    form->addRow("Max Volume:", m_maxVolCombo);
    form->addRow("Subtitle Font Size:", m_subSizeSpin);
    form->addRow("", m_resumeCheck);
    form->addRow("", m_autoHideCheck);
    root->addLayout(form);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &PreferencesDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &PreferencesDialog::reject);
    root->addWidget(btns);
}

void PreferencesDialog::accept() {
    QSettings s("Tavrynexo", "TavrynexoPlayer");
    s.setValue("ui/theme", m_themeCombo->currentData().toString());
    s.setValue("player/hwdec", m_hwdecCombo->currentData().toString());
    s.setValue("player/resume", m_resumeCheck->isChecked());
    s.setValue("player/autoHideFs", m_autoHideCheck->isChecked());
    s.setValue("player/maxVolume", m_maxVolCombo->currentData().toInt());
    s.setValue("sub/fontSize", m_subSizeSpin->value());
    QDialog::accept();
}

MediaInfoDialog::MediaInfoDialog(Tavrynexo::Player::MpvVideoWidget* p, QWidget* parent) : QDialog(parent) {
    setWindowTitle("Media Information");
    setMinimumWidth(460);
    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    QString file = p ? p->currentFile() : QString();
    QString title = (p && !p->mediaTitle().isEmpty()) ? p->mediaTitle() : QFileInfo(file).fileName();
    QString res = (p && p->videoWidth() > 0) ? QString("%1 x %2").arg(p->videoWidth()).arg(p->videoHeight()) : "N/A";
    QString hw = (p && !p->hwdecCurrent().isEmpty()) ? p->hwdecCurrent() : "Software / None";
    double dur = p ? p->duration() : 0.0;
    qint64 s = qRound64(dur);
    QString durStr = dur > 0 ? QString("%1:%2:%3").arg(s/3600,2,10,QChar('0')).arg((s%3600)/60,2,10,QChar('0')).arg(s%60,2,10,QChar('0')) : "--:--";

    QStringList trSummary;
    if (p) {
        for (const auto& t : p->tracks())
            trSummary << QString("[%1 #%2] %3 (%4)%5").arg(t.type).arg(t.id).arg(t.codec, t.lang.isEmpty() ? "und" : t.lang, t.selected ? " *" : "");
    }
    form->addRow("Title:", new QLabel(title.isEmpty() ? "No media loaded" : title, this));
    form->addRow("Source:", new QLabel(file.isEmpty() ? "-" : file, this));
    form->addRow("Resolution:", new QLabel(res, this));
    form->addRow("HW Decoder:", new QLabel(hw, this));
    form->addRow("Duration:", new QLabel(durStr, this));
    auto* trLbl = new QLabel(trSummary.isEmpty() ? "None" : trSummary.join("\n"), this);
    trLbl->setWordWrap(true);
    form->addRow("Streams:", trLbl);
    root->addLayout(form);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

ShortcutsDialog::ShortcutsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Keyboard Shortcuts");
    resize(420, 360);
    auto* root = new QVBoxLayout(this);
    auto* tbl = new QTableWidget(14, 2, this);
    tbl->setHorizontalHeaderLabels({"Shortcut", "Action"});
    tbl->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tbl->verticalHeader()->hide();
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    const QPair<QString, QString> rows[] = {
        {"Space", "Play / Pause"}, {"S", "Stop"}, {"F / F11 / Double-Click", "Toggle Fullscreen"},
        {"Esc", "Exit Fullscreen"}, {"Left / Right", "Seek -10s / +10s"}, {"Ctrl+Left / Ctrl+Right", "Seek -1m / +1m"},
        {"Up / Down / Wheel", "Volume +5% / -5%"}, {"M", "Mute / Unmute"}, {"[ / ]", "Speed -0.1x / +0.1x"},
        {"N / P", "Next / Previous Item"}, {"E", "Frame Step"}, {"Shift+S", "Take Snapshot"},
        {"Ctrl+O / Ctrl+F", "Open File / Folder"}, {"Ctrl+L / Ctrl+P", "Playlist / Preferences"}
    };
    for (int i = 0; i < 14; ++i) {
        tbl->setItem(i, 0, new QTableWidgetItem(rows[i].first));
        tbl->setItem(i, 1, new QTableWidgetItem(rows[i].second));
    }
    root->addWidget(tbl);
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

} // namespace Tavrynexo::UI
