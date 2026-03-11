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

#include "shijima-qt/ui/menus/ShijimaContextMenu.hpp"

#include <vector>

#include "shijima-qt/ShijimaManager.hpp"

QAction *ShijimaContextMenu::addLabeledAction(QString const& label) {
    return addAction(label);
}

void ShijimaContextMenu::addBehaviorActions() {
    std::vector<std::string> behaviors;
    auto &list = shijimaParent()->m_mascot->initial_behavior_list();
    auto flat = list.flatten_unconditional();
    for (auto &behavior : flat) {
        if (!behavior->hidden) {
            behaviors.push_back(behavior->name);
        }
    }

    auto behaviorsMenu = addMenu(QString::fromUtf8("\xf0\x9f\x8e\xad ") + tr("Behaviors"));
    for (std::string &behavior : behaviors) {
        QAction *action = behaviorsMenu->addAction(QString::fromStdString(behavior));
        connect(action, &QAction::triggered, [this, behavior]() {
            shijimaParent()->m_mascot->next_behavior(behavior);
        });
    }
}

void ShijimaContextMenu::addControlActions() {
    QAction *action = addLabeledAction(QString::fromUtf8("\xe2\x8f\xb8 ") + tr("Pause"));
    action->setCheckable(true);
    action->setChecked(shijimaParent()->m_paused);
    connect(action, &QAction::triggered, [this](bool checked) {
        shijimaParent()->m_paused = checked;
    });

    action = addLabeledAction(QString::fromUtf8("\xe2\x9c\xa8 ") + tr("Call another"));
    connect(action, &QAction::triggered, [this]() {
        ShijimaManager::defaultManager()->spawn(shijimaParent()->mascotName()
            .toStdString());
    });
}

void ShijimaContextMenu::addManagementActions() {
    QAction *action = addLabeledAction(QString::fromUtf8("\xf0\x9f\x93\x8b ") + tr("Show manager"));
    connect(action, &QAction::triggered, []() {
        ShijimaManager::defaultManager()->setManagerVisible(true);
    });

    action = addLabeledAction(QString::fromUtf8("\xf0\x9f\x94\x8d ") + tr("Inspect"));
    connect(action, &QAction::triggered, [this]() {
        shijimaParent()->showInspector();
    });
}

void ShijimaContextMenu::addDismissActions() {
    QAction *action = addLabeledAction(tr("Dismiss all but one"));
    connect(action, &QAction::triggered, [this]() {
        ShijimaManager::defaultManager()->killAllButOne(shijimaParent());
    });

    action = addLabeledAction(tr("Dismiss all"));
    connect(action, &QAction::triggered, []() {
        ShijimaManager::defaultManager()->killAll();
    });

    action = addLabeledAction(QString::fromUtf8("\xc3\x97 ") + tr("Dismiss"));
    connect(action, &QAction::triggered, shijimaParent(), &ShijimaWidget::closeAction);
}
