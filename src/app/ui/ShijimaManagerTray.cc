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
#include "ShijimaManagerUiInternal.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QMenu>
#include <QStyle>
#include <QSystemTrayIcon>

namespace {

QSystemTrayIcon *g_trayIcon = nullptr;
QMenu *g_trayMenu = nullptr;

QIcon makeTrayIconFallback(QWidget *widget) {
    QIcon ico { QStringLiteral(":/neurolingsce.ico") };
    if (!ico.isNull()) {
        return ico;
    }

    ico = QIcon { QStringLiteral(":/neurolingsce.png") };
    if (!ico.isNull()) {
        return ico;
    }

    if (qApp != nullptr) {
        QIcon appIcon = qApp->windowIcon();
        if (!appIcon.isNull()) {
            return appIcon;
        }
    }

    if (widget != nullptr) {
        QIcon windowIcon = widget->windowIcon();
        if (!windowIcon.isNull()) {
            return windowIcon;
        }
    }

    QIcon themed = QIcon::fromTheme("shijima-qt");
    if (!themed.isNull()) {
        return themed;
    }
    if (qApp != nullptr && qApp->style() != nullptr) {
        return qApp->style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    return {};
}

}

namespace ShijimaManagerUiInternal {

void refreshTrayMenu(ShijimaManager *manager) {
    if (g_trayMenu == nullptr || manager == nullptr) {
        return;
    }

    g_trayMenu->clear();

    QAction *toggleAction = g_trayMenu->addAction(
        manager->isVisible()
            ? QCoreApplication::translate("ShijimaManager", "Hide")
            : QCoreApplication::translate("ShijimaManager", "Show"));
    QObject::connect(toggleAction, &QAction::triggered, [manager]() {
        manager->setManagerVisible(!manager->isVisible());
        refreshTrayMenu(manager);
    });

    QMenu *spawnMenu = g_trayMenu->addMenu(
        QCoreApplication::translate("ShijimaManager", "Spawn"));
    auto names = manager->loadedMascots().keys();
    names.sort(Qt::CaseInsensitive);
    for (auto const& name : names) {
        QAction *action = spawnMenu->addAction(name);
        QObject::connect(action, &QAction::triggered, [manager, name]() {
            manager->spawn(name.toStdString());
        });
    }
    if (names.isEmpty()) {
        QAction *empty = spawnMenu->addAction(
            QCoreApplication::translate("ShijimaManager", "(none)"));
        empty->setEnabled(false);
    }

    g_trayMenu->addSeparator();

    QAction *killAllAction = g_trayMenu->addAction(
        QCoreApplication::translate("ShijimaManager", "Kill all"));
    QObject::connect(killAllAction, &QAction::triggered, [manager]() {
        manager->killAll();
    });

    QAction *quitAction = g_trayMenu->addAction(
        QCoreApplication::translate("ShijimaManager", "Quit"));
    QObject::connect(quitAction, &QAction::triggered, [manager]() {
        QMetaObject::invokeMethod(manager, [manager]() {
            manager->quitAction();
        }, Qt::QueuedConnection);
    });
}

void setupTrayIcon(ShijimaManager *manager) {
    if (manager == nullptr || !QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    if (g_trayIcon != nullptr) {
        return;
    }

    g_trayIcon = new QSystemTrayIcon(manager);
    g_trayIcon->setToolTip(QCoreApplication::translate("ShijimaManager", APP_NAME));
    g_trayIcon->setIcon(makeTrayIconFallback(manager));

    g_trayMenu = new QMenu(manager);
    g_trayIcon->setContextMenu(g_trayMenu);
    refreshTrayMenu(manager);

    QObject::connect(g_trayIcon, &QSystemTrayIcon::activated,
        [manager](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                manager->setManagerVisible(!manager->isVisible());
                refreshTrayMenu(manager);
            }
        });

    g_trayIcon->show();
}

}
