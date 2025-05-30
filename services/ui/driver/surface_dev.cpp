/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "surface_dev.h"
#include <unistd.h>
#include "drm_driver.h"
#include "fbdev_driver.h"
#include "log/log.h"
#include "updater_ui_const.h"

namespace Updater {
std::vector<std::string> SurfaceDev::fbDeviceList_ {FB_DEV_PATH};

std::unique_ptr<GraphicDrv> SurfaceDev::MakeDevDrv(DevType devType)
{
    std::unique_ptr<GraphicDrv> drv = nullptr;
    switch (devType) {
        case DevType::DRM_DEVICE:
            drv = std::make_unique<DrmDriver>();
            break;
        case DevType::FB_DEVICE:
            drv = std::make_unique<FbdevDriver>();
            break;
        default:
            LOG(ERROR) << " not Support.";
            break;
    }
    return drv;
}

void SurfaceDev::Flip(const uint8_t *buf) const
{
    if (buf == nullptr) {
        LOG(ERROR) << "buf is null";
        return;
    }
    if (drv_ != nullptr) {
        drv_->Flip(buf);
    }
}

DevType SurfaceDev::GetDevType() const
{
    bool isFbDevice = std::find_if(fbDeviceList_.begin(), fbDeviceList_.end(), [] (auto &devicePath) {
        if (access(devicePath.c_str(), 0) != 0) {
            LOG(WARNING) << devicePath << " not existed";
            return false;
        }
        return true;
    }) != fbDeviceList_.end();
    if (access(DRM_DEV_PATH, 0) == 0) {
        return DevType::DRM_DEVICE;
    } else if (isFbDevice) {
        return DevType::FB_DEVICE;
    }
    return DevType::UNKNOW_DEVICE;
}

bool SurfaceDev::Init()
{
    drv_ = MakeDevDrv(GetDevType());
    if (drv_ != nullptr) {
        return drv_->Init();
    }
    return false;
}

void SurfaceDev::Blank(bool blank)
{
    if (drv_ != nullptr) {
        drv_->Blank(blank);
    }
}

void SurfaceDev::Exit(void)
{
    if (drv_ != nullptr) {
        drv_->Exit();
    }
}

void SurfaceDev::AddFbDevice(const std::string &devicePath)
{
    fbDeviceList_.push_back(devicePath);
}

void SurfaceDev::GetScreenSize(uint16_t &w, uint16_t &h, GrSurface &surface) const
{
    if (drv_ != nullptr) {
        drv_->GetGrSurface(surface);
    }
    w = surface.width;
    h = surface.height;
    LOG(INFO) << "weight=" << w << " " << "height=" << h;
}

SurfaceDev::~SurfaceDev()
{
    LOG(INFO) << "SurfaceDev end";
}
} // namespace Updater
