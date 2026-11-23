# Architecture and ownership

| Responsibility | Repository |
| --- | --- |
| Public scripting APIs, reference, examples, editor definitions | [m1script](https://github.com/m1nsook/m1script) |
| RED4ext DLL, embedded LuaJIT VM, sandbox, native RTTI binding and game hooks | [m1core](https://github.com/m1nsook/CP2077/tree/dev/m1core-cet-foundation/m1core) |
| Game overlay, input, persistence and game-root ZIP | m1core |
| Future API specifications | m1script first; native bindings implemented in m1core |

m1script is the scripting **authoring and API repository**, not a new runtime dependency. Existing `.m1s`, `.m1c` and `.lua` mods continue to run through m1core.

## Licensing

m1script has **GPL-3.0** licensing; the m1core standalone DLL has an **MIT** project license. Public API specifications and tooling can live here while runtime C++ remains in m1core. Moving native implementations into a shared component would require a deliberate licensing decision first. Features not yet implemented in m1core must be marked as proposed rather than advertised as usable.

