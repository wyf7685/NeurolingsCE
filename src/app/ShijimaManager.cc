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
    return std::unique_lock<std::mutex> { m_mutex };
}

QString const& ShijimaManager::mascotsPath() {
    return m_mascotsPath;
}

ShijimaManager::~ShijimaManager() {
    disconnect(qApp, &QGuiApplication::screenAdded,
        this, &ShijimaManager::screenAdded);
    disconnect(qApp, &QGuiApplication::screenRemoved,
        this, &ShijimaManager::screenRemoved);
}

void ShijimaManager::abortPendingCallbacks() {
    auto lock = acquireLock();
    m_shuttingDown.store(true);
    m_tickCallbacks.clear();
    m_hasTickCallbacks = false;
    m_tickCallbackCompletion.notify_all();
}

void ShijimaManager::onTickSync(std::function<void(ShijimaManager *)> callback) {
    if (m_shuttingDown.load()) {
        return;
    }

    auto lock = acquireLock();
    if (m_shuttingDown.load()) {
        return;
    }

    m_hasTickCallbacks = true;
    m_tickCallbacks.push_back(callback);
    m_tickCallbackCompletion.wait(lock, [this]{
        return m_shuttingDown.load() || m_tickCallbacks.empty();
    });
}

ShijimaManager::ShijimaManager(QWidget *parent):
    PlatformWidget(parent),
    m_sandboxWidget(nullptr),
    m_settings("pixelomer", "Shijima-Qt"),
    m_windowedModeAction(nullptr),
    m_idCounter(0), m_httpApi(this),
    m_hasTickCallbacks(false),
    m_translator(nullptr),
    m_qtTranslator(nullptr),
    m_currentLanguage("en")
{
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
    m_mascotsPath = mascotsPath;
    std::cout << "Mascots path: " << m_mascotsPath.toStdString() << std::endl;

    loadDefaultMascot();
    loadAllMascots();
    setAcceptDrops(true);

    m_mascotTimer = startTimer(40 / ShijimaManagerInternal::kSubtickCount);
    if (m_windowObserver.tickFrequency() > 0) {
        m_windowObserverTimer = startTimer(m_windowObserver.tickFrequency());
    }

    setUserInfoCardVisible(false);
    setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
    setIsDefaultClosed(false);
    setMinimumHeight(450);
    connect(this, &ElaWindow::closeButtonClicked, this, [this]() {
#if defined(_WIN32)
        if (m_mascots.size() == 0) {
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

    connect(&m_listWidget, &QListWidget::itemDoubleClicked,
        this, &ShijimaManager::itemDoubleClicked);
    m_listWidget.setIconSize({ 64, 64 });
    m_listWidget.installEventFilter(this);
    m_listWidget.setSelectionMode(QListWidget::ExtendedSelection);
    ShijimaManagerInternal::applyMascotListTheme(m_listWidget);
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this]() {
        ShijimaManagerInternal::applyMascotListTheme(m_listWidget);
    });

    setWindowTitle(tr(APP_NAME " \u2014 Mascot Manager"));
    auto *elaStatusBar = new ElaStatusBar(this);
    setStatusBar(elaStatusBar);
    m_statusLabel = new QLabel(this);
    elaStatusBar->addWidget(m_statusLabel, 1);
    updateStatusBar();

    QString savedLang = m_settings.value("language", "en").toString();
    if (savedLang != "en") {
        m_currentLanguage = "en";
        switchLanguage(savedLang);
    }

    m_detachThreshold = m_settings.value("detachThreshold",
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
        if (m_mascots.size() == 0) {
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
    if (m_mascotTimer != 0) {
        killTimer(m_mascotTimer);
        m_mascotTimer = 0;
    }
    if (m_windowObserverTimer != 0) {
        killTimer(m_windowObserverTimer);
        m_windowObserverTimer = 0;
    }
    event->accept();
#else
    event->ignore();
    setManagerVisible(false);
#endif
}

void ShijimaManager::timerEvent(QTimerEvent *event) {
    int timerId = event->timerId();
    if (timerId == m_mascotTimer) {
        tick();
    }
    else if (timerId == m_windowObserverTimer) {
        m_windowObserver.tick();
    }
}

bool ShijimaManager::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        auto key = keyEvent->key();
        if (key == Qt::Key::Key_Return || key == Qt::Key::Key_Enter) {
            for (auto item : m_listWidget.selectedItems()) {
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
