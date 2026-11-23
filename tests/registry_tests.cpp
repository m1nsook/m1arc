#include "../src/registry.hpp"
#include "check.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using nlohmann::json;

int main()
{
    namespace fs = std::filesystem;
    const auto path = fs::temp_directory_path() / "m1arc_registry_fresh_tests" / "settings.json";
    std::error_code ec;
    fs::remove_all(path.parent_path(), ec);
    m1arc::StateStore store;
    store.Configure(path);
    REQUIRE(store.Ready());
    m1arc::Registry registry(store);

    const auto schema = json::array({
        {{"id","active"},{"kind","boolean"},{"default",true},
         {"label","Enabled"},{"group","General"}},
        {{"id","capacity"},{"kind","integer"},{"min",0},{"max",250},
         {"default",50},{"group","Gameplay"}},
        {{"id","multiplier"},{"kind","float"},{"min",0.1},{"max",5.0},
         {"default",1.0}},
        {{"id","mode"},{"kind","choice"},{"choices",json::array({"auto","manual"})},
         {"default","auto"}}
    });
    REQUIRE(registry.Register("example","Demo Settings",schema));
    REQUIRE(registry.Snapshot().size()==1);
    REQUIRE(registry.Get("example","active").value()==true);
    REQUIRE(registry.Set("example","active",false));
    REQUIRE(registry.Get("example","active").value()==false);
    REQUIRE(registry.Set("example","capacity",10000));
    REQUIRE(registry.Get("example","capacity").value()==250);
    REQUIRE(!registry.Set("example","capacity","broken"));
    REQUIRE(registry.Set("example","mode","manual"));
    REQUIRE(!registry.Set("example","mode","no_such_value"));

    // A setting changed through the UI is saved at the time of Set().
    // Simulate a game restart BEFORE resetting the value: the next
    // registry and state store must recover the non-default settings.
    {
        std::ifstream disk(path, std::ios::binary);
        REQUIRE(disk.good());
        const auto savedOnDisk = json::parse(disk);
        REQUIRE(savedOnDisk["values"]["example"]["active"] == false);
        REQUIRE(savedOnDisk["values"]["example"]["capacity"] == 250);
        REQUIRE(savedOnDisk["values"]["example"]["mode"] == "manual");
    }
    m1arc::StateStore restartBeforeReset;
    restartBeforeReset.Configure(path);
    REQUIRE(restartBeforeReset.Ready());
    m1arc::Registry restoredBeforeReset(restartBeforeReset);
    REQUIRE(restoredBeforeReset.Register("example","Demo Settings",schema));
    REQUIRE(restoredBeforeReset.Get("example","active").value() == false);
    REQUIRE(restoredBeforeReset.Get("example","capacity").value() == 250);
    REQUIRE(restoredBeforeReset.Get("example","mode").value() == "manual");

    // Unregistered mod values must survive a profile save.
    auto persisted = json::parse(store.Snapshot());
    persisted["values"]["offline-mod"] = {{"enabled",false}};
    REQUIRE(store.Replace(persisted.dump()));
    REQUIRE(registry.SaveProfile("stealth"));
    const auto profileSaved = json::parse(store.Snapshot());
    REQUIRE(profileSaved["profiles"]["stealth"]["offline-mod"]["enabled"] == false);
    REQUIRE(profileSaved["profiles"]["stealth"]["example"]["active"] == false);
    REQUIRE(registry.Set("example","active",true));
    // Unregistered/offline mods must also restore when loading the profile.
    auto edited = json::parse(store.Snapshot());
    edited["values"]["offline-mod"]["enabled"] = true;
    edited["values"]["offline-mod"]["newer_setting"] = 42;
    REQUIRE(store.Replace(edited.dump()));
    REQUIRE(registry.LoadProfile("stealth"));
    REQUIRE(registry.Get("example","active").value()==false);
    REQUIRE(registry.Get("example","mode").value()=="manual");
    const auto restored = json::parse(store.Snapshot());
    REQUIRE(restored["values"]["offline-mod"]["enabled"] == false);
    REQUIRE(restored["values"]["offline-mod"]["newer_setting"] == 42);
    // A mod registering after profile load must read its restored value.
    m1arc::Registry lateRegistration(store);
    const auto offlineSchema = json::array({
        {{"id","enabled"},{"kind","boolean"},{"default",true}}
    });
    REQUIRE(lateRegistration.Register("offline-mod","Offline Mod",offlineSchema));
    REQUIRE(lateRegistration.Get("offline-mod","enabled").value()==false);

    // A profile may contain a formerly valid value outside the CURRENT
    // schema bounds. Disk and live settings must both use the clamped value.
    auto outOfRange = json::parse(store.Snapshot());
    outOfRange["profiles"]["old_schema"] = {
        {"example", {{"capacity", 9999}, {"active", false}}}
    };
    REQUIRE(store.Replace(outOfRange.dump()));
    REQUIRE(registry.LoadProfile("old_schema"));
    REQUIRE(registry.Get("example","capacity").value() == 250);
    REQUIRE(registry.LastError().empty());
    const auto normalizedDisk = json::parse(store.Snapshot());
    REQUIRE(normalizedDisk["values"]["example"]["capacity"] == 250);
    REQUIRE(json::parse(std::ifstream(path))["values"]["example"]["capacity"] == 250);
    REQUIRE(registry.DeleteProfile("old_schema"));

    // Missing/malformed profile loads must not mutate any values or
    // advance the persisted revision.
    REQUIRE(!registry.LoadProfile("missing"));
    REQUIRE(registry.LastError() == "Profile not found or malformed");
    REQUIRE(!registry.LoadProfile("../unsafe"));
    REQUIRE(registry.LastError() == "Invalid profile name");
    edited = json::parse(store.Snapshot());
    edited["profiles"]["invalid_known"] = {{"example",{{"active","not-a-bool"}}}};
    edited["profiles"]["invalid_unknown"] = {{"offline-mod","not-an-object"}};
    REQUIRE(store.Replace(edited.dump()));
    const auto beforeInvalid = store.Snapshot();
    const auto revisionBeforeInvalid = store.Revision();
    REQUIRE(!registry.LoadProfile("invalid_known"));
    REQUIRE(!registry.LoadProfile("invalid_unknown"));
    REQUIRE(!registry.LastError().empty());
    REQUIRE(store.Snapshot() == beforeInvalid);
    REQUIRE(store.Revision() == revisionBeforeInvalid);
    REQUIRE(registry.Get("example","active").value()==false);
    REQUIRE(registry.DeleteProfile("invalid_known"));
    REQUIRE(registry.DeleteProfile("invalid_unknown"));
    REQUIRE(!registry.DeleteProfile("../unsafe"));
    REQUIRE(!registry.DeleteProfile("missing"));
    REQUIRE(registry.DeleteProfile("stealth"));
    REQUIRE(registry.Profiles().empty());
    REQUIRE(registry.Reset("example","capacity"));
    REQUIRE(registry.Get("example","capacity").value()==50);
    REQUIRE(registry.ResetMod("example"));
    REQUIRE(registry.Get("example","active").value()==true);
    REQUIRE(registry.Get("example","mode").value()=="auto");

    m1arc::StateStore reopenStore;
    reopenStore.Configure(path);
    REQUIRE(reopenStore.Ready());
    m1arc::Registry reopened(reopenStore);
    REQUIRE(reopened.Register("example","Demo Settings",schema));
    REQUIRE(reopened.Get("example","active").value()==true);
    REQUIRE(reopened.Get("example","capacity").value()==50);
    m1arc::Registry offlineRestart(reopenStore);
    REQUIRE(offlineRestart.Register("offline-mod","Offline Mod",offlineSchema));
    REQUIRE(offlineRestart.Get("offline-mod","enabled").value()==false);

    // Atomic descriptor rejection: any broken item rejects the whole mod.
    const auto invalid=json::array({
        {{"id","okay"},{"kind","boolean"},{"default",false}},
        {{"id","bad"},{"kind","choice"},{"choices",json::array({"x"})},
         {"default","wrong"}}
    });
    REQUIRE(!registry.Register("broken","Broken",invalid));
    REQUIRE(registry.Snapshot().size()==1);
    REQUIRE(!registry.Register("../evil","Evil",schema));

    // Direct registration also enforces scope and description constraints.
    m1arc::Registry metadataRegistry(store);
    REQUIRE(!metadataRegistry.Register("scope.bad","Bad",schema,"Unknown"));
    REQUIRE(!metadataRegistry.Register("scope.long","Bad",schema,"Gameplay",
                                       std::string(181,'x')));
    REQUIRE(metadataRegistry.Register("scope.good","Scoped",schema,"UI/UX",
                                      "Adjusts interface and HUD"));
    const auto scoped=metadataRegistry.Snapshot();
    REQUIRE(scoped.size()==1);
    REQUIRE(scoped[0].scope=="UI/UX");
    REQUIRE(scoped[0].shortDesc=="Adjusts interface and HUD");

    // Checkbox styles, grouped sections, standalone actions and an inline
    // slider action. Only real settings are saved; clicks are ephemeral.
    const auto actionsSchema = json::array({
        {{"id","ammo"},{"kind","integer"},{"min",1},{"max",999},
         {"default",50},{"label","Ammunition"},{"group","Weapons"},
         {"button",{{"id","spawn_ammo"},{"label","Spawn"}}}},
        {{"id","light"},{"kind","boolean"},{"default",true},
         {"label","Light Ammo"},{"group","Weapons"},{"presentation","checkbox"}},
        {{"id","heavy"},{"kind","boolean"},{"default",false},
         {"label","Heavy Ammo"},{"group","Weapons"},{"presentation","checkbox"}},
        {{"id","spawn_all"},{"kind","action"},{"label","Spawn selected ammunition"},
         {"buttonLabel","Spawn all"},{"group","Weapons"}}
    });
    m1arc::Registry actionsRegistry(store);
    REQUIRE(actionsRegistry.Register("ammo.mod","Ammo",actionsSchema));
    const auto ammoMod=actionsRegistry.Snapshot().front();
    REQUIRE(ammoMod.settings.size()==4);
    REQUIRE(ammoMod.settings[0].group=="Weapons");
    REQUIRE(ammoMod.settings[0].buttonId=="spawn_ammo");
    REQUIRE(ammoMod.settings[0].buttonLabel=="Spawn");
    REQUIRE(ammoMod.settings[1].checkbox);
    REQUIRE(ammoMod.settings[2].checkbox);
    REQUIRE(ammoMod.settings[3].kind==m1arc::SettingKind::Action);
    REQUIRE(ammoMod.settings[3].buttonLabel=="Spawn all");
    REQUIRE(!actionsRegistry.Get("ammo.mod","spawn_all").has_value());
    REQUIRE(!actionsRegistry.Set("ammo.mod","spawn_all",true));
    REQUIRE(!actionsRegistry.Reset("ammo.mod","spawn_all"));
    REQUIRE(!actionsRegistry.QueueAction("ammo.mod","wrong"));
    REQUIRE(!actionsRegistry.ConsumeAction("ammo.mod","wrong"));
    const auto beforeClick=store.Snapshot();
    const auto revisionBeforeClick=store.Revision();
    REQUIRE(actionsRegistry.QueueAction("ammo.mod","spawn_ammo"));
    REQUIRE(actionsRegistry.QueueAction("ammo.mod","spawn_ammo"));
    REQUIRE(actionsRegistry.QueueAction("ammo.mod","spawn_all"));
    REQUIRE(store.Snapshot()==beforeClick);
    REQUIRE(store.Revision()==revisionBeforeClick);
    REQUIRE(actionsRegistry.ConsumeAction("ammo.mod","spawn_ammo"));
    REQUIRE(actionsRegistry.ConsumeAction("ammo.mod","spawn_ammo"));
    REQUIRE(!actionsRegistry.ConsumeAction("ammo.mod","spawn_ammo"));
    REQUIRE(actionsRegistry.ConsumeAction("ammo.mod","spawn_all"));
    REQUIRE(!actionsRegistry.ConsumeAction("ammo.mod","spawn_all"));
    for (int i=0;i<30;++i) REQUIRE(actionsRegistry.QueueAction("ammo.mod","spawn_all"));
    for (int i=0;i<16;++i) REQUIRE(actionsRegistry.ConsumeAction("ammo.mod","spawn_all"));
    REQUIRE(!actionsRegistry.ConsumeAction("ammo.mod","spawn_all"));
    REQUIRE(actionsRegistry.SaveProfile("ammo_profile"));
    const auto savedActions=json::parse(store.Snapshot());
    REQUIRE(!savedActions["profiles"]["ammo_profile"]["ammo.mod"].contains("spawn_all"));
    REQUIRE(savedActions["profiles"]["ammo_profile"]["ammo.mod"]["ammo"]==50);
    REQUIRE(actionsRegistry.ResetMod("ammo.mod"));
    REQUIRE(!json::parse(store.Snapshot())["values"]["ammo.mod"].contains("spawn_all"));
    REQUIRE(actionsRegistry.QueueAction("ammo.mod","spawn_ammo"));
    REQUIRE(actionsRegistry.Register("ammo.mod","Ammo",actionsSchema));
    REQUIRE(!actionsRegistry.ConsumeAction("ammo.mod","spawn_ammo"));
    REQUIRE(actionsRegistry.Set("ammo.mod","light",false));
    REQUIRE(actionsRegistry.Get("ammo.mod","light").value()==false);
    REQUIRE(actionsRegistry.LoadProfile("ammo_profile"));
    REQUIRE(actionsRegistry.Get("ammo.mod","light").value()==true);

    const auto noDefaultAction=json::array({
        {{"id","press"},{"kind","action"},{"label","Press"}}
    });
    REQUIRE(actionsRegistry.Register("action.only","Only Button",noDefaultAction));
    auto invalidAction=noDefaultAction;
    invalidAction[0]["default"]=true;
    REQUIRE(!actionsRegistry.Register("bad.action","Bad",invalidAction));
    auto invalidStyle=actionsSchema;
    invalidStyle[1]["presentation"]="radio";
    REQUIRE(!actionsRegistry.Register("bad.style","Bad",invalidStyle));
    invalidStyle=actionsSchema;
    invalidStyle[0]["presentation"]="checkbox";
    REQUIRE(!actionsRegistry.Register("bad.number.checkbox","Bad",invalidStyle));
    invalidStyle=actionsSchema;
    invalidStyle[0]["button"]["id"]="heavy";
    REQUIRE(!actionsRegistry.Register("bad.duplicate.button","Bad",invalidStyle));
    invalidStyle=actionsSchema;
    invalidStyle[3]["id"]="spawn_ammo";
    REQUIRE(!actionsRegistry.Register("bad.duplicate.action","Bad",invalidStyle));
    invalidStyle=actionsSchema;
    invalidStyle[0]["button"]["id"]="../unsafe";
    REQUIRE(!actionsRegistry.Register("bad.unsafe.button","Bad",invalidStyle));

    // Native hotkey controls persist key codes but trigger one-shot
    // in-memory events on physical key down, never on UI/game focus loss.
    const auto keysSchema = json::array({
        {{"id","enabled"},{"kind","boolean"},{"default",true}},
        {{"id","spawn_hotkey"},{"kind","hotkey"},{"label","Spawn hotkey"},
         {"group","Weapons"},{"default",0}},
        {{"id","another_hotkey"},{"kind","hotkey"},{"default",0}}
    });
    m1arc::Registry keyRegistry(store);
    REQUIRE(keyRegistry.Register("keys.mod","Hotkeys",keysSchema));
    REQUIRE(keyRegistry.Get("keys.mod","spawn_hotkey").value()==0);
    REQUIRE(keyRegistry.Snapshot().front().settings[1].kind==m1arc::SettingKind::Hotkey);
    REQUIRE(!keyRegistry.Set("keys.mod","spawn_hotkey","F7"));
    REQUIRE(!keyRegistry.Set("keys.mod","spawn_hotkey",0x1B)); // Escape
    REQUIRE(!keyRegistry.Set("keys.mod","spawn_hotkey",0x11)); // Ctrl
    REQUIRE(!keyRegistry.Set("keys.mod","spawn_hotkey",0x5B)); // Win
    REQUIRE(!keyRegistry.Set("keys.mod","spawn_hotkey",1000));
    REQUIRE(keyRegistry.Set("keys.mod","spawn_hotkey",0x76)); // F7
    keyRegistry.QueueHotkeyKey(0x77);
    REQUIRE(!keyRegistry.ConsumeAction("keys.mod","spawn_hotkey"));
    keyRegistry.QueueHotkeyKey(0x76);
    keyRegistry.QueueHotkeyKey(0x76);
    REQUIRE(keyRegistry.ConsumeAction("keys.mod","spawn_hotkey"));
    REQUIRE(keyRegistry.ConsumeAction("keys.mod","spawn_hotkey"));
    REQUIRE(!keyRegistry.ConsumeAction("keys.mod","spawn_hotkey"));
    REQUIRE(keyRegistry.Set("keys.mod","enabled",false));
    keyRegistry.QueueHotkeyKey(0x76);
    REQUIRE(!keyRegistry.ConsumeAction("keys.mod","spawn_hotkey"));
    REQUIRE(keyRegistry.Set("keys.mod","enabled",true));
    REQUIRE(keyRegistry.QueueAction("keys.mod","spawn_hotkey"));
    REQUIRE(keyRegistry.ConsumeAction("keys.mod","spawn_hotkey"));
    REQUIRE(keyRegistry.SaveProfile("hotkeys"));
    REQUIRE(json::parse(store.Snapshot())["profiles"]["hotkeys"]["keys.mod"]
            ["spawn_hotkey"]==0x76);
    m1arc::StateStore hotkeyRestart;
    hotkeyRestart.Configure(path);
    REQUIRE(hotkeyRestart.Ready());
    m1arc::Registry hotkeyRestored(hotkeyRestart);
    REQUIRE(hotkeyRestored.Register("keys.mod","Hotkeys",keysSchema));
    REQUIRE(hotkeyRestored.Get("keys.mod","spawn_hotkey").value()==0x76);
    REQUIRE(!hotkeyRestored.ConsumeAction("keys.mod","spawn_hotkey"));
    REQUIRE(keyRegistry.Reset("keys.mod","spawn_hotkey"));
    REQUIRE(keyRegistry.Get("keys.mod","spawn_hotkey").value()==0);
    keyRegistry.QueueHotkeyKey(0x76);
    REQUIRE(!keyRegistry.ConsumeAction("keys.mod","spawn_hotkey"));
    REQUIRE(keyRegistry.LoadProfile("hotkeys"));
    REQUIRE(keyRegistry.Get("keys.mod","spawn_hotkey").value()==0x76);
    auto invalidHotkey=keysSchema;
    invalidHotkey[1]["default"]=0x1B;
    REQUIRE(!keyRegistry.Register("invalid.key","Invalid",invalidHotkey));
    invalidHotkey=keysSchema;
    invalidHotkey[1]["min"]=0;
    REQUIRE(!keyRegistry.Register("invalid.constraints","Invalid",invalidHotkey));

    // Corrupt settings state must be read-only: profile/reset operations
    // must return useful errors and never pretend to persist live changes.
    const auto brokenPath = path.parent_path() / "broken.json";
    {
        std::ofstream broken(brokenPath, std::ios::binary | std::ios::trunc);
        broken << "{not-valid-json";
    }
    m1arc::StateStore frozenStore;
    frozenStore.Configure(brokenPath);
    REQUIRE(!frozenStore.Ready());
    m1arc::Registry frozenRegistry(frozenStore);
    REQUIRE(frozenRegistry.Register("frozen","Frozen",schema));
    REQUIRE(!frozenRegistry.Set("frozen","capacity",150));
    REQUIRE(!frozenRegistry.LastError().empty());
    REQUIRE(frozenRegistry.Get("frozen","capacity").value() == 50);
    REQUIRE(!frozenRegistry.ResetMod("frozen"));
    REQUIRE(!frozenRegistry.LastError().empty());
    REQUIRE(!frozenRegistry.SaveProfile("backup"));
    REQUIRE(!frozenRegistry.LastError().empty());
    REQUIRE(!frozenRegistry.LoadProfile("backup"));
    REQUIRE(!frozenRegistry.LastError().empty());
    REQUIRE(!frozenRegistry.DeleteProfile("backup"));
    REQUIRE(!frozenRegistry.LastError().empty());
    REQUIRE(std::ifstream(brokenPath).good());

    // Successful operations clear stale diagnostics; missing profiles are
    // reported without modifying the stored values.
    REQUIRE(!registry.DeleteProfile("nonexistent"));
    REQUIRE(registry.LastError() == "Profile not found");
    REQUIRE(registry.SaveProfile("temporary"));
    REQUIRE(registry.LastError().empty());
    REQUIRE(registry.DeleteProfile("temporary"));
    REQUIRE(registry.LastError().empty());

    fs::remove_all(path.parent_path(),ec);
    std::cout << "Native typed-settings registry tests passed\n";
}
