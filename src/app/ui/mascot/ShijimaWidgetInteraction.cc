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

#include "shijima-qt/ShijimaWidget.hpp"

#include <QMouseEvent>
#include <QSettings>
#include <QVariant>

#include <stdexcept>

#include "shijima-qt/ShijimaManager.hpp"
#include "shijima-qt/SpeechBubbleWidget.hpp"

void ShijimaWidget::setDragTarget(ShijimaWidget *target) {
    if (m_dragTarget != nullptr) {
        m_dragTarget->m_dragTargetPt = nullptr;
    }
    if (target != nullptr) {
        if (target->m_dragTargetPt != nullptr) {
            throw std::runtime_error("target widget being dragged by multiple widgets");
        }
        m_dragTarget = target;
        m_dragTarget->m_dragTargetPt = &m_dragTarget;
    }
    else {
        m_dragTarget = nullptr;
    }
}

void ShijimaWidget::mousePressEvent(QMouseEvent *event) {
    auto pos = event->pos();
    if (m_dragTarget != nullptr) {
        m_dragTarget->m_mascot->state->dragging = false;
    }
    if (pointInside(pos)) {
        setDragTarget(this);
    }
    else {
        QPoint envPos;
        if (m_windowedMode) {
            envPos = mapToParent(pos);
        }
        else {
            envPos = mapToGlobal(pos);
        }
        ShijimaWidget *target = ShijimaManager::defaultManager()->hitTest(envPos);
        setDragTarget(target);
        if (target == nullptr) {
            event->ignore();
            return;
        }
    }
    if (event->button() == Qt::MouseButton::LeftButton) {
        m_dragTarget->m_mascot->state->dragging = true;
        // Record press info for click detection
        m_lastPressGlobalPos = mapToGlobal(pos);
        m_pressElapsedTimer.start();
    }
    else if (event->button() == Qt::MouseButton::RightButton) {
        auto screenPos = mapToGlobal(pos);
        m_dragTarget->showContextMenu(screenPos);
        setDragTarget(nullptr);
    }
}

void ShijimaWidget::closeAction() {
    close();
}

void ShijimaWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (m_dragTarget == nullptr) {
        return;
    }
    if (event->button() == Qt::MouseButton::LeftButton) {
        // Detect click vs drag: small movement + short duration
        auto releaseGlobalPos = mapToGlobal(event->pos());
        int distance = (releaseGlobalPos - m_lastPressGlobalPos).manhattanLength();
        qint64 elapsed = m_pressElapsedTimer.elapsed();
        ShijimaWidget *clickTarget = m_dragTarget;

        m_dragTarget->m_mascot->state->dragging = false;
        setDragTarget(nullptr);

        // Click threshold: less than 6px movement and less than 400ms
        if (distance <= 6 && elapsed <= 400) {
            clickTarget->handleClick();
        }
    }
}

void ShijimaWidget::handleClick() {
    m_clickCount++;
    m_clickResetTimer.start(500); // Reset click count after 500ms of no clicks

    // Trigger speech bubble when click count reaches threshold
    QSettings bubbleSettings("pixelomer", "Shijima-Qt");
    int threshold = bubbleSettings.value("speechBubbleClickCount", 1).toInt();
    if (m_clickCount == threshold) {
        showSpeechBubble();
    }
}

void ShijimaWidget::showSpeechBubble() {
    // Check if speech bubbles are enabled in settings
    QSettings settings("pixelomer", "Shijima-Qt");
    bool enabled = settings.value("speechBubbleEnabled",
        QVariant::fromValue(true)).toBool();
    if (!enabled) {
        return;
    }

    // Get random text
    QString text = SpeechBubbleWidget::randomBubbleText(m_data->path());

    // Create bubble widget if needed
    if (m_speechBubble == nullptr) {
        m_speechBubble = new SpeechBubbleWidget();
    }

    // Calculate anchor position (top-center of the mascot widget in screen coords)
    QPoint anchorPos = mapToGlobal(QPoint(width() / 2, 0));

    m_speechBubble->showBubble(text, anchorPos);
}
