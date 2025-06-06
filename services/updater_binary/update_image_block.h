/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef UPDATER_UPDATE_IMAGE_BLOCK_H
#define UPDATER_UPDATE_IMAGE_BLOCK_H
#include "pkg_manager.h"
#include "script_instruction.h"
#include "script_manager.h"

namespace Updater {
struct UpdateBlockInfo {
    std::string partitionName;
    std::string transferName;
    std::string newDataName;
    std::string patchDataName;
    std::string devPath;
};

class UScriptInstructionBlockUpdate : public Uscript::UScriptInstruction {
public:
    UScriptInstructionBlockUpdate() {}
    virtual ~UScriptInstructionBlockUpdate() {}
    int32_t Execute(Uscript::UScriptEnv &env, Uscript::UScriptContext &context) override;
};

class UScriptInstructionBlockCheck : public Uscript::UScriptInstruction {
public:
    UScriptInstructionBlockCheck() {}
    virtual ~UScriptInstructionBlockCheck() {}
    int32_t Execute(Uscript::UScriptEnv &env, Uscript::UScriptContext &context) override;
private:
    bool ExecReadBlockInfo(const std::string &devPath, Uscript::UScriptContext &context,
        time_t &mountTime, uint16_t &mountCount);
};

class UScriptInstructionShaCheck : public Uscript::UScriptInstruction {
public:
    struct ShaInfo {
        std::string blockPairs {};
        std::string contrastSha {};
        std::string targetPairs {};
        std::string targetSha {};
    };
    UScriptInstructionShaCheck() {}
    virtual ~UScriptInstructionShaCheck() {}
    int32_t Execute(Uscript::UScriptEnv &env, Uscript::UScriptContext &context) override;
private:
    int ExecReadShaInfo(Uscript::UScriptEnv &env, const std::string &devPath, const ShaInfo &shaInfo,
        const std::string &partitionName);
    int32_t DoBlocksVerify(Uscript::UScriptEnv &env, const std::string &partitionName, const std::string &devPath);
    void PrintAbnormalBlockHash(const std::string &devPath, const std::string &blockPairs);
    std::string CalculateBlockSha(const std::string &devPath, const std::string &blockPairs);
    int32_t SetShaInfo(Uscript::UScriptContext &context, ShaInfo &shaInfo);
    bool IsTargetShaDiff(const std::string &devPath, const ShaInfo &shaInfo);
};

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
void GetWriteDevPath(const std::string &path, [[maybe_unused]]const std::string &partitionName,
                     std::string &devPath);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
}

#endif // UPDATER_UPDATE_IMAGE_BLOCK_H
