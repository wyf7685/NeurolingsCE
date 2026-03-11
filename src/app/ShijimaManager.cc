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
#include "ShijimaManagerInternal.hpp"
#include <iostream>
#include <memory>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QScreen>
#include <QStandardPaths>
#include <QStyleHints>
#include <QVariant>
#include "ElaStatusBar.h"
#include "ElaTheme.h"

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

ShijimaManager::ShijimaManager(QWidget *parent):
    PlatformWidget(parent),
    m_runtime(std::make_unique<ShijimaManagerRuntimeState>()),
    m_ui(std::make_unique<ShijimaManagerUiState>()),
    m_settings("pixelomer", "Shijima-Qt"),
    m_httpApi(this)
{
    m_ui->listWidget = new QListWidget(this);

    for (auto screen : QGuiApplication::screens()) {
        screenAdded(screen);
    }
    screenAdded(nullptr);

    connect(qApp, &QGuiApplication::screenAdded,
        this, &ShijimaManager::screenAdded);
    connect(qApp, &QGuiApplication::screenRemoved,
        this, &ShijimaManager::screenRemoved);

    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString mascotsPath = QDir::cleanPath(dataPath + QDir::separator() + "mascots");
    QDir mascotsDir(mascotsPath);
    if (!mascotsDir.exists()) {
        mascotsDir.mkpath(mascotsPath);
    }
    if (QFile readme { mascotsDir.absoluteFilePath("README.txt") };
        readme.open(QFile::WriteOnly | QFile::NewOnly | QFile::Text))
    {
        readme.write(""
"Manually importing shimeji by copying its contents into this folder may\n"
"cause problems. You should use the import dialog in Shijima-Qt unless you\n"
"have a good reason not to.\n"
        );
        readme.close();
    }
    m_runtime->mascotsPath = mascotsPath;
    std::cout << "Mascots path: " << m_runtime->mascotsPath.toStdString() << std::endl;

    loadDefaultMascot();
    loadAllMascots();
    setAcceptDrops(true);

    m_runtime->mascotTimer = startTimer(40 / ShijimaManagerInternal::kSubtickCount);
    if (m_runtime->windowObserver.tickFrequency() > 0) {
        m_runtime->windowObserverTimer = startTimer(m_runtime->windowObserver.tickFrequency());
    }

    setUserInfoCardVisible(false);
    setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
    setIsDefaultClosed(false);
    setMinimumHeight(450);
    connect(this, &ElaWindow::closeButtonClicked, this, [this]() {
#if defined(_WIN32)
        if (m_runtime->mascots.size() == 0) {
            askClose();
        } else {
            setManagerVisible(false);
        }
#else
        askClose();
#endif
    });

    auto syncTheme = [](Qt::ColorScheme scheme) {
        eTheme->setThemeMode(scheme == Qt::ColorScheme::Dark
            ? ElaThemeType::Dark : ElaThemeType::Light);
    };
    syncTheme(QGuiApplication::styleHints()->colorScheme());
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
        this, syncTheme);

    connect(m_ui->listWidget, &QListWidget::itemDoubleClicked,
        this, &ShijimaManager::itemDoubleClicked);
    m_ui->listWidget->setIconSize({ 64, 64 });
    m_ui->listWidget->installEventFilter(this);
    m_ui->listWidget->setSelectionMode(QListWidget::ExtendedSelection);
    ShijimaManagerInternal::applyMascotListTheme(*m_ui->listWidget);
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this]() {
        ShijimaManagerInternal::applyMascotListTheme(*m_ui->listWidget);
    });

    setWindowTitle(tr(APP_NAME " \u2014 Mascot Manager"));
    auto *elaStatusBar = new ElaStatusBar(this);
    setStatusBar(elaStatusBar);
    m_ui->statusLabel = new QLabel(this);
    elaStatusBar->addWidget(m_ui->statusLabel, 1);
    updateStatusBar();

    QString savedLang = m_settings.value("language", "en").toString();
    if (savedLang != "en") {
        m_ui->currentLanguage = "en";
        switchLanguage(savedLang);
    }

    m_runtime->detachThreshold = m_settings.value("detachThreshold",
        QVariant::fromValue(30.0)).toDouble();

    setupNavigation();
    setManagerVisible(true);
    m_constructing = false;

    ShijimaManagerInternal::setupTrayIcon(this);
    m_httpApi.start("127.0.0.1", 32456);
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
    m_httpApi.stop();
    if (m_runtime->mascotTimer != 0) {
        killTimer(m_runtime->mascotTimer);
        m_runtime->mascotTimer = 0;
    }
    if (m_runtime->windowObserverTimer != 0) {
        killTimer(m_runtime->windowObserverTimer);
        m_runtime->windowObserverTimer = 0;
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

bool ShijimaManager::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        auto key = keyEvent->key();
        if (key == Qt::Key::Key_Return || key == Qt::Key::Key_Enter) {
            for (auto item : m_ui->listWidget->selectedItems()) {
                itemDoubleClicked(item);
            }
            return true;
        }
    }
    return ElaWindow::eventFilter(obj, event);
}

void ShijimaManager::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && !m_constructing) {
        retranslateUi();
    }
    else if (event->type() == QEvent::WindowStateChange) {
        ShijimaManagerInternal::refreshTrayMenu(this);
    }
    PlatformWidget::changeEvent(event);
}

#include "ShijimaManager.moc"
