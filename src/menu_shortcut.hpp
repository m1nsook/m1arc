#pragma once
namespace m1arc {
constexpr int kMenuCtrl=1, kMenuShift=2, kMenuModifiers=3;
constexpr int PackMenuShortcut(int key,int mods) { return key | (mods<<8); }
constexpr int MenuKey(int shortcut) { return shortcut & 0xff; }
constexpr int MenuMods(int shortcut) { return shortcut >> 8; }
constexpr int kRecoveryShortcut=PackMenuShortcut(0x7a,kMenuModifiers);
constexpr bool IsMenuKey(int key) {
    if (key<0x08 || key>=0xe7) return false;
    switch(key) {
    case 0x10: case 0x11: case 0x12: case 0x1b:
    case 0x5b: case 0x5c: case 0x5f:
    case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5:
    case 0xe5: return false;
    default: return true;
    }
}
constexpr bool ValidMenuShortcut(int key,int mods) {
    return IsMenuKey(key) && mods>=0 && mods<=kMenuModifiers &&
           PackMenuShortcut(key,mods)!=kRecoveryShortcut;
}
constexpr bool MatchesMenuShortcut(int shortcut,int key,int mods) {
    return shortcut!=0 && ValidMenuShortcut(key,mods) && shortcut==PackMenuShortcut(key,mods);
}
class MenuCapture {
public:
    void Reset() { armed_=false; candidate_=0; }
    void Arm() { Reset(); armed_=true; }
    bool Armed() const { return armed_; }
    int Candidate() const { return candidate_; }
    void Offer(int key,int mods) {
        if (!armed_ || !ValidMenuShortcut(key,mods)) return;
        candidate_=PackMenuShortcut(key,mods);
        armed_=false;
    }
private:
    bool armed_=false;
    int candidate_=0;
};
class MenuPress {
public:
    void Reset() { pending_=0; }
    void Down(int shortcut,int key,int mods) {
        if (MatchesMenuShortcut(shortcut,key,mods)) pending_=key;
    }
    bool Up(int key) {
        if (!pending_ || pending_!=key) return false;
        Reset();
        return true;
    }
private:
    int pending_=0;
};
}
