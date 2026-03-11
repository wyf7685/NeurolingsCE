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

#include "shijima-qt/AppLog.hpp"

#include <cstdlib>
#include <cstdio>
#include <mutex>
#include <QString>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMessageLogContext>
#include <QStandardPaths>
#include <QThread>

namespace {

std::mutex g_logMutex;
QFile *g_logFile = nullptr;
QtMessageHandler g_previousHandler = nullptr;
thread_local bool g_inMessageHandler = false;

char const *levelName(AppLog::Level level) {
    switch (level) {
        case AppLog::Level::Debug:
            return "DEBUG";
        case AppLog::Level::Info:
            return "INFO ";
        case AppLog::Level::Warning:
            return "WARN ";
        case AppLog::Level::Error:
            return "ERROR";
    }
    return "INFO ";
}

AppLog::Level fromQtType(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:
            return AppLog::Level::Debug;
        case QtInfoMsg:
            return AppLog::Level::Info;
        case QtWarningMsg:
            return AppLog::Level::Warning;
        case QtCriticalMsg:
        case QtFatalMsg:
            return AppLog::Level::Error;
    }
    return AppLog::Level::Info;
}

QString formatLine(AppLog::Level level, char const *category, QString const& message,
    char const *file, int line, char const *function)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString location;
    if (file != nullptr && *file != 0) {
        location = QFileInfo(QString::fromUtf8(file)).fileName();
        if (line > 0) {
            location += ":" + QString::number(line);
        }
    }
    else if (function != nullptr && *function != 0) {
        location = QString::fromUtf8(function);
    }
    else {
        location = "?";
    }

    QString formatted = QStringLiteral("[%1] [%2] [%3] [tid:%4] %5")
        .arg(timestamp,
            QString::fromUtf8(levelName(level)),
            QString::fromUtf8(category != nullptr ? category : "app"),
            QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16),
            message);
    if (!location.isEmpty()) {
        formatted += QStringLiteral(" (%1)").arg(location);
    }
    return formatted;
}

void writeLineUnlocked(QString const& line) {
    QByteArray utf8 = line.toUtf8();
    utf8.append('\n');
    std::fwrite(utf8.constData(), 1, static_cast<size_t>(utf8.size()), stderr);
    std::fflush(stderr);

    if (g_logFile != nullptr && g_logFile->isOpen()) {
        g_logFile->write(utf8);
        g_logFile->flush();
    }
}

void appMessageHandler(QtMsgType type, QMessageLogContext const& context,
    QString const& message)
{
    if (g_inMessageHandler) {
        if (g_previousHandler != nullptr) {
            g_previousHandler(type, context, message);
        }
        return;
    }

    g_inMessageHandler = true;
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        QString line = formatLine(fromQtType(type), context.category, message,
            context.file, context.line, context.function);
        writeLineUnlocked(line);
    }
    g_inMessageHandler = false;

    if (type == QtFatalMsg) {
        std::abort();
    }
}

}

namespace AppLog {

void initialize(QCoreApplication *) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile == nullptr) {
        QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (basePath.isEmpty()) {
            basePath = QDir::currentPath();
        }
        QDir dir(basePath);
        dir.mkpath(".");
        dir.mkpath("logs");

        auto *file = new QFile(dir.filePath("logs/neurolingsce.log"));
        if (file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            g_logFile = file;
        }
        else {
            delete file;
        }
    }

    if (g_previousHandler == nullptr) {
        g_previousHandler = qInstallMessageHandler(appMessageHandler);
    }

    writeLineUnlocked(formatLine(Level::Info, "app",
        QStringLiteral("Logging initialized. Session log: %1").arg(sessionLogPath()),
        __FILE__, __LINE__, __func__));
}

void shutdown() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    writeLineUnlocked(formatLine(Level::Info, "app", QStringLiteral("Logging shutdown"),
        __FILE__, __LINE__, __func__));
    if (g_previousHandler != nullptr) {
        qInstallMessageHandler(g_previousHandler);
        g_previousHandler = nullptr;
    }
    if (g_logFile != nullptr) {
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
}

void write(Level level, char const *category, std::string const& message,
    char const *file, int line, char const *function)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    writeLineUnlocked(formatLine(level, category,
        QString::fromUtf8(message.c_str()), file, line, function));
}

QString sessionLogPath() {
    if (g_logFile != nullptr) {
        return g_logFile->fileName();
    }
    return {};
}

Line::Line(Level level, char const *category, char const *file, int line,
    char const *function)
    : m_level(level), m_category(category), m_file(file), m_line(line),
      m_function(function)
{
}

Line::~Line() {
    AppLog::write(m_level, m_category, m_stream.str(), m_file, m_line, m_function);
}

std::ostringstream& Line::stream() {
    return m_stream;
}

}
