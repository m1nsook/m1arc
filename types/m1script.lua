---@meta
-- m1script Lua Language Server definitions; editor-only, NEVER install as a mod.
-- Reference: api/contract.json. Runtime functions are supplied by m1core.dll.

---@alias M1ScriptEvent
---| '"onHook"'
---| '"onTweak"'
---| '"onInit"'
---| '"onUpdate"'
---| '"onDraw"'
---| '"onOverlayOpen"'
---| '"onOverlayClose"'
---| '"onShutdown"'

---@class M1ScriptRegistry
---@field registerJson fun(modId:string,title:string,schemaJson:string):boolean
---@field getJson fun(modId:string,settingId:string):string
---@field setJson fun(modId:string,settingId:string,valueJson:string):boolean
---@field resetSetting fun(modId:string,settingId:string):boolean
---@field resetMod fun(modId:string):boolean
---@field saveProfile fun(name:string):boolean
---@field loadProfile fun(name:string):boolean
---@field deleteProfile fun(name:string):boolean
---@field profiles fun():string[]
---@field lastError fun():string
m1core = {} ---@type M1ScriptRegistry

---@param eventName M1ScriptEvent
---@param callback fun(deltaTime?:number)
function registerForEvent(eventName, callback) end

---@param id string
---@param name string
---@param description string
---@param callback fun()
---@overload fun(id:string,name:string,callback:fun())
function registerHotkey(id,name,description,callback) end

---@param id string
---@param name string
---@param description string
---@param callback fun(isDown:boolean)
---@overload fun(id:string,name:string,callback:fun(isDown:boolean))
function registerInput(id,name,description,callback) end

---@param id string
---@return boolean
function IsBound(id) end

---@param id string
---@return string
function GetBind(id) end

---@return string
function GetVersion() end

---@return number width
---@return number height
function GetDisplayResolution() end

---@param archiveName string
---@return boolean
function ModArchiveExists(archiveName) end
