#include "localization.hpp"
#include "registry.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>
#include <utility>

namespace m1arc {
namespace {
using json = nlohmann::json;
constexpr std::uintmax_t kMaxTranslationBytes = 256 * 1024;

bool SafeLocale(std::string_view value) {
    if (value.size() < 2 || value.size() > 16) return false;
    if (!std::isalpha(static_cast<unsigned char>(value.front()))) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '-';
    });
}

bool SafeModId(std::string_view value) {
    if (value.empty() || value.size() > 96) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
    });
}

json ReadTranslation(const std::filesystem::path& path) {
    std::error_code ec;
    if (std::filesystem::is_symlink(path, ec) || ec) return json::object();
    if (!std::filesystem::is_regular_file(path, ec) || ec) return json::object();
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || !size || size > kMaxTranslationBytes) return json::object();
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return json::object();
    try {
        json doc = json::parse(stream);
        return doc.is_object() ? doc : json::object();
    } catch (...) {
        return json::object();
    }
}

std::string Field(const json& doc, const char* field, const std::string& fallback) {
    if (doc.is_object()) {
        const auto it = doc.find(field);
        if (it != doc.end() && it->is_string()) {
            const auto value = it->get<std::string>();
            if (!value.empty() && value.size() <= 1024) return value;
        }
    }
    return fallback;
}

const json& Object(const json& parent, const std::string& key) {
    static const json empty = json::object();
    if (!parent.is_object()) return empty;
    const auto it = parent.find(key);
    return it != parent.end() && it->is_object() ? *it : empty;
}
} // namespace

Localization& Translations() {
    static Localization instance;
    return instance;
}

void Localization::Configure(const std::filesystem::path& directory, StateStore& store) {
    std::vector<Language> loaded;
    const auto root = directory / "locales";
    std::error_code ec;
    if (std::filesystem::is_directory(root, ec) && !ec) {
        for (std::filesystem::directory_iterator it(root, ec), end; !ec && it != end;
             it.increment(ec)) {
            if (loaded.size() >= 64) break;
            const auto path = it->path();
            if (path.extension() != ".json") continue;
            const std::string code = path.stem().string();
            if (!SafeLocale(code)) continue;
            const json doc = ReadTranslation(path);
            if (!doc.contains("strings") || !doc["strings"].is_object()) continue;
            loaded.push_back({{code, Field(doc, "name", code)}, doc["strings"]});
        }
    }
    if (std::none_of(loaded.begin(), loaded.end(), [](const auto& lang) {
            return lang.option.code == "en_US";
        })) {
        loaded.push_back({{"en_US", "English"}, json::object()});
    }
    std::sort(loaded.begin(), loaded.end(), [](const auto& a, const auto& b) {
        return a.option.code < b.option.code;
    });

    std::string saved = "en_US";
    try {
        const json config = json::parse(store.Snapshot());
        if (config.contains("overlay") && config["overlay"].is_object())
            saved = config["overlay"].value("language", std::string("en_US"));
    } catch (...) {}
    if (!SafeLocale(saved) ||
        std::none_of(loaded.begin(), loaded.end(), [&](const auto& lang) {
            return lang.option.code == saved;
        })) saved = "en_US";

    std::lock_guard lock(mutex_);
    pluginDirectory_ = directory;
    store_ = &store;
    languages_ = std::move(loaded);
    current_ = std::move(saved);
    modCache_.clear();
}

std::vector<LanguageOption> Localization::Languages() const {
    std::lock_guard lock(mutex_);
    std::vector<LanguageOption> result;
    for (const auto& lang : languages_) result.push_back(lang.option);
    return result;
}

std::string Localization::Current() const {
    std::lock_guard lock(mutex_);
    return current_;
}

bool Localization::Select(std::string_view code) {
    if (!SafeLocale(code)) return false;
    std::lock_guard lock(mutex_);
    if (std::none_of(languages_.begin(), languages_.end(), [&](const auto& lang) {
            return lang.option.code == code;
        })) return false;
    if (current_ == code) return true;
    if (!store_ || !store_->Update([&](json& document) {
            document["overlay"]["language"] = std::string(code);
        })) return false;
    current_ = std::string(code);
    modCache_.clear();
    return true;
}

std::string Localization::Text(std::string_view key, std::string_view fallback) const {
    std::lock_guard lock(mutex_);
    for (const auto& lang : languages_) {
        if (lang.option.code != current_) continue;
        const auto it = lang.strings.find(std::string(key));
        if (it != lang.strings.end() && it->is_string()) {
            const auto value = it->get<std::string>();
            if (!value.empty() && value.size() <= 1024) return value;
        }
    }
    return std::string(fallback);
}

const json& Localization::ModDictionaryLocked(std::string_view id) const {
    static const json empty = json::object();
    if (!SafeModId(id)) return empty;
    const std::string name(id);
    if (const auto found = modCache_.find(name); found != modCache_.end())
        return found->second;
    const auto file = pluginDirectory_ / "mods" / name / "language" /
                      (current_ + ".json");
    auto [it, inserted] = modCache_.emplace(name, ReadTranslation(file));
    (void)inserted;
    return it->second;
}

void Localization::Localize(ModView& mod) const {
    std::lock_guard lock(mutex_);
    const json& doc = ModDictionaryLocked(mod.id);
    mod.title = Field(doc, "name", mod.title);
    mod.shortDesc = Field(doc, "shortDesc", mod.shortDesc);
    const json& definitions = Object(doc, "settings");
    for (auto& setting : mod.settings) {
        const json& localized = Object(definitions, setting.id);
        setting.label = Field(localized, "label", setting.label);
        setting.description = Field(localized, "description", setting.description);
        setting.group = Field(localized, "group", setting.group);
        setting.buttonLabel = Field(localized, "buttonLabel", setting.buttonLabel);
        // Intentionally keep original choices/defaults/values intact.
    }
}

std::string Localization::Choice(std::string_view modId, std::string_view settingId,
                                 std::string_view original) const {
    std::lock_guard lock(mutex_);
    const json& doc = ModDictionaryLocked(modId);
    const json& settings = Object(doc, "settings");
    const json& entry = Object(settings, std::string(settingId));
    const json& choices = Object(entry, "choices");
    const auto it = choices.find(std::string(original));
    if (it != choices.end() && it->is_string()) {
        const auto value = it->get<std::string>();
        if (!value.empty() && value.size() <= 1024) return value;
    }
    return std::string(original);
}
} // namespace m1arc
