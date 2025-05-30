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

#include "applypatch/block_set.h"
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "applypatch/command.h"
#include "applypatch/store.h"
#include "applypatch/transfer_manager.h"
#include "log/dump.h"
#include "log/log.h"
#include "patch/update_patch.h"
#include "securec.h"
#include "utils.h"

using namespace Updater;
using namespace Updater::Utils;

namespace Updater {
BlockSet::BlockSet(std::vector<BlockPair> &&pairs)
{
    blockSize_ = 0;
    if (pairs.empty()) {
        LOG(ERROR) << "Invalid block.";
        return;
    }

    for (const auto &pair : pairs) {
        if (!CheckReliablePair(pair)) {
            return;
        }
        PushBack(pair);
    }
}

bool BlockSet::CheckReliablePair(BlockPair pair)
{
    if (pair.first >= pair.second) {
        LOG(ERROR) << "Invalid number of block size";
        return false;
    }
    size_t size = pair.second - pair.first;
    if (blockSize_ >= (SIZE_MAX - size)) {
        LOG(ERROR) << "Block size overflow";
        return false;
    }
    return true;
}

void BlockSet::PushBack(BlockPair blockPair)
{
    blocks_.push_back(std::move(blockPair));
    blockSize_ += (blockPair.second - blockPair.first);
}

void BlockSet::ClearBlocks()
{
    blockSize_ = 0;
    blocks_.clear();
}

bool BlockSet::ParserAndInsert(const std::string &blockStr)
{
    if (blockStr == "") {
        LOG(ERROR) << "Invalid argument, this argument is empty";
        return false;
    }
    std::vector<std::string> pairs = SplitString(blockStr, ",");
    return ParserAndInsert(pairs);
}

bool BlockSet::ParserAndInsert(const std::vector<std::string> &blockToken)
{
    ClearBlocks();
    if (blockToken.empty()) {
        LOG(ERROR) << "Invalid block token argument";
        return false;
    }
    if (blockToken.size() < 3) { // 3:blockToken.size() < 3 means too small blocks_ in argument
        LOG(ERROR) << "Too small blocks_ in argument";
        return false;
    }
    // Get number of blockToken
    unsigned long long int blockPairSize;
    std::vector<std::string> bt = blockToken;
    std::vector<std::string>::iterator bp = bt.begin();
    blockPairSize = String2Int<unsigned long long int>(*bp, N_DEC);
    if (blockPairSize == 0 || blockPairSize % 2 != 0 || // 2:Check whether blockPairSize is valid.
        blockPairSize != bt.size() - 1) {
        LOG(ERROR) << "Invalid number in block token";
        return false;
    }

    while (++bp != bt.end()) {
        size_t first = String2Int<size_t>(*bp++, N_DEC);
        size_t second = String2Int<size_t>(*bp, N_DEC);
        blocks_.push_back(BlockPair {
            first, second
        });
        blockSize_ += (second - first);
    }
    return true;
}

std::vector<BlockPair>::iterator BlockSet::Begin()
{
    return blocks_.begin();
}

std::vector<BlockPair>::iterator BlockSet::End()
{
    return blocks_.end();
}

std::vector<BlockPair>::const_iterator BlockSet::CBegin() const
{
    return blocks_.cbegin();
}

std::vector<BlockPair>::const_iterator BlockSet::CEnd() const
{
    return blocks_.cend();
}

std::vector<BlockPair>::const_reverse_iterator BlockSet::CrBegin() const
{
    return blocks_.crbegin();
}

std::vector<BlockPair>::const_reverse_iterator BlockSet::CrEnd() const
{
    return blocks_.crend();
}

size_t BlockSet::ReadDataFromBlock(int fd, std::vector<uint8_t> &buffer)
{
    size_t pos = 0;
    std::vector<BlockPair>::iterator it = blocks_.begin();
    int ret;
    for (; it != blocks_.end(); ++it) {
        ret = lseek64(fd, static_cast<off64_t>(it->first * H_BLOCK_SIZE), SEEK_SET);
        if (ret == -1) {
            LOG(ERROR) << "Fail to seek";
            return 0;
        }
        size_t size = (it->second - it->first) * H_BLOCK_SIZE;
        if (!Utils::ReadFully(fd, buffer.data() + pos, size)) {
            LOG(ERROR) << "Fail to read";
            return 0;
        }
        pos += size;
    }
    return pos;
}

size_t BlockSet::WriteDataToBlock(int fd, std::vector<uint8_t> &buffer)
{
    size_t pos = 0;
    std::vector<BlockPair>::iterator it = blocks_.begin();
    int ret = 0;
    for (; it != blocks_.end(); ++it) {
        off64_t offset = static_cast<off64_t>(it->first * H_BLOCK_SIZE);
        size_t writeSize = (it->second - it->first) * H_BLOCK_SIZE;

        ret = lseek64(fd, offset, SEEK_SET);
        if (ret == -1) {
            LOG(ERROR) << "BlockSet::WriteDataToBlock Fail to seek";
            return 0;
        }
        if (Utils::WriteFully(fd, buffer.data() + pos, writeSize) == false) {
            LOG(ERROR) << "Write data to block error, errno : " << errno;
            return 0;
        }
        pos += writeSize;
    }
    if (fsync(fd) == -1) {
        LOG(ERROR) << "Failed to fsync" << strerror(errno);
        return 0;
    }
    return pos;
}

size_t BlockSet::CountOfRanges() const
{
    return blocks_.size();
}

size_t BlockSet::TotalBlockSize() const
{
    return blockSize_;
}

int32_t BlockSet::VerifySha256(const std::vector<uint8_t> &buffer, const size_t size, const std::string &expected)
{
    UPDATER_INIT_RECORD;
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(buffer.data(), size * H_BLOCK_SIZE, digest);
    std::string hexdigest = Utils::ConvertSha256Hex(digest, SHA256_DIGEST_LENGTH);
    if (hexdigest == expected) {
        return 0;
    }
    return -1;
}

bool BlockSet::IsTwoBlocksOverlap(const BlockSet &source, BlockSet &target)
{
    auto firstIter = source.CBegin();
    for (; firstIter != source.CEnd(); ++firstIter) {
        std::vector<BlockPair>::iterator secondIter = target.Begin();
        for (; secondIter != target.End(); ++secondIter) {
            if (!(secondIter->first >= firstIter->second ||
                firstIter->first >= secondIter->second)) {
                return true;
            }
        }
    }
    return false;
}

void BlockSet::MoveBlock(std::vector<uint8_t> &target, const BlockSet& locations,
    const std::vector<uint8_t>& source)
{
    const uint8_t *sd = source.data();
    uint8_t *td = target.data();
    size_t start = locations.TotalBlockSize();
    for (auto it = locations.CrBegin(); it != locations.CrEnd(); it++) {
        size_t blocks = it->second - it->first;
        start -= blocks;
        if (memmove_s(td + (it->first * H_BLOCK_SIZE), blocks * H_BLOCK_SIZE, sd + (start *
            H_BLOCK_SIZE), blocks * H_BLOCK_SIZE) != EOK) {
                LOG(ERROR) << "MoveBlock memmove_s failed!";
                return;
            }
    }
}

int32_t BlockSet::LoadSourceBuffer(const Command &cmd, size_t &pos, std::vector<uint8_t> &sourceBuffer,
    bool &isOverlap, size_t &srcBlockSize)
{
    std::string targetCmd = cmd.GetArgumentByPos(pos++);
    std::string storeBase = cmd.GetTransferParams()->storeBase;
    if (targetCmd != "-") {
        BlockSet srcBlk;
        srcBlk.ParserAndInsert(targetCmd);
        if (!cmd.IsStreamCmd()) {
            isOverlap = IsTwoBlocksOverlap(srcBlk, *this);
        }
        // read source data
        if (srcBlk.ReadDataFromBlock(cmd.GetSrcFileDescriptor(), sourceBuffer) == 0) {
            LOG(ERROR) << "ReadDataFromBlock failed";
            return -1;
        }
        std::string nextArgv = cmd.GetArgumentByPos(pos++);
        if (nextArgv == "") {
            return 1;
        }
        BlockSet locations;
        locations.ParserAndInsert(nextArgv);
        MoveBlock(sourceBuffer, locations, sourceBuffer);
    }

    std::string lastArg = cmd.GetArgumentByPos(pos++);
    while (lastArg != "") {
        std::vector<std::string> tokens = SplitString(lastArg, ":");
        if (tokens.size() != H_CMD_ARGS_LIMIT) {
            LOG(ERROR) << "invalid parameter";
            return -1;
        }
        std::vector<uint8_t> stash;
        auto ret = Store::LoadDataFromStore(storeBase, tokens[H_ZERO_NUMBER], stash);
        if (ret == -1) {
            LOG(ERROR) << "Failed to load tokens";
            return -1;
        }
        BlockSet locations;
        locations.ParserAndInsert(tokens[1]);
        MoveBlock(sourceBuffer, locations, stash);

        lastArg = cmd.GetArgumentByPos(pos++);
    }
    return 1;
}

__attribute__((weak)) int32_t BlockVerify(const Command &cmd, std::vector<uint8_t> &buffer,
    const size_t size, const std::string srcHash, size_t &pos)
{
    return -1;
}


int32_t BlockSet::LoadTargetBuffer(const Command &cmd, std::vector<uint8_t> &buffer, size_t &blockSize,
    size_t pos, std::string &srcHash)
{
    bool isOverlap = false;
    auto ret = LoadSourceBuffer(cmd, pos, buffer, isOverlap, blockSize);
    if (ret != 1) {
        LOG(ERROR) << "LoadSourceBuffer failed";
        return ret;
    }
    if (cmd.IsStreamCmd()) {
        return 0;
    }

    std::string storeBase = cmd.GetTransferParams()->storeBase;
    std::string storePath = storeBase + "/" + srcHash;
    struct stat storeStat {};
    int res = stat(storePath.c_str(), &storeStat);
    int32_t verifyRes = VerifySha256(buffer, blockSize, srcHash);
    if (verifyRes != 0 && !cmd.GetTransferParams()->canWrite) {
        return BlockVerify(cmd, buffer, blockSize, srcHash, pos);
    }
    if (verifyRes == 0) {
        if (isOverlap && res != 0) {
            cmd.GetTransferParams()->freeStash = srcHash;
            ret = Store::WriteDataToStore(storeBase, srcHash, buffer, blockSize * H_BLOCK_SIZE);
            if (ret != 0) {
                LOG(ERROR) << "failed to stash overlapping source blocks";
                return -1;
            }
        }
        return 0;
    }
    if (Store::LoadDataFromStore(storeBase, srcHash, buffer) == 0) {
        return 0;
    }
    return -1;
}

int32_t BlockSet::WriteZeroToBlock(int fd, bool isErase)
{
    std::vector<uint8_t> buffer;
    buffer.resize(H_BLOCK_SIZE);
    if (memset_s(buffer.data(), H_BLOCK_SIZE, 0, H_BLOCK_SIZE) != EOK) {
        LOG(ERROR) << "memset_s failed";
        return -1;
    }

    auto iter = blocks_.begin();
    while (iter != blocks_.end()) {
        off64_t offset = static_cast<off64_t>(iter->first * H_BLOCK_SIZE);
        int ret = 0;

        if (isErase && Utils::IsUpdaterMode()) {
#ifndef UPDATER_UT
            size_t writeSize = (iter->second - iter->first) * H_BLOCK_SIZE;
            uint64_t arguments[2] = {static_cast<uint64_t>(offset), writeSize};
            ret = ioctl(fd, BLKDISCARD, &arguments);
            if (ret == -1 && errno != EOPNOTSUPP) {
                LOG(ERROR) << "Error to write block set to memory";
                return -1;
            }
#endif
            iter++;
            continue;
        }
        ret = lseek64(fd, offset, SEEK_SET);
        if (ret == -1) {
            LOG(ERROR) << "BlockSet::WriteZeroToBlock Fail to seek";
            return -1;
        }
        for (size_t pos = iter->first; pos < iter->second; pos++) {
            if (Utils::WriteFully(fd, buffer.data(), H_BLOCK_SIZE)) {
                continue;
            }
            if (errno == EIO) {
                return 1;
            }
            LOG(ERROR) << "BlockSet::WriteZeroToBlock Write 0 to block error";
            return -1;
        }
        iter++;
    }
    return 0;
}

int32_t BlockSet::WriteDiffToBlock(const Command &cmd, std::vector<uint8_t> &sourceBuffer, uint8_t *patchBuffer,
                                   size_t patchLength, bool isImgDiff)
{
    size_t srcBuffSize =  sourceBuffer.size();
    if (isImgDiff) {
        std::vector<uint8_t> empty;
        UpdatePatch::PatchParam patchParam = {sourceBuffer.data(), srcBuffSize, patchBuffer, patchLength};
        std::unique_ptr<BlockWriter> writer = std::make_unique<BlockWriter>(cmd.GetTargetFileDescriptor(), *this);
        if (writer.get() == nullptr) {
            LOG(ERROR) << "Cannot create block writer, pkgdiff patch abort!";
            return -1;
        }
        int32_t ret = UpdatePatch::UpdateApplyPatch::ApplyImagePatch(patchParam, empty,
            [&](size_t start, const UpdatePatch::BlockBuffer &data, size_t size) -> int {
                return (writer->Write(data.buffer, size, nullptr)) ? 0 : -1;
            }, cmd.GetArgumentByPos(H_DIFF_CMD_ARGS_START + 1));
        writer.reset();
        if (ret != 0) {
            LOG(ERROR) << "Fail to ApplyImagePatch";
            return -1;
        }
    } else {
        LOG(DEBUG) << "Run bsdiff patch.";
        UpdatePatch::PatchBuffer patchInfo = {patchBuffer, 0, patchLength};
        std::unique_ptr<BlockWriter> writer = std::make_unique<BlockWriter>(cmd.GetTargetFileDescriptor(), *this);
        if (writer.get() == nullptr) {
            LOG(ERROR) << "Cannot create block writer, pkgdiff patch abort!";
            return -1;
        }
        auto ret = UpdatePatch::UpdateApplyPatch::ApplyBlockPatch(patchInfo, {sourceBuffer.data(), srcBuffSize},
            [&](size_t start, const UpdatePatch::BlockBuffer &data, size_t size) -> int {
                return (writer->Write(data.buffer, size, nullptr)) ? 0 : -1;
            }, cmd.GetArgumentByPos(H_DIFF_CMD_ARGS_START + 1));
        writer.reset();
        if (ret != 0) {
            LOG(ERROR) << "Fail to ApplyBlockPatch";
            return -1;
        }
    }
    if (fsync(cmd.GetTargetFileDescriptor()) == -1) {
        LOG(ERROR) << "Failed to sync restored data";
        return -1;
    }
    return 0;
}
} // namespace Updater
