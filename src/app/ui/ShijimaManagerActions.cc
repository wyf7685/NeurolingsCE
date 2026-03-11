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
#include "shijima-qt/MascotData.hpp"
#include "../runtime/ShijimaManagerRuntimeInternal.hpp"
#include "ShijimaManagerUiInternal.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QTranslator>
#include "ElaTheme.h"

namespace ShijimaManagerUiInternal {

QString colorToString(QColor const& color) {
    auto rgb = color.toRgb();
    std::array<char, 8> buf;
    snprintf(&buf[0], buf.size(), "#%02hhX%02hhX%02hhX",
        (uint8_t)rgb.red(), (uint8_t)rgb.green(),
        (uint8_t)rgb.blue());
    buf[buf.size() - 1] = 0;
    return QString { &buf[0] };
}

}

namespace ShijimaManagerRuntimeInternal {

void dispatchToMainThread(std::function<void()> callback) {
    QTimer *timer = new QTimer;
    timer->moveToThread(qApp->thread());
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [timer, callback]() {
        callback();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

}

namespace ShijimaManagerUiInternal {

void applyMascotListTheme(QListWidget& listWidget) {
    auto mode = eTheme->getThemeMode();
    QColor bg = eTheme->getThemeColor(mode, ElaThemeType::WindowBase);
    QColor text = eTheme->getThemeColor(mode, ElaThemeType::BasicText);
    QColor bgAlt = eTheme->getThemeColor(mode, ElaThemeType::BasicBase);
    QColor hover = eTheme->getThemeColor(mode, ElaThemeType::BasicHover);
    QColor selected = eTheme->getThemeColor(mode, ElaThemeType::PrimaryNormal);
    QColor border = eTheme->getThemeColor(mode, ElaThemeType::BasicBorder);
    listWidget.setStyleSheet(QString(
        "QListWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 6px;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  padding: 4px;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: %4;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: %5;"
        "  color: white;"
        "}"
        "QScrollBar:vertical {"
        "  background: %6;"
        "  width: 8px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %7;"
        "  min-height: 30px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    ).arg(bg.name(), text.name(), border.name(),
          hover.name(), selected.name(), bgAlt.name(),
          border.name()));
}

}

void ShijimaManager::importAction() {
    auto paths = QFileDialog::getOpenFileNames(this, tr("Choose shimeji archive..."));
    if (paths.isEmpty()) {
        return;
    }
    importWithDialog(paths);
}

void ShijimaManager::quitAction() {
    m_allowClose = true;
    closeWindow();
}

void ShijimaManager::deleteAction() {
    if (m_runtime->loadedMascots.size() == 0) {
        return;
    }

    auto selected = m_ui->listWidget->selectedItems();
    for (long i = (long)selected.size() - 1; i >= 0; --i) {
        auto mascotData = m_runtime->loadedMascots[selected[i]->text()];
        if (!mascotData->deletable()) {
            selected.remove(i);
        }
    }
    if (selected.size() == 0) {
        return;
    }

    QString msg = tr("Are you sure you want to delete these shimeji?");
    for (long i = 0; i < selected.size() && i < 5; ++i) {
        msg += "\n* " + selected[i]->text();
    }
    if (selected.size() > 5) {
        msg += tr("\n... and %1 other(s)").arg(selected.size() - 5);
    }

    QMessageBox msgBox { this };
    msgBox.setWindowTitle(tr("Delete shimeji"));
    msgBox.setText(msg);
    msgBox.setStandardButtons(QMessageBox::StandardButton::Yes |
        QMessageBox::StandardButton::No);
    msgBox.setIcon(QMessageBox::Icon::Question);
    int ret = msgBox.exec();
    if (ret == QMessageBox::StandardButton::Yes) {
        for (auto item : selected) {
            auto mascotData = m_runtime->loadedMascots[item->text()];
            if (!mascotData->deletable()) {
                continue;
            }

            std::filesystem::path path = mascotData->path().toStdString();
            std::cout << "Deleting mascot: " << item->text().toStdString() << std::endl;
            try {
                std::filesystem::remove_all(path / "img");
                std::filesystem::remove_all(path / "sound");
                std::filesystem::remove(path / "actions.xml");
                std::filesystem::remove(path / "behaviors.xml");
                std::filesystem::remove(path);
            }
            catch (std::exception &ex) {
                std::cerr << "failed to delete mascot: " << path.string()
                    << ": " << ex.what() << std::endl;
            }
            reloadMascot(item->text());
        }
        refreshListWidget();
    }
}

void ShijimaManager::updateSandboxBackground() {
    if (m_ui->sandboxWidget != nullptr) {
        m_ui->sandboxWidget->setStyleSheet("#sandboxWindow { background-color: " +
            ShijimaManagerUiInternal::colorToString(m_ui->sandboxBackground) + "; }");
    }
}

void ShijimaManager::updateStatusBar() {
    if (m_ui->statusLabel == nullptr) {
        return;
    }
    int mascotCount = static_cast<int>(m_runtime->mascots.size());
    int templateCount = m_runtime->loadedMascots.size();
    m_ui->statusLabel->setText(tr("  Mascots: %1  |  Templates: %2")
        .arg(mascotCount).arg(templateCount));
}

void ShijimaManager::itemDoubleClicked(QListWidgetItem *qItem) {
    spawn(qItem->text().toStdString());
}

void ShijimaManager::askClose() {
    setManagerVisible(true);
    QMessageBox msgBox { this };
    msgBox.setWindowTitle(tr("Close NeurolingsCE"));
    msgBox.setIcon(QMessageBox::Icon::Question);
    msgBox.setStandardButtons(QMessageBox::StandardButton::Yes |
        QMessageBox::StandardButton::No);
    msgBox.setText(tr("Do you want to close NeurolingsCE?"));
    int ret = msgBox.exec();
    if (ret == QMessageBox::Button::Yes) {
#if defined(__APPLE__)
        QCoreApplication::quit();
#else
        m_allowClose = true;
        closeWindow();
#endif
    }
}

void ShijimaManager::setManagerVisible(bool visible) {
    if (!m_wasVisible && visible) {
        if (isMinimized()) {
            setWindowState(windowState() & ~Qt::WindowMinimized);
        }
        show();
        if (window() != nullptr) {
            window()->activateWindow();
        }
        m_wasVisible = true;
    }
    else if (m_wasVisible && !visible) {
#if defined(__APPLE__)
        if (m_runtime->mascots.size() == 0) {
            askClose();
            return;
        }
#endif
        if (isMinimized()) {
            setWindowState(windowState() & ~Qt::WindowMinimized);
        }
        hide();
        clearFocus();
        m_wasVisible = false;
    }
}

void ShijimaManager::switchLanguage(const QString &langCode) {
    if (langCode == m_ui->currentLanguage) {
        return;
    }

    if (m_ui->translator != nullptr) {
        qApp->removeTranslator(m_ui->translator);
        delete m_ui->translator;
        m_ui->translator = nullptr;
    }
    if (m_ui->qtTranslator != nullptr) {
        qApp->removeTranslator(m_ui->qtTranslator);
        delete m_ui->qtTranslator;
        m_ui->qtTranslator = nullptr;
    }

    m_ui->currentLanguage = langCode;
    m_settings.setValue("language", langCode);
    if (langCode != "en") {
        m_ui->translator = new QTranslator(this);
        if (m_ui->translator->load("shijima-qt_" + langCode, ":/i18n")) {
            qApp->installTranslator(m_ui->translator);
        }
        m_ui->qtTranslator = new QTranslator(this);
        if (m_ui->qtTranslator->load("qt_" + langCode, ":/i18n")) {
            qApp->installTranslator(m_ui->qtTranslator);
        }
    }

    if (!m_constructing) {
        QMessageBox::information(this,
            tr("Language Changed"),
            tr("The application will restart to apply the new language."));

        m_httpApi.stop();

        const QString program = QCoreApplication::applicationFilePath();
        const QStringList args = QCoreApplication::arguments().mid(1);
        QProcess::startDetached(program, args);
        m_allowClose = true;
        closeWindow();
    }
}

void ShijimaManager::retranslateUi() {
    setWindowTitle(tr(APP_NAME " \u2014 Mascot Manager"));
    updateStatusBar();
    ShijimaManagerUiInternal::refreshTrayMenu(this);
}
