/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "input_event.h"
#include <unistd.h>
#include "log/log.h"
#include "keys_input_device.h"

namespace Updater {
extern "C" __attribute__((constructor)) void RegisterAddInputDeviceHelper(void)
{
    InputEvent::GetInstance().RegisterAddInputDeviceHelper(AddInputDevice);
}

void InputEvent::RegisterAddInputDeviceHelper(AddInputDeviceFunc ptr)
{
    addInputDeviceHelper_ = std::move(ptr);
}

extern "C" __attribute__((constructor)) void RegisterHandleEventHelper(void)
{
    InputEvent::GetInstance().RegisterHandleEventHelper(HandlePointersEvent);
}

void InputEvent::RegisterHandleEventHelper(HandlePointersEventFunc ptr)
{
    handlePointersEventHelper_ = std::move(ptr);
}

InputEvent &InputEvent::GetInstance()
{
    static InputEvent instance;
    return instance;
}

int InputEvent::HandleInputEvent(const struct input_event *iev, uint32_t type)
{
    struct input_event ev {};
    ev.type = iev->type;
    ev.code = iev->code;
    ev.value = iev->value;

    KeysInputDevice::GetInstance().HandleKeyEvent(ev, type);
    handlePointersEventHelper_(ev, type);
    return 0;
}

void InputEvent::GetInputDeviceType(uint32_t devIndex, uint32_t &type)
{
    auto it = devTypeMap_.find(devIndex);
    if (it == devTypeMap_.end()) {
        LOG(ERROR) << "devTypeMap_ devIndex: " << devIndex << "not valid";
        return;
    }
    type = it->second;
}

int32_t InputEvent::HdfInputEventCallback::EventPkgCallback(
    const std::vector<OHOS::HDI::Input::V1_0::EventPackage>& pkgs, uint32_t devIndex)
{
    if (pkgs.empty()) {
        LOG(WARNING) << "pkgs is empty";
        return HDF_FAILURE;
    }
    for (uint32_t i = 0; i < pkgs.size(); i++) {
        struct input_event ev = {
            .type = static_cast<__u16>(pkgs[i].type),
            .code = static_cast<__u16>(pkgs[i].code),
            .value = pkgs[i].value,
        };
        uint32_t type = 0;
        InputEvent::GetInstance().GetInputDeviceType(devIndex, type);
        InputEvent::GetInstance().HandleInputEvent(&ev, type);
    }
    return HDF_SUCCESS;
}

int32_t InputEvent::HdfInputEventCallback::HotPlugCallback(const OHOS::HDI::Input::V1_0::HotPlugEvent &event)
{
    return HDF_SUCCESS;
}

int InputEvent::HdfInit()
{
    inputInterface_ = OHOS::HDI::Input::V1_0::IInputInterfaces::Get(true);
    if (inputInterface_ == nullptr) {
        LOG(ERROR) << "get input driver interface failed";
        return HDF_FAILURE;
    }
 
    sleep(1); // need wait thread running

    std::vector<OHOS::HDI::Input::V1_0::DevDesc> sta = {};
    int ret = inputInterface_->ScanInputDevice(sta);
    if (ret != HDF_SUCCESS) {
        LOG(ERROR) << "scan device failed";
        return ret;
    }

    for (size_t i = 0; i < sta.size(); i++) {
        uint32_t idx = sta[i].devIndex;
        uint32_t dev = sta[i].devType;
        if ((idx == 0) || (inputInterface_->OpenInputDevice(idx) == HDF_FAILURE)) {
            continue;
        }
        devTypeMap_.insert(std::pair<uint32_t, uint32_t>(idx, dev));
 
        LOG(INFO) << "hdf devType:" << dev << ", devIndex:" << idx;
    }

    /* first param not necessary, pass default 1 */
    callback_ = new (std::nothrow) HdfInputEventCallback();
    if (callback_ == nullptr) {
        LOG(ERROR) << "callback is nullptr";
        return ret;
    }
    ret = inputInterface_->RegisterReportCallback(1, callback_);
    if (ret != HDF_SUCCESS) {
        LOG(ERROR) << "register callback failed for device 1";
        return ret;
    }

    OHOS::InputDeviceManager::GetInstance()->Add(&KeysInputDevice::GetInstance());
    OHOS::InputDeviceManager::GetInstance()->SetPeriod(0);
    addInputDeviceHelper_();
    LOG(INFO) << "add InputDevice done";

    return 0;
}
} // namespace Updater
