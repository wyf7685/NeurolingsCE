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
#include "../ManagerUiHelpers.hpp"
#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include "shijima-qt/ui/dialogs/licenses/ShijimaLicensesDialog.hpp"
#include "ElaTheme.h"

#ifndef NEUROLINGSCE_VERSION
#define NEUROLINGSCE_VERSION "0.1.0"
#endif

void ShijimaManager::setupAboutPage() {
    addFooterNode(tr("About"), m_ui->aboutKey, 0, ElaIconType::User);
    connect(this, &ElaWindow::navigationNodeClicked,
        [this](ElaNavigationType::NavigationNodeType nodeType, QString nodeKey) {
            if (nodeKey != m_ui->aboutKey) {
                return;
            }
            Q_UNUSED(nodeType);

            QString version = QStringLiteral(NEUROLINGSCE_VERSION);
            QString authorName = QString::fromUtf8("\xe8\xbd\xbb\xe5\xb0\x98\xe5\x91\xa6");

            QDialog *aboutDialog = new QDialog(this);
            aboutDialog->setWindowTitle(tr("About NeurolingsCE"));
            aboutDialog->setFixedWidth(420);
            aboutDialog->setAttribute(Qt::WA_DeleteOnClose);

            auto themeMode = eTheme->getThemeMode();
            QString primaryColor = ElaThemeColor(themeMode, PrimaryNormal).name();
            QString textColor = ElaThemeColor(themeMode, BasicText).name();
            QString detailsColor = ElaThemeColor(themeMode, BasicDetailsText).name();
            QString cardBg = ElaThemeColor(themeMode, BasicBase).name();
            QString cardBorder = ElaThemeColor(themeMode, BasicBorder).name();
            QString dialogBg = ElaThemeColor(themeMode, DialogBase).name();
            QString badgeBg = ElaThemeColor(themeMode, BasicHover).name();

            aboutDialog->setStyleSheet(QString("QDialog { background-color: %1; }").arg(dialogBg));

            QVBoxLayout *layout = new QVBoxLayout;
            layout->setSpacing(12);
            layout->setContentsMargins(24, 24, 24, 24);
            aboutDialog->setLayout(layout);

            QLabel *iconLabel = new QLabel;
            QIcon appIcon = qApp->windowIcon();
            if (!appIcon.isNull()) {
                iconLabel->setPixmap(appIcon.pixmap(64, 64));
            }
            iconLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(iconLabel);

            QLabel *titleLabel = new QLabel(
                QString("<h2 style='color: %1; margin: 0;'>NeurolingsCE</h2>").arg(primaryColor));
            titleLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(titleLabel);

            QLabel *versionLabel = new QLabel(
                QString("<span style='background-color: %1; color: %2; "
                    "padding: 3px 12px; border-radius: 10px; font-size: 12px;'>v%3</span>")
                .arg(badgeBg, primaryColor, version));
            versionLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(versionLabel);

            QLabel *descLabel = new QLabel(
                tr("A cross-platform shimeji desktop pet runner."));
            descLabel->setAlignment(Qt::AlignCenter);
            descLabel->setStyleSheet(QString("color: %1; margin: 4px 0;").arg(detailsColor));
            layout->addWidget(descLabel);

            QString infoHtml = QStringLiteral(
                "<table cellpadding='4' style='color: %1;'>"
                "<tr><td style='color: %2; font-weight: bold;'>%3</td>"
                "<td><a href='https://space.bilibili.com/178381315' "
                "style='color: %2; text-decoration: none;'>%4</a></td></tr>"
                "<tr><td style='color: %2; font-weight: bold;'>%5</td>"
                "<td><a href='https://github.com/pixelomer/Shijima-Qt' "
                "style='color: %2; text-decoration: none;'>Shijima-Qt</a> by pixelomer</td></tr>"
                "<tr><td style='color: %2; font-weight: bold;'>%6</td>"
                "<td><a href='https://github.com/qingchenyouforcc/NeurolingsCE' "
                "style='color: %2; text-decoration: none;'>GitHub</a></td></tr>"
                "<tr><td style='color: %2; font-weight: bold;'>%7</td>"
                "<td>423902950</td></tr>"
                "<tr><td style='color: %2; font-weight: bold;'>%8</td>"
                "<td>125081756</td></tr>"
                "</table>")
                .arg(textColor, primaryColor,
                     tr("Author"), authorName,
                     tr("Based on"),
                     tr("Project"),
                     tr("Feedback QQ"),
                     tr("Chat QQ"));
            QLabel *infoLabel = new QLabel(infoHtml);
            infoLabel->setOpenExternalLinks(true);
            infoLabel->setAlignment(Qt::AlignCenter);
            infoLabel->setStyleSheet(QString("background-color: %1; border: 1px solid %2; "
                "border-radius: 8px; padding: 12px;").arg(cardBg, cardBorder));
            layout->addWidget(infoLabel);

            QString buttonStyle = QString(
                "QPushButton { background-color: %1; color: %2; border: 1px solid %3; "
                "border-radius: 6px; padding: 6px 16px; font-size: 13px; }"
                "QPushButton:hover { background-color: %4; }"
                "QPushButton:pressed { background-color: %5; }"
            ).arg(cardBg, textColor, cardBorder,
                 ElaThemeColor(themeMode, BasicHover).name(),
                 ElaThemeColor(themeMode, BasicPress).name());

            QPushButton *licensesButton = new QPushButton(tr("View Licenses"));
            licensesButton->setStyleSheet(buttonStyle);
            connect(licensesButton, &QPushButton::clicked, [this]() {
                ShijimaLicensesDialog dialog { this };
                dialog.exec();
            });
            layout->addWidget(licensesButton);

            QHBoxLayout *linksRow = new QHBoxLayout;
            linksRow->addStretch();
            auto *btnWeb = new QPushButton(tr("Shijima Website"));
            btnWeb->setStyleSheet(buttonStyle);
            connect(btnWeb, &QPushButton::clicked, []() {
                QDesktopServices::openUrl(QUrl { "https://getshijima.app" });
            });
            linksRow->addWidget(btnWeb);
            auto *btnBug = new QPushButton(tr("Report Issue"));
            btnBug->setStyleSheet(buttonStyle);
            connect(btnBug, &QPushButton::clicked, []() {
                QDesktopServices::openUrl(QUrl { "https://github.com/qingchenyouforcc/NeurolingsCE/issues" });
            });
            linksRow->addWidget(btnBug);
            linksRow->addStretch();
            layout->addLayout(linksRow);

            QHBoxLayout *buttonRow = new QHBoxLayout;
            buttonRow->addStretch();
            QPushButton *closeButton = new QPushButton(tr("Close"));
            closeButton->setStyleSheet(buttonStyle);
            closeButton->setMinimumWidth(100);
            connect(closeButton, &QPushButton::clicked, aboutDialog, &QDialog::accept);
            buttonRow->addWidget(closeButton);
            buttonRow->addStretch();
            layout->addLayout(buttonRow);

            aboutDialog->exec();
        });
}
