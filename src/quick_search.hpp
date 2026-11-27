#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace m1arc::quick {
// Inputs are folded by the caller. Prefer exact/prefix matches, then
// substring, then bounded one-edit typos and subsequences.
inline bool OneEdit(std::string_view a,std::string_view b) {
    if (a.size()<b.size()) std::swap(a,b);
    if (a.size()>b.size()+1) return false;
    if (a.size()==b.size()) {
        std::size_t i=0;
        while(i<a.size() && a[i]==b[i]) ++i;
        if(i+1<a.size() && a[i]==b[i+1] && a[i+1]==b[i] && a.substr(i+2)==b.substr(i+2)) return true;
    }
    std::size_t i=0,j=0; int edits=0;
    while(i<a.size() && j<b.size()) {
        if(a[i]==b[j]) {++i;++j;continue;}
        if(++edits>1) return false;
        if(a.size()==b.size()) ++j;
        ++i;
    }
    return edits+static_cast<int>(a.size()-i)<=1;
}
inline int Token(std::string_view text,std::string_view word) {
    if(text==word) return 120;
    if(text.starts_with(word)) return 100;
    if(text.find(word)!=std::string_view::npos) return 80;
    if(word.size()<3) return 0;
    for(std::size_t i=0;i<text.size();) {
        auto end=text.find_first_of(" /-_.,:;|",i);
        if(end==std::string_view::npos) end=text.size();
        if(OneEdit(text.substr(i,end-i),word)) return 50;
        i=end+1;
    }
    std::size_t at=0;
    for(char c:text) if(at<word.size() && c==word[at]) ++at;
    return at==word.size()?20:0;
}
inline int Score(std::string_view name,std::string_view text,std::string_view query) {
    int total=0;
    for(std::size_t i=0;i<query.size();) {
        auto end=query.find(' ',i);
        if(end==std::string_view::npos) end=query.size();
        const auto word=query.substr(i,end-i);
        if(!word.empty()) {
            const int score=std::max(Token(name,word),Token(text,word)/2);
            if(!score) return -1;
            total+=score;
        }
        i=end+1;
    }
    return total;
}
}
