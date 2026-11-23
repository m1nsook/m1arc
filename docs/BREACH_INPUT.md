# Breach Protocol native selection interface (experimental)

The native M1 Arc plugin exposes **one narrowly-scoped internal action**:

- \`M1Arc_BreachInputAvailable() -> Bool\` — true only when the exact
  Cyberpunk 2077 2.5 executable version and a unique verified native
  function-entry fingerprint are present.
- \`M1Arc_BreachSelectCell(wref<inkLogicController>, Vector2)\` — supplies a
  native (row, column) selection to the game's hacking logic controller.

**Reference source:** the user's supplied AutoBreach 0.4.0 game-root ZIP
for Cyberpunk 2077 2.5. We learned the *interface*, game version and
entry signature, but do not redistribute its DLL, Lua or REDscript.
M1 Arc's implementation is standalone C++.

**Native declaration correction:** The previous native `@addMethod`
declaration triggered a RED4ext invalid-native-definition startup error.
This version registers a global RTTI native that receives the weak hacking
logic controller explicitly. It requires reinstallation of both the DLL
and the matching `M1Arc.reds`. Native script validation still needs an
in-game confirmation.

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
