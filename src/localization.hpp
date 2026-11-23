#pragma once

#include "state.hpp"

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace m1arc {
struct ModView;

struct LanguageOption {
    std::string code;
    std::string name;
};

// Native JSON translations. Does not alter the registry's saved value IDs.
class Localization {
public:
    void Configure(const std::filesystem::path& pluginDirectory, StateStore& store);
    [[nodiscard]] std::vector<LanguageOption> Languages() const;
    [[nodiscard]] std::string Current() const;
    [[nodiscard]] bool Select(std::string_view code);
    [[nodiscard]] std::string Text(std::string_view key, std::string_view fallback) const;
    void Localize(ModView& mod) const;
    [[nodiscard]] std::string Choice(std::string_view modId,
                                     std::string_view settingId,
                                     std::string_view original) const;
private:
    struct Language {
        LanguageOption option;
        nlohmann::json strings;
    };
    [[nodiscard]] const nlohmann::json& ModDictionaryLocked(std::string_view id) const;
    std::filesystem::path pluginDirectory_;
    StateStore* store_ = nullptr;
    std::vector<Language> languages_;
    std::string current_ = "en_US";
    mutable std::unordered_map<std::string, nlohmann::json> modCache_;
    mutable std::mutex mutex_;
};

Localization& Translations();
} // namespace m1arc
