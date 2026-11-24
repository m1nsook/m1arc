#include "menu_shortcut.hpp"
#include "check.hpp"
using namespace m1arc;
int main() {
    constexpr int f10=0x79, f11=0x7a, space=0x20;
    REQUIRE(ValidMenuShortcut(space,0));
    REQUIRE(ValidMenuShortcut(f10,0));
    REQUIRE(ValidMenuShortcut(f10,kMenuCtrl));
    REQUIRE(ValidMenuShortcut(f10,kMenuShift));
    REQUIRE(ValidMenuShortcut(f10,kMenuModifiers));
    REQUIRE(!ValidMenuShortcut(f11,kMenuModifiers));
    REQUIRE(!ValidMenuShortcut(0x1b,0));
    REQUIRE(!ValidMenuShortcut(0x11,0));
    REQUIRE(!ValidMenuShortcut(f10,4));
    REQUIRE(MatchesMenuShortcut(PackMenuShortcut(f10,0),f10,0));
    REQUIRE(!MatchesMenuShortcut(PackMenuShortcut(f10,0),f10,kMenuShift));
    REQUIRE(!MatchesMenuShortcut(PackMenuShortcut(f10,kMenuCtrl),f10,0));
    MenuCapture capture;
    capture.Offer(space,0);
    REQUIRE(capture.Candidate()==0);
    capture.Arm();
    capture.Offer(0x11,0);
    capture.Offer(f11,kMenuModifiers);
    REQUIRE(capture.Armed());
    capture.Offer(f10,kMenuCtrl);
    REQUIRE(!capture.Armed());
    REQUIRE(capture.Candidate()==PackMenuShortcut(f10,kMenuCtrl));
    capture.Offer(space,0);
    REQUIRE(capture.Candidate()==PackMenuShortcut(f10,kMenuCtrl));
    capture.Reset();
    REQUIRE(capture.Candidate()==0);
    MenuPress press;
    REQUIRE(!press.Up(f10));
    press.Down(PackMenuShortcut(f10,kMenuShift),f10,0);
    REQUIRE(!press.Up(f10));
    press.Down(PackMenuShortcut(f10,kMenuShift),f10,kMenuShift);
    REQUIRE(!press.Up(0xa0));
    REQUIRE(press.Up(f10));
    REQUIRE(!press.Up(f10));
    press.Down(PackMenuShortcut(f10,0),f10,0);
    press.Reset();
    REQUIRE(!press.Up(f10));
}
