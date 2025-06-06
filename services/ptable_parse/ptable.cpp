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

#include "ptable.h"

#include <algorithm>
#include <map>
#include <sys/stat.h>

#include "applypatch/data_writer.h"
#include "log/log.h"
#include "securec.h"
#include "utils.h"
#include "fs_manager.h"

namespace Updater {
constexpr const char *PTABLE_CONFIG_PATH = "/etc/ptable_data.json";
constexpr const char *PTABLE_DATA_LABEL = "ptableData";
constexpr const char *EMMC_GPT_DATA_LEN_LABEL = "emmcGptDataLen";
constexpr const char *LBA_LEN_LABEL = "lbaLen";
constexpr const char *GPT_HEADER_LEN_LABEL = "gptHeaderLen";
constexpr const char *BLOCK_SIZE_LABEL = "blockSize";
constexpr const char *IMG_LUN_SIZE_LABEL = "imgLuSize";
constexpr const char *START_LUN_NUM_LABEL = "startLunNumber";
constexpr const char *WRITE_DEVICE_LUN_SIZE_LABEL = "writeDeviceLunSize";
constexpr const char *DEFAULT_LUN_NUM_LABEL = "defaultLunNum";

std::vector<Ptable::PtnInfo> Ptable::GetPtablePartitionInfo() const
{
    return partitionInfo_;
}

bool Ptable::CorrectBufByPtnList(uint8_t *imageBuf, uint64_t imgBufSize, const std::vector<PtnInfo> &srcInfo,
                                 const std::vector<PtnInfo> &dstInfo)
{
    return false;
}

uint32_t Ptable::GetPtablePartitionNum() const
{
    return partitionInfo_.size();
}

bool Ptable::LoadPtnInfo(const std::vector<PtnInfo> &ptnInfo)
{
    if (ptnInfo.empty()) {
        LOG(ERROR) << "ptnInfo is empty";
        return false;
    }
    partitionInfo_ = ptnInfo;
    return true;
}

void Ptable::SetReservedSize(uint64_t reservedSize)
{
    reservedSize_ = reservedSize;
}

std::vector<Ptable::PtnInfo>& Ptable::GetPtablePartitionInfoInstance()
{
    return partitionInfo_;
}

bool Ptable::InitPtable()
{
    if (!partitionInfo_.empty()) {
        std::vector<PtnInfo>().swap(partitionInfo_);
    }
    if (!ParsePtableData()) {
        LOG(ERROR) << "parse PtableData from json file error";
        return false;
    }
    return true;
}

bool Ptable::ParsePtableDataNode(const JsonNode &ptableDataNode)
{
    std::map<std::string, uint32_t*> ptableDataVars = {
        {EMMC_GPT_DATA_LEN_LABEL, &ptableData_.emmcGptDataLen},
        {LBA_LEN_LABEL, &ptableData_.lbaLen},
        {GPT_HEADER_LEN_LABEL, &ptableData_.gptHeaderLen},
        {BLOCK_SIZE_LABEL, &ptableData_.blockSize},
        {IMG_LUN_SIZE_LABEL, &ptableData_.imgLuSize},
        {START_LUN_NUM_LABEL, &ptableData_.startLunNumber},
        {WRITE_DEVICE_LUN_SIZE_LABEL, &ptableData_.writeDeviceLunSize},
        {DEFAULT_LUN_NUM_LABEL, &ptableData_.defaultLunNum},
    };

    for (auto dataVar : ptableDataVars) {
        auto dataValue = ptableDataNode[dataVar.first.c_str()].As<uint32_t>();
        if (!dataValue) {
            LOG(ERROR) << "parse json failed! " << dataVar.first << " is nullptr!";
            return false;
        }
        *(dataVar.second) = *dataValue;
        LOG(INFO) << "set " << dataVar.first << " : " << *dataValue;
    }
    return true;
}

bool Ptable::ParsePtableData()
{
    (void)memset_s(&ptableData_, sizeof(ptableData_), 0, sizeof(ptableData_));
    std::ifstream ifs(std::string {PTABLE_CONFIG_PATH});
    if (!ifs.is_open()) {
        LOG(ERROR) << PTABLE_CONFIG_PATH << " not exist";
        return false;
    }

    // get root node
    std::string content {std::istreambuf_iterator<char> {ifs}, {}};
    cJSONPtr root(cJSON_Parse(content.c_str()), cJSON_Delete);
    if (root == nullptr) {
        LOG(ERROR) << PTABLE_CONFIG_PATH << " contained json invalid";
        return false;
    }

    JsonNode node(root.get(), false);
    const JsonNode &ptableDataNode = node[PTABLE_DATA_LABEL];
    bool ret = ParsePtableDataNode(ptableDataNode);
    ptableData_.dataValid = ret;
    return ret;
}

uint32_t Ptable::GetDefaultImageSize() const
{
    return ptableData_.emmcGptDataLen + ptableData_.defaultLunNum * ptableData_.imgLuSize;
}

bool Ptable::CheckFileExist(const std::string &fileName)
{
    struct stat buffers;
    if (memset_s(&buffers, sizeof(buffers), 0, sizeof(buffers)) != EOK) {
        LOG(WARNING) << "memset_s fail";
    }
    if (stat(fileName.c_str(), &buffers) == 0) {
        LOG(INFO) << fileName << " is exist";
        return true;
    }
    LOG(INFO) << fileName << " is not exist";
    return false;
}

bool Ptable::MemReadWithOffset(const std::string &filePath, const uint64_t offset,
    uint8_t *outData, const uint32_t dataSize)
{
    if (filePath.length() == 0 || outData == nullptr || dataSize == 0) {
        LOG(ERROR) << "invaild input";
        return false;
    }

    std::ifstream fin(filePath, std::ios::in);
    if (fin.fail()) {
        LOG(ERROR) << "open " << filePath << " fail";
        return false;
    }

    fin.seekg(offset, std::ios::beg);
    if (fin.tellg() != static_cast<long long>(offset)) {
        LOG(ERROR) << "seekp 0x" << std::hex << offset << " bytes in " << filePath <<
            " failed. Now is in 0x" << std::hex << fin.tellg() << std::dec;
        fin.close();
        return false;
    }

    if (!fin.read(reinterpret_cast<char *>(outData), dataSize)) {
        LOG(ERROR) << "read 0x" << std::hex << dataSize << " bytes in " << filePath <<
            " failed. only read 0x" << std::hex << fin.gcount() << std::dec;
        fin.close();
        return false;
    }
    fin.close();
    return true;
}

uint32_t Ptable::Reflect(uint32_t data, const uint32_t len)
{
    uint32_t ref = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (data & 0x1) {
            ref |= (1 << ((len - 1) - i));
        }
        data = (data >> 1);
    }
    return ref;
}

uint32_t Ptable::CalculateCrc32(const uint8_t *buffer, const uint32_t len)
{
    if (buffer == nullptr || len == 0) {
        LOG(INFO) << "invaild input";
        return 0;
    }
    const uint32_t byteLen = 8; // 8:length of unit (i.e. byte)
    uint32_t msb;
    const uint64_t polynomial = 0x104C11DB7LL; // IEEE 32bit polynomial
    uint32_t regs = 0xFFFFFFFF; // init to all ones
    const uint32_t regsMask = 0xFFFFFFFF; // ensure only 32 bit answer
    uint32_t regsMsb;
    for (uint32_t i = 0; i < len; i++) {
        uint32_t dataByte = buffer[i];
        dataByte = Reflect(dataByte, 8); // 8:length of unit (i.e. byte)
        for (uint32_t j = 0; j < byteLen; j++) {
            msb = dataByte >> (byteLen - 1); // get MSB
            msb &= 1; // ensure just 1 bit
            regsMsb = (regs >> 31) & 1; // 31:32-1, MSB of regs
            regs = regs << 1; // shift regs for CRC-CCITT
            if (regsMsb ^ msb) { // MSB is a 1
                regs = regs ^ polynomial; // XOR with generator poly
            }
            regs = regs & regsMask; // Mask off excess upper bits
            dataByte <<= 1; // get to next bit
        }
    }
    regs = regs & regsMask;
    uint32_t ret = Reflect(regs, 32) ^ 0xFFFFFFFF; // 32:32bit
    return ret;
}

bool Ptable::VerifyMbrMagicNum(const uint8_t *buffer, const uint32_t size)
{
    // avoid checking past end of buffer
    if (size < (MBR_MAGIC_NUM_POS + 1)) {
        LOG(ERROR) << "size < (TABLE_SIGNATURE + 1)";
        return false;
    }
    // check to see if magic number(0x55AA) exists at pos 0x1FE
    if ((buffer[MBR_MAGIC_NUM_POS] != MBR_MAGIC_NUM_0) ||
        (buffer[MBR_MAGIC_NUM_POS + 1] != MBR_MAGIC_NUM_1)) {
        LOG(ERROR) << "MBR magic number does not match, magic buffer is " << unsigned(*(buffer + MBR_MAGIC_NUM_POS));
        return false;
    }
    return true;
}

bool Ptable::CheckProtectiveMbr(const uint8_t *gptImage, const uint32_t imgLen)
{
    if (!VerifyMbrMagicNum(gptImage, imgLen)) {
        LOG(ERROR) << "MBR magic number verify failed!";
        return false;
    }

    // process each of the four partitions in the MBR, find a Protective MBR(0xEE)
    uint32_t type;
    for (uint32_t i = 0; i < MBR_GPT_MAX_NUM; i++) {
        // type 0xEE indicates the protective MBR and GPT partitions exist
        if (MBR_GPT_ENTRY + i * MBR_GPT_ENTRY_SIZE + GPT_TYPE_SIGN_OFFSET >= imgLen) {
            LOG(INFO) << "not find Protective MBR(type: 0xEE) in this partition";
            return false;
        }
        type = gptImage[MBR_GPT_ENTRY + i * MBR_GPT_ENTRY_SIZE + GPT_TYPE_SIGN_OFFSET];
        if (type == MBR_PROTECTIVE_GPT_TYPE) {
            LOG(INFO) << "type is MBR_PROTECTIVE_GPT_TYPE(0xEE), GPT partitions exist";
            return true;
        }
        LOG(INFO) << "the " << i << " main GPT's type=0x" << std::hex << type << std::dec;
    }
    LOG(INFO) << "not find Protective MBR(type: 0xEE) in this partition";
    return false;
}

bool Ptable::CheckIfValidGpt(const uint8_t *gptImage, const uint32_t gptImageLen)
{
    // 8 is the length of EFI_MAGIC_NUMBER
    if (gptImageLen < 8) {
        LOG(ERROR) << "gptImageLen is less than 8.";
        return false;
    }
    // get magic number
    uint64_t gptMagic = GET_LLWORD_FROM_BYTE(gptImage);
    if (gptMagic != EFI_MAGIC_NUMBER) {
        LOG(ERROR) << "invaild partiton with gptMagic:0x" << std::hex << gptMagic << std::dec;
        return false;
    }
    return true;
}

bool Ptable::GetCapacity(const std::string &filePath, uint64_t &lunCapacity)
{
    if (filePath.empty()) {
        LOG(ERROR) << "filePath is empty or lunCapacity is nullptr";
        return false;
    }
    std::ifstream fin(filePath, std::ios::in);
    if (!fin.is_open()) {
        LOG(ERROR) << "open " << filePath << " fail";
        return false;
    }

    uint64_t sector = 0;
    fin >> sector;
    if (sector == 0) {
        LOG(ERROR) << "read data from " << filePath << " fail";
        fin.close();
        return false;
    }

    uint64_t capacity = sector * SECTOR_SIZE;
    LOG(INFO) << "lun capacity = 0x" << std::hex << capacity << std::dec;
    lunCapacity = capacity;
    fin.close();
    return true;
}

bool Ptable::GetPartitionGptHeaderInfo(const uint8_t *buffer, const uint32_t bufferLen, GPTHeaderInfo& gptHeaderInfo)
{
    if (buffer == nullptr || bufferLen < LBA_LENGTH) {
        LOG(ERROR) << "input invalid";
        return false;
    }

    // Check GPT Signature
    if (!CheckIfValidGpt(buffer, bufferLen)) {
        LOG(ERROR) << "invaild partiton with gptMagic";
        return false;
    }
    gptHeaderInfo.headerSize = GET_LWORD_FROM_BYTE(buffer + HEADER_SIZE_OFFSET);
    gptHeaderInfo.firstUsableLba = GET_LLWORD_FROM_BYTE(buffer + FIRST_USABLE_LBA_OFFSET);
    gptHeaderInfo.maxPartitionCount = GET_LWORD_FROM_BYTE(buffer + PARTITION_COUNT_OFFSET);
    gptHeaderInfo.partitionEntrySize = GET_LWORD_FROM_BYTE(buffer + PENTRY_SIZE_OFFSET);
    if (gptHeaderInfo.maxPartitionCount == 0 || gptHeaderInfo.partitionEntrySize == 0) {
        LOG(ERROR) << "invalid gpt header info";
        return false;
    }
    return true;
}

void Ptable::PatchBackUpGptHeader(uint8_t *gptHeader, const uint32_t len, uint64_t backGptEntryStart)
{
    if (std::max({GPT_HEADER_OFFSET, BACKUP_HEADER_OFFSET, PARTITION_ENTRY_OFFSET}) + sizeof(uint64_t) > len ||
        HEADER_CRC_OFFSET + sizeof(uint32_t) > len) {
        LOG(ERROR) << "input param invalid";
        return;
    }
    uint64_t gptHeaderOffset = GET_LLWORD_FROM_BYTE(gptHeader + GPT_HEADER_OFFSET);
    uint64_t backHeaderOffset = GET_LLWORD_FROM_BYTE(gptHeader + BACKUP_HEADER_OFFSET);
    PUT_LONG_LONG(gptHeader + GPT_HEADER_OFFSET, backHeaderOffset);
    PUT_LONG_LONG(gptHeader + BACKUP_HEADER_OFFSET, gptHeaderOffset);
    PUT_LONG_LONG(gptHeader + PARTITION_ENTRY_OFFSET, backGptEntryStart);
    PUT_LONG(gptHeader + HEADER_CRC_OFFSET, 0);
    uint32_t crcValue = CalculateCrc32(gptHeader, GPT_CRC_LEN);
    PUT_LONG(gptHeader + HEADER_CRC_OFFSET, crcValue);
    LOG(INFO) << "gpt header offset " << gptHeaderOffset << ", back header offset " << backHeaderOffset <<
        ", crc value " << crcValue;
}

bool Ptable::CheckGptHeader(uint8_t *buffer, const uint32_t bufferLen, const uint64_t lbaNum,
    const GPTHeaderInfo& gptHeaderInfo)
{
    if (bufferLen < LBA_LENGTH || lbaNum == 0) {
        LOG(ERROR) << "bufferLen < LBA_LENGTH || lbaNum == 0";
        return false;
    }

    if (gptHeaderInfo.headerSize < GPT_HEADER_SIZE || gptHeaderInfo.headerSize > bufferLen) {
        LOG(ERROR) << "GPT Header size is invaild";
        return false;
    }
    uint32_t orgCrcVal = GET_LWORD_FROM_BYTE(buffer + HEADER_CRC_OFFSET);
    // Write CRC field to 0 before calculating the crc of the whole rest of GPT header
    PUT_LONG(buffer + HEADER_CRC_OFFSET, 0);
    uint32_t crcVal = CalculateCrc32(buffer, gptHeaderInfo.headerSize);
    if (crcVal != orgCrcVal) {
        LOG(ERROR) << "Header crc mismatch crcVal = " << std::hex << crcVal << " with orgCrcVal = " <<
            orgCrcVal << std::dec;
        return false;
    }
    PUT_LONG(buffer + HEADER_CRC_OFFSET, crcVal);

    uint32_t currentLba = GET_LLWORD_FROM_BYTE(buffer + PRIMARY_HEADER_OFFSET);
    uint32_t lastUsableLba = GET_LLWORD_FROM_BYTE(buffer + LAST_USABLE_LBA_OFFSET);
    uint32_t partition0 = GET_LLWORD_FROM_BYTE(buffer + PARTITION_ENTRIES_OFFSET);

    // check for first and last lba range
    if (gptHeaderInfo.firstUsableLba > lbaNum || lastUsableLba > lbaNum) {
        LOG(ERROR) << "invalid usable lba " << gptHeaderInfo.firstUsableLba << ", last is " << lastUsableLba <<
            " lbaNum is " << lbaNum;
        return false;
    }
    // check for partition entry size
    if (gptHeaderInfo.partitionEntrySize != PARTITION_ENTRY_SIZE ||
        gptHeaderInfo.maxPartitionCount > (MIN_PARTITION_ARRAY_SIZE / PARTITION_ENTRY_SIZE)) {
        LOG(ERROR) << "invalid parition entry size or max count";
        return false;
    }
    // GPT header should always be the 0x1 LBA, partition entry should always the 0x2 LBA
    if (currentLba != 0x1 || partition0 != 0x2) {
        LOG(ERROR) << "starting LBA mismatch";
        return false;
    }
    LOG(INFO) << "gpt header check ok";
    return true;
}

bool Ptable::PartitionCheckGptHeader(const uint8_t *gptImage, const uint32_t len, const uint64_t lbaNum,
    const uint32_t blockSize, GPTHeaderInfo& gptHeaderInfo)
{
    if (len < ptableData_.writeDeviceLunSize || lbaNum == 0) {
        LOG(ERROR) << "len" << len << "ptableData_.writeDeviceLunSize" << ptableData_.writeDeviceLunSize
          << "lbaNum" << lbaNum;
        return false;
    }

    uint8_t *buffer = new(std::nothrow) uint8_t[blockSize]();
    if (buffer == nullptr) {
        LOG(ERROR) << "new buffer failed!";
        return false;
    }
    if (memcpy_s(buffer, blockSize, gptImage + blockSize, blockSize) != EOK) {
        LOG(ERROR) << "copy gpt header fail";
        delete [] buffer;
        return false;
    }

    if (!CheckGptHeader(buffer, blockSize, lbaNum, gptHeaderInfo)) {
        LOG(ERROR) << "CheckGptHeader fail";
        delete [] buffer;
        return false;
    }

    uint32_t partition0 = GET_LLWORD_FROM_BYTE(&buffer[PARTITION_ENTRIES_OFFSET]);
    uint32_t orgCrcVal = GET_LWORD_FROM_BYTE(&buffer[PARTITION_CRC_OFFSET]);
    delete [] buffer;

    uint32_t crcVal = CalculateCrc32(gptImage + partition0 * blockSize,
        gptHeaderInfo.maxPartitionCount * gptHeaderInfo.partitionEntrySize);
    if (crcVal != orgCrcVal) {
        LOG(ERROR) << "partition entires crc mismatch crcVal =" << std::hex << crcVal << " with orgCrcVal =" <<
            orgCrcVal << std::dec;
        return false;
    }
    LOG(INFO) << "PartitionCheckGptHeader ok";
    return true;
}

void Ptable::PrintPtableInfo() const
{
    if (partitionInfo_.empty()) {
        LOG(ERROR) << "ptable vector is empty!";
        return;
    }

    LOG(INFO) << "ptnInfo : ===========================================";
    LOG(INFO) << "partition count = " << std::dec << partitionInfo_.size();
    for (size_t i = 0; i < partitionInfo_.size(); i++) {
        LOG(INFO) << "ptable.entry[" << i << "].name=" << partitionInfo_[i].dispName.c_str() <<
            ", startAddr=0x" << std::hex << partitionInfo_[i].startAddr << ", size=0x" <<
            partitionInfo_[i].partitionSize << ", lun=" << std::dec << partitionInfo_[i].lun
            << ", partType=" << partitionInfo_[i].partType;
    }
    LOG(INFO) << "ptnInfo : ===========================================";
}

void Ptable::PrintPtableInfo(const std::vector<PtnInfo> &ptnInfo) const
{
    if (ptnInfo.empty()) {
        LOG(ERROR) << "ptable vector is empty!";
        return;
    }

    LOG(INFO) << "ptnInfo : ===========================================";
    LOG(INFO) << "partition count = " << std::dec << ptnInfo.size();
    for (size_t i = 0; i < ptnInfo.size(); i++) {
        LOG(INFO) << "ptable.entry[" << i << "].name=" << ptnInfo[i].dispName.c_str() << ", startAddr=0x" <<
        std::hex << ptnInfo[i].startAddr << ", size=0x" << ptnInfo[i].partitionSize << ", lun=" <<
        std::dec << ptnInfo[i].lun << ", partType=" << ptnInfo[i].partType;
    }
    LOG(INFO) << "ptnInfo : ===========================================";
}

void Ptable::SetPartitionType(const std::string &partName, PtnInfo &ptnInfo)
{
    if (PARTITION_AB_SUFFIX_SIZE > partName.size()) {
        ptnInfo.partType = PARTITION_NORMAL_TYPE;
        return;
    }
    std::string partSuffix = partName.substr(partName.size() - PARTITION_AB_SUFFIX_SIZE,
        PARTITION_AB_SUFFIX_SIZE);
    if (strcasecmp(partSuffix.c_str(), PARTITION_A_SUFFIX) == 0 ||
        strcasecmp(partSuffix.c_str(), PARTITION_B_SUFFIX) == 0) {
        ptnInfo.partType = PARTITION_AB_TYPE;
        return;
    }
    ptnInfo.partType = PARTITION_NORMAL_TYPE;
}

void Ptable::ParsePartitionName(const uint8_t *data, const uint32_t dataLen,
    std::string &name, const uint32_t nameLen)
{
    if (data == nullptr || dataLen == 0 || nameLen == 0) {
        LOG(ERROR) << "dataLen == 0 || nameLen == 0";
        return;
    }
    char utf16Name[MAX_GPT_NAME_SIZE] = {0};
    if (memcpy_s(utf16Name, sizeof(utf16Name), data, dataLen) != EOK) {
        LOG(ERROR) << "memcpy name fail";
        return;
    }

    std::string outName;
    // convert utf8 to utf16, 2 bytes for 1 charactor of partition name
    for (uint32_t n = 0; n < nameLen && n < (MAX_GPT_NAME_SIZE / 2) && utf16Name[n * 2] != '\0'; n++) {
        outName = outName + utf16Name[n * 2];
    }
    for (uint32_t i = 0; i < outName.size(); i++) {
        outName[i] = static_cast<char>(toupper(outName[i]));
    }
    name = outName;
    return;
}

bool Ptable::WriteBufferToPath(const std::string &path, const uint64_t offset,
    const uint8_t *buffer, const uint32_t size)
{
    std::unique_ptr<DataWriter> writer = DataWriter::CreateDataWriter(WRITE_RAW, path, offset);
    if (writer == nullptr) {
        LOG(ERROR) << "create writer class failed!";
        return false;
    }
    bool ret = writer->Write(buffer, size, nullptr);
    if (!ret) {
        LOG(ERROR) << "writer to " << path << " with offset " << offset << " failed ";
        DataWriter::ReleaseDataWriter(writer);
        return false;
    }
    DataWriter::ReleaseDataWriter(writer);
    return true;
}

bool Ptable::GetPartionInfoByName(const std::string &partitionName, PtnInfo &ptnInfo, int32_t &index)
{
    if (partitionInfo_.empty()) {
        LOG(ERROR) << "get partition failed! partitionInfo_ is empty";
        return false;
    }
    auto findPart = [&ptnInfo, &index, this] (const std::string &partitionName) {
        for (int32_t i = 0; i < static_cast<int32_t>(partitionInfo_.size()); i++) {
            if (partitionInfo_[i].dispName.size() == partitionName.size() &&
                strcasecmp(partitionInfo_[i].dispName.c_str(), partitionName.c_str()) == 0) {
                index = i;
                ptnInfo = partitionInfo_[i];
                return true;
            }
        }
        return false;
    };
    if (findPart(partitionName)) {
        LOG(INFO) << "find partition name " << partitionName;
        return true;
    }
    std::string partitionNameAB = partitionName;
    int updateSlot = Utils::GetUpdateSlot();
    if (updateSlot < 1 || updateSlot > 2) { // 2 : slot b
        LOG(ERROR) << "get update slot fail";
        return false;
    }
    partitionNameAB += (updateSlot == SLOT_A ? PARTITION_A_SUFFIX : PARTITION_B_SUFFIX);
    if (findPart(partitionNameAB)) {
        LOG(INFO) << "find partitionAB name " << partitionNameAB;
        return true;
    }
    LOG(ERROR) << "get partition info failed! Not found partition:" << partitionName;
    return false;
}

bool Ptable::AdjustGpt(uint8_t *ptnInfoBuf, uint64_t bufSize, const std::string &ptnName, uint64_t preLastLBA,
    uint64_t lastPtnLastLBA)
{
    if (ptnInfoBuf == nullptr || bufSize == 0 || ptnName.empty()) {
        LOG(ERROR) << "invalid input";
        return false;
    }
    if (ptnName != LAST_PATITION_NAME) {
        uint64_t firstLBA = GET_LLWORD_FROM_BYTE(&ptnInfoBuf[FIRST_LBA_OFFSET]);
        uint64_t lastLBA = GET_LLWORD_FROM_BYTE(&ptnInfoBuf[LAST_LBA_OFFSET]);
        lastLBA = lastLBA - firstLBA + preLastLBA + 1;
        firstLBA = preLastLBA + 1;
        PUT_LONG_LONG(ptnInfoBuf + FIRST_LBA_OFFSET, firstLBA);
        PUT_LONG_LONG(ptnInfoBuf + LAST_LBA_OFFSET, lastLBA);
    } else { /* this is USERDATA partition */
        uint64_t firstLBA = preLastLBA + 1;
        if (lastPtnLastLBA < firstLBA) {
            LOG(ERROR) << "patch last partition fail";
            return false;
        }
        PUT_LONG_LONG(ptnInfoBuf + FIRST_LBA_OFFSET, firstLBA);
        /* resize last partition by device density */
        PUT_LONG_LONG(ptnInfoBuf + LAST_LBA_OFFSET, lastPtnLastLBA);
    }
    return true;
}

bool Ptable::ChangeGpt(uint8_t *gptBuf, uint64_t gptSize, GptParseInfo gptInfo, PtnInfo &modifyInfo)
{
    if (gptBuf == nullptr || gptSize == 0 || gptSize <= gptInfo.imgBlockSize || gptInfo.devBlockSize == 0) {
        LOG(ERROR) << "input param invalid";
        return false;
    }
    bool modifyDectect = false;
    uint8_t *gptHead = gptBuf + gptInfo.imgBlockSize; // skip pmbr
    uint32_t ptnEntrySize = GET_LLWORD_FROM_BYTE(&gptHead[PENTRY_SIZE_OFFSET]);
    uint64_t ptnStart = GET_LLWORD_FROM_BYTE(&gptHead[PARTITION_ENTRIES_OFFSET]);
    uint64_t readSize = ptnStart * gptInfo.imgBlockSize;
    uint8_t *ptnInfoBuf = gptBuf + readSize;
    uint64_t preLastLBA = 0;
    uint64_t lastPtnLastLBA = gptInfo.devDensity / gptInfo.devBlockSize - 1;

    while (readSize < gptSize) {
        std::string dispName;
        // convert utf8 to utf16, 2 bytes for 1 charactor of partition name
        ParsePartitionName(&ptnInfoBuf[GPT_PARTITION_NAME_OFFSET], MAX_GPT_NAME_SIZE, dispName, MAX_GPT_NAME_SIZE / 2);
        if (dispName.empty()) {
            break;
        }
        if (modifyDectect) {
            /* partition after modify part */
            if (!AdjustGpt(ptnInfoBuf, gptSize - readSize, dispName, preLastLBA, lastPtnLastLBA)) {
                return false;
            }
            preLastLBA = GET_LLWORD_FROM_BYTE(&ptnInfoBuf[LAST_LBA_OFFSET]);
            ptnInfoBuf += ptnEntrySize;
            readSize += static_cast<uint64_t>(ptnEntrySize);
            continue;
        }
        if (dispName == modifyInfo.dispName) {
            LOG(INFO) << "modify part dectected!! dispName = " << dispName;
            uint64_t firstLBA = modifyInfo.startAddr / gptInfo.devBlockSize;
            uint64_t lastLBA = firstLBA + modifyInfo.partitionSize / gptInfo.devBlockSize - 1;
            if ((dispName == LAST_PATITION_NAME) && (lastLBA != lastPtnLastLBA)) {
                return false;
            }
            PUT_LONG_LONG(ptnInfoBuf + FIRST_LBA_OFFSET, firstLBA);
            PUT_LONG_LONG(ptnInfoBuf + LAST_LBA_OFFSET, lastLBA);
            modifyDectect = true;
            preLastLBA = lastLBA;
        }
        ptnInfoBuf += ptnEntrySize;
        readSize += static_cast<uint64_t>(ptnEntrySize);
    }
    return true;
}

bool Ptable::WritePartitionBufToFile(uint8_t *ptbImgBuffer, const uint32_t imgBufSize)
{
    if (ptbImgBuffer == nullptr || imgBufSize == 0) {
        LOG(ERROR) << "Invalid param ";
        return false;
    }
    std::ofstream ptbFile(PTABLE_TEMP_PATH, std::ios::ate | std::ios::binary);
    if (ptbFile.fail()) {
        LOG(ERROR) << "Failed to open " << PTABLE_TEMP_PATH << strerror(errno);
        return false;
    }
    ptbFile.write(reinterpret_cast<const char*>(ptbImgBuffer), imgBufSize);
    if (ptbFile.bad()) {
        LOG(ERROR) << "Failed to write ptable tmp file " << imgBufSize << strerror(errno);
        return false;
    }
    ptbFile.flush();
    sync();
    return true;
}

bool Ptable::ReadPartitionFileToBuffer(uint8_t *ptbImgBuffer, uint32_t &imgBufSize)
{
    if (ptbImgBuffer == nullptr || imgBufSize == 0) {
        LOG(ERROR) << "Invalid param ";
        return false;
    }

    std::ifstream ptbFile(PTABLE_TEMP_PATH, std::ios::in | std::ios::binary);
    if (!ptbFile.is_open()) {
        LOG(ERROR) << "open " << PTABLE_TEMP_PATH << " failed " << strerror(errno);
        return false;
    }

    ptbFile.seekg(0, std::ios::end);
    uint32_t fileSize = static_cast<uint32_t>(ptbFile.tellg());
    if (ptbFile.tellg() <= 0 || fileSize > imgBufSize) {
        LOG(ERROR) << "ptbFile is error";
        return false;
    }

    ptbFile.seekg(0, std::ios::beg);
    if (!ptbFile.read(reinterpret_cast<char*>(ptbImgBuffer), fileSize)) {
        LOG(ERROR) << "read ptable file to buffer failed " << fileSize << strerror(errno);
        return false;
    }
    imgBufSize = fileSize;
    return true;
}

void Ptable::DeletePartitionTmpFile()
{
    if (Utils::DeleteFile(PTABLE_TEMP_PATH) != 0) {
        LOG(ERROR) << "delete ptable tmp file fail " << PTABLE_TEMP_PATH << strerror(errno);
        return;
    }
    LOG(INFO) << "delete ptable tmp file success " << PTABLE_TEMP_PATH;
}
} // namespace Updater