/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef FLASHD_FORMAT_COMMANDER_H
#define FLASHD_FORMAT_COMMANDER_H

#include "commander.h"
#include "partition.h"

namespace Flashd {
class FormatCommander : public Commander {
public:
    explicit FormatCommander(callbackFun callback) : Commander(callback) {}
    ~FormatCommander() override {};
    void DoCommand(const uint8_t *payload, int payloadSize) override;
    void PostCommand() override;
    void DoCommand(const std::string &cmdParam, size_t fileSize) override;

private:
    int DoFormat(const std::string &partitionName) const;
    bool IsOnlySupportErase(const std::string &partitionName) const;
};
} // namespace Flashd
#endif