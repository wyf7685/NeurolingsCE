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

#pragma once

#include <sstream>

class QString;
class QCoreApplication;

namespace AppLog {

enum class Level {
    Debug,
    Info,
    Warning,
    Error
};

void initialize(QCoreApplication *app);
void shutdown();
void write(Level level, char const *category, std::string const& message,
    char const *file, int line, char const *function);
QString sessionLogPath();
QString sessionLogDirectoryPath();

class Line {
public:
    Line(Level level, char const *category, char const *file, int line,
        char const *function);
    ~Line();

    std::ostringstream& stream();

private:
    Level m_level;
    char const *m_category;
    char const *m_file;
    int m_line;
    char const *m_function;
    std::ostringstream m_stream;
};

}

#define APP_LOG_DEBUG(category) \
    AppLog::Line(AppLog::Level::Debug, (category), __FILE__, __LINE__, __func__).stream()

#define APP_LOG_INFO(category) \
    AppLog::Line(AppLog::Level::Info, (category), __FILE__, __LINE__, __func__).stream()

#define APP_LOG_WARN(category) \
    AppLog::Line(AppLog::Level::Warning, (category), __FILE__, __LINE__, __func__).stream()

#define APP_LOG_ERROR(category) \
    AppLog::Line(AppLog::Level::Error, (category), __FILE__, __LINE__, __func__).stream()
