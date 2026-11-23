#include "../src/mod_manifest.hpp"
#include "check.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
void Write(const std::filesystem::path& path,const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    out<<content;
    REQUIRE(out.good());
}
void Make(const std::filesystem::path& root,const std::string& name,
          const std::string& id,const std::string& version,
          const std::string& minArc) {
    const auto folder=root/"mods"/name;
    nlohmann::json manifest{
        {"format",1},{"id",id},{"name","Weight Limit"},{"version",version},
        {"requires",{{"m1arc",minArc}}}
    };
    Write(folder/"mod.json",manifest.dump(2));
    // Explicit raw-string delimiter: "(kg)" would otherwise prematurely
    // terminate the short R"( ... )" syntax used in the first CI revision.
    Write(folder/"settings.json",R"JSON(
      [{"id":"enabled","kind":"boolean","default":true,"label":"Enabled"},
       {"id":"limit","kind":"integer","min":50,"max":5000,
        "default":500,"label":"Carry capacity (kg)"}]
    )JSON");
}
}
int main() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/"m1arc_manifest_test";
    std::error_code ec;
    fs::remove_all(root,ec);
    fs::create_directories(root,ec);
    REQUIRE(!ec);

    m1arc::StateStore store;
    store.Configure(root/"settings.json");
    REQUIRE(store.Ready());
    m1arc::Registry registry(store);
    REQUIRE(m1arc::LoadManifests(root,registry).registered==0);

    Make(root,"m1nsook.weight","m1nsook.weight","0.1.0","0.1.0");
    const auto valid=m1arc::LoadManifests(root,registry);
    REQUIRE(valid.registered==1);
    REQUIRE(valid.rejected==0);
    REQUIRE(registry.Get("m1nsook.weight","limit").value()==500);
    REQUIRE(registry.Set("m1nsook.weight","limit",900));
    REQUIRE(registry.Get("m1nsook.weight","limit").value()==900);

    // New Registry, same on-disk state: automatic registration restores value.
    m1arc::StateStore restart;
    restart.Configure(root/"settings.json");
    REQUIRE(restart.Ready());
    m1arc::Registry newRegistry(restart);
    REQUIRE(m1arc::LoadManifests(root,newRegistry).registered==1);
    REQUIRE(newRegistry.Get("m1nsook.weight","limit").value()==900);

    Make(root,"m1nsook.future","m1nsook.future","0.1.0","99.0.0");
    auto rejectFuture=m1arc::LoadManifests(root,newRegistry);
    REQUIRE(rejectFuture.registered==1);
    REQUIRE(rejectFuture.rejected==1);

    Make(root,"wrong.folder","m1nsook.wrong","0.1.0","0.1.0");
    auto rejectMismatch=m1arc::LoadManifests(root,newRegistry);
    REQUIRE(rejectMismatch.registered==1);
    REQUIRE(rejectMismatch.rejected==2);

    Make(root,"bad.json","bad.json","0.1.0","0.1.0");
    Write(root/"mods"/"bad.json"/"settings.json","{ broken");
    auto rejectSettings=m1arc::LoadManifests(root,newRegistry);
    REQUIRE(rejectSettings.registered==1);
    REQUIRE(rejectSettings.rejected==3);

    fs::remove_all(root,ec);
    std::cout<<"M1 Arc manifest discovery tests passed\n";
    return 0;
}
