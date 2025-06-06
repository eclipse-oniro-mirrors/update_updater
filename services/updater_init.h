/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef UPDATER_INIT_H
#define UPDATER_INIT_H

#include <vector>
#include "macros_updater.h"

namespace Updater {
enum UpdaterInitEvent {
    // main
    UPDATER_MAIN_PRE_EVENT = 0,

    // updater
    UPDATER_PRE_INIT_EVENT,
    UPDATER_INIT_EVENT,
    UPDATER_PRE_UPDATE_PACKAGE_EVENT,
    UPDATER_PRE_VERIFY_PACKAGE_EVENT,
    UPDATER_POST_UPDATE_PACKAGE_EVENT,
    UPDATER_POST_INIT_EVENT,

    // flashd
    FLAHSD_PRE_INIT_EVENT,

    // binary
    UPDATER_BINARY_INIT_EVENT,
    UPDATER_BINARY_INIT_DONE_EVENT,

    // factory reset
    FACTORY_RESET_INIT_EVENT,

    UPDATER_INIT_EVENT_BUTT
};

using InitHandler = void (*)(void);

class UpdaterInit {
    DISALLOW_COPY_MOVE(UpdaterInit);
public:
    static UpdaterInit &GetInstance()
    {
        static UpdaterInit instance;
        return instance;
    }
    void InvokeEvent(enum UpdaterInitEvent eventId) const
    {
        if (eventId >= UPDATER_INIT_EVENT_BUTT) {
            return;
        }
        for (const auto &handler : initEvent_[eventId]) {
            if (handler != nullptr) {
                handler();
            }
        }
    }
    void SubscribeEvent(enum UpdaterInitEvent eventId, InitHandler handler)
    {
        if (eventId < UPDATER_INIT_EVENT_BUTT) {
            initEvent_[eventId].push_back(handler);
        }
    }
private:
    UpdaterInit() = default;
    ~UpdaterInit() = default;
    std::vector<InitHandler> initEvent_[UPDATER_INIT_EVENT_BUTT];
};

#define DEFINE_INIT_EVENT(name, event, ...)                                      \
    static void name##_##event##_Init(void)                                      \
    {                                                                            \
        __VA_ARGS__;                                                             \
    }                                                                            \
    __attribute((constructor)) static void Register_##name##_##event(void)       \
    {                                                                            \
        UpdaterInit::GetInstance().SubscribeEvent(event, name##_##event##_Init); \
    }                                                                            \
    static_assert(true)

// mode related macro
#define MODE_ENTRY(name) name##Main

#define MODE_CONDITION(name) Is##name

#define BOOT_MODE(name, para) BootMode { MODE_CONDITION(name), STRINGFY(name), para, MODE_ENTRY(name) }

#define REGISTER_MODE(name, para, ...) \
    DEFINE_INIT_EVENT(name, UPDATER_MAIN_PRE_EVENT, RegisterMode(BOOT_MODE(name, para)))
}

#endif