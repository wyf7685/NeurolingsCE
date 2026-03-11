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
#include <cmath>
#include <iostream>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

void ShijimaManager::screenAdded(QScreen *screen) {
    if (!m_env.contains(screen)) {
        auto env = std::make_shared<shijima::mascot::environment>();
        m_env[screen] = env;
        m_reverseEnv[env.get()] = screen;
        auto primary = QGuiApplication::primaryScreen();
        if (screen != primary && m_env.contains(primary)) {
            m_env[screen]->allows_breeding = m_env[primary]->allows_breeding;
        }
    }
}

void ShijimaManager::screenRemoved(QScreen *screen) {
    if (m_env.contains(screen) && screen != nullptr) {
        auto primary = QGuiApplication::primaryScreen();
        for (auto &mascot : m_mascots) {
            mascot->setEnv(m_env[primary]);
            mascot->mascot().reset_position();
        }
        m_reverseEnv.remove(m_env[primary].get());
        m_env.remove(screen);
    }
}

void ShijimaManager::setWindowedMode(bool windowedMode) {
    if (!!this->windowedMode() == !!windowedMode) {
        return;
    }

    m_windowedModeAction->setChecked(windowedMode);
    for (auto mascot : m_mascots) {
        mascot->close();
        mascot->setParent(nullptr);
    }

    if (windowedMode) {
        QWidget *parent;
#if defined(_WIN32)
        parent = nullptr;
#else
        parent = this;
#endif
        m_sandboxWidget = new QWidget { parent, Qt::Window };
        m_sandboxWidget->setAttribute(Qt::WA_StyledBackground, true);
        m_sandboxWidget->resize(640, 480);
        m_sandboxWidget->setObjectName("sandboxWindow");
        m_sandboxWidget->show();
        updateSandboxBackground();
    }
    else {
        m_sandboxWidget->close();
        delete m_sandboxWidget;
        m_sandboxWidget = nullptr;
    }

    updateEnvironment();
    std::shared_ptr<shijima::mascot::environment> env;
    if (windowedMode) {
        env = m_env[nullptr];
    }
    else {
        env = m_env[mascotScreen()];
    }

    for (auto &mascot : m_mascots) {
        bool inspectorWasVisible = mascot->inspectorVisible();
        auto newMascot = new ShijimaWidget(*mascot, windowedMode,
            mascotParent());
        newMascot->setEnv(env);
        delete mascot;
        mascot = newMascot;
        m_mascotsById[mascot->mascotId()] = mascot;
        mascot->mascot().reset_position();
        mascot->show();
        if (inspectorWasVisible) {
            mascot->showInspector();
        }
    }
}

void ShijimaManager::updateEnvironment(QScreen *screen) {
    if (!m_env.contains(screen)) {
        return;
    }

    auto &env = m_env[screen];
    QRect geometry, available;
    QPoint cursor;
    if (screen == nullptr) {
        if (m_sandboxWidget != nullptr) {
            geometry = m_sandboxWidget->geometry();
            cursor = m_sandboxWidget->cursor().pos() - geometry.topLeft();
            geometry.setCoords(0, 0, geometry.width(), geometry.height());
            available = geometry;
        }
        else {
            std::cerr << "warning: sandboxWidget is not initialized" << std::endl;
        }
    }
    else {
        cursor = QCursor::pos();
        geometry = screen->geometry();
        available = screen->availableGeometry();
    }

    int taskbarHeight = geometry.bottom() - available.bottom();
    int statusBarHeight = available.top() - geometry.top();
    if (taskbarHeight < 0) {
        taskbarHeight = 0;
    }
    if (statusBarHeight < 0) {
        statusBarHeight = 0;
    }

    env->screen = { (double)geometry.top() + statusBarHeight,
        (double)geometry.right(),
        (double)geometry.bottom(),
        (double)geometry.left() };
    env->floor = { (double)geometry.bottom() - taskbarHeight,
        (double)geometry.left(), (double)geometry.right() };
    env->work_area = { (double)geometry.top(),
        (double)geometry.right(),
        (double)geometry.bottom() - taskbarHeight,
        (double)geometry.left() };
    env->ceiling = { (double)geometry.top(), (double)geometry.left(),
        (double)geometry.right() };
    if (!windowedMode() && m_currentWindow.available &&
        std::fabs(m_currentWindow.x) > 1 && std::fabs(m_currentWindow.y) > 1)
    {
        env->active_ie = { m_currentWindow.y,
            m_currentWindow.x + m_currentWindow.width,
            m_currentWindow.y + m_currentWindow.height,
            m_currentWindow.x };
        if (m_previousWindow.available &&
            m_previousWindow.uid == m_currentWindow.uid)
        {
            env->active_ie.dy = m_currentWindow.y - m_previousWindow.y;
            if (env->active_ie.dy == 0) {
                env->active_ie.dy = m_currentWindow.height - m_previousWindow.height;
            }
            env->active_ie.dx = m_currentWindow.x - m_previousWindow.x;
            if (env->active_ie.dx == 0) {
                env->active_ie.dx = m_currentWindow.width - m_previousWindow.width;
            }

            if (m_detachThreshold > 0) {
                double speed = std::sqrt(env->active_ie.dx * env->active_ie.dx
                    + env->active_ie.dy * env->active_ie.dy);
                double upperBound = m_detachThreshold * 3.0;
                if (speed >= upperBound) {
                    env->active_ie = { -50, -50, -50, -50 };
                }
                else if (speed > m_detachThreshold) {
                    double ratio = 1.0 - (speed - m_detachThreshold)
                        / (upperBound - m_detachThreshold);
                    env->active_ie.dx *= ratio;
                    env->active_ie.dy *= ratio;
                }
            }
        }
    }
    else {
        env->active_ie = { -50, -50, -50, -50 };
    }

    int x = cursor.x(), y = cursor.y();
    env->cursor = { (double)x, (double)y, x - env->cursor.x, y - env->cursor.y };
    env->subtick_count = ShijimaManagerInternal::kSubtickCount;
    env->set_scale(1.0 / std::sqrt(m_userScale));
}

void ShijimaManager::updateEnvironment() {
    m_currentWindow = m_windowObserver.getActiveWindow();
    if (windowedMode()) {
        updateEnvironment(nullptr);
    }
    else {
        for (auto screen : QGuiApplication::screens()) {
            updateEnvironment(screen);
        }
    }
    m_previousWindow = m_currentWindow;
}

bool ShijimaManager::windowedMode() {
    return m_sandboxWidget != nullptr;
}

QWidget *ShijimaManager::mascotParent() {
    if (windowedMode()) {
        return m_sandboxWidget;
    }
    return this;
}

QScreen *ShijimaManager::mascotScreen() {
    if (windowedMode()) {
        return nullptr;
    }

    QScreen *screen = this->screen();
    if (screen == nullptr) {
        screen = qApp->primaryScreen();
    }
    return screen;
}
