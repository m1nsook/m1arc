#include "../src/dev_fixture.hpp"
#include "check.hpp"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    namespace fs = std::filesystem;
    const auto root=fs::temp_directory_path()/"m1arc_fixture_native_test";
    std::error_code ec;
    fs::remove_all(root,ec);
    fs::create_directories(root,ec);
    REQUIRE(!ec);
    m1arc::StateStore state;
    state.Configure(root/"settings.json");
    REQUIRE(state.Ready());
    m1arc::Registry registry(state);

    REQUIRE(!m1arc::dev::RegisterIfEnabled(root,registry));
    REQUIRE(registry.Snapshot().empty());
    std::ofstream(root/"dev-preview.enabled") << "M1 Arc development-only preview\n";
    REQUIRE(m1arc::dev::RegisterIfEnabled(root,registry));
    auto mods=registry.Snapshot();
    REQUIRE(mods.size()==m1arc::dev::kDemoCount);
    const auto fixture=std::find_if(mods.begin(),mods.end(),[](const auto& mod){return mod.id=="m1arc.ui-test";});
    REQUIRE(fixture!=mods.end());
    REQUIRE(fixture->scope=="UI/UX");
    REQUIRE(fixture->shortDesc=="Native settings and typography stress test");
    REQUIRE(fixture->settings.size() >= 35);
    const auto toast=std::find_if(mods.begin(),mods.end(),[](const auto& mod){return mod.id==m1arc::dev::kToastDemoId;});
    REQUIRE(toast!=mods.end());
    REQUIRE(toast->settings.size()==9);
    REQUIRE(registry.QueueAction(toast->id,"toast_burst"));
    REQUIRE(registry.ConsumeAction(toast->id,"toast_burst"));
    REQUIRE(!registry.ConsumeAction(toast->id,"toast_burst"));
    REQUIRE(m1arc::dev::previewEnabled.load());
    bool hasBool=false,hasInteger=false,hasFloat=false,hasChoice=false,hasLong=false;
    bool hasCheckbox=false,hasAction=false,hasInlineAction=false,hasWeapons=false,hasHotkey=false;
    for(const auto& setting:fixture->settings) {
        hasBool |= setting.kind==m1arc::SettingKind::Boolean;
        hasInteger |= setting.kind==m1arc::SettingKind::Integer;
        hasFloat |= setting.kind==m1arc::SettingKind::Float;
        hasChoice |= setting.kind==m1arc::SettingKind::Choice;
        hasLong |= setting.label.size()>100;
        hasCheckbox |= setting.checkbox;
        hasAction |= setting.kind==m1arc::SettingKind::Action;
        hasInlineAction |= setting.buttonId=="spawn_ammo";
        hasWeapons |= setting.group=="Weapons";
        hasHotkey |= setting.kind==m1arc::SettingKind::Hotkey;
    }
    REQUIRE(hasBool && hasInteger && hasFloat && hasChoice && hasLong);
    REQUIRE(hasCheckbox && hasAction && hasInlineAction && hasWeapons && hasHotkey);
    REQUIRE(registry.QueueAction("m1arc.ui-test","spawn_ammo"));
    REQUIRE(registry.ConsumeAction("m1arc.ui-test","spawn_ammo"));
    REQUIRE(!registry.ConsumeAction("m1arc.ui-test","spawn_ammo"));
    REQUIRE(registry.Set("m1arc.ui-test","short_integer",8));
    REQUIRE(registry.Get("m1arc.ui-test","short_integer").value()==8);
    REQUIRE(registry.Set("m1arc.ui-test","enabled",false));
    REQUIRE(registry.Get("m1arc.ui-test","enabled").value()==false);
    REQUIRE(registry.ResetMod("m1arc.ui-test"));
    REQUIRE(registry.Get("m1arc.ui-test","enabled").value()==true);

    fs::remove(root/"dev-preview.enabled",ec);
    REQUIRE(!m1arc::dev::RegisterIfEnabled(root,registry));
    m1arc::Registry restarted(state);
    REQUIRE(!m1arc::dev::RegisterIfEnabled(root,restarted));
    REQUIRE(restarted.Snapshot().empty());
    REQUIRE(!m1arc::dev::previewEnabled.load());
    fs::remove_all(root,ec);
    std::cout<<"UI preview fixture tests passed\n";
    return 0;
}
