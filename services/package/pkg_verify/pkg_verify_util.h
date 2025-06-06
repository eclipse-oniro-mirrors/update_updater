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

#ifndef PKG_VERIFY_UTIL_H
#define PKG_VERIFY_UTIL_H

#include <vector>
#include "pkcs7_signed_data.h"
#include "pkg_stream.h"

namespace Hpackage {
class PkgVerifyUtil {
public:
    PkgVerifyUtil() {}

    ~PkgVerifyUtil() {}

    int32_t VerifyPackageSign(const Hpackage::PkgStreamPtr PkgStream, const std::string &path) const;
    int32_t VerifySign(std::vector<uint8_t> &signData, std::vector<uint8_t> &digest) const;
    int32_t VerifySourceDigest(std::vector<uint8_t> &signature, std::vector<uint8_t> &sourceDigest,
        const std::string &keyPath) const;
    int32_t VerifyAccPackageSign(const PkgStreamPtr pkgStream, const std::string &keyPath) const;
    int32_t GetSignature(const PkgStreamPtr pkgStream, size_t &signatureSize,
        std::vector<uint8_t> &signature, uint16_t &commentTotalLenAll) const;
#ifndef UPDATER_UT
private:
#else
public:
#endif

    int32_t ParsePackage(const PkgStreamPtr pkgStream, size_t &signatureStart,
        size_t &signatureSize, uint16_t &commentTotalLenAll) const;

    int32_t Pkcs7verify(std::vector<uint8_t> &signature, std::vector<uint8_t> &hash) const;

    int32_t HashCheck(const PkgStreamPtr srcData, const size_t dataLen,
        const std::vector<uint8_t> &hash, const std::string &path) const;

    std::string GetPkgTime(const std::string &pkgPath) const;

    void WriteHash(std::vector<uint8_t> &hash, const std::string &pkgPath) const;
};
} // namespace Hpackage
#endif
