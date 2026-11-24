#include "registry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>
#include <ctime>

namespace m1arc {
namespace {
using json = nlohmann::json;

std::optional<SettingKind> ParseKind(const std::string& name)
{
    if (name == "boolean") return SettingKind::Boolean;
    if (name == "integer") return SettingKind::Integer;
    if (name == "float") return SettingKind::Float;
    if (name == "choice") return SettingKind::Choice;
    if (name == "action") return SettingKind::Action;
    if (name == "hotkey") return SettingKind::Hotkey;
    return std::nullopt;
}

bool ValidHotkey(int vk) noexcept
{
    // Win32 VK_* values; keep this cross-platform for native CTest.
    if (vk == 0) return true; // unassigned
    if (vk < 0x08 || vk >= 0xE7) return false;
    switch (vk) {
    case 0x1B: // Escape
    case 0x10: case 0x11: case 0x12: // generic modifiers
    case 0xA0: case 0xA1: case 0xA2: case 0xA3:
    case 0xA4: case 0xA5: // left and right modifiers
    case 0x5B: case 0x5C: // Windows keys
    case 0x5F: case 0xE5: // sleep / process
        return false;
    default: return true;
    }
}

json ReadSaved(const StateStore& store)
{
    try { return json::parse(store.Snapshot()); }
    catch (...) { return json::object(); }
}
}

std::optional<LogLevel> ParseLogLevel(std::string_view level) noexcept
{
    if (level=="DEBUG" || level=="debug") return LogLevel::Debug;
    if (level=="INFO" || level=="info") return LogLevel::Info;
    if (level=="WARNING" || level=="warning" || level=="WARN" || level=="warn") return LogLevel::Warning;
    if (level=="ERROR" || level=="error") return LogLevel::Error;
    return std::nullopt;
}

const char* LogLevelName(LogLevel level) noexcept
{
    switch(level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

void Registry::AppendLogLocked(std::string_view mod,LogLevel level,std::string_view message)
{
    if (message.empty() || message.size()>kLogMessageBytes || message.find('\0')!=std::string_view::npos)
        return;
    // Never allow terminal/control characters other than newline and tab.
    std::string clean(message);
    for (char& c:clean) {
        const auto byte=static_cast<unsigned char>(c);
        if (byte<0x20 && c!='\n' && c!='\t') c=' ';
    }
    const auto now=std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local,&now);
#else
    localtime_r(&now,&local);
#endif
    char timestamp[32]{};
    std::strftime(timestamp,sizeof(timestamp),"%Y-%m-%d %H:%M:%S",&local);
    const auto count=std::count_if(logs_.begin(),logs_.end(),[&](const auto& row){return row.modId==mod;});
    if (count>=kLogsPerMod) {
        const auto oldest=std::find_if(logs_.begin(),logs_.end(),[&](const auto& row){return row.modId==mod;});
        if (oldest!=logs_.end()) logs_.erase(oldest);
    }
    if (logs_.size()>=kLogsTotal) logs_.pop_front();
    logs_.push_back({std::string(mod),timestamp,level,std::move(clean)});
}

bool Registry::AppendLog(std::string_view mod,LogLevel level,std::string_view message)
{
    if (message.empty() || message.size()>kLogMessageBytes ||
        message.find('\0')!=std::string_view::npos) return false;
    std::lock_guard guard(mutex_);
    if (!mods_.contains(std::string(mod))) return false;
    AppendLogLocked(mod,level,message);
    return true;
}

std::vector<LogEntry> Registry::Logs(std::string_view mod) const
{
    std::lock_guard guard(mutex_);
    std::vector<LogEntry> result;
    for (auto it=logs_.rbegin();it!=logs_.rend();++it)
        if (it->modId==mod) result.push_back(*it);
    return result;
}

bool Registry::ClearLogs(std::string_view mod)
{
    std::lock_guard guard(mutex_);
    if (!mods_.contains(std::string(mod))) return false;
    std::erase_if(logs_,[&](const auto& row){return row.modId==mod;});
    return true;
}

bool ValidModScope(std::string_view scope) noexcept
{
    constexpr std::array<std::string_view, 9> scopes{{
        "Gameplay", "Crafting", "Cheats", "Weapons", "Locations",
        "Appearance", "Characters", "Miscellaneous", "UI/UX"
    }};
    return std::find(scopes.begin(),scopes.end(),scope)!=scopes.end();
}

Registry& Settings()
{
    static Registry registry(Store());
    return registry;
}

bool Registry::ValidId(std::string_view value)
{
    if (value.empty() || value.size() > 96) return false;
    return std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    });
}

std::optional<nlohmann::json> Registry::Normalize(
    const SettingView& setting, const nlohmann::json& input)
{
    switch (setting.kind) {
    case SettingKind::Action:
        return std::nullopt; // Buttons are ephemeral events, not setting values.
    case SettingKind::Hotkey:
        if (!input.is_number_integer() && !input.is_number_unsigned()) return std::nullopt;
        if (input.get<double>() > 255.0 || input.get<double>() < 0.0) return std::nullopt;
        if (!ValidHotkey(input.get<int>())) return std::nullopt;
        return input.get<int>();
    case SettingKind::Boolean:
        if (!input.is_boolean()) return std::nullopt;
        return input;
    case SettingKind::Integer:
        if (!input.is_number_integer() && !input.is_number_unsigned())
            return std::nullopt;
        {
            const auto value = input.get<double>();
            if (!std::isfinite(value)) return std::nullopt;
            const double clamped = std::clamp(value, setting.minimum, setting.maximum);
            if (clamped < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
                clamped > static_cast<double>(std::numeric_limits<std::int32_t>::max()))
                return std::nullopt;
            return static_cast<std::int32_t>(clamped);
        }
    case SettingKind::Float:
        if (!input.is_number()) return std::nullopt;
        {
            const double value = input.get<double>();
            if (!std::isfinite(value)) return std::nullopt;
            return std::clamp(value, setting.minimum, setting.maximum);
        }
    case SettingKind::Choice:
        if (!input.is_string()) return std::nullopt;
        {
            const auto value = input.get<std::string>();
            if (std::find(setting.choices.begin(), setting.choices.end(), value) ==
                setting.choices.end()) return std::nullopt;
            return value;
        }
    }
    return std::nullopt;
}

bool Registry::Register(std::string id, std::string title, const nlohmann::json& definitions,
                        std::string scope, std::string shortDesc)
{
    // Metadata is visual only. Preserve the old 3-parameter REDscript API.
    if (!ValidId(id) || title.empty() || title.size() > 150 ||
        !ValidModScope(scope) || shortDesc.size() > 180 ||
        std::any_of(shortDesc.begin(),shortDesc.end(),[](unsigned char c) {
            return c < 0x20u;
        }) ||
        !definitions.is_array() || definitions.empty() || definitions.size() > 250)
        return false;

    ModView prepared{.id = id, .title = std::move(title),
                     .scope = std::move(scope), .shortDesc = std::move(shortDesc)};
    const json saved = ReadSaved(store_);
    for (const auto& item : definitions) {
        if (!item.is_object()) return false;
        if (!item.contains("id") || !item["id"].is_string() ||
            !item.contains("kind") || !item["kind"].is_string() ||
            (item["kind"] != "action" && !item.contains("default"))) return false;

        SettingView row;
        row.id = item["id"].get<std::string>();
        if (!ValidId(row.id)) return false;
        if (std::any_of(prepared.settings.begin(), prepared.settings.end(),
                [&](const SettingView& old) { return old.id == row.id; }))
            return false;

        const auto kind = ParseKind(item["kind"].get<std::string>());
        if (!kind) return false;
        row.kind = *kind;
        row.label = item.value("label", row.id);
        row.description = item.value("description", std::string{});
        row.group = item.value("group", std::string("General"));
        if (row.label.empty() || row.group.empty()) return false;
        if (row.label.size() > 140 || row.description.size() > 500 ||
            row.group.size() > 96) return false;

        if (item.contains("presentation")) {
            if (row.kind != SettingKind::Boolean || !item["presentation"].is_string())
                return false;
            const std::string style = item["presentation"].get<std::string>();
            if (style != "switch" && style != "checkbox") return false;
            row.checkbox = style == "checkbox";
        }
        if (item.contains("button")) {
            // A data-only inline action ID, never a console command.
            if ((row.kind != SettingKind::Integer && row.kind != SettingKind::Float) ||
                !item["button"].is_object()) return false;
            const auto& button = item["button"];
            if (!button.contains("id") || !button["id"].is_string() ||
                !button.contains("label") || !button["label"].is_string()) return false;
            row.buttonId = button["id"].get<std::string>();
            row.buttonLabel = button["label"].get<std::string>();
            if (!ValidId(row.buttonId) || row.buttonLabel.empty() ||
                row.buttonLabel.size() > 100) return false;
        }
        if (row.kind == SettingKind::Action) {
            if (item.contains("default") || item.contains("choices") ||
                item.contains("min") || item.contains("max") || item.contains("step"))
                return false;
            if (item.contains("buttonLabel") && !item["buttonLabel"].is_string())
                return false;
            row.buttonLabel = item.value("buttonLabel", row.label);
            if (row.buttonLabel.empty() || row.buttonLabel.size() > 100)
                return false;
            prepared.settings.push_back(std::move(row));
            continue;
        }
        if (row.kind == SettingKind::Hotkey && (
                item.contains("button") || item.contains("presentation") ||
                item.contains("choices") || item.contains("min") ||
                item.contains("max") || item.contains("step")))
            return false;
        if (row.kind == SettingKind::Integer || row.kind == SettingKind::Float) {
            if (!item.value("min", json(0)).is_number() ||
                !item.value("max", json(100)).is_number()) return false;
            row.minimum = item.value("min", 0.0);
            row.maximum = item.value("max", 100.0);
            if (!std::isfinite(row.minimum) || !std::isfinite(row.maximum) ||
                row.minimum > row.maximum) return false;
            row.step = item.value("step", 1.0);
            if (!std::isfinite(row.step) || row.step <= 0.0) return false;
        }
        if (row.kind == SettingKind::Choice) {
            if (!item.contains("choices") || !item["choices"].is_array() ||
                item["choices"].empty() || item["choices"].size() > 100) return false;
            for (const auto& option : item["choices"]) {
                if (!option.is_string()) return false;
                auto word = option.get<std::string>();
                if (word.empty() || word.size() > 100 ||
                    std::find(row.choices.begin(), row.choices.end(), word) != row.choices.end())
                    return false;
                row.choices.push_back(std::move(word));
            }
        }

        const auto def = Normalize(row, item["default"]);
        if (!def) return false;
        row.defaultValue = *def;
        row.value = row.defaultValue;

        if (saved.contains("values") && saved["values"].is_object() &&
            saved["values"].contains(id) && saved["values"][id].is_object() &&
            saved["values"][id].contains(row.id)) {
            const auto restored = Normalize(row, saved["values"][id][row.id]);
            if (restored) row.value = *restored;
        }
        prepared.settings.push_back(std::move(row));
    }
    // Button IDs must not collide with any setting ID or other button IDs.
    std::vector<std::string> actionIds;
    for (const auto& row : prepared.settings) {
        if (row.kind == SettingKind::Action) actionIds.push_back(row.id);
        if (!row.buttonId.empty()) actionIds.push_back(row.buttonId);
    }
    std::sort(actionIds.begin(), actionIds.end());
    if (std::adjacent_find(actionIds.begin(), actionIds.end()) != actionIds.end())
        return false;
    for (const auto& action : actionIds)
        if (std::any_of(prepared.settings.begin(), prepared.settings.end(),
            [&](const SettingView& row) {
                return row.id == action && row.kind != SettingKind::Action;
            })) return false;

    std::lock_guard guard(mutex_);
    pendingActions_.erase(id); // Re-registration does not replay stale clicks.
    const bool first=!mods_.contains(id);
    mods_[id] = std::move(prepared);
    if (first) AppendLogLocked(id,LogLevel::Info,"M1 Arc: mod registered");
    return true;
}

std::vector<ModView> Registry::Snapshot() const
{
    std::lock_guard guard(mutex_);
    std::vector<ModView> result;
    result.reserve(mods_.size());
    for (const auto& [id, mod] : mods_) result.push_back(mod);
    std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return a.title < b.title;
    });
    return result;
}

std::optional<nlohmann::json> Registry::Get(
    std::string_view mod, std::string_view key) const
{
    std::lock_guard guard(mutex_);
    const auto it = mods_.find(std::string(mod));
    if (it == mods_.end()) return std::nullopt;
    const auto found = std::find_if(it->second.settings.begin(), it->second.settings.end(),
        [&](const SettingView& s) { return s.id == key; });
    if (found == it->second.settings.end() || found->kind == SettingKind::Action)
        return std::nullopt;
    return found->value;
}

bool Registry::QueueAction(std::string_view mod, std::string_view action)
{
    std::lock_guard guard(mutex_);
    const auto it = mods_.find(std::string(mod));
    if (it == mods_.end()) return false;
    const bool declared = std::any_of(it->second.settings.begin(),
        it->second.settings.end(), [&](const SettingView& s) {
            return ((s.kind == SettingKind::Action || s.kind == SettingKind::Hotkey) && s.id == action) ||
                   (!s.buttonId.empty() && s.buttonId == action);
        });
    if (!declared) {
        AppendLogLocked(mod,LogLevel::Warning,"M1 Arc: unknown action");
        return false;
    }
    auto& pending = pendingActions_[it->first][std::string(action)];
    // Bound the queue: repeatedly clicking during a paused game cannot
    // create unlimited delayed game effects.
    if (pending >= 16u) {
        AppendLogLocked(mod,LogLevel::Warning,"M1 Arc: action queue full: "+std::string(action));
        return false;
    }
    ++pending;
    AppendLogLocked(mod,LogLevel::Info,"M1 Arc: action requested: "+std::string(action));
    return true;
}

void Registry::QueueHotkeyKey(int virtualKey)
{
    if (!ValidHotkey(virtualKey) || virtualKey == 0) return;
    std::lock_guard guard(mutex_);
    for (const auto& [modId, mod] : mods_) {
        // A disabled mod must not receive gameplay shortcuts.
        bool enabled = true;
        for (const auto& row : mod.settings)
            if ((row.id == "enabled" || row.id == "active") &&
                row.kind == SettingKind::Boolean && row.value.is_boolean()) {
                enabled = row.value.get<bool>();
                break;
            }
        if (!enabled) continue;
        for (const auto& row : mod.settings) {
            if (row.kind != SettingKind::Hotkey || !row.value.is_number_integer() ||
                row.value.get<int>() != virtualKey) continue;
            auto& pending = pendingActions_[modId][row.id];
            if (pending < 16u) {
                ++pending;
                AppendLogLocked(modId,LogLevel::Debug,"M1 Arc: shortcut requested: "+row.id);
            }
        }
    }
}

bool Registry::ConsumeAction(std::string_view mod, std::string_view action)
{
    std::lock_guard guard(mutex_);
    const auto modIt = pendingActions_.find(std::string(mod));
    if (modIt == pendingActions_.end()) return false;
    const auto it = modIt->second.find(std::string(action));
    if (it == modIt->second.end() || it->second == 0u) return false;
    --it->second;
    if (it->second == 0u) modIt->second.erase(it);
    if (modIt->second.empty()) pendingActions_.erase(modIt);
    AppendLogLocked(mod,LogLevel::Debug,"M1 Arc: action consumed: "+std::string(action));
    return true;
}

bool Registry::Persist(const std::function<void(json&)>& update)
{
    if (!store_.Update(update)) {
        error_ = store_.LastError();
        if (error_.empty()) error_ = "Failed to persist settings";
        return false;
    }
    error_.clear();
    return true;
}

bool Registry::Commit(std::string_view mod, std::string_view key, const json& next)
{
    const auto modId = std::string(mod);
    const auto settingId = std::string(key);
    return Persist([&](json& data) {
        auto& values = data["values"];
        if (!values.contains(modId) || !values[modId].is_object())
            values[modId] = json::object();
        values[modId][settingId] = next;
    });
}

bool Registry::Set(std::string_view mod, std::string_view key, const json& input)
{
    std::lock_guard guard(mutex_);
    const auto it = mods_.find(std::string(mod));
    if (it == mods_.end()) {
        error_ = "Unknown mod";
        return false;
    }
    auto found = std::find_if(it->second.settings.begin(), it->second.settings.end(),
        [&](const SettingView& s) { return s.id == key; });
    if (found == it->second.settings.end()) {
        error_ = "Unknown setting";
        return false;
    }
    const auto next = Normalize(*found, input);
    if (!next) {
        error_ = "Invalid setting value";
        return false;
    }
    if (*next == found->value) {
        error_.clear();
        return true;
    }
    if (!Commit(mod, key, *next)) return false;
    found->value = *next;
    return true;
}

bool Registry::Reset(std::string_view mod, std::string_view key)
{
    const auto snapshot = Snapshot();
    for (const auto& candidate : snapshot) if (candidate.id == mod)
        for (const auto& setting : candidate.settings) if (setting.id == key)
            return Set(mod, key, setting.defaultValue);
    std::lock_guard guard(mutex_);
    error_ = "Unknown mod or setting";
    return false;
}

bool Registry::ResetMod(std::string_view mod)
{
    std::lock_guard guard(mutex_);
    const auto it = mods_.find(std::string(mod));
    if (it == mods_.end()) {
        error_ = "Unknown mod";
        return false;
    }
    if (!Persist([&](json& document) {
        auto& values = document["values"];
        values[it->first] = json::object();
        for (const auto& setting : it->second.settings)
            if (setting.kind != SettingKind::Action)
                values[it->first][setting.id] = setting.defaultValue;
    })) return false;
    for (auto& setting : it->second.settings)
        if (setting.kind != SettingKind::Action)
            setting.value = setting.defaultValue;
    return true;
}

bool Registry::SaveProfile(std::string name)
{
    std::lock_guard guard(mutex_);
    if (!ValidId(name)) {
        error_ = "Invalid profile name";
        return false;
    }
    return Persist([&](json& document) {
        // Preserve values for mods that are not currently registered.
        auto profile = document["values"];
        for (const auto& [id, mod] : mods_) {
            if (!profile.contains(id) || !profile[id].is_object())
                profile[id] = json::object();
            for (const auto& setting : mod.settings)
                if (setting.kind != SettingKind::Action)
                    profile[id][setting.id] = setting.value;
        }
        document["profiles"][name] = std::move(profile);
    });
}

bool Registry::LoadProfile(std::string_view name)
{
    std::lock_guard guard(mutex_);
    if (!ValidId(name)) {
        error_ = "Invalid profile name";
        return false;
    }
    struct Change { std::string mod, key; json value; };
    std::vector<Change> updates;
    bool valid = true;
    std::string invalidReason;
    const bool saved = Persist([&](json& document) {
        const auto profileIt = document["profiles"].find(std::string(name));
        if (profileIt == document["profiles"].end() || !profileIt->is_object()) {
            valid = false;
            invalidReason = "Profile not found or malformed";
            return;
        }

        // Validate every registered setting BEFORE touching the document.
        // Unregistered mods are preserved for registration after a reload.
        const json profile = *profileIt;
        for (auto it = profile.begin(); it != profile.end(); ++it) {
            const auto& modId = it.key();
            const auto& record = it.value();
            if (!record.is_object()) {
                valid = false;
                invalidReason = "Malformed profile mod record";
                return;
            }
            const auto registered = mods_.find(modId);
            if (registered == mods_.end()) continue;
            for (const auto& setting : registered->second.settings) {
                if (setting.kind == SettingKind::Action || !record.contains(setting.id)) continue;
                const auto next = Normalize(setting, record[setting.id]);
                if (!next) {
                    valid = false;
                    invalidReason = "Invalid profile setting value";
                    return;
                }
                updates.push_back({modId, setting.id, *next});
            }
        }

        // Preserve newly introduced settings and offline-mod values.
        for (auto it = profile.begin(); it != profile.end(); ++it) {
            const auto& modId = it.key();
            if (!document["values"].contains(modId) ||
                !document["values"][modId].is_object())
                document["values"][modId] = json::object();
            for (auto setting = it.value().begin(); setting != it.value().end(); ++setting)
                document["values"][modId][setting.key()] = setting.value();
        }
        // Apply the SAME normalized values to disk as to the live registry:
        // otherwise an out-of-range profile could leave JSON and UI divergent.
        for (const auto& change : updates)
            document["values"][change.mod][change.key] = change.value;
    });
    if (!saved) return false;
    if (!valid) {
        error_ = std::move(invalidReason);
        return false;
    }
    for (const auto& change : updates)
        for (auto& setting : mods_.at(change.mod).settings)
            if (setting.id == change.key) setting.value = change.value;
    return true;
}

bool Registry::DeleteProfile(std::string_view name)
{
    std::lock_guard guard(mutex_);
    if (!ValidId(name)) {
        error_ = "Invalid profile name";
        return false;
    }
    bool found = false;
    const bool saved = Persist([&](json& document) {
        found = document["profiles"].erase(std::string(name)) != 0;
    });
    if (!saved) return false;
    if (!found) {
        error_ = "Profile not found";
        return false;
    }
    return true;
}

std::vector<std::string> Registry::Profiles() const
{
    auto document = ReadSaved(store_);
    std::vector<std::string> result;
    if (!document.contains("profiles") || !document["profiles"].is_object()) return result;
    for (auto it = document["profiles"].begin(); it != document["profiles"].end(); ++it)
        result.push_back(it.key());
    std::sort(result.begin(), result.end());
    return result;
}

std::string Registry::LastError() const
{
    std::lock_guard guard(mutex_);
    return error_;
}
} // namespace m1arc
