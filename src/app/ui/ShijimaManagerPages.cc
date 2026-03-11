//
// Shijima-Qt - Cross-platform shimeji simulation app for desktop
// Copyright (C) 2025 pixelomer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "shijima-qt/ShijimaManager.hpp"
#include "../ShijimaManagerInternal.hpp"
#include <QAction>
#include <QColorDialog>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QUrl>
#include <QVariant>
#include <QVBoxLayout>
#include "shijima-qt/ShijimaLicensesDialog.hpp"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ElaTheme.h"
#include "ElaToggleSwitch.h"

#ifndef NEUROLINGSCE_VERSION
#define NEUROLINGSCE_VERSION "0.1.0"
#endif

void ShijimaManager::setupNavigation() {
    m_ui->homePage = new QWidget(this);
    auto *homeLayout = new QVBoxLayout(m_ui->homePage);
    homeLayout->setContentsMargins(10, 10, 10, 10);
    homeLayout->setSpacing(8);

    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(8);

    auto *btnSpawn = new ElaPushButton(tr("Spawn Random"));
    connect(btnSpawn, &ElaPushButton::clicked, this, &ShijimaManager::spawnClicked);
    actionRow->addWidget(btnSpawn);

    auto *btnImport = new ElaPushButton(tr("Import"));
    connect(btnImport, &ElaPushButton::clicked, this, &ShijimaManager::importAction);
    actionRow->addWidget(btnImport);

    auto *btnDelete = new ElaPushButton(tr("Delete"));
    connect(btnDelete, &ElaPushButton::clicked, this, &ShijimaManager::deleteAction);
    actionRow->addWidget(btnDelete);

    auto *btnFolder = new ElaPushButton(tr("Show Folder"));
    connect(btnFolder, &ElaPushButton::clicked, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_runtime->mascotsPath));
    });
    actionRow->addWidget(btnFolder);

    actionRow->addStretch();
    homeLayout->addLayout(actionRow);

    m_ui->listWidget->setParent(m_ui->homePage);
    homeLayout->addWidget(m_ui->listWidget, 1);

    addPageNode(tr("Home"), m_ui->homePage, ElaIconType::House);

    m_ui->settingsPage = new QWidget(this);
    auto *settingsLayout = new QVBoxLayout(m_ui->settingsPage);
    settingsLayout->setContentsMargins(10, 10, 10, 10);
    settingsLayout->setSpacing(12);

    {
        static const QString key = "multiplicationEnabled";
        bool initial = m_settings.value(key, QVariant::fromValue(true)).toBool();
        for (auto &env : m_runtime->env) {
            env->allows_breeding = initial;
        }

        auto *area = new ElaScrollPageArea(m_ui->settingsPage);
        auto *row = new QHBoxLayout(area);
        auto *label = new ElaText(tr("Multiplication"), m_ui->settingsPage);
        label->setWordWrap(false);
        label->setTextPixelSize(15);
        row->addWidget(label);
        row->addStretch();
        auto *toggle = new ElaToggleSwitch(m_ui->settingsPage);
        toggle->setIsToggled(initial);
        connect(toggle, &ElaToggleSwitch::toggled, [this](bool checked) {
            for (auto &env : m_runtime->env) {
                env->allows_breeding = checked;
            }
            m_settings.setValue("multiplicationEnabled", QVariant::fromValue(checked));
        });
        row->addWidget(toggle);
        settingsLayout->addWidget(area);
    }

    {
        static const QString key = "speechBubbleEnabled";
        bool initial = m_settings.value(key, QVariant::fromValue(true)).toBool();

        auto *area = new ElaScrollPageArea(m_ui->settingsPage);
        auto *row = new QHBoxLayout(area);
        auto *label = new ElaText(tr("Speech Bubble"), m_ui->settingsPage);
        label->setWordWrap(false);
        label->setTextPixelSize(15);
        row->addWidget(label);
        row->addStretch();
        auto *toggle = new ElaToggleSwitch(m_ui->settingsPage);
        toggle->setIsToggled(initial);
        connect(toggle, &ElaToggleSwitch::toggled, [this](bool checked) {
            m_settings.setValue("speechBubbleEnabled", QVariant::fromValue(checked));
        });
        row->addWidget(toggle);
        settingsLayout->addWidget(area);
    }

    {
        static const QString key = "speechBubbleClickCount";
        int initial = m_settings.value(key, 1).toInt();

        auto *area = new ElaScrollPageArea(m_ui->settingsPage);
        auto *row = new QHBoxLayout(area);
        auto *label = new ElaText(tr("Speech Bubble Click Count"), m_ui->settingsPage);
        label->setWordWrap(false);
        label->setTextPixelSize(15);
        row->addWidget(label);
        row->addStretch();
        auto *spinBox = new QSpinBox(m_ui->settingsPage);
        spinBox->setRange(1, 10);
        spinBox->setValue(initial);
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
            m_settings.setValue("speechBubbleClickCount", val);
        });
        row->addWidget(spinBox);
        settingsLayout->addWidget(area);
    }

    {
        auto *area = new ElaScrollPageArea(m_ui->settingsPage);
        auto *row = new QHBoxLayout(area);
        auto *label = new ElaText(tr("Windowed Mode"), m_ui->settingsPage);
        label->setWordWrap(false);
        label->setTextPixelSize(15);
        row->addWidget(label);
        row->addStretch();
        auto *toggle = new ElaToggleSwitch(m_ui->settingsPage);
        toggle->setIsToggled(windowedMode());

        if (!m_ui->windowedModeAction) {
            m_ui->windowedModeAction = new QAction(this);
        }
        m_ui->windowedModeAction->setCheckable(true);
        m_ui->windowedModeAction->setChecked(windowedMode());

        connect(toggle, &ElaToggleSwitch::toggled, [this](bool checked) {
            setWindowedMode(checked);
        });
        connect(m_ui->windowedModeAction, &QAction::toggled, toggle, &ElaToggleSwitch::setIsToggled);
        row->addWidget(toggle);
        settingsLayout->addWidget(area);
    }

    {
        static const QString key = "windowedModeBackground";
        QColor initial = m_settings.value(key, "#FF0000").toString();
        m_ui->sandboxBackground = initial;
        updateSandboxBackground();

        auto *area = new ElaScrollPageArea(m_ui->settingsPage);
        auto *row = new QHBoxLayout(area);
        auto *label = new ElaText(tr("Background Color"), m_ui->settingsPage);
        label->setWordWrap(false);
        label->setTextPixelSize(15);
        row->addWidget(label);
        row->addStretch();
        auto *btn = new ElaPushButton("...", m_ui->settingsPage);
        connect(btn, &ElaPushButton::clicked, [this]() {
            QColorDialog dialog { this };
            dialog.setCurrentColor(m_ui->sandboxBackground);
            if (dialog.exec() == 1) {
                m_ui->sandboxBackground = dialog.selectedColor();
                m_settings.setValue("windowedModeBackground",
                    ShijimaManagerInternal::colorToString(dialog.selectedColor()));
                updateSandboxBackground();
            }
        });
        row->addWidget(btn);
        settingsLayout->addWidget(area);
    }

    {
        auto *area = new ElaScrollPageArea(m_ui->settingsPage);
        auto *row = new QHBoxLayout(area);
        auto *label = new ElaText(tr("Scale"), m_ui->settingsPage);
        label->setWordWrap(false);
        label->setTextPixelSize(15);
        row->addWidget(label);
        row->addStretch();
        auto *btn = new ElaPushButton("...", m_ui->settingsPage);
        connect(btn, &ElaPushButton::clicked, [this]() {
            static const QString key = "userScale";
            QDialog dialog { this };
            dialog.setWindowTitle(tr("Custom Scale"));
            dialog.setMinimumWidth(300);
            auto mainLayout = new QVBoxLayout(&dialog);

            auto slider = new QSlider(Qt::Horizontal);
            slider->setRange(100, 10000);
            slider->setValue(m_runtime->userScale * 1000);

            auto spin = new QDoubleSpinBox;
            spin->setRange(0.1, 10.0);
            spin->setDecimals(3);
            spin->setSingleStep(0.05);
            spin->setValue(m_runtime->userScale);

            connect(slider, &QSlider::valueChanged, [this, spin](int v) {
                m_runtime->userScale = v / 1000.0;
                spin->blockSignals(true);
                spin->setValue(m_runtime->userScale);
                spin->blockSignals(false);
            });
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [this, slider](double v) {
                    m_runtime->userScale = v;
                    slider->blockSignals(true);
                    slider->setValue(v * 1000);
                    slider->blockSignals(false);
                });

            mainLayout->addWidget(new QLabel(tr("Adjust Scale:")));
            mainLayout->addWidget(slider);
            mainLayout->addWidget(spin);

            auto btnSave = new QPushButton(tr("Save"));
            connect(btnSave, &QPushButton::clicked, &dialog, &QDialog::accept);
            mainLayout->addWidget(btnSave);

            if (dialog.exec() == QDialog::Accepted) {
                m_settings.setValue(key, m_runtime->userScale);
            }
        });
        row->addWidget(btn);
        settingsLayout->addWidget(area);
    }

    {
        auto *area = new ElaScrollPageArea(m_ui->settingsPage);
        auto *row = new QHBoxLayout(area);
        auto *label = new ElaText(tr("Language"), m_ui->settingsPage);
        label->setWordWrap(false);
        label->setTextPixelSize(15);
        row->addWidget(label);
        row->addStretch();
        auto *btn = new ElaPushButton("...", m_ui->settingsPage);
        connect(btn, &ElaPushButton::clicked, [this]() {
            QDialog d(this);
            d.setWindowTitle(tr("Select Language"));
            auto l = new QVBoxLayout(&d);

            auto btnEn = new QRadioButton("English");
            auto btnZh = new QRadioButton("中文(简体)");

            if (m_ui->currentLanguage == "zh_CN") {
                btnZh->setChecked(true);
            }
            else {
                btnEn->setChecked(true);
            }

            l->addWidget(btnEn);
            l->addWidget(btnZh);

            auto bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            connect(bb, &QDialogButtonBox::accepted, &d, &QDialog::accept);
            connect(bb, &QDialogButtonBox::rejected, &d, &QDialog::reject);
            l->addWidget(bb);

            if (d.exec() == QDialog::Accepted) {
                if (btnZh->isChecked()) {
                    switchLanguage("zh_CN");
                }
                else {
                    switchLanguage("en");
                }
            }
        });
        row->addWidget(btn);
        settingsLayout->addWidget(area);
    }

    {
        auto *area = new ElaScrollPageArea(m_ui->settingsPage);
        auto *row = new QHBoxLayout(area);
        auto *label = new ElaText(tr("Detach Speed"), m_ui->settingsPage);
        label->setWordWrap(false);
        label->setTextPixelSize(15);
        row->addWidget(label);
        row->addStretch();
        auto *btn = new ElaPushButton("...", m_ui->settingsPage);
        connect(btn, &ElaPushButton::clicked, [this]() {
            static const QString key = "detachThreshold";
            QDialog dialog(this);
            dialog.setWindowTitle(tr("Detach Speed"));
            auto l = new QVBoxLayout(&dialog);

            auto slider = new QSlider(Qt::Horizontal);
            slider->setRange(0, 200);
            slider->setValue(m_runtime->detachThreshold);

            auto spin = new QSpinBox;
            spin->setRange(0, 200);
            spin->setValue(m_runtime->detachThreshold);

            connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);

            l->addWidget(new QLabel(tr("Threshold (px/tick):")));
            l->addWidget(slider);
            l->addWidget(spin);

            auto btnSave = new QPushButton(tr("Save"));
            connect(btnSave, &QPushButton::clicked, &dialog, &QDialog::accept);
            l->addWidget(btnSave);

            if (dialog.exec() == QDialog::Accepted) {
                m_runtime->detachThreshold = spin->value();
                m_settings.setValue(key, m_runtime->detachThreshold);
            }
        });
        row->addWidget(btn);
        settingsLayout->addWidget(area);
    }

    settingsLayout->addStretch();
    addFooterNode(tr("Settings"), m_ui->settingsPage, m_ui->settingsKey, 0, ElaIconType::GearComplex);

    addFooterNode(tr("About"), m_ui->aboutKey, 0, ElaIconType::User);
    connect(this, &ElaWindow::navigationNodeClicked,
        [this](ElaNavigationType::NavigationNodeType nodeType, QString nodeKey) {
            if (nodeKey != m_ui->aboutKey) {
                return;
            }
            Q_UNUSED(nodeType);

            QString version = QStringLiteral(NEUROLINGSCE_VERSION);
            QString authorName = QString::fromUtf8("\xe8\xbd\xbb\xe5\xb0\x98\xe5\x91\xa6");

            QDialog *aboutDialog = new QDialog(this);
            aboutDialog->setWindowTitle(tr("About NeurolingsCE"));
            aboutDialog->setFixedWidth(420);
            aboutDialog->setAttribute(Qt::WA_DeleteOnClose);

            auto themeMode = eTheme->getThemeMode();
            QString primaryColor = ElaThemeColor(themeMode, PrimaryNormal).name();
            QString textColor = ElaThemeColor(themeMode, BasicText).name();
            QString detailsColor = ElaThemeColor(themeMode, BasicDetailsText).name();
            QString cardBg = ElaThemeColor(themeMode, BasicBase).name();
            QString cardBorder = ElaThemeColor(themeMode, BasicBorder).name();
            QString dialogBg = ElaThemeColor(themeMode, DialogBase).name();
            QString badgeBg = ElaThemeColor(themeMode, BasicHover).name();

            aboutDialog->setStyleSheet(QString("QDialog { background-color: %1; }").arg(dialogBg));

            QVBoxLayout *layout = new QVBoxLayout;
            layout->setSpacing(12);
            layout->setContentsMargins(24, 24, 24, 24);
            aboutDialog->setLayout(layout);

            QLabel *iconLabel = new QLabel;
            QIcon appIcon = qApp->windowIcon();
            if (!appIcon.isNull()) {
                iconLabel->setPixmap(appIcon.pixmap(64, 64));
            }
            iconLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(iconLabel);

            QLabel *titleLabel = new QLabel(
                QString("<h2 style='color: %1; margin: 0;'>NeurolingsCE</h2>").arg(primaryColor));
            titleLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(titleLabel);

            QLabel *versionLabel = new QLabel(
                QString("<span style='background-color: %1; color: %2; "
                    "padding: 3px 12px; border-radius: 10px; font-size: 12px;'>v%3</span>")
                .arg(badgeBg, primaryColor, version));
            versionLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(versionLabel);

            QLabel *descLabel = new QLabel(
                tr("A cross-platform shimeji desktop pet runner."));
            descLabel->setAlignment(Qt::AlignCenter);
            descLabel->setStyleSheet(QString("color: %1; margin: 4px 0;").arg(detailsColor));
            layout->addWidget(descLabel);

            QString infoHtml = QStringLiteral(
                "<table cellpadding='4' style='color: %1;'>"
                "<tr><td style='color: %2; font-weight: bold;'>%3</td>"
                "<td><a href='https://space.bilibili.com/178381315' "
                "style='color: %2; text-decoration: none;'>%4</a></td></tr>"
                "<tr><td style='color: %2; font-weight: bold;'>%5</td>"
                "<td><a href='https://github.com/pixelomer/Shijima-Qt' "
                "style='color: %2; text-decoration: none;'>Shijima-Qt</a> by pixelomer</td></tr>"
                "<tr><td style='color: %2; font-weight: bold;'>%6</td>"
                "<td><a href='https://github.com/qingchenyouforcc/NeurolingsCE' "
                "style='color: %2; text-decoration: none;'>GitHub</a></td></tr>"
                "<tr><td style='color: %2; font-weight: bold;'>%7</td>"
                "<td>423902950</td></tr>"
                "<tr><td style='color: %2; font-weight: bold;'>%8</td>"
                "<td>125081756</td></tr>"
                "</table>")
                .arg(textColor, primaryColor,
                     tr("Author"), authorName,
                     tr("Based on"),
                     tr("Project"),
                     tr("Feedback QQ"),
                     tr("Chat QQ"));
            QLabel *infoLabel = new QLabel(infoHtml);
            infoLabel->setOpenExternalLinks(true);
            infoLabel->setAlignment(Qt::AlignCenter);
            infoLabel->setStyleSheet(QString("background-color: %1; border: 1px solid %2; "
                "border-radius: 8px; padding: 12px;").arg(cardBg, cardBorder));
            layout->addWidget(infoLabel);

            QString buttonStyle = QString(
                "QPushButton { background-color: %1; color: %2; border: 1px solid %3; "
                "border-radius: 6px; padding: 6px 16px; font-size: 13px; }"
                "QPushButton:hover { background-color: %4; }"
                "QPushButton:pressed { background-color: %5; }"
            ).arg(cardBg, textColor, cardBorder,
                 ElaThemeColor(themeMode, BasicHover).name(),
                 ElaThemeColor(themeMode, BasicPress).name());

            QPushButton *licensesButton = new QPushButton(tr("View Licenses"));
            licensesButton->setStyleSheet(buttonStyle);
            connect(licensesButton, &QPushButton::clicked, [this]() {
                ShijimaLicensesDialog dialog { this };
                dialog.exec();
            });
            layout->addWidget(licensesButton);

            QHBoxLayout *linksRow = new QHBoxLayout;
            linksRow->addStretch();
            auto *btnWeb = new QPushButton(tr("Shijima Website"));
            btnWeb->setStyleSheet(buttonStyle);
            connect(btnWeb, &QPushButton::clicked, []() {
                QDesktopServices::openUrl(QUrl { "https://getshijima.app" });
            });
            linksRow->addWidget(btnWeb);
            auto *btnBug = new QPushButton(tr("Report Issue"));
            btnBug->setStyleSheet(buttonStyle);
            connect(btnBug, &QPushButton::clicked, []() {
                QDesktopServices::openUrl(QUrl { "https://github.com/qingchenyouforcc/NeurolingsCE/issues" });
            });
            linksRow->addWidget(btnBug);
            linksRow->addStretch();
            layout->addLayout(linksRow);

            QHBoxLayout *buttonRow = new QHBoxLayout;
            buttonRow->addStretch();
            QPushButton *closeButton = new QPushButton(tr("Close"));
            closeButton->setStyleSheet(buttonStyle);
            closeButton->setMinimumWidth(100);
            connect(closeButton, &QPushButton::clicked, aboutDialog, &QDialog::accept);
            buttonRow->addWidget(closeButton);
            buttonRow->addStretch();
            layout->addLayout(buttonRow);

            aboutDialog->exec();
        });
}
