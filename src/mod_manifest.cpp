#include "mod_manifest.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

namespace m1arc {
namespace {
using json = nlohmann::json;
constexpr std::size_t kMaxMods = 128;
constexpr std::uintmax_t kMaxJsonBytes = 256 * 1024;
constexpr std::array<int, 3> kM1ArcVersion{0, 1, 0};

bool ValidId(const std::string& name) {
    return !name.empty() && name.size() <= 96 &&
        std::all_of(name.begin(),name.end(),[](unsigned char c) {
            return (c>='a' && c<='z') || (c>='A' && c<='Z') ||
                   (c>='0' && c<='9') || c=='.' || c=='_' || c=='-';
        });
}

bool ParseVersion(const std::string& text,std::array<int,3>& version) {
    if (text.empty() || text.size() > 32) return false;
    std::size_t at=0;
    for (int part=0;part<3;part++) {
        if (at>=text.size() || !std::isdigit(static_cast<unsigned char>(text[at])))
            return false;
        int value=0;
        int digits=0;
        while (at<text.size() &&
               std::isdigit(static_cast<unsigned char>(text[at]))) {
            if (++digits>5) return false;
            value=value*10+(text[at++]-'0');
        }
        version[part]=value;
        if (part<2) {
            if (at>=text.size() || text[at++]!='.') return false;
        }
    }
    return at==text.size();
}

bool ReadJsonFile(const std::filesystem::path& path,json& output) {
    std::error_code ec;
    if (std::filesystem::is_symlink(path,ec) || ec) return false;
    if (!std::filesystem::is_regular_file(path,ec) || ec) return false;
    const auto bytes=std::filesystem::file_size(path,ec);
    if (ec || bytes==0 || bytes>kMaxJsonBytes) return false;
    std::ifstream stream(path,std::ios::binary);
    if (!stream) return false;
    try {
        output=json::parse(stream);
        return true;
    } catch (...) { return false; }
}

void Reject(ModLoadReport& report,const std::string& folder,const char* reason) {
    ++report.rejected;
    report.errors.push_back(folder+": "+reason);
}

} // namespace

ModLoadReport LoadManifests(const std::filesystem::path& pluginDirectory,
                            Registry& registry) {
    ModLoadReport report;
    const auto root=pluginDirectory / L"mods";
    std::error_code ec;
    if (!std::filesystem::exists(root,ec)) return report;
    if (ec || !std::filesystem::is_directory(root,ec) || ec) {
        Reject(report,"mods","cannot read mod directory");
        return report;
    }

    // Deterministic traversal, with no recursive discovery or executable
    // entrypoints. The manifest is NOT a new scripting language.
    std::vector<std::filesystem::path> dirs;
    for (std::filesystem::directory_iterator it(root,ec),end;
         !ec && it!=end;it.increment(ec)) {
        if (it->is_symlink(ec)) { ec.clear(); continue; }
        if (it->is_directory(ec)) {
            if (dirs.size() >= kMaxMods) {
                Reject(report,"mods","too many mod directories (max 128)");
                break;
            }
            dirs.push_back(it->path());
        }
        ec.clear();
    }
    if (ec) Reject(report,"mods","directory enumeration failed");
    std::sort(dirs.begin(),dirs.end());

    for (const auto& folder:dirs) {
        const auto id=folder.filename().string();
        try {
        if (!ValidId(id)) {
            Reject(report,id,"invalid mod folder name");
            continue;
        }
        json manifest;
        if (!ReadJsonFile(folder / L"mod.json",manifest) ||
            !manifest.is_object()) {
            Reject(report,id,"missing or invalid mod.json");
            continue;
        }
        if (!manifest.contains("format") ||
            !manifest["format"].is_number_integer() ||
            manifest["format"].get<int>()!=1 ||
            !manifest.contains("id") || !manifest["id"].is_string() ||
            manifest["id"].get<std::string>()!=id ||
            !manifest.contains("name") || !manifest["name"].is_string() ||
            !manifest.contains("version") || !manifest["version"].is_string()) {
            Reject(report,id,"invalid manifest fields or folder/id mismatch");
            continue;
        }
        std::array<int,3> modVersion{};
        if (!ParseVersion(manifest["version"].get<std::string>(),modVersion)) {
            Reject(report,id,"invalid mod version");
            continue;
        }
        if (manifest.contains("requires")) {
            const auto& req=manifest["requires"];
            if (!req.is_object() || !req.contains("m1arc") ||
                !req["m1arc"].is_string()) {
                Reject(report,id,"invalid requires.m1arc version");
                continue;
            }
            std::array<int,3> minimum{};
            if (!ParseVersion(req["m1arc"].get<std::string>(),minimum) ||
                minimum>kM1ArcVersion) {
                Reject(report,id,"requires newer M1 Arc");
                continue;
            }
        }
        json settings;
        // Fixed filename: no untrusted executable names or paths in manifests.
        if (!ReadJsonFile(folder / L"settings.json",settings) ||
            !registry.Register(id,manifest["name"].get<std::string>(),settings)) {
            Reject(report,id,"invalid settings.json or registration failed");
            continue;
        }
        ++report.registered;
        } catch (...) {
            // Malformed inputs must NEVER unwind across the RED4ext DLL entrypoint.
            Reject(report,id,"invalid manifest or settings field type");
        }
    }
    return report;
}

} // namespace m1arc
