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
#include "shijima-qt/AppLog.hpp"

#include "../runtime/ManagerRuntimeHelpers.hpp"
#include "ManagerUiHelpers.hpp"

#include <memory>

#include <QDir>
#include <QEvent>
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
    APP_LOG_INFO("startup") << "Mascot storage path=\""
        << m_runtime->mascotsPath.toStdString() << "\"";

    loadDefaultMascot();
    loadAllMascots();
    setAcceptDrops(true);

    m_runtime->mascotTimer = startTimer(40 / ShijimaManagerRuntimeInternal::kSubtickCount);
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
    ShijimaManagerUiInternal::applyMascotListTheme(*m_ui->listWidget);
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this]() {
        ShijimaManagerUiInternal::applyMascotListTheme(*m_ui->listWidget);
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

    ShijimaManagerUiInternal::setupTrayIcon(this);
    m_httpApi.start("127.0.0.1", 32456);
    APP_LOG_INFO("startup") << "Manager window initialized";
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
        ShijimaManagerUiInternal::refreshTrayMenu(this);
    }
    PlatformWidget::changeEvent(event);
}
