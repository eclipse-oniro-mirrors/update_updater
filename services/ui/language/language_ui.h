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
#ifndef UPDATE_UI_LANGUAGE_UI_H
#define UPDATE_UI_LANGUAGE_UI_H

#ifdef UPDATER_UI_SUPPORT
#include <string>
#include <unordered_map>
#include <vector>
#include "json_visitor.h"
#include "language_ui_msg_id.h"
#include "language/language_ui.h"
#include "updater_ui_traits.h"

namespace Updater {
namespace Lang {
enum class Language {
    CHINESE = 0,
    ENGLISH = 1,
    SPANISH = 2
};

class LanguageUI final {
    DISALLOW_COPY_MOVE(LanguageUI);
public:
    static LanguageUI &GetInstance();
    [[nodiscard]] bool Init(Language language);
    const std::string &Translate(const std::string &key) const;
    [[nodiscard]] bool LoadLangRes(const JsonNode &node);
    Language ParseLanguage() const;
    Language GetCurLanguage() const;
    void SetDefaultLanguage(Language language);
    Language ParseLangIfZh(const std::string &globalLang) const;
private:
    ~LanguageUI() = default;
    LanguageUI();
    bool Parse();
    bool SetRes(const Res &res);
    bool CheckLevel(int level);
    bool ParseJson(const std::string &file);
    std::unordered_map<std::string, std::string> strMap_;
    std::vector<std::string> res_;
    LangResource langRes_;
    Language language_;
    Language defaultLanguage_ = Language::ENGLISH;
    static constexpr auto LANG_RES_KEY = "locale";
    const static std::unordered_map<std::string, Language> LOCALES;
};
}

#define TR(tag) Lang::LanguageUI::GetInstance().Translate(STRINGFY(tag))
} // namespace Updater
#else
#define TR(tag) ""
#endif
#endif /* UPDATE_UI_HOS_UPDATER_H */
