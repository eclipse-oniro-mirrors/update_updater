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
#ifndef UPDATE_PROCESSOR_H
#define UPDATE_PROCESSOR_H

#include <cstdio>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include "applypatch/data_writer.h"
#include "pkg_manager.h"
#include "script_instruction.h"
#include "script_manager.h"

using Uscript::UScriptEnv;
using Uscript::UScriptInstructionFactory;
using Uscript::UScriptInstructionFactoryPtr;
using Uscript::UScriptInstructionPtr;

namespace Updater {
enum EXIT_CODES {
    EXIT_INVALID_ARGS = EXIT_SUCCESS + 1,
    EXIT_READ_PACKAGE_ERROR = 2,
    EXIT_FOUND_SCRIPT_ERROR = 3,
    EXIT_PARSE_SCRIPT_ERROR = 4,
    EXIT_EXEC_SCRIPT_ERROR = 5,
    EXIT_VERIFY_SCRIPT_ERROR = 6,
};

int ProcessUpdater(bool retry, int pipeFd, const std::string &packagePath, const std::string &keyPath);
void GetPartitionPathFromName(const std::string &partitionName, std::string &partitionPath);

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
int32_t GetFinalBinaryResult(int32_t result);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

class UpdaterInstructionFactory : public UScriptInstructionFactory {
public:
    virtual int32_t CreateInstructionInstance(UScriptInstructionPtr& instr, const std::string& name);
    UpdaterInstructionFactory() {}
    virtual ~UpdaterInstructionFactory() {}
};

class UScriptInstructionRawImageWrite : public Uscript::UScriptInstruction {
public:
    UScriptInstructionRawImageWrite() {}
    virtual ~UScriptInstructionRawImageWrite() {}
    int32_t Execute(Uscript::UScriptEnv &env, Uscript::UScriptContext &context) override;

private:
    static int RawImageWriteProcessor(const Hpackage::PkgBuffer &buffer, size_t size, size_t start, bool isFinish,
        const void* context);
    int GetWritePathAndOffset(const std::string &partitionName, std::string &writePath, uint64_t &offset,
        uint64_t &partitionSize);
    bool WriteRawImage(const std::string &partitionName, const std::unique_ptr<DataWriter> &writer,
        uint64_t partitionSize, Uscript::UScriptEnv &env);
    static size_t totalSize_;
    static size_t readSize_;
};

class UScriptInstructionPkgExtract : public Uscript::UScriptInstruction {
public:
    UScriptInstructionPkgExtract() {}
    virtual ~UScriptInstructionPkgExtract() {}
    int32_t Execute(Uscript::UScriptEnv &env, Uscript::UScriptContext &context) override;
};

class UScriptInstructionPkgExtractRetSuc : public UScriptInstructionPkgExtract {
public:
    UScriptInstructionPkgExtractRetSuc() {}
    ~UScriptInstructionPkgExtractRetSuc() override {}
    int32_t Execute(Uscript::UScriptEnv &env, Uscript::UScriptContext &context) override;
private:
    std::mutex extractNoRetMutex_;
};

class UScriptInstructionUpdateFromBin : public Uscript::UScriptInstruction {
public:
    UScriptInstructionUpdateFromBin() {}
    virtual ~UScriptInstructionUpdateFromBin() {}
    int32_t Execute(Uscript::UScriptEnv &env, Uscript::UScriptContext &context) override;
    constexpr static uint32_t STASH_BUFFER_SIZE = 1024 * 1024 * 4;
    constexpr static uint32_t BUFFER_NUM = 16;
    static int UnCompressDataProducer(const Hpackage::PkgBuffer &buffer, size_t size, size_t start, bool isFinish,
        const void* context);
private:
    static size_t stashDataSize_;
};
} // Updater
#endif /* UPDATE_PROCESSOR_H */
