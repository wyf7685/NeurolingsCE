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
#include "../../runtime/ManagerRuntimeState.hpp"
#include "../ManagerUiState.hpp"
#include "../ManagerUiHelpers.hpp"
#include <QDesktopServices>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QListWidget>
#include <QSizePolicy>
#include <QUrl>
#include <QVBoxLayout>
#include "ElaPushButton.h"

static void configureActionButton(ElaPushButton *button)
{
    const QFontMetrics metrics(button->font());
    const int horizontalPadding = 44;
    button->setMinimumWidth(metrics.horizontalAdvance(button->text()) + horizontalPadding);
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void ShijimaManager::setupHomePage() {
    m_ui->homePage = new QWidget(this);
    auto *homeLayout = new QVBoxLayout(m_ui->homePage);
    homeLayout->setContentsMargins(10, 10, 10, 10);
    homeLayout->setSpacing(8);

    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(8);

    auto *btnSpawn = new ElaPushButton(tr("Spawn Random"));
    configureActionButton(btnSpawn);
    connect(btnSpawn, &ElaPushButton::clicked, this, &ShijimaManager::spawnClicked);
    actionRow->addWidget(btnSpawn);

    auto *btnImport = new ElaPushButton(tr("Import"));
    configureActionButton(btnImport);
    connect(btnImport, &ElaPushButton::clicked, this, &ShijimaManager::importAction);
    actionRow->addWidget(btnImport);

    auto *btnDelete = new ElaPushButton(tr("Delete"));
    configureActionButton(btnDelete);
    connect(btnDelete, &ElaPushButton::clicked, this, &ShijimaManager::deleteAction);
    actionRow->addWidget(btnDelete);

    auto *btnFolder = new ElaPushButton(tr("Show Folder"));
    configureActionButton(btnFolder);
    connect(btnFolder, &ElaPushButton::clicked, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_runtime->mascotsPath));
    });
    actionRow->addWidget(btnFolder);

    actionRow->addStretch();
    homeLayout->addLayout(actionRow);

    m_ui->listWidget->setParent(m_ui->homePage);
    homeLayout->addWidget(m_ui->listWidget, 1);

    addPageNode(tr("Home"), m_ui->homePage, ElaIconType::House);
}
