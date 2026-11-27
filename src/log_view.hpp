#pragma once
#include "registry.hpp"
#include <array>
#include <optional>
#include <string_view>
#include <vector>

namespace m1arc::logview {
inline std::size_t Index(LogLevel level) {
    switch(level) {
    case LogLevel::Debug: return 0;
    case LogLevel::Info: return 1;
    case LogLevel::Warning: return 2;
    case LogLevel::Error: return 3;
    }
    return 0;
}
inline std::array<std::size_t,4> Counts(const std::vector<LogEntry>& rows) {
    std::array<std::size_t,4> counts{};
    for (const auto& row:rows) ++counts[Index(row.level)];
    return counts;
}
inline unsigned char Fold(unsigned char c) {
    return c>='A' && c<='Z' ? static_cast<unsigned char>(c+('a'-'A')) : c;
}
// ASCII case-insensitive, non-ASCII UTF-8 byte sequences are matched exactly.
inline bool Contains(std::string_view haystack,std::string_view needle) {
    if (needle.empty()) return true;
    if (needle.size()>haystack.size()) return false;
    for (std::size_t i=0;i<=haystack.size()-needle.size();++i) {
        std::size_t j=0;
        while (j<needle.size() && Fold(haystack[i+j])==Fold(needle[j])) ++j;
        if (j==needle.size()) return true;
    }
    return false;
}
inline std::vector<std::size_t> Filter(const std::vector<LogEntry>& rows,
        std::optional<LogLevel> level,std::string_view query) {
    std::vector<std::size_t> result;
    result.reserve(rows.size());
    for (std::size_t i=0;i<rows.size();++i) {
        const auto& row=rows[i];
        if ((!level || row.level==*level) && (Contains(row.message,query) ||
            Contains(row.timestamp,query) || Contains(LogLevelName(row.level),query) ||
            (row.level==LogLevel::Warning && Contains("WARN",query)))) result.push_back(i);
    }
    return result;
}
}
