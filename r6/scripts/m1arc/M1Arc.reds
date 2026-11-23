// M1 Arc: native RED4ext functions for regular REDscript mods.
// These are declarations only. Implementation lives in m1arc.dll.
// Install only alongside the matching RED4ext plugin.
native func M1Arc_Version() -> String;
native func M1Arc_RegisterJson(modId: String, displayName: String, schemaJson: String) -> Bool;
native func M1Arc_GetJson(modId: String, settingId: String) -> String;
native func M1Arc_GetBool(modId: String, settingId: String) -> Bool;
native func M1Arc_GetInt(modId: String, settingId: String) -> Int32;
native func M1Arc_GetFloat(modId: String, settingId: String) -> Float;
native func M1Arc_GetString(modId: String, settingId: String) -> String;
native func M1Arc_Revision() -> Int32;
native func M1Arc_SetJson(modId: String, settingId: String, payload: String) -> Bool;
native func M1Arc_SetBool(modId: String, settingId: String, value: Bool) -> Bool;
native func M1Arc_SetInt(modId: String, settingId: String, value: Int32) -> Bool;
native func M1Arc_SetFloat(modId: String, settingId: String, value: Float) -> Bool;
native func M1Arc_SetString(modId: String, settingId: String, value: String) -> Bool;
native func M1Arc_ResetSetting(modId: String, settingId: String) -> Bool;
native func M1Arc_ResetMod(modId: String) -> Bool;
native func M1Arc_SaveProfile(name: String) -> Bool;
native func M1Arc_LoadProfile(name: String) -> Bool;
native func M1Arc_DeleteProfile(name: String) -> Bool;

// Optional internal game-action bridge for compatible Cyberpunk 2077 2.5
// builds. False on unsupported versions or signature mismatch.
native func M1Arc_BreachInputAvailable() -> Bool;
// Global native declaration, like the proven M1 Arc typed settings API.
// Pass the weak game-controller handle explicitly instead of declaring
// a native method added to an engine-owned class.
native func M1Arc_BreachSelectCell(controller: wref<inkLogicController>, position: Vector2) -> Bool;
