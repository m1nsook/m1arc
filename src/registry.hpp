#pragma once

#include "state.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace m1arc {
enum class SettingKind { Boolean, Integer, Float, Choice, Action, Hotkey };

struct SettingView {
    std::string id;
    std::string label;
    std::string description;
    std::string group;
    SettingKind kind{SettingKind::Boolean};
    double minimum{0.0};
    double maximum{100.0};
    double step{1.0};
    std::vector<std::string> choices;
    // Display-only controls: the legacy segmented switch stays the default.
    bool checkbox{false};
    // An optional in-row action next to a numeric slider.
    std::string buttonId;
    std::string buttonLabel;
    nlohmann::json value;
    nlohmann::json defaultValue;
};

struct ModView {
    std::string id;
    std::string title;
    // Optional data-only mod.json metadata, never stored as a setting.
    std::string scope{"Miscellaneous"};
    std::string shortDesc;
    std::vector<SettingView> settings;
};

// Exact author-selectable manifest scopes. Missing scope defaults to Miscellaneous.
[[nodiscard]] bool ValidModScope(std::string_view scope) noexcept;

// Native registry, independent of any UI scripting runtime.
// The ImGui renderer will consume a snapshot of these data-only rows.
class Registry {
public:
    explicit Registry(StateStore& store) : store_(store) {}
    bool Register(std::string id, std::string title, const nlohmann::json& definitions,
                  std::string scope = "Miscellaneous",
                  std::string shortDesc = {});
    [[nodiscard]] std::vector<ModView> Snapshot() const;
    [[nodiscard]] std::optional<nlohmann::json> Get(std::string_view mod, std::string_view key) const;
    bool Set(std::string_view mod, std::string_view key, const nlohmann::json& value);
    bool Reset(std::string_view mod, std::string_view key);
    bool ResetMod(std::string_view mod);
    // In-memory one-shot button queue. REDscript consumes clicks to perform
    // game actions; no commands/scripts are executed by the framework.
    bool QueueAction(std::string_view mod, std::string_view action);
    [[nodiscard]] bool ConsumeAction(std::string_view mod, std::string_view action);
    // Called exclusively on a physical key-down edge while gameplay has focus.
    void QueueHotkeyKey(int virtualKey);
    bool SaveProfile(std::string name);
    bool LoadProfile(std::string_view name);
    bool DeleteProfile(std::string_view name);
    [[nodiscard]] std::vector<std::string> Profiles() const;
    [[nodiscard]] std::string LastError() const;
private:
    [[nodiscard]] static bool ValidId(std::string_view value);
    [[nodiscard]] static std::optional<nlohmann::json>
        Normalize(const SettingView& setting, const nlohmann::json& incoming);
    // Must be called under mutex_. Reports errors for every persistence path.
    [[nodiscard]] bool Persist(const std::function<void(nlohmann::json&)>& update);
    [[nodiscard]] bool Commit(std::string_view mod, std::string_view key,
                              const nlohmann::json& next);
    StateStore& store_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ModView> mods_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::uint32_t>> pendingActions_;
    std::string error_;
};

Registry& Settings();
} // namespace m1arc
