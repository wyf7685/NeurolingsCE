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

#include "shijima-qt/ui/dialogs/inspector/ShimejiInspectorDialog.hpp"
#include "shijima-qt/ui/mascot/ShijimaWidget.hpp"
#include "ShimejiInspectorFormatting.hpp"
#include <QFormLayout>
#include <QLabel>
#include "ElaTheme.h"

void ShimejiInspectorDialog::registerRows() {
    using namespace ShimejiInspectorFormatting;

    addRow(tr("Window"), [this](shijima::mascot::manager &) {
        return vecToString(shijimaParent()->pos());
    });
    addRow(tr("Anchor"), [](shijima::mascot::manager &mascot) {
        return vecToString(mascot.state->anchor);
    });
    addRow(tr("Cursor"), [](shijima::mascot::manager &mascot) {
        return vecToString(mascot.state->get_cursor());
    });
    addRow(tr("Behavior"), [this](shijima::mascot::manager &mascot) {
        auto activeBehavior = mascot.active_behavior();
        if (activeBehavior == nullptr) {
            return tr("not available").toStdString();
        }
        return activeBehavior->name;
    });
    addRow(tr("Image"), [](shijima::mascot::manager &mascot) {
        return mascot.state->active_frame.get_name(mascot.state->looking_right);
    });
    addRow(tr("Screen"), [](shijima::mascot::manager &mascot) {
        return areaToString(mascot.state->env->screen);
    });
    addRow(tr("Work Area"), [](shijima::mascot::manager &mascot) {
        return areaToString(mascot.state->env->work_area);
    });
    addRow(tr("Active IE"), [this](shijima::mascot::manager &mascot) {
        if (mascot.state->env->active_ie.visible()) {
            return areaToString(mascot.state->env->active_ie);
        }
        return tr("not visible").toStdString();
    });
}

void ShimejiInspectorDialog::addRow(QString const& label,
    std::function<std::string(shijima::mascot::manager &)> tick)
{
    auto themeMode = eTheme->getThemeMode();
    QString primaryColor = ElaThemeColor(themeMode, PrimaryNormal).name();
    QString textColor = ElaThemeColor(themeMode, BasicText).name();
    QString baseBg = ElaThemeColor(themeMode, BasicBase).name();

    auto labelWidget = new QLabel { label };
    labelWidget->setStyleSheet(QString("font-weight: bold; color: %1;").arg(primaryColor));
    auto dataWidget = new QLabel {};
    dataWidget->setStyleSheet(
        QString("font-family: 'Consolas', 'Cascadia Mono', 'Source Code Pro', monospace;"
        "color: %1; padding: 2px 4px;"
        "background-color: %2; border-radius: 3px;").arg(textColor, baseBg));
    m_tickCallbacks.push_back([this, dataWidget, tick]() {
        auto newText = tick(shijimaParent()->mascot());
        dataWidget->setText(QString::fromStdString(newText));
    });
    m_formLayout->addRow(labelWidget, dataWidget);
}
