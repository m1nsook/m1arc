#include "registry.hpp"
#include "check.hpp"
#include <filesystem>
using namespace m1arc;
int main() {
    StateStore state;
    Registry registry(state);
    const auto schema=nlohmann::json::array({{{"id","enabled"},{"kind","boolean"},{"default",true}}});
    REQUIRE(registry.Register("one","First",schema));
    REQUIRE(registry.Register("two","Second",schema));
    REQUIRE(!registry.AppendLog("missing",LogLevel::Info,"message"));
    REQUIRE(!registry.AppendLog("one",LogLevel::Info,""));
    REQUIRE(!registry.AppendLog("one",LogLevel::Info,std::string(2049,'a')));
    REQUIRE(ParseLogLevel("warning")==LogLevel::Warning);
    REQUIRE(!ParseLogLevel("bad"));
    REQUIRE(registry.ClearLogs("one"));
    REQUIRE(registry.ClearLogs("two"));
    REQUIRE(registry.AppendLog("one",LogLevel::Info,"first"));
    REQUIRE(registry.AppendLog("one",LogLevel::Error,"second"));
    REQUIRE(registry.AppendLog("two",LogLevel::Warning,"other"));
    auto rows=registry.Logs("one");
    REQUIRE(rows.size()==2 && rows[0].message=="second" && rows[1].message=="first");
    REQUIRE(rows[0].level==LogLevel::Error && !rows[0].timestamp.empty());
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
