#include "upgrade.hpp"
#include "check.hpp"
#include <filesystem>
using namespace m1arc;
int main() {
    namespace fs=std::filesystem;
    const auto path=fs::temp_directory_path()/"m1arc_upgrade_tests"/"settings.json";
    std::error_code ec;
    fs::remove_all(path.parent_path(),ec);
    StateStore store;
    store.Configure(path);
    REQUIRE(store.Ready());
    REQUIRE(upgrade::Version("1.0.10")>upgrade::Version("1.0.2"));
    REQUIRE(store.Update([](auto& d){
        d["overlay"]={{"hotkey",32},{"dark",true}};
        d["values"]["example"]["enabled"]=false;
        d["profiles"]["custom"]={{"example",{{"enabled",true}}}};
        d["language"]="ko_KR";
    }));
    auto legacy=nlohmann::json::parse(store.Snapshot());
    REQUIRE(upgrade::NeedsShortcut(legacy));
    REQUIRE(upgrade::SavedShortcut(legacy)==0);
    REQUIRE(upgrade::Prepare(store));
    REQUIRE(nlohmann::json::parse(store.Snapshot())==legacy);
    // Exiting before saving must not mark the migration completed.
    StateStore interrupted;
    interrupted.Configure(path);
    REQUIRE(upgrade::NeedsShortcut(nlohmann::json::parse(interrupted.Snapshot())));
    const int chord=PackMenuShortcut(0x79,kMenuCtrl);
    REQUIRE(store.Update([&](auto& d){upgrade::SaveShortcut(d,chord);}));
    StateStore restarted;
    restarted.Configure(path);
    auto next=nlohmann::json::parse(restarted.Snapshot());
    REQUIRE(!upgrade::NeedsShortcut(next));
    REQUIRE(upgrade::SavedShortcut(next)==chord);
    REQUIRE(upgrade::Completed(next)==upgrade::Current);
    REQUIRE(upgrade::Field(next,"previous_version")=="1.0.0");
    REQUIRE(upgrade::WelcomePending(next));
    REQUIRE(next["values"]==legacy["values"]);
    REQUIRE(next["profiles"]==legacy["profiles"]);
    REQUIRE(next["language"]==legacy["language"]);
    REQUIRE(next["overlay"]["dark"]==legacy["overlay"]["dark"]);
    REQUIRE(upgrade::Acknowledge(restarted));
    StateStore again;
    again.Configure(path);
    REQUIRE(!upgrade::WelcomePending(nlohmann::json::parse(again.Snapshot())));
    REQUIRE(again.Update([](auto& d){upgrade::SaveShortcut(d,PackMenuShortcut(0x78,0));}));
    REQUIRE(!upgrade::WelcomePending(nlohmann::json::parse(again.Snapshot())));
    nlohmann::json fresh={{"schema",1},{"values",nlohmann::json::object()},{"profiles",nlohmann::json::object()}};
    REQUIRE(!upgrade::NeedsShortcut(fresh));
    upgrade::SaveShortcut(fresh,chord);
    REQUIRE(upgrade::Field(fresh,"previous_version").empty());
    REQUIRE(upgrade::WelcomePending(fresh));
    fs::remove_all(path.parent_path(),ec);
}
