#include "../src/state.hpp"

#include "check.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

int main()
{
    namespace fs = std::filesystem;
    // Native durable state remains authoritative across DLL reloads.
    const auto dir = fs::temp_directory_path() / "m1arc_native_new_tests";
    fs::create_directories(dir);
    const auto path = dir / "settings.json";
    std::error_code ec;
    fs::remove(path, ec);

    m1arc::StateStore store;
    store.Configure(path);
    REQUIRE(store.Ready());
    REQUIRE(store.Revision() == 0);
    REQUIRE(store.Replace(R"({"schema":1,"values":{"demo":{"enabled":false,"size":25}},"profiles":{}})"));
    REQUIRE(store.Revision() == 1);
    REQUIRE(store.Snapshot().find("\"enabled\":false") != std::string::npos);
    REQUIRE(!store.Replace("{this is not json"));
    REQUIRE(store.Revision() == 1);
    REQUIRE(!store.Replace(R"({"schema":2,"values":{},"profiles":{}})"));
    REQUIRE(store.Revision() == 1);

    // Independent writers must merge, not overwrite, another field.
    std::thread a([&] {
        for (int i = 0; i < 8; ++i)
            REQUIRE(store.Update([i](nlohmann::json& d) {
                d["values"]["writer_a"]["count"] = i;
            }));
    });
    std::thread b([&] {
        for (int i = 0; i < 8; ++i)
            REQUIRE(store.Update([i](nlohmann::json& d) {
                d["values"]["writer_b"]["count"] = i;
            }));
    });
    a.join();
    b.join();
    const auto merged = nlohmann::json::parse(store.Snapshot());
    REQUIRE(merged["values"]["writer_a"]["count"] == 7);
    REQUIRE(merged["values"]["writer_b"]["count"] == 7);
    const auto revisionBeforeInvalid = store.Revision();
    REQUIRE(!store.Update([](nlohmann::json& d) {
        d["schema"] = 99;
    }));
    REQUIRE(store.Revision() == revisionBeforeInvalid);

    m1arc::StateStore restart;
    restart.Configure(path);
    REQUIRE(restart.Ready());
    REQUIRE(restart.Snapshot() == store.Snapshot());

    // Regression: the independent overlay shortcut is saved in the
    // same settings.json as the mods. It must be readable on a fresh
    // startup BEFORE any HWND/input hook is attached.
    constexpr int f11 = 0x7A; // Win32 VK_F11
    REQUIRE(store.Update([](nlohmann::json& document) {
        document["overlay"]["hotkey"] = f11;
        document["overlay"]["theme"] = "light";
    }));
    m1arc::StateStore restartWithHotkey;
    restartWithHotkey.Configure(path);
    REQUIRE(restartWithHotkey.Ready());
    const auto restoredSettings =
        nlohmann::json::parse(restartWithHotkey.Snapshot());
    REQUIRE(restoredSettings["overlay"]["hotkey"] == f11);
    REQUIRE(restoredSettings["overlay"]["theme"] == "light");
    REQUIRE(restoredSettings["values"]["writer_a"]["count"] == 7);
    REQUIRE(restoredSettings["values"]["writer_b"]["count"] == 7);
    REQUIRE(restartWithHotkey.Update([](nlohmann::json& document) {
        document["overlay"]["hotkey"] = 0x79; // rebind to F10
    }));
    m1arc::StateStore restartAfterRebind;
    restartAfterRebind.Configure(path);
    REQUIRE(restartAfterRebind.Ready());
    REQUIRE(nlohmann::json::parse(restartAfterRebind.Snapshot())
            ["overlay"]["hotkey"] == 0x79);

    // Invalid files are never overwritten as a "repair".
    std::ofstream(path,std::ios::trunc) << "{ corrupt configuration";
    m1arc::StateStore invalid;
    invalid.Configure(path);
    REQUIRE(!invalid.Ready());
    REQUIRE(!invalid.LastError().empty());
    REQUIRE(!invalid.Replace(R"({"schema":1,"values":{},"profiles":{}})"));
    std::ifstream keep(path);
    std::string content((std::istreambuf_iterator<char>(keep)), {});
    REQUIRE(content == "{ corrupt configuration");

    fs::remove_all(dir,ec);
    std::cout << "DLL native state tests passed\n";
    return 0;
}
