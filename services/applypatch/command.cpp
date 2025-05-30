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
#include "applypatch/command.h"
#include <cstdio>
#include <vector>
#include "applypatch/block_set.h"
#include "log/log.h"
#include "utils.h"

namespace Updater {
bool Command::Init(const std::string &cmdLine)
{
    if (cmdLine.empty()) return false;
    cmdLine_ = std::move(cmdLine);
    tokens_.clear();
    tokens_ = Utils::SplitString(cmdLine_, " ");
    cmdhead_ = tokens_[H_ZERO_NUMBER];
    type_ = ParseCommandType(tokens_[H_ZERO_NUMBER]);
    return true;
}

Command::~Command()
{
    srcFd_.reset();
    targetFd_.reset();
}

CommandType Command::GetCommandType() const
{
    return type_;
}

std::string Command::GetCommandHead() const
{
    return cmdhead_;
}

std::string Command::GetArgumentByPos(size_t pos) const
{
    if (pos >= tokens_.size()) {
        return "";
    }
    return tokens_[pos];
}

std::string Command::GetCommandLine() const
{
    return cmdLine_;
}

void Command::SetSrcFileDescriptor(int fd)
{
    srcFd_ = std::make_unique<int>(fd);
}

int Command::GetSrcFileDescriptor() const
{
    return *srcFd_;
}

void Command::SetTargetFileDescriptor(int fd)
{
    targetFd_ = std::make_unique<int>(fd);
}

int Command::GetTargetFileDescriptor() const
{
    return *targetFd_;
}
    
void Command::SetIsStreamCmd(bool isStreamCmd)
{
    isStreamCmd_ = isStreamCmd;
}

bool Command::IsStreamCmd() const
{
    return isStreamCmd_;
}

TransferParams* Command::GetTransferParams() const
{
    return transferParams_;
}

CommandType Command::ParseCommandType(const std::string &firstCmd)
{
    if (firstCmd == "abort") {
        return CommandType::ABORT;
    } else if (firstCmd == "bsdiff") {
        return CommandType::BSDIFF;
    } else if (firstCmd == "erase") {
        return CommandType::ERASE;
    } else if (firstCmd == "free") {
        return CommandType::FREE;
    } else if (firstCmd == "pkgdiff") {
        return CommandType::IMGDIFF;
    } else if (firstCmd == "move") {
        return CommandType::MOVE;
    } else if (firstCmd == "new") {
        return CommandType::NEW;
    } else if (firstCmd == "stash") {
        return CommandType::STASH;
    } else if (firstCmd == "zero") {
        return CommandType::ZERO;
    } else if (firstCmd == "copy") {
        return CommandType::COPY;
    }
    return CommandType::LAST;
}
}
