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

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <QMap>
#include <QSet>
#include <QString>
#include "Platform/ActiveWindowObserver.hpp"
#include <shijima/mascot/factory.hpp>

class MascotData;
class QScreen;
class ShijimaWidget;

struct ShijimaManagerRuntimeState {
    Platform::ActiveWindow previousWindow;
    Platform::ActiveWindow currentWindow;
    Platform::ActiveWindowObserver windowObserver;
    int mascotTimer = -1;
    int windowObserverTimer = -1;
    int idCounter = 0;
    double userScale = 1.0;
    double detachThreshold = 30.0;
    QMap<QString, MascotData *> loadedMascots;
    QMap<int, MascotData *> loadedMascotsById;
    QSet<QString> listItemsToRefresh;
    QMap<QScreen *, std::shared_ptr<shijima::mascot::environment>> env;
    QMap<shijima::mascot::environment *, QScreen *> reverseEnv;
    shijima::mascot::factory factory;
    QString importOnShowPath;
    std::list<ShijimaWidget *> mascots;
    std::map<int, ShijimaWidget *> mascotsById;
    QString mascotsPath;
    bool hasTickCallbacks = false;
    std::atomic<bool> shuttingDown{false};
    std::mutex mutex;
    std::condition_variable tickCallbackCompletion;
    std::list<std::function<void(class ShijimaManager *)>> tickCallbacks;
};
