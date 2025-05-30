/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef UPDATER_UI_BOX_PROGRESS_ADAPTER_H
#define UPDATER_UI_BOX_PROGRESS_ADAPTER_H

#include <string>
#include "component_common.h"
#include "components/ui_box_progress.h"
#include "macros_updater.h"

namespace Updater {
struct UxBoxProgressInfo {
    uint32_t defaultValue;
    std::string fgColor;
    std::string bgColor;
    std::string endPoint;
    bool hasEp;
};
struct UxViewInfo;
class ImgViewAdapter;
class BoxProgressAdapter : public OHOS::UIBoxProgress, public ComponentCommon<BoxProgressAdapter> {
    static constexpr uint32_t MAX_PROGRESS_VALUE = 100;
    DISALLOW_COPY_MOVE(BoxProgressAdapter);
public:
    using SpecificInfoType = UxBoxProgressInfo;
    static constexpr auto COMPONENT_TYPE = "UIBoxProgress";
    BoxProgressAdapter() = default;
    explicit BoxProgressAdapter(const UxViewInfo &info);
    virtual ~BoxProgressAdapter() {}
    void SetValue(float value);
    void SetVisible(bool isVisible);
    [[nodiscard]] bool InitEp();
    static bool IsValid(const UxBoxProgressInfo &info);
private:
    OHOS::Point GetPosOfEp();
    std::string epId_ = {};
    bool hasEp_ = false;
    ImgViewAdapter *ep_ = {};
};
} // namespace Updater
#endif // UPDATER_UI_PROGRESS_BAR_H
