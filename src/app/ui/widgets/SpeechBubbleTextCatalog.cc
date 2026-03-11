// 
// NeurolingsCE - Cross-platform shimeji simulation app for desktop
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

#include "shijima-qt/ui/widgets/SpeechBubbleWidget.hpp"

#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTextStream>

QMap<QString, QStringList> SpeechBubbleWidget::s_bubbleTextsCache;

static QStringList loadTextsFromFile(const QString &filePath) {
    QStringList texts;
    QFile file(filePath);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (!line.isEmpty()) {
                texts.append(line);
            }
        }
        file.close();
    }
    return texts;
}

QStringList SpeechBubbleWidget::loadBubbleTexts(const QString &mascotPath) {
    QStringList texts;

    // Priority 1: Per-mascot bubble_context.txt in mascot folder
    if (!mascotPath.isEmpty() && mascotPath != "@") {
        QString mascotBubbleFile = QDir::cleanPath(
            mascotPath + QDir::separator() + "bubble_context.txt");
        texts = loadTextsFromFile(mascotBubbleFile);
        if (!texts.isEmpty()) return texts;
    }

    // Priority 2: User's global bubbles.txt in app data directory
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString userFile = QDir::cleanPath(dataPath + QDir::separator() + "bubbles.txt");
    texts = loadTextsFromFile(userFile);
    if (!texts.isEmpty()) return texts;

    // Priority 3: Embedded resource
    QFile resFile(":/bubbles.txt");
    if (resFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&resFile);
        stream.setEncoding(QStringConverter::Utf8);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (!line.isEmpty()) {
                texts.append(line);
            }
        }
        resFile.close();
    }

    // Priority 4: Hardcoded fallback
    if (texts.isEmpty()) {
        texts.append("Hello!");
        texts.append("Hi there~");
        texts.append("(^_^)");
    }

    return texts;
}

QString SpeechBubbleWidget::randomBubbleText(const QString &mascotPath) {
    // Use mascotPath as cache key (empty string for global)
    QString cacheKey = mascotPath.isEmpty() ? QStringLiteral("__global__") : mascotPath;
    if (!s_bubbleTextsCache.contains(cacheKey)) {
        s_bubbleTextsCache[cacheKey] = loadBubbleTexts(mascotPath);
    }
    const QStringList &texts = s_bubbleTextsCache[cacheKey];
    if (texts.isEmpty()) {
        return "Hello!";
    }
    int index = QRandomGenerator::global()->bounded(texts.size());
    return texts.at(index);
}
