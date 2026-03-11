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

#include "shijima-qt/ui/dialogs/inspector/ShimejiInspectorDialog.hpp"
#include "shijima-qt/ShijimaWidget.hpp"
#include <QFormLayout>
#include "ElaTheme.h"

ShimejiInspectorDialog::ShimejiInspectorDialog(ShijimaWidget *parent):
    QDialog(parent), m_formLayout(new QFormLayout)
{
    setWindowFlags((windowFlags() | Qt::CustomizeWindowHint |
        Qt::WindowCloseButtonHint) &
        ~(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint));
    setWindowTitle(tr("Inspector — %1").arg(parent->mascotName()));
    setStyleSheet(QString("QDialog { background-color: %1; }")
        .arg(ElaThemeColor(eTheme->getThemeMode(), DialogBase).name()));
    setLayout(m_formLayout);
    m_formLayout->setFormAlignment(Qt::AlignLeft);
    m_formLayout->setLabelAlignment(Qt::AlignRight);
    m_formLayout->setHorizontalSpacing(16);
    m_formLayout->setVerticalSpacing(6);
    m_formLayout->setContentsMargins(16, 16, 16, 16);

    registerRows();
}

ShijimaWidget *ShimejiInspectorDialog::shijimaParent() {
    return static_cast<ShijimaWidget *>(parent());
}

void ShimejiInspectorDialog::tick() {
    for (auto &callback : m_tickCallbacks) {
        callback();
    }
}

#include "ShimejiInspectorDialog.moc"
