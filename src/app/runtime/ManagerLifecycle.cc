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

#include "ManagerRuntimeHelpers.hpp"
#include "../ui/ManagerUiHelpers.hpp"
#include "shijima-qt/ui/mascot/ShijimaWidget.hpp"

#include <QCloseEvent>
#include <QGuiApplication>

static ShijimaManager *m_defaultManager = nullptr;

ShijimaManager *ShijimaManager::defaultManager() {
    if (m_defaultManager == nullptr) {
        m_defaultManager = new ShijimaManager;
    }
    return m_defaultManager;
}

void ShijimaManager::finalize() {
    if (m_defaultManager != nullptr) {
        delete m_defaultManager;
        m_defaultManager = nullptr;
    }
}

std::unique_lock<std::mutex> ShijimaManager::acquireLock() {
    return std::unique_lock<std::mutex> { m_runtime->mutex };
}

QString const& ShijimaManager::mascotsPath() {
    return m_runtime->mascotsPath;
}

ShijimaManager::~ShijimaManager() {
    disconnect(qApp, &QGuiApplication::screenAdded,
        this, &ShijimaManager::screenAdded);
    disconnect(qApp, &QGuiApplication::screenRemoved,
        this, &ShijimaManager::screenRemoved);
}

void ShijimaManager::abortPendingCallbacks() {
    auto lock = acquireLock();
    m_runtime->shuttingDown.store(true);
    m_runtime->tickCallbacks.clear();
    m_runtime->hasTickCallbacks = false;
    m_runtime->tickCallbackCompletion.notify_all();
}

void ShijimaManager::onTickSync(std::function<void(ShijimaManager *)> callback) {
    if (m_runtime->shuttingDown.load()) {
        return;
    }

    auto lock = acquireLock();
    if (m_runtime->shuttingDown.load()) {
        return;
    }

    m_runtime->hasTickCallbacks = true;
    m_runtime->tickCallbacks.push_back(callback);
    m_runtime->tickCallbackCompletion.wait(lock, [this]{
        return m_runtime->shuttingDown.load() || m_runtime->tickCallbacks.empty();
    });
}

void ShijimaManager::closeEvent(QCloseEvent *event) {
#if !defined(__APPLE__)
    if (!m_allowClose) {
        event->ignore();
#if defined(_WIN32)
        if (m_runtime->mascots.size() == 0) {
            askClose();
        }
        else {
            setManagerVisible(false);
        }
#else
        askClose();
#endif
        return;
    }

    abortPendingCallbacks();
    ShijimaManagerUiInternal::teardownTrayIcon();
    if (m_runtime->mascotTimer != 0) {
        killTimer(m_runtime->mascotTimer);
        m_runtime->mascotTimer = 0;
    }
    if (m_runtime->windowObserverTimer != 0) {
        killTimer(m_runtime->windowObserverTimer);
        m_runtime->windowObserverTimer = 0;
    }
    m_httpApi.stop();

    while (!m_runtime->mascots.empty()) {
        ShijimaWidget *mascot = m_runtime->mascots.front();
        m_runtime->mascots.pop_front();
        if (mascot != nullptr) {
            mascot->close();
            delete mascot;
        }
    }
    m_runtime->mascotsById.clear();

    if (m_ui->sandboxWidget != nullptr) {
        m_ui->sandboxWidget->close();
        delete m_ui->sandboxWidget;
        m_ui->sandboxWidget = nullptr;
    }

    event->accept();
#else
    event->ignore();
    setManagerVisible(false);
#endif
}

void ShijimaManager::timerEvent(QTimerEvent *event) {
    int timerId = event->timerId();
    if (timerId == m_runtime->mascotTimer) {
        tick();
    }
    else if (timerId == m_runtime->windowObserverTimer) {
        m_runtime->windowObserver.tick();
    }
}
