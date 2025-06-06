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
#ifndef UPDATER_UTILS_H
#define UPDATER_UTILS_H

#include "utils_fs.h"
#include "utils_common.h"
#include <cerrno>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>

namespace Updater {
enum SlotType {
    NOT_AB,
    SLOT_A,
    SLOT_B,
};
namespace Utils {
constexpr int N_BIN = 2;
constexpr int N_OCT = 8;
constexpr int N_DEC = 10;
constexpr int N_HEX = 16;
constexpr int O_USER_GROUP_ID = 1000;
constexpr int ARGC_TWO_NUMS = 2;
constexpr int USER_ROOT_AUTHORITY = 0;
constexpr int USER_UPDATE_AUTHORITY = 6666;
constexpr int GROUP_SYS_AUTHORITY = 1000;
constexpr int GROUP_UPDATE_AUTHORITY = 6666;
constexpr int GROUP_ROOT_AUTHORITY = 0;
constexpr const char* ON_SERVER = "ON_SERVER";
template<class T>
T String2Int(const std::string &str, int base = N_HEX)
{
    static_assert(std::is_same_v<T, int> || std::is_same_v<T, size_t> || std::is_same_v<T, unsigned long long int>,
                  "type should be int or size_t or unsigned long long int");
    char *end = nullptr;
    if (str.empty()) {
        errno = EINVAL;
        return 0;
    }
    if (((str[0] == '0') && (str[1] == 'x')) || (str[1] == 'X')) {
        base = N_HEX;
    }
    T result = 0;
    if constexpr (std::is_same_v<T, int>) {
        result = strtol(str.c_str(), &end, base);
    } else if constexpr (std::is_same_v<T, size_t> || std::is_same_v<T, unsigned long long int>) {
        result = strtoull(str.c_str(), &end, base);
    } else {
        errno = EINVAL;
    }
    return result;
}
int32_t DeleteFile(const std::string& filename);
std::vector<std::string> SplitString(const std::string &str, const std::string del = " \t");
std::string Trim(const std::string &str);
std::string ConvertSha256Hex(const uint8_t* shaDigest, size_t length);
void UpdaterDoReboot(const std::string& rebootTarget, const std::string &rebootReason,
    const std::string& extData = "");
void DoShutdown(const std::string &shutdownReason);
std::string GetCertName();
bool WriteFully(int fd, const uint8_t *data, size_t size);
bool ReadFully(int fd, void* data, size_t size);
bool ReadFileToString(int fd, std::string &content);
bool ReadStringFromProcFile(const std::string &filePath, std::string &content);
bool CopyFile(const std::string &src, const std::string &dest, bool isAppend = false);
bool CopyDir(const std::string &srcPath, const std::string &dstPath);
bool WriteStringToFile(int fd, const std::string& content);
std::string GetLocalBoardId();
bool CopyUpdaterLogs(const std::string &sLog, const std::string &dLog);
void CompressFiles(std::vector<std::string> &files, const std::string &zipFile);
void CompressLogs(const std::string &name);
bool CheckResultFail();
void WriteDumpResult(const std::string &result, const std::string &fileName);
long long int GetDirSize(const std::string &folderPath);
size_t GetFileSize(const std::string &filePath);
long long int GetDirSizeForFile(const std::string &filePath);
bool DeleteOldFile(const std::string dest);
void SaveLogs();
std::vector<std::string> ParseParams(int argc, char **argv);
bool CheckUpdateMode(const std::string &mode);
std::string DurationToString(std::vector<std::chrono::duration<double>> &durations, std::size_t pkgPosition,
    int precision = 2);
std::string GetRealPath(const std::string &path);
std::string GetPartitionRealPath(const std::string &name);
void SetMessageToMisc(const std::string &miscCmd, const int message, const std::string headInfo);
bool CheckFaultInfo(const std::string &faultInfo);
void SetCmdToMisc(const std::string &miscCmd);
void AddUpdateInfoToMisc(const std::string headInfo, const std::optional<int> message);
void RemoveUpdateInfoFromMisc(const std::string &headInfo);
void SetFaultInfoToMisc(const std::string &faultInfo);
void GetTagValInStr(const std::string& str, const std::string &tag, std::string &val);
bool IsValidHexStr(const std::string &str);
void TrimString (std::string &str);
bool RestoreconPath(const std::string &path);
std::string TrimUpdateMode(const std::string &mode);
bool IsEsDevice();
bool ConvertToUnsignedLongLong(const std::string &str, uint64_t &value);
bool ConvertToLongLong(const std::string &str, int64_t &value);
bool ConvertToLong(const std::string &str, int32_t &value);
bool ConvertToUnsignedLong(const std::string &str, uint32_t &value, int base = 0);
bool ConvertToDouble(const std::string &str, double &value);
bool ConvertToFloat(const std::string &str, float &value);
bool IsVabDevice();
bool SetUpdateSlot(int updateSlot);
int GetUpdateSlot();
#ifndef __WIN32
void SetFileAttributes(const std::string& file, uid_t owner, gid_t group, mode_t mode);
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
int SetParameter(const char *key, const char *value);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
} // Utils
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
void InitLogger(const std::string &tag);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
} // Updater
#endif // UPDATER_UTILS_H
