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

#ifndef UPDATER_DATA_WRITER_H
#define UPDATER_DATA_WRITER_H

#include <cstdio>
#include <memory>
#include <string>
#include <unistd.h>
#include "updater_env.h"

namespace Updater {
enum WriteMode : int {
    WRITE_RAW = 1,
    WRITE_DECRYPT = 2,
    WRITE_BLOCK = 3,
};

class DataWriter {
public:
    using DataWriterPtr = DataWriter *;
    using WriterConstructor = std::unique_ptr<DataWriter> (*)(const std::string &, const std::string &,
        uint64_t, uint64_t);
    virtual bool Write(const uint8_t *addr, size_t len, const void *context) = 0;
    virtual bool Sync(void);
    virtual ~DataWriter() {}
    static std::unique_ptr<DataWriter> CreateDataWriter(WriteMode mode, const std::string &path, UpdaterEnv *env,
        uint64_t offset = 0);
    static std::unique_ptr<DataWriter> CreateDataWriter(WriteMode mode, const std::string &path, uint64_t offset = 0);
    static std::unique_ptr<DataWriter> CreateDataWriter(const std::string &mode, const std::string &path,
        const std::string &partName = {}, uint64_t startAddr = 0, uint64_t offset = 0);
    static UpdaterEnv *GetUpdaterEnv();
    void SetUpdaterEnv(UpdaterEnv *env);
    static void ReleaseDataWriter(std::unique_ptr<DataWriter> &writer);
    static void RegisterDataWriter(const std::string &mode, WriterConstructor constructor);

protected:
    virtual int OpenPath(const std::string &path);

private:
    static UpdaterEnv *env_;
    static inline std::unordered_map<std::string, WriterConstructor> constructorMap_;
};

// Maybe we should read sector size from flash.
#define DEFAULT_SECTOR_SIZE (512)
} // Updater
#endif // UPDATER_DATA_WRITER_H
