/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef USCRIPT_COMMAND_H
#define USCRIPT_COMMAND_H

#include <memory>
#include <string>
#include <pthread.h>
#include "applypatch/block_set.h"
#include "applypatch/block_writer.h"
#include "applypatch/command_const.h"
#include "applypatch/transfer_manager.h"
#include "pkg_manager.h"
#include "script_instruction.h"
#include "script_manager.h"

namespace Updater {
struct TransferParams;
class Command {
public:
    explicit Command(TransferParams* transferParams):transferParams_(transferParams) {}
    virtual ~Command();

    virtual bool Init(const std::string &cmd_line);
    CommandType GetCommandType() const;
    std::string GetArgumentByPos(size_t pos) const;
    void SetSrcFileDescriptor(int fd);
    int GetSrcFileDescriptor() const;
    void SetTargetFileDescriptor(int fd);
    int GetTargetFileDescriptor() const;
    
    TransferParams* GetTransferParams() const;
    std::string GetCommandLine() const;
    std::string GetCommandHead() const;
    void SetIsStreamCmd(bool isStreamCmd);
    bool IsStreamCmd() const;
    
private:
    CommandType ParseCommandType(const std::string &first_cmd);

    CommandType type_ {LAST};
    std::string cmdhead_ {};
    std::string cmdLine_ {};
    std::vector<std::string> tokens_ {};
    std::unique_ptr<int> srcFd_ {};
    std::unique_ptr<int> targetFd_ {};
    TransferParams* transferParams_;
    bool isStreamCmd_ {false};
};
} // namespace Updater
#endif
