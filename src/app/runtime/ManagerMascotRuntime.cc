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
#include "shijima-qt/ui/mascot/ShijimaWidget.hpp"
#include "../ui/ManagerUiHelpers.hpp"
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <QDir>
#include <QDirIterator>
#include <QGuiApplication>
#include <QListWidget>
#include <QRandomGenerator>
#include <QScreen>

using namespace shijima;

void ShijimaManager::killAll() {
    for (auto mascot : m_runtime->mascots) {
        mascot->markForDeletion();
    }
}

void ShijimaManager::killAll(QString const& name) {
    for (auto mascot : m_runtime->mascots) {
        if (mascot->mascotName() == name) {
            mascot->markForDeletion();
        }
    }
}

void ShijimaManager::killAllButOne(ShijimaWidget *widget) {
    for (auto mascot : m_runtime->mascots) {
        if (widget == mascot) {
            continue;
        }
        mascot->markForDeletion();
    }
}

void ShijimaManager::killAllButOne(QString const& name) {
    bool foundOne = false;
    for (auto mascot : m_runtime->mascots) {
        if (mascot->mascotName() == name) {
            if (!foundOne) {
                foundOne = true;
                continue;
            }
            mascot->markForDeletion();
        }
    }
}

void ShijimaManager::loadData(MascotData *data) {
    if (data == nullptr || !data->valid()) {
        throw std::runtime_error("loadData() called with invalid data");
    }

    shijima::mascot::factory::tmpl tmpl;
    tmpl.actions_xml = data->actionsXML().toStdString();
    tmpl.behaviors_xml = data->behaviorsXML().toStdString();
    tmpl.name = data->name().toStdString();
    tmpl.path = data->path().toStdString();
    m_runtime->factory.register_template(tmpl);
    m_runtime->loadedMascots.insert(data->name(), data);
    m_runtime->loadedMascotsById.insert(data->id(), data);
    std::cout << "Loaded mascot: " << data->name().toStdString() << std::endl;
}

void ShijimaManager::loadDefaultMascot() {
    auto data = new MascotData { "@", m_runtime->idCounter++ };
    loadData(data);
}

QMap<QString, MascotData *> const& ShijimaManager::loadedMascots() {
    return m_runtime->loadedMascots;
}

QMap<int, MascotData *> const& ShijimaManager::loadedMascotsById() {
    return m_runtime->loadedMascotsById;
}

std::list<ShijimaWidget *> const& ShijimaManager::mascots() {
    return m_runtime->mascots;
}

std::map<int, ShijimaWidget *> const& ShijimaManager::mascotsById() {
    return m_runtime->mascotsById;
}

void ShijimaManager::reloadMascot(QString const& name) {
    if (m_runtime->loadedMascots.contains(name) && !m_runtime->loadedMascots[name]->deletable()) {
        std::cout << "Refusing to unload mascot: " << name.toStdString()
            << std::endl;
        return;
    }

    MascotData *newData = nullptr;
    try {
        newData = new MascotData {
            m_runtime->mascotsPath + QDir::separator() + name + ".mascot",
            m_runtime->idCounter++
        };
    }
    catch (std::exception &ex) {
        std::cerr << "couldn't load mascot: " << name.toStdString() << std::endl;
        std::cerr << ex.what() << std::endl;
    }

    if (m_runtime->loadedMascots.contains(name)) {
        MascotData *oldData = m_runtime->loadedMascots[name];
        m_runtime->factory.deregister_template(name.toStdString());
        oldData->unloadCache();
        killAll(name);
        m_runtime->loadedMascots.remove(name);
        m_runtime->loadedMascotsById.remove(oldData->id());
        delete oldData;
        std::cout << "Unloaded mascot: " << name.toStdString() << std::endl;
    }

    if (newData != nullptr) {
        if (newData->name() != name) {
            throw std::runtime_error("Impossible condition: New mascot name is incorrect");
        }
        loadData(newData);
    }

    m_runtime->listItemsToRefresh.insert(name);
    ShijimaManagerUiInternal::refreshTrayMenu(this);
}

void ShijimaManager::refreshListWidget() {
    m_ui->listWidget->clear();
    auto names = m_runtime->loadedMascots.keys();
    names.sort(Qt::CaseInsensitive);
    for (auto &name : names) {
        auto item = new QListWidgetItem;
        item->setText(name);
        item->setIcon(m_runtime->loadedMascots[name]->preview());
        m_ui->listWidget->addItem(item);
    }
    m_runtime->listItemsToRefresh.clear();
}

void ShijimaManager::loadAllMascots() {
    QDirIterator iter { m_runtime->mascotsPath, QDir::Dirs | QDir::NoDotAndDotDot,
        QDirIterator::NoIteratorFlags };
    while (iter.hasNext()) {
        auto name = iter.nextFileInfo().fileName();
        if (!name.endsWith(".mascot") || name.length() <= 7) {
            continue;
        }
        reloadMascot(name.sliced(0, name.length() - 7));
    }
    refreshListWidget();
}

void ShijimaManager::reloadMascots(std::set<std::string> const& mascots) {
    for (auto &mascot : mascots) {
        reloadMascot(QString::fromStdString(mascot));
    }
    refreshListWidget();
}

void ShijimaManager::tick() {
    if (m_runtime->hasTickCallbacks) {
        auto lock = acquireLock();
        for (auto &callback : m_runtime->tickCallbacks) {
            callback(this);
        }
        m_runtime->tickCallbacks.clear();
        m_runtime->hasTickCallbacks = false;
        m_runtime->tickCallbackCompletion.notify_all();
    }

    if (m_ui->sandboxWidget != nullptr && !m_ui->sandboxWidget->isVisible()) {
        setWindowedMode(false);
#if !defined(__APPLE__)
        if (m_runtime->mascots.size() == 0) {
            setManagerVisible(true);
        }
#endif
    }

    if (m_runtime->mascots.size() == 0) {
#if !defined(__APPLE__)
        if (!windowedMode() && !m_wasVisible) {
            setManagerVisible(true);
        }
#endif
        return;
    }

    updateEnvironment();

    for (auto iter = m_runtime->mascots.end(); iter != m_runtime->mascots.begin(); ) {
        --iter;
        ShijimaWidget *shimeji = *iter;
        if (!shimeji->isVisible()) {
            int mascotId = shimeji->mascotId();
            delete shimeji;
            auto erasePos = iter;
            ++iter;
            m_runtime->mascots.erase(erasePos);
            m_runtime->mascotsById.erase(mascotId);
            continue;
        }

        double savedFloorY = 0.0;
        bool didOverrideFloor = false;
        if (shimeji->isFallThroughMode()) {
            savedFloorY = shimeji->env()->floor.y;
            shimeji->env()->floor.y = shimeji->env()->screen.bottom;
            shimeji->env()->work_area.bottom = shimeji->env()->screen.bottom;
            didOverrideFloor = true;
        }

        double anchorYBefore = shimeji->mascot().state->anchor.y;
        shimeji->tick();

        if (didOverrideFloor) {
            shimeji->env()->floor.y = savedFloorY;
            shimeji->env()->work_area.bottom = savedFloorY;
        }

        if (!windowedMode()) {
            double anchorYAfter = shimeji->mascot().state->anchor.y;
            bool onLand = shimeji->mascot().state->on_land();
            bool isDragging = shimeji->mascot().state->dragging;

            if (isDragging && shimeji->isFallThroughMode()) {
                shimeji->m_fallThroughMode = false;
                shimeji->m_fallTracking = false;
            }

            if (!onLand && !isDragging && anchorYAfter > anchorYBefore) {
                if (!shimeji->m_fallTracking) {
                    shimeji->m_fallTracking = true;
                    shimeji->m_fallStartY = anchorYBefore;
                }
                double fallDistance = anchorYAfter - shimeji->m_fallStartY;
                if (fallDistance >= 700.0) {
                    shimeji->m_fallThroughMode = true;
                }
            }
            else {
                shimeji->m_fallTracking = false;
            }
        }

        auto &mascot = shimeji->mascot();
        auto &breedRequest = mascot.state->breed_request;
        if (mascot.state->dragging && !windowedMode()) {
            auto oldScreen = m_runtime->reverseEnv[mascot.state->env.get()];
            auto newScreen = QGuiApplication::screenAt(QPoint {
                (int)mascot.state->anchor.x, (int)mascot.state->anchor.y });
            if (newScreen != nullptr && oldScreen != newScreen) {
                mascot.state->env = m_runtime->env[newScreen];
            }
        }
        if (breedRequest.available) {
            if (breedRequest.name == "") {
                breedRequest.name = shimeji->mascotName().toStdString();
            }
            breedRequest.name = breedRequest.name.substr(breedRequest.name.rfind('\\') + 1);
            breedRequest.name = breedRequest.name.substr(breedRequest.name.rfind('/') + 1);

            std::optional<shijima::mascot::factory::product> product;
            try {
                product = m_runtime->factory.spawn(breedRequest);
            }
            catch (std::exception &ex) {
                std::cerr << "couldn't fulfill breed request for "
                    << breedRequest.name << std::endl;
                std::cerr << ex.what() << std::endl;
            }

            if (product.has_value()) {
                ShijimaWidget *child = new ShijimaWidget(
                    m_runtime->loadedMascots[QString::fromStdString(breedRequest.name)],
                    std::move(product->manager), m_runtime->idCounter++,
                    windowedMode(), mascotParent());
                child->setEnv(shimeji->env());
                child->show();
                m_runtime->mascots.push_back(child);
                m_runtime->mascotsById[child->mascotId()] = child;
            }
            breedRequest.available = false;
        }
    }

    for (auto &env : m_runtime->env) {
        env->reset_scale();
    }

    if (m_runtime->mascots.size() == 0 && !windowedMode()) {
        setManagerVisible(true);
    }

    updateStatusBar();
}

ShijimaWidget *ShijimaManager::hitTest(QPoint const& screenPos) {
    for (auto mascot : m_runtime->mascots) {
        QPoint localPos = { screenPos.x() - mascot->x(),
            screenPos.y() - mascot->y() };
        if (mascot->pointInside(localPos)) {
            return mascot;
        }
    }
    return nullptr;
}

ShijimaWidget *ShijimaManager::spawn(std::string const& name) {
    QScreen *screen = mascotScreen();
    updateEnvironment(screen);
    auto &env = m_runtime->env[screen];
    auto product = m_runtime->factory.spawn(name, {});
    product.manager->state->env = env;
    product.manager->reset_position();
    ShijimaWidget *shimeji = new ShijimaWidget(
        m_runtime->loadedMascots[QString::fromStdString(name)],
        std::move(product.manager), m_runtime->idCounter++,
        windowedMode(), mascotParent());
    shimeji->show();
    m_runtime->mascots.push_back(shimeji);
    m_runtime->mascotsById[shimeji->mascotId()] = shimeji;
    env->reset_scale();
    return shimeji;
}

void ShijimaManager::spawnClicked() {
    auto &allTemplates = m_runtime->factory.get_all_templates();
    int target = QRandomGenerator::global()->bounded((int)allTemplates.size());
    int i = 0;
    for (auto &pair : allTemplates) {
        if (i++ != target) {
            continue;
        }
        std::cout << "Spawning: " << pair.first << std::endl;
        spawn(pair.first);
        break;
    }
}
