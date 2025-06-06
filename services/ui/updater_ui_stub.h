/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UPDATER_UI_STUB_H
#define UPDATER_UI_STUB_H

#include "macros_updater.h"
#ifdef UPDATER_UI_SUPPORT
#include "updater_ui_facade.h"
#else
#include "updater_ui_empty.h"
#endif

namespace Updater {
#ifdef UPDATER_UI_SUPPORT
#define UPDATER_UI_INSTANCE UpdaterUiFacade::GetInstance()
#else
#define UPDATER_UI_INSTANCE UpdaterUiEmpty::GetInstance()
#endif
} // namespace Updater
#endif
