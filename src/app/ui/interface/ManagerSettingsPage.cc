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
#include "../ManagerUiHelpers.hpp"
#include <QAction>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QVariant>
#include <QVBoxLayout>
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

void ShijimaManager::setupSettingsPage() {
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
                ShijimaManagerUiInternal::colorToString(dialog.selectedColor()));
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
}
