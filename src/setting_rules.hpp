#pragma once
#include "registry.hpp"
#include <algorithm>
#include <vector>
#include <string>

namespace m1arc::rules {
inline bool Master(const SettingView& s) {
    return s.kind==SettingKind::Boolean && (s.id=="enabled" || s.id=="active");
}
inline bool SameGroup(const SettingView& a,const SettingView& b) {
    return a.tab==b.tab && (a.groupKey.empty()?a.group:a.groupKey)==(b.groupKey.empty()?b.group:b.groupKey);
}
inline std::size_t Find(const ModView& m,std::string_view id) {
    for (std::size_t i=0;i<m.settings.size();++i) if (m.settings[i].id==id) return i;
    return m.settings.size();
}
inline std::vector<std::size_t> Parents(const ModView& m,std::size_t i) {
    const auto& s=m.settings[i];
    std::vector<std::size_t> parents;
    if (!s.enabledBy.empty()) parents.push_back(Find(m,s.enabledBy));
    if (!Master(s) && !s.controlsGroup)
        for (std::size_t j=0;j<m.settings.size();++j)
            if (j!=i && m.settings[j].controlsGroup && SameGroup(s,m.settings[j])) parents.push_back(j);
    return parents;
}
inline bool Visit(const ModView& m,std::size_t i,std::vector<int>& marks) {
    if (i>=marks.size() || marks[i]==1) return false;
    if (marks[i]==2) return true;
    marks[i]=1;
    for (auto j:Parents(m,i)) if (!Visit(m,j,marks)) return false;
    marks[i]=2;
    return true;
}
inline bool Valid(const ModView& m) {
    std::vector<int> marks(m.settings.size());
    for (std::size_t i=0;i<m.settings.size();++i) {
        const auto& s=m.settings[i];
        if (Master(s) && !s.enabledBy.empty()) return false;
        if (s.controlsGroup)
            for (std::size_t j=0;j<i;++j)
                if (m.settings[j].controlsGroup && SameGroup(s,m.settings[j])) return false;
        if (!Visit(m,i,marks)) return false;
    }
    return true;
}
// Return the first unmet dependency by stable ID; empty means interactive.
// The master switch remains reachable even if another section is disabled.
inline std::string Blocked(const ModView& m,std::size_t i,std::vector<int>& marks) {
    if (i>=m.settings.size()) return "unknown";
    const auto& s=m.settings[i];
    if (Master(s)) return {};
    if (marks[i]==1) return s.id;
    marks[i]=1;
    for (const auto& parent:m.settings)
        if (Master(parent) && parent.value==false) { marks[i]=0; return parent.id; }
    for (auto j:Parents(m,i)) {
        if (j>=m.settings.size()) { marks[i]=0; return s.enabledBy; }
        const auto& parent=m.settings[j];
        const bool group=parent.controlsGroup && SameGroup(s,parent) && !s.controlsGroup;
        if ((group && parent.value!=true) || (parent.id==s.enabledBy && parent.value!=s.enabledEquals)) {
            marks[i]=0; return parent.id;
        }
        const auto upstream=Blocked(m,j,marks);
        if (!upstream.empty()) { marks[i]=0; return upstream; }
    }
    marks[i]=0;
    return {};
}
inline std::string Blocked(const ModView& m,const SettingView& s) {
    std::vector<int> marks(m.settings.size());
    return Blocked(m,Find(m,s.id),marks);
}
}
