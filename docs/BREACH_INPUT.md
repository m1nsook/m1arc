# Breach Protocol native selection interface (experimental)

The native M1 Arc plugin exposes **one narrowly-scoped internal action**:

- \`M1Arc_BreachInputAvailable() -> Bool\` — true only when the exact
  Cyberpunk 2077 2.5 executable version and a unique verified native
  function-entry fingerprint are present.
- \`M1Arc_BreachSelectCell(wref<inkLogicController>, Vector2)\` — supplies a
  native (row, column) selection to the game's hacking logic controller.

**Skip Breach runtime log (development builds):** the paired addon declares
`M1Arc_BreachTrace(message: String) -> Bool` next to the native selector
functions. The M1 Arc DLL registers this narrow native and forwards REDscript
controller/capture/state/acknowledgement messages to the **same RED4ext
M1 Arc plugin log** at `Cyberpunk 2077/red4ext/logs/m1arc*.log`, prefixed
`[Skip Breach]`. The old `LogChannel(DEBUG)` was not readable in that log.
The bridge is diagnostic only: max 480 message bytes per call, no
scripting engine and no modification of the game/puzzle. Every dispatched
cell is logged with row/column even when consecutive dispatches have the
same native state (previously `ActionState` hid them). The REDscript
consumer still waits for its real game callback; a native "dispatched"
line is **not proof** that the game acknowledged it.

**Match DLL + addon revisions.** Older M1 Arc DLLs do not register
`M1Arc_BreachTrace`. Don't install only the newer addon, or the optional
REDscript native declaration can fail validation at startup.

**Reference source:** the user's supplied AutoBreach 0.4.0 game-root ZIP
for Cyberpunk 2077 2.5. We learned the *interface*, game version and
entry signature, but do not redistribute its DLL, Lua or REDscript.
M1 Arc's implementation is standalone C++.

**Native declaration correction:** The previous native `@addMethod`
declaration triggered a RED4ext invalid-native-definition startup error.
This version registers a global RTTI native that receives the weak hacking
logic controller explicitly. Its experimental REDscript declarations
are shipped **inside Skip Breach** (`M1ArcBreachNative.reds`), not in the
working M1 Arc core `M1Arc.reds` declarations. Update the M1 Arc DLL and
install the matching Skip Breach package together. Native script validation
still needs an in-game confirmation.

**Safeguards:** fail closed on any different executable version, missing
signature, duplicate signature, non-function-entry match, inactive game
window, open M1 Arc overlay/keybinding dialog, incorrect logic controller
RTTI type, non-finite or out-of-bounds/non-integer coordinates. There is
no generic mouse movement or raw input injection.

A return value of \`true\` means the call was *dispatched* — not that the
game accepted it. The Skip Breach REDscript consumer **must** observe its
own \`HackingMinigameGameController.OnPositionSelected(Vector2)\` event and
advance only after matching the acknowledgement. Gamepad/cursor input
must not be intercepted.

**Compatibility:** fingerprint and version checks are based on the
observed user reference, NOT a successfully tested live game executable
by this project. Native compilation alone cannot guarantee that direct
calling is safe on a user's installation; keep the draft and test on a
version-matched copy of game 2.5 before release.

This interface is an optional helper. M1 Arc settings and manifests
continue to function when it is unavailable.
