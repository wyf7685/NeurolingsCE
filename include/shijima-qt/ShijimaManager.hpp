#pragma once

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

#include <QString>
#include <shijima/mascot/manager.hpp>
#include <shijima/mascot/factory.hpp>
#include <QList>
#include <QMap>
#include <QSet>
#include <QSettings>
#include "shijima-qt/PlatformWidget.hpp"
#include <map>
#include <memory>
#include <set>
#include <list>
#include <functional>
#include <mutex>
#include "shijima-qt/ShijimaHttpApi.hpp"
#include "ElaWindow.h"

class MascotData;
class QAction;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QLabel;
class QListWidgetItem;
class QObject;
class QPoint;
class QScreen;
class QShowEvent;
class QTimerEvent;
class QTranslator;
class QWidget;
class ShijimaWidget;
struct ShijimaManagerRuntimeState;
struct ShijimaManagerUiState;

class ShijimaManager : public PlatformWidget<ElaWindow>
{
    Q_OBJECT
public:
    static ShijimaManager *defaultManager();
    static void finalize();
    void updateEnvironment();
    void updateEnvironment(QScreen *);
    QString const& mascotsPath();
    ShijimaWidget *spawn(std::string const& name);
    void killAll();
    void killAll(QString const& name);
    void killAllButOne(ShijimaWidget *widget);
    void killAllButOne(QString const& name);
    void setManagerVisible(bool visible);
    void importOnShow(QString const& path);
    void quitAction();
    QMap<QString, MascotData *> const& loadedMascots();
    QMap<int, MascotData *> const& loadedMascotsById();
    std::list<ShijimaWidget *> const& mascots();
    std::map<int, ShijimaWidget *> const& mascotsById();
    ShijimaWidget *hitTest(QPoint const& screenPos);
    void onTickSync(std::function<void(ShijimaManager *)> callback);
    ~ShijimaManager();
protected:
    void timerEvent(QTimerEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void changeEvent(QEvent *event) override;
private:
    explicit ShijimaManager(QWidget *parent = nullptr);
    std::unique_lock<std::mutex> acquireLock();
    void abortPendingCallbacks();
    void loadDefaultMascot();
    void loadData(MascotData *data);
    void spawnClicked();
    void reloadMascot(QString const& name);
    void askClose();
    void itemDoubleClicked(QListWidgetItem *qItem);
    void reloadMascots(std::set<std::string> const& mascots);
    void loadAllMascots();
    void refreshListWidget();
    void setupNavigation();
    void setupHomePage();
    void setupSettingsPage();
    void setupAboutPage();
    void importAction();
    void deleteAction();
    void updateSandboxBackground();
    bool windowedMode();
    QWidget *mascotParent();
    void setWindowedMode(bool windowedMode);
    void screenAdded(QScreen *);
    void screenRemoved(QScreen *);
    std::set<std::string> import(QString const& path) noexcept;
    void importWithDialog(QList<QString> const& paths);
    void tick();
    void retranslateUi();
    void switchLanguage(const QString &langCode);
    void updateStatusBar();
    QScreen *mascotScreen();
    std::unique_ptr<ShijimaManagerRuntimeState> m_runtime;
    std::unique_ptr<ShijimaManagerUiState> m_ui;
    QSettings m_settings;
    bool m_allowClose = false;
    bool m_firstShow = true;
    bool m_wasVisible = false;
    bool m_constructing = true;
    ShijimaHttpApi m_httpApi;
};
