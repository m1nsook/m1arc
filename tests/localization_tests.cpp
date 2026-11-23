#include "../src/localization.hpp"
#include "../src/registry.hpp"
#include "check.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "m1arc_localization_native_tests";
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root / "locales");
    fs::create_directories(root / "mods" / "sample.weight" / "language");

    std::ofstream(root / "locales" / "en_US.json", std::ios::binary)
        << R"({"name":"English","strings":{"ui.close":"CLOSE"}})";
    std::ofstream(root / "locales" / "pl_PL.json", std::ios::binary)
        << R"({"name":"Polski","strings":{"ui.close":"ZAMKNIJ"}})";
    std::ofstream(root / "locales" / "ko_KR.json", std::ios::binary)
        << R"({"name":"한국어","strings":{"ui.close":"닫기"}})";
    std::ofstream(root / "mods" / "sample.weight" / "language" / "ko_KR.json",
                  std::ios::binary)
        << R"({"name":"무게 제한","shortDesc":"소지 용량을 조절합니다","settings":{"mode":{"label":"모드","description":"옵션을 선택하세요","group":"기본","choices":{"High":"높음","Low":"낮음"}}}})";

    m1arc::StateStore store;
    store.Configure(root / "settings.json");
    REQUIRE(store.Ready());
    m1arc::Localization tr;
    tr.Configure(root, store);
    REQUIRE(tr.Languages().size() == 3);
    REQUIRE(tr.Current() == "en_US");
    REQUIRE(tr.Text("ui.close", "Fallback") == "CLOSE");
    REQUIRE(tr.Text("not.found", "Fallback") == "Fallback");
    REQUIRE(!tr.Select("../unsafe"));
    REQUIRE(!tr.Select("zz_ZZ"));
    REQUIRE(tr.Select("ko_KR"));
    REQUIRE(tr.Current() == "ko_KR");
    REQUIRE(tr.Text("ui.close", "Fallback") == "닫기");

    m1arc::ModView mod;
    mod.id = "sample.weight";
    mod.title = "Weight Limit";
    mod.shortDesc = "Adjust carrying capacity";
    m1arc::SettingView mode;
    mode.id = "mode";
    mode.label = "Mode";
    mode.description = "Select a mode";
    mode.group = "General";
    mode.kind = m1arc::SettingKind::Choice;
    mode.choices = {"High", "Low"};
    mode.value = "High";
    mode.defaultValue = "Low";
    mod.settings.push_back(mode);
    tr.Localize(mod);
    REQUIRE(mod.title == "무게 제한");
    REQUIRE(mod.shortDesc == "소지 용량을 조절합니다");
    REQUIRE(mod.settings.front().label == "모드");
    REQUIRE(mod.settings.front().group == "기본");
    REQUIRE(mod.settings.front().value == "High");
    REQUIRE(mod.settings.front().defaultValue == "Low");
    REQUIRE(mod.settings.front().choices[0] == "High");
    REQUIRE(tr.Choice("sample.weight", "mode", "High") == "높음");
    REQUIRE(tr.Choice("sample.weight", "mode", "Unknown") == "Unknown");
    REQUIRE(tr.Choice("../unsafe", "mode", "High") == "High");

    // Language changes persist, without modifying profiles or setting values.
    REQUIRE(store.Update([](nlohmann::json& doc) {
        doc["values"]["sample.weight"]["mode"] = "High";
        doc["profiles"]["slot1"] = nlohmann::json::object();
    }));
    m1arc::StateStore restart;
    restart.Configure(root / "settings.json");
    REQUIRE(restart.Ready());
    m1arc::Localization reopened;
    reopened.Configure(root, restart);
    REQUIRE(reopened.Current() == "ko_KR");
    REQUIRE(reopened.Select("pl_PL"));
    REQUIRE(reopened.Text("ui.close", "Fallback") == "ZAMKNIJ");
    REQUIRE(reopened.Choice("sample.weight", "mode", "High") == "High");
    const auto saved = nlohmann::json::parse(restart.Snapshot());
    REQUIRE(saved["overlay"]["language"] == "pl_PL");
    REQUIRE(saved["values"]["sample.weight"]["mode"] == "High");
    REQUIRE(saved["profiles"].contains("slot1"));

    fs::remove_all(root, ec);
    std::cout << "Native JSON localization tests passed\n";
    return 0;
}
