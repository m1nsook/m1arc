#include "registry.hpp"
#include "log_view.hpp"
#include "check.hpp"
#include <filesystem>
using namespace m1arc;
int main() {
    StateStore state;
    Registry registry(state);
    const auto schema=nlohmann::json::array({{{"id","enabled"},{"kind","boolean"},{"default",true}}});
    REQUIRE(registry.Register("one","First",schema,"Gameplay","","1.2.3"));
    REQUIRE(registry.Register("two","Second",schema));
    REQUIRE(!registry.AppendLog("missing",LogLevel::Info,"message"));
    REQUIRE(!registry.AppendLog("one",LogLevel::Info,""));
    REQUIRE(!registry.AppendLog("one",LogLevel::Info,std::string(2049,'a')));
    REQUIRE(ParseLogLevel("warning")==LogLevel::Warning);
    REQUIRE(!ParseLogLevel("bad"));
    registry.SetDiagnosticEnvironment("3.0.88.46235","test-loader");
    registry.AddStartupDiagnostic("ARC_MANIFEST_REJECTED: bad.mod: invalid settings.json");
    REQUIRE(!registry.Set("one","not_registered",true));
    REQUIRE(!registry.Set("one","enabled","bad"));
    REQUIRE(!registry.Set("one","enabled",false)); // unconfigured store
    const auto report=registry.DiagnosticReport("one");
    REQUIRE(report.find("Manifest version: 1.2.3")!=std::string::npos);
    REQUIRE(report.find("ARC_SETTING_UNKNOWN")!=std::string::npos);
    REQUIRE(report.find("ARC_SETTING_INVALID")!=std::string::npos);
    REQUIRE(report.find("ARC_SETTING_SAVE_FAILED")!=std::string::npos);
    REQUIRE(report.find("ARC_MANIFEST_REJECTED")!=std::string::npos);
    REQUIRE(registry.DiagnosticReport("two").find("ARC_SETTING_INVALID")==std::string::npos);
    REQUIRE(registry.DiagnosticReport("").find("ARC_MANIFEST_REJECTED")!=std::string::npos);
    REQUIRE(registry.ClearLogs("one"));
    REQUIRE(registry.ClearLogs("two"));
    REQUIRE(registry.AppendLog("one",LogLevel::Info,"first"));
    REQUIRE(registry.AppendLog("one",LogLevel::Error,"second"));
    REQUIRE(registry.AppendLog("two",LogLevel::Warning,"other"));
    auto rows=registry.Logs("one");
    REQUIRE(rows.size()==2 && rows[0].message=="second" && rows[1].message=="first");
    REQUIRE(rows[0].level==LogLevel::Error && !rows[0].timestamp.empty());
    const auto counts=logview::Counts(rows);
    REQUIRE(counts[0]==0 && counts[1]==1 && counts[2]==0 && counts[3]==1);
    REQUIRE(logview::Filter(rows,std::nullopt,"SECOND")==std::vector<std::size_t>{0});
    REQUIRE(logview::Filter(rows,LogLevel::Info,"first")==std::vector<std::size_t>{1});
    REQUIRE(logview::Filter(rows,LogLevel::Error,"first").empty());
    REQUIRE(logview::Filter(rows,std::nullopt,"").size()==2);
    // A copied paused view is independent of further registry activity.
    const auto frozen=rows;
    REQUIRE(registry.AppendLog("one",LogLevel::Debug,"new while paused"));
    REQUIRE(frozen.size()==2 && registry.Logs("one").size()==3);
    REQUIRE(registry.DiagnosticReport("one").find("new while paused")!=std::string::npos);
    REQUIRE(registry.ClearLogs("one"));
    REQUIRE(registry.Logs("one").empty());
    REQUIRE(registry.Logs("two").size()==1);
    for (std::size_t i=0;i<Registry::kLogsPerMod+10;++i)
        REQUIRE(registry.AppendLog("one",LogLevel::Debug,std::to_string(i)));
    rows=registry.Logs("one");
    REQUIRE(rows.size()==Registry::kLogsPerMod);
    REQUIRE(rows.back().message=="10");
    REQUIRE(!registry.ClearLogs("missing"));
    // Global retention remains bounded even with many independent mods.
    for (int i=0;i<17;++i) {
        const std::string id="many."+std::to_string(i);
        REQUIRE(registry.Register(id,id,schema));
        for (std::size_t j=0;j<Registry::kLogsPerMod;++j)
            REQUIRE(registry.AppendLog(id,LogLevel::Info,std::to_string(j)));
    }
    std::size_t retained=0;
    for (const auto& mod:registry.Snapshot()) retained+=registry.Logs(mod.id).size();
    REQUIRE(retained==Registry::kLogsTotal);
    REQUIRE(registry.Logs("many.16").size()==Registry::kLogsPerMod);
    REQUIRE(registry.ClearLogs("many.16"));
    REQUIRE(registry.AppendLog("many.16",LogLevel::Info,"after clear"));
    REQUIRE(registry.Logs("many.16").size()==1);
    Registry fresh(state);
    REQUIRE(fresh.Logs("one").empty());
}
