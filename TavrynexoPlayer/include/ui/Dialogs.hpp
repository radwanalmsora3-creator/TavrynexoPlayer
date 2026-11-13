#pragma once
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QSpinBox>
#include "player/MpvVideoWidget.hpp"

namespace Tavrynexo::UI {

class PreferencesDialog : public QDialog {
    Q_OBJECT
public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    void accept() override;

private:
    QComboBox* m_themeCombo{nullptr};
    QComboBox* m_hwdecCombo{nullptr};
    QCheckBox* m_resumeCheck{nullptr};
    QCheckBox* m_autoHideCheck{nullptr};
    QComboBox* m_maxVolCombo{nullptr};
    QSpinBox* m_subSizeSpin{nullptr};
};

class MediaInfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit MediaInfoDialog(Tavrynexo::Player::MpvVideoWidget* player, QWidget* parent = nullptr);
};

class ShortcutsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShortcutsDialog(QWidget* parent = nullptr);
};

} // namespace Tavrynexo::UI
