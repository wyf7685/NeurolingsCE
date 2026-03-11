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
#include "../ShijimaManagerInternal.hpp"
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <QDirIterator>
#include <QGuiApplication>
#include <QListWidget>
#include <QRandomGenerator>
#include <QScreen>

using namespace shijima;

void ShijimaManager::killAll() {
    for (auto mascot : m_mascots) {
        mascot->markForDeletion();
    }
}

void ShijimaManager::killAll(QString const& name) {
    for (auto mascot : m_mascots) {
        if (mascot->mascotName() == name) {
            mascot->markForDeletion();
        }
    }
}

void ShijimaManager::killAllButOne(ShijimaWidget *widget) {
    for (auto mascot : m_mascots) {
        if (widget == mascot) {
            continue;
        }
        mascot->markForDeletion();
    }
}

void ShijimaManager::killAllButOne(QString const& name) {
    bool foundOne = false;
    for (auto mascot : m_mascots) {
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
    m_factory.register_template(tmpl);
    m_loadedMascots.insert(data->name(), data);
    m_loadedMascotsById.insert(data->id(), data);
    std::cout << "Loaded mascot: " << data->name().toStdString() << std::endl;
}

void ShijimaManager::loadDefaultMascot() {
    auto data = new MascotData { "@", m_idCounter++ };
    loadData(data);
}

QMap<QString, MascotData *> const& ShijimaManager::loadedMascots() {
    return m_loadedMascots;
}

QMap<int, MascotData *> const& ShijimaManager::loadedMascotsById() {
    return m_loadedMascotsById;
}

std::list<ShijimaWidget *> const& ShijimaManager::mascots() {
    return m_mascots;
}

std::map<int, ShijimaWidget *> const& ShijimaManager::mascotsById() {
    return m_mascotsById;
}

void ShijimaManager::reloadMascot(QString const& name) {
    if (m_loadedMascots.contains(name) && !m_loadedMascots[name]->deletable()) {
        std::cout << "Refusing to unload mascot: " << name.toStdString()
            << std::endl;
        return;
    }

    MascotData *newData = nullptr;
    try {
        newData = new MascotData {
            m_mascotsPath + QDir::separator() + name + ".mascot",
            m_idCounter++
        };
    }
    catch (std::exception &ex) {
        std::cerr << "couldn't load mascot: " << name.toStdString() << std::endl;
        std::cerr << ex.what() << std::endl;
    }

    if (m_loadedMascots.contains(name)) {
        MascotData *oldData = m_loadedMascots[name];
        m_factory.deregister_template(name.toStdString());
        oldData->unloadCache();
        killAll(name);
        m_loadedMascots.remove(name);
        m_loadedMascotsById.remove(oldData->id());
        delete oldData;
        std::cout << "Unloaded mascot: " << name.toStdString() << std::endl;
    }

    if (newData != nullptr) {
        if (newData->name() != name) {
            throw std::runtime_error("Impossible condition: New mascot name is incorrect");
        }
        loadData(newData);
    }

    m_listItemsToRefresh.insert(name);
    ShijimaManagerInternal::refreshTrayMenu(this);
}

void ShijimaManager::refreshListWidget() {
    m_listWidget.clear();
    auto names = m_loadedMascots.keys();
    names.sort(Qt::CaseInsensitive);
    for (auto &name : names) {
        auto item = new QListWidgetItem;
        item->setText(name);
        item->setIcon(m_loadedMascots[name]->preview());
        m_listWidget.addItem(item);
    }
    m_listItemsToRefresh.clear();
}

void ShijimaManager::loadAllMascots() {
    QDirIterator iter { m_mascotsPath, QDir::Dirs | QDir::NoDotAndDotDot,
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
    if (m_hasTickCallbacks) {
        auto lock = acquireLock();
        for (auto &callback : m_tickCallbacks) {
            callback(this);
        }
        m_tickCallbacks.clear();
        m_hasTickCallbacks = false;
        m_tickCallbackCompletion.notify_all();
    }

    if (m_sandboxWidget != nullptr && !m_sandboxWidget->isVisible()) {
        setWindowedMode(false);
#if !defined(__APPLE__)
        if (m_mascots.size() == 0) {
            setManagerVisible(true);
        }
#endif
    }

    if (m_mascots.size() == 0) {
#if !defined(__APPLE__)
        if (!windowedMode() && !m_wasVisible) {
            setManagerVisible(true);
        }
#endif
        return;
    }

    updateEnvironment();

    for (auto iter = m_mascots.end(); iter != m_mascots.begin(); ) {
        --iter;
        ShijimaWidget *shimeji = *iter;
        if (!shimeji->isVisible()) {
            int mascotId = shimeji->mascotId();
            delete shimeji;
            auto erasePos = iter;
            ++iter;
            m_mascots.erase(erasePos);
            m_mascotsById.erase(mascotId);
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
            auto oldScreen = m_reverseEnv[mascot.state->env.get()];
            auto newScreen = QGuiApplication::screenAt(QPoint {
                (int)mascot.state->anchor.x, (int)mascot.state->anchor.y });
            if (newScreen != nullptr && oldScreen != newScreen) {
                mascot.state->env = m_env[newScreen];
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
                product = m_factory.spawn(breedRequest);
            }
            catch (std::exception &ex) {
                std::cerr << "couldn't fulfill breed request for "
                    << breedRequest.name << std::endl;
                std::cerr << ex.what() << std::endl;
            }

            if (product.has_value()) {
                ShijimaWidget *child = new ShijimaWidget(
                    m_loadedMascots[QString::fromStdString(breedRequest.name)],
                    std::move(product->manager), m_idCounter++,
                    windowedMode(), mascotParent());
                child->setEnv(shimeji->env());
                child->show();
                m_mascots.push_back(child);
                m_mascotsById[child->mascotId()] = child;
            }
            breedRequest.available = false;
        }
    }

    for (auto &env : m_env) {
        env->reset_scale();
    }

    if (m_mascots.size() == 0 && !windowedMode()) {
        setManagerVisible(true);
    }

    updateStatusBar();
}

ShijimaWidget *ShijimaManager::hitTest(QPoint const& screenPos) {
    for (auto mascot : m_mascots) {
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
    auto &env = m_env[screen];
    auto product = m_factory.spawn(name, {});
    product.manager->state->env = env;
    product.manager->reset_position();
    ShijimaWidget *shimeji = new ShijimaWidget(
        m_loadedMascots[QString::fromStdString(name)],
        std::move(product.manager), m_idCounter++,
        windowedMode(), mascotParent());
    shimeji->show();
    m_mascots.push_back(shimeji);
    m_mascotsById[shimeji->mascotId()] = shimeji;
    env->reset_scale();
    return shimeji;
}

void ShijimaManager::spawnClicked() {
    auto &allTemplates = m_factory.get_all_templates();
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
