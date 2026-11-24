#pragma once
#include "state.hpp"
#include "menu_shortcut.hpp"
#include <array>
#include <charconv>
#include <string>
#include <string_view>
namespace m1arc::upgrade {
inline constexpr std::string_view Current=M1ARC_BUILD_VERSION;
inline constexpr std::string_view Legacy="1.0.0";
using json=nlohmann::json;
inline std::array<int,3> Version(std::string_view value) {
    std::array<int,3> parts{};
    for (int i=0;i<3;++i) {
        const auto dot=value.find('.');
        const auto token=value.substr(0,dot);
        if (token.empty() || token.size()>5 || (i<2)==(dot==std::string_view::npos)) return {};
        const auto parsed=std::from_chars(token.data(),token.data()+token.size(),parts[i]);
        if (parsed.ec!=std::errc{} || parsed.ptr!=token.data()+token.size() || parts[i]<0) return {};
        if (dot!=std::string_view::npos) value.remove_prefix(dot+1);
    }
    return parts;
}
inline std::string Field(const json& doc,const char* key) {
    const auto it=doc.find("upgrade");
    if (it==doc.end() || !it->is_object()) return {};
    const auto field=it->find(key);
    return field!=it->end() && field->is_string() ? field->get<std::string>() : std::string{};
}
inline std::string Completed(const json& doc) { return Field(doc,"completed_version"); }
inline bool LegacyShortcut(const json& doc) {
    const auto it=doc.find("overlay");
    return Completed(doc).empty() && it!=doc.end() && it->is_object() && it->contains("hotkey");
}
inline bool NeedsShortcut(const json& doc) {
    return Version(Current)>=Version("1.0.1") && LegacyShortcut(doc);
}
inline int SavedShortcut(const json& doc) {
    if (NeedsShortcut(doc)) return 0;
    const auto it=doc.find("overlay");
    if (it==doc.end() || !it->is_object()) return 0;
    try {
        const int key=it->value("hotkey",0), mods=it->value("hotkey_modifiers",0);
        return ValidMenuShortcut(key,mods) ? PackMenuShortcut(key,mods) : 0;
    } catch (...) { return 0; }
}
inline void Complete(json& doc,std::string from) {
    if (!doc["upgrade"].is_object()) doc["upgrade"]=json::object();
    auto& state=doc["upgrade"];
    state["completed_version"]=std::string(Current);
    state["welcome_version"]=std::string(Current);
    state["previous_version"]=std::move(from);
}
inline void SaveShortcut(json& doc,int candidate) {
    // Determine the migration source BEFORE writing the new key.
    const auto previous=Completed(doc);
    const bool legacy=LegacyShortcut(doc);
    if (!doc["overlay"].is_object()) doc["overlay"]=json::object();
    doc["overlay"]["hotkey"]=MenuKey(candidate);
    doc["overlay"]["hotkey_modifiers"]=MenuMods(candidate);
    if (previous.empty() || Version(previous)<Version(Current))
        Complete(doc,legacy ? std::string(Legacy) : previous);
}
inline bool Prepare(StateStore& store) {
    if (!store.Ready()) return false;
    const auto doc=json::parse(store.Snapshot());
    const auto previous=Completed(doc);
    // Legacy users complete atomically with their confirmed shortcut.
    // Future releases can show a new welcome without forcing another rebind.
    if (previous.empty() || Version(previous)>=Version(Current) || !SavedShortcut(doc)) return true;
    return store.Update([](json& data) {
        const auto from=Completed(data);
        if (!from.empty() && Version(from)<Version(Current) && SavedShortcut(data))
            Complete(data,from);
    });
}
inline bool WelcomePending(const json& doc) {
    return Field(doc,"welcome_version")==Current &&
           Field(doc,"acknowledged_version")!=Current &&
           Completed(doc)==Current && SavedShortcut(doc)!=0;
}
inline bool Acknowledge(StateStore& store) {
    return store.Update([](json& data) {
        if (WelcomePending(data)) data["upgrade"]["acknowledged_version"]=std::string(Current);
    });
}
}
