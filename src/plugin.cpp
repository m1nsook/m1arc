// M1 Arc RED4ext plugin: native REDscript settings and standalone ImGui.
// CET may coexist; we neither depend on it nor modify its state.
#include "registry.hpp"
#include "mod_manifest.hpp"
#include "localization.hpp"
#include "dev_fixture.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "breach_select.hpp"
#include "state.hpp"


#include <RED4ext/Api/v1/GameState.hpp>
#include <RED4ext/GameStates.hpp>

#include <RED4ext/RED4ext.hpp>
#include <Windows.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <cmath>
#include <limits>
#include <string>

namespace {
using RED4ext::CString;
using RED4ext::CStackFrame;
using RED4ext::IScriptable;

void Version(IScriptable*,CStackFrame* frame,CString* output,std::int64_t)
{
    frame->code++;
    if (output) *output=CString("1.0.1");
}
void Register(IScriptable*,CStackFrame* frame,bool* output,std::int64_t)
{
    CString id,title,definitions;
    RED4ext::GetParameter(frame,&id);
    RED4ext::GetParameter(frame,&title);
    RED4ext::GetParameter(frame,&definitions);
    frame->code++;
    bool ok=false;
    try {
        const auto schema=nlohmann::json::parse(definitions.c_str());
        ok=m1arc::Settings().Register(id.c_str(),title.c_str(),schema);
    } catch (...) {}
    if (output) *output=ok;
}
void GetValue(IScriptable*,CStackFrame* frame,CString* output,std::int64_t)
{
    CString mod,key;
    RED4ext::GetParameter(frame,&mod);
    RED4ext::GetParameter(frame,&key);
    frame->code++;
    if (output) {
        const auto found=m1arc::Settings().Get(mod.c_str(),key.c_str());
        const auto value=found ? found->dump() : "null";
        *output=CString(value.c_str());
    }
}
void SetValue(IScriptable*,CStackFrame* frame,bool* output,std::int64_t)
{
    CString mod,key,value;
    RED4ext::GetParameter(frame,&mod);
    RED4ext::GetParameter(frame,&key);
    RED4ext::GetParameter(frame,&value);
    frame->code++;
    bool ok=false;
    try {
        ok=m1arc::Settings().Set(mod.c_str(),key.c_str(),
                                 nlohmann::json::parse(value.c_str()));
    } catch (...) {}
    if (output) *output=ok;
}

// Called by the owning REDscript mod from its ordinary update logic.
// Actions do not execute code from JSON and are not persistent.
void ConsumeAction(IScriptable*, CStackFrame* frame, bool* output, std::int64_t)
{
    CString mod, action;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &action);
    frame->code++;
    if (output) *output = m1arc::Settings().ConsumeAction(mod.c_str(), action.c_str());
}
void GetBool(IScriptable*, CStackFrame* frame, bool* output, std::int64_t)
{
    CString mod, key;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &key);
    frame->code++;
    if (!output) return;
    const auto value = m1arc::Settings().Get(mod.c_str(), key.c_str());
    *output = value && value->is_boolean() ? value->get<bool>() : false;
}
void GetInt(IScriptable*, CStackFrame* frame, std::int32_t* output, std::int64_t)
{
    CString mod, key;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &key);
    frame->code++;
    if (!output) return;
    const auto value = m1arc::Settings().Get(mod.c_str(), key.c_str());
    *output = value && value->is_number_integer()
        ? value->get<std::int32_t>() : 0;
}
void GetFloat(IScriptable*, CStackFrame* frame, float* output, std::int64_t)
{
    CString mod, key;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &key);
    frame->code++;
    if (!output) return;
    const auto value = m1arc::Settings().Get(mod.c_str(), key.c_str());
    *output = 0.0f;
    if (!value || !value->is_number_float()) return;
    const double number = value->get<double>();
    if (std::isfinite(number) &&
        std::abs(number) <= std::numeric_limits<float>::max())
        *output = static_cast<float>(number);
}
void GetString(IScriptable*, CStackFrame* frame, CString* output, std::int64_t)
{
    CString mod, key;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &key);
    frame->code++;
    if (!output) return;
    const auto value = m1arc::Settings().Get(mod.c_str(), key.c_str());
    const std::string text = value && value->is_string()
        ? value->get<std::string>() : std::string{};
    *output = CString(text.c_str());
}
void Revision(IScriptable*, CStackFrame* frame, std::int32_t* output, std::int64_t)
{
    frame->code++;
    if (output) *output = m1arc::Store().Revision();
}
void SetBool(IScriptable*, CStackFrame* frame, bool* output, std::int64_t)
{
    CString mod, key;
    bool value = false;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &key);
    RED4ext::GetParameter(frame, &value);
    frame->code++;
    if (output) *output = m1arc::Settings().Set(mod.c_str(), key.c_str(), value);
}
void SetInt(IScriptable*, CStackFrame* frame, bool* output, std::int64_t)
{
    CString mod, key;
    std::int32_t value = 0;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &key);
    RED4ext::GetParameter(frame, &value);
    frame->code++;
    if (output) *output = m1arc::Settings().Set(mod.c_str(), key.c_str(), value);
}
void SetFloat(IScriptable*, CStackFrame* frame, bool* output, std::int64_t)
{
    CString mod, key;
    float value = 0.0f;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &key);
    RED4ext::GetParameter(frame, &value);
    frame->code++;
    if (output) *output = m1arc::Settings().Set(mod.c_str(), key.c_str(), value);
}
void SetString(IScriptable*, CStackFrame* frame, bool* output, std::int64_t)
{
    CString mod, key, value;
    RED4ext::GetParameter(frame, &mod);
    RED4ext::GetParameter(frame, &key);
    RED4ext::GetParameter(frame, &value);
    frame->code++;
    if (output) *output = m1arc::Settings().Set(mod.c_str(), key.c_str(), std::string(value.c_str()));
}

void ResetValue(IScriptable*,CStackFrame* frame,bool* output,std::int64_t)
{
    CString mod,key;
    RED4ext::GetParameter(frame,&mod);
    RED4ext::GetParameter(frame,&key);
    frame->code++;
    if (output) *output=m1arc::Settings().Reset(mod.c_str(),key.c_str());
}
void ResetMod(IScriptable*,CStackFrame* frame,bool* output,std::int64_t)
{
    CString mod;
    RED4ext::GetParameter(frame,&mod);
    frame->code++;
    if (output) *output=m1arc::Settings().ResetMod(mod.c_str());
}
void SaveProfile(IScriptable*,CStackFrame* frame,bool* output,std::int64_t)
{
    CString name;
    RED4ext::GetParameter(frame,&name);
    frame->code++;
    if (output) *output=m1arc::Settings().SaveProfile(name.c_str());
}
void LoadProfile(IScriptable*,CStackFrame* frame,bool* output,std::int64_t)
{
    CString name;
    RED4ext::GetParameter(frame,&name);
    frame->code++;
    if (output) *output=m1arc::Settings().LoadProfile(name.c_str());
}
void DeleteProfile(IScriptable*,CStackFrame* frame,bool* output,std::int64_t)
{
    CString name;
    RED4ext::GetParameter(frame,&name);
    frame->code++;
    if (output) *output=m1arc::Settings().DeleteProfile(name.c_str());
}

bool GameStateContinue(RED4ext::CGameApplication*) { return true; }
bool BaseInitializationFinished(RED4ext::CGameApplication*)
{
    m1arc::InputRouter::Get().ArmFirstRun();
    m1arc::RenderBridge::Get().LogInfo(
        "m1arc: game base initialization complete; shortcut setup armed");
    return true;
}
RED4ext::v1::GameState baseGameCallbacks{
    &GameStateContinue, &GameStateContinue, &BaseInitializationFinished
};

void RegisterTypes() {}
void PostRegisterTypes()
{
    auto* rtti=RED4ext::CRTTISystem::Get();
    if (!rtti) return;
    const auto add=[&](const char* name,
                       RED4ext::ScriptingFunction_t<void*> callback,
                       const char* type,const char* const* args,
                       std::size_t count) {
        auto* fn=RED4ext::CGlobalFunction::Create(name,name,callback);
        fn->flags=RED4ext::CBaseFunction::Flags{
            .isNative=true,.isStatic=true
        };
        fn->SetReturnType(type);
        for (std::size_t i=0;i<count;++i)
            fn->AddParam("String",args[i]);
        rtti->RegisterFunction(fn);
    };
    const char* one[]={"name"};
    const char* two[]={"modId","settingId"};
    const char* three[]={"modId","settingId","payload"};
    const char* registerArgs[]={"modId","displayName","schemaJson"};

    add("M1Arc_Version",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&Version),"String",nullptr,0);
    add("M1Arc_RegisterJson",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&Register),"Bool",registerArgs,3);
    add("M1Arc_GetJson",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&GetValue),"String",two,2);
    add("M1Arc_ConsumeAction",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&ConsumeAction),"Bool",two,2);
    add("M1Arc_GetBool",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&GetBool),"Bool",two,2);
    add("M1Arc_GetInt",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&GetInt),"Int32",two,2);
    add("M1Arc_GetFloat",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&GetFloat),"Float",two,2);
    add("M1Arc_GetString",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&GetString),"String",two,2);
    add("M1Arc_Revision",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&Revision),"Int32",nullptr,0);
    add("M1Arc_SetJson",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&SetValue),"Bool",three,3);
    // Typed setters keep REDscript consumers free from hand-escaped JSON.
    const auto addTypedSetter=[&](const char* name,
                                  RED4ext::ScriptingFunction_t<void*> callback,
                                  const char* valueType) {
        auto* fn=RED4ext::CGlobalFunction::Create(name,name,callback);
        fn->flags=RED4ext::CBaseFunction::Flags{
            .isNative=true,.isStatic=true
        };
        fn->SetReturnType("Bool");
        fn->AddParam("String","modId");
        fn->AddParam("String","settingId");
        fn->AddParam(valueType,"value");
        rtti->RegisterFunction(fn);
    };
    addTypedSetter("M1Arc_SetBool",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&SetBool),"Bool");
    addTypedSetter("M1Arc_SetInt",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&SetInt),"Int32");
    addTypedSetter("M1Arc_SetFloat",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&SetFloat),"Float");
    addTypedSetter("M1Arc_SetString",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&SetString),"String");

    add("M1Arc_ResetSetting",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&ResetValue),"Bool",two,2);
    add("M1Arc_ResetMod",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&ResetMod),"Bool",one,1);
    add("M1Arc_SaveProfile",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&SaveProfile),"Bool",one,1);
    add("M1Arc_LoadProfile",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&LoadProfile),"Bool",one,1);
    add("M1Arc_DeleteProfile",reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&DeleteProfile),"Bool",one,1);
    // Optional, strictly version-gated game controller selection bridge.
    m1arc::breach::RegisterNative();
}

std::filesystem::path ModuleDirectory()
{
    HMODULE dll=nullptr;
    const auto address=reinterpret_cast<LPCWSTR>(&ModuleDirectory);
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,address,&dll))
        return {};
    wchar_t buffer[MAX_PATH+1]{};
    const auto size=GetModuleFileNameW(dll,buffer,MAX_PATH);
    return size && size<MAX_PATH ?
        std::filesystem::path(buffer).parent_path() : std::filesystem::path{};
}
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(
    RED4ext::v1::PluginHandle handle,RED4ext::v1::EMainReason reason,
    const RED4ext::v1::Sdk* sdk)
{
    if (reason==RED4ext::v1::EMainReason::Unload) {
        m1arc::RenderBridge::Get().Uninstall();
        return true;
    }
    if (reason!=RED4ext::v1::EMainReason::Load) return true;

    const auto folder=ModuleDirectory();
    if (folder.empty()) return false;
    m1arc::Store().Configure(folder / L"settings.json");
    m1arc::Translations().Configure(folder, m1arc::Store());
    m1arc::breach::Initialize();
    // Log exact reason even if the addon's own board capture never fires.
    if (sdk && sdk->logger && sdk->logger->Info)
        sdk->logger->Info(handle,m1arc::breach::StatusMessage());
    // Manifest discovery is native and data-only; REDscript supplies the
    // actual gameplay logic. No CET, Lua, or automatic code execution.
    const auto manifestReport=m1arc::LoadManifests(folder,m1arc::Settings());
    if (sdk && sdk->logger && sdk->logger->Warn) {
        for (const auto& error:manifestReport.errors) {
            const std::string warning="m1arc mod: "+error;
            sdk->logger->Warn(handle,warning.c_str());
        }
    }
    // Draft/dev artifact includes a marker; delete the marker to hide the
    // temporary UI test mod. Normal releases must NOT include the marker.
    (void)m1arc::dev::RegisterIfEnabled(folder,m1arc::Settings());
    auto* rtti=RED4ext::CRTTISystem::Get();
    if (!rtti) return false;
    rtti->AddRegisterCallback(RegisterTypes);
    rtti->AddPostRegisterCallback(PostRegisterTypes);

    // Preserve the proven first-run lifecycle: no input takeover before base initialization.
    // Use RED4ext's public game-state lifecycle callback, independently.
    if (!sdk || !sdk->gameStates || !sdk->gameStates->Add ||
        !sdk->gameStates->Add(handle, RED4ext::EGameStateType::BaseInitialization,
                             &baseGameCallbacks)) {
        if (sdk && sdk->logger && sdk->logger->Warn)
            sdk->logger->Warn(handle,
                "m1arc: base initialization callback unavailable; first-run setup deferred");
    }

    if (!m1arc::RenderBridge::Get().Install(handle,sdk)) {
        // A missing or version-incompatible graphics symbol must never
        // prevent Cyberpunk from starting or block its input.
        if (sdk && sdk->logger && sdk->logger->Warn)
            sdk->logger->Warn(handle,
                "m1arc: renderer unavailable for this game build; overlay disabled");
    }
    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* info)
{
    info->name=L"m1arc";
    info->author=L"MISO";
    info->version=RED4EXT_V1_SEMVER(1,0,1);
    info->runtime=RED4EXT_V1_RUNTIME_VERSION_LATEST;
    info->sdk=RED4EXT_V1_SDK_VERSION_CURRENT;
}
RED4EXT_C_EXPORT std::uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1;
}
