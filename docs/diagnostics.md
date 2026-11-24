# Logs and diagnostics

Available in **M1 Arc 1.0.1 and later**. M1 Arc provides a separate diagnostic history for each registered mod, without requiring CET or an external log viewer.

## For players

1. Open M1 Arc with your assigned menu shortcut.
2. Click the clipboard icon in the top header to switch from settings to **Logs**.
3. Select the affected mod in the left sidebar.
4. Reproduce the problem in-game, reopen the panel and select that mod again.
5. Click **Copy diagnostics** and paste the report into your support message.

Click the header switch again to return to settings. The selected mod remains the same.

**Clear logs** removes only the selected mod's session history, after confirmation. It does not change settings, clear pending actions or delete RED4ext/REDscript log files. Copy diagnostics remains available even when no mod is registered or its history is empty.

Reports contain:

- M1 Arc version, detected game executable and RED4ext DLL file versions (when available).
- Selected mod name, ID, manifest version and current settings.
- Settings storage status and revision.
- Pending action counts, framework startup diagnostics and manifest rejection reasons.
- Timestamped diagnostic entries for the selected mod, newest first.

Review the report before sharing it. The selected mod's settings and mod-authored messages may contain information you do not want to publish. The report may also contain other mods' manifest rejection reasons in its framework startup section. Share it on [Nexus Mods discussions](https://www.nexusmods.com/cyberpunk2077/mods/39311?tab=posts) or [GitHub Issues](https://github.com/m1nsook/m1arc/issues), with steps to reproduce the issue.

## For mod authors

Declare a minimum M1 Arc version of 1.0.1 in your [manifest](/mod-manifest) when using the new native function:

~~~json
"requires": { "m1arc": "1.0.1" }
~~~

The native declaration, provided by M1 Arc itself, is:

~~~reds
native func M1Arc_Log(modId: String, level: String, message: String) -> Bool;
~~~

Call it using your own registered mod ID (not the display name). The function returns true if the entry was accepted, or false for an unregistered ID, unsupported level, empty or oversized message, or logging failure.

| Level | Use |
| --- | --- |
| DEBUG | State transitions and diagnostic details |
| INFO | Initialization or confirmed successful operations |
| WARNING | Recoverable problems, missing optional data |
| ERROR | Failed operations and their actual reasons |

Uppercase or lowercase level names are supported, including WARN/warn as aliases for WARNING. Messages may be at most 2048 UTF-8 bytes.

### Write useful failure messages

~~~reds
M1Arc_Log("example.mod", "INFO", "Initialization completed");
M1Arc_Log("example.mod", "DEBUG", "CONVERT_START: requested item conversion");
M1Arc_Log("example.mod", "ERROR", "CONVERT_FAILED: inventory system unavailable");
~~~

These calls are illustrative: only emit the failure message after your code actually detects that condition. Use a stable error code, describe the failing operation and include relevant context, such as the requested item ID, expected state or observed reason. Record confirmed success only after the game operation succeeds.

Do not log every frame, every unsuccessful polling call, secrets, or unnecessary personal information. Prefer logging once per meaningful state transition. A failed log call should not cause gameplay failure or repeated logging.

### What M1 Arc detects automatically

The framework records registration, action requests and consumption, menu setting changes and certain framework errors. Examples include:

| Code | Meaning |
| --- | --- |
| ARC_SETTING_UNKNOWN | Setting ID was not registered |
| ARC_SETTING_INVALID | Value did not match the registered schema |
| ARC_SETTING_SAVE_FAILED | Persistence failed; the previous value was preserved |
| ARC_DEFAULTS_SAVE_FAILED | Restoring defaults could not be persisted |
| ARC_ACTION_UNKNOWN | Requested action was not declared |
| ARC_ACTION_QUEUE_FULL | 16 requests for the action are already pending |
| ARC_MANIFEST_REJECTED | Manifest or settings registration failed at startup |

An action consumed by REDscript is **not** proof of gameplay success. M1 Arc cannot automatically determine the cause of every internal mod failure; the mod must call the logging API at the relevant failure branch. Existing mods remain compatible without logging changes.

### Retention and limitations

History is held in memory for the current game session: up to 256 entries per mod, 4096 globally, with older entries evicted when limits are reached. Restarting the game clears the session history. Copy diagnostics before restarting when possible.

The viewer is not a crash dump, automatic log-file importer or replacement for RED4ext/REDscript logs. If the game or overlay cannot start, use the original framework log files and [troubleshooting guide](/troubleshooting).

See the complete [native API](/native-api).
