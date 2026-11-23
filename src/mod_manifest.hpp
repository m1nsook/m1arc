#pragma once

#include "registry.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace m1arc {

// Declarative manifests are data only. The native RED4ext plugin reads them;
// gameplay changes remain in each mod's ordinary REDscript files.
struct ModLoadReport {
    std::size_t registered = 0;
    std::size_t rejected = 0;
    std::vector<std::string> errors;
};

// Scans <m1arc dll directory>/mods/<mod-id>/mod.json at startup.
// Strict folder/id matching and fixed settings.json paths prevent traversal.
// Failures in one mod do not prevent the others from loading.
ModLoadReport LoadManifests(const std::filesystem::path& pluginDirectory,
                            Registry& registry);
} // namespace m1arc
