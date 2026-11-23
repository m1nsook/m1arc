#pragma once

#include <RED4ext/Api/v1/PluginHandle.hpp>
#include <RED4ext/Api/v1/Sdk.hpp>
#include <RED4ext/GameStates.hpp>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <imgui.h>
#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

namespace m1arc {
std::uintptr_t ResolveGameSymbol(std::uint32_t hash) noexcept;

struct GameFrame {
    IDXGISwapChain4* swapchain = nullptr;
    ID3D12CommandQueue* queue = nullptr;
    HWND window = nullptr;
};
GameFrame DiscoverGameFrame(std::int32_t deviceIndex) noexcept;

// M1 Arc uses the two-thread ImGui staging design of the WORKING
// m1core/core/src/d3d12/D3D12_Functions.cpp implementation:
// Win32/NewFrame/ImGui::Render on the game tick, DX12 submission on Present.
class Renderer final {
public:
    static Renderer& Get() noexcept;
    void Present(const GameFrame& frame) noexcept;
    void PrepareFrame() noexcept;
    void Reset(bool shutdown = false) noexcept;
private:
    Renderer() = default;
    bool Initialize(const GameFrame& frame) noexcept;
    void ReleaseDrawData(ImDrawData& draw) noexcept;
    struct Backbuffer {
        Microsoft::WRL::ComPtr<ID3D12Resource> texture;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commands;
        D3D12_CPU_DESCRIPTOR_HANDLE target{};
    };
    std::recursive_mutex mutex_;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapchain_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptors_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> targets_;
    std::vector<Backbuffer> buffers_;
    std::array<ImDrawData,3> staging_{};
    ImGuiStyle style_{};
    SIZE size_{};
    HWND window_ = nullptr;
    bool initialized_ = false;
    bool win32Ready_ = false;
    bool dx12Ready_ = false;
    bool ownContext_ = false;
};

// All hooks are RED4ext plugin hooks and always call through to their
// original game function. Do not install CET hooks, patch CET memory, or
// create an independent scripting runtime.
class RenderBridge final {
public:
    static RenderBridge& Get() noexcept;
    bool Install(RED4ext::v1::PluginHandle, const RED4ext::v1::Sdk*) noexcept;
    void Uninstall() noexcept;
    void LogInfo(const char* message) const noexcept;
    void LogWarn(const char* message) const noexcept;
private:
    using PresentFn = void* (*)(std::int32_t*,std::uint8_t,UINT);
    using ResizeFn = void* (*)(std::uint32_t,std::uint32_t,std::uint32_t,
                               std::uint8_t,std::int32_t*);
    using ShutdownFn = void* (*)(std::uint64_t,std::uint64_t,
                                 std::uint64_t,std::uint64_t);
    using StateTickFn = bool (*)(RED4ext::IGameState*,RED4ext::CGameApplication*);
    static void* OnPresent(std::int32_t*,std::uint8_t,UINT);
    static void* OnResize(std::uint32_t,std::uint32_t,std::uint32_t,
                          std::uint8_t,std::int32_t*);
    static void* OnShutdown(std::uint64_t,std::uint64_t,
                            std::uint64_t,std::uint64_t);
    static bool OnStateTick(RED4ext::IGameState*, RED4ext::CGameApplication*);
    RED4ext::v1::PluginHandle handle_{};
    const RED4ext::v1::Sdk* sdk_ = nullptr;
    void* presentAddress_ = nullptr;
    void* resizeAddress_ = nullptr;
    void* shutdownAddress_ = nullptr;
    std::array<void*,4> tickAddresses_{};
    std::array<StateTickFn,4> originalTicks_{};
    PresentFn originalPresent_ = nullptr;
    ResizeFn originalResize_ = nullptr;
    ShutdownFn originalShutdown_ = nullptr;
    std::atomic_bool frameFound_{false};
    std::atomic_bool frameMissing_{false};
};

} // namespace m1arc
