#include "graphics.hpp"
#include "input.hpp"
#include "ui.hpp"
#include "backends/dx12.h"
#include "backends/win32.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>

namespace m1arc {
namespace {
constexpr std::uint32_t kPresent = 2468877568u;
constexpr std::uint32_t kResize = 239671909u;
constexpr std::uint32_t kShutdown = 3192982283u;
constexpr std::uint32_t kRenderContext = 1239944840u;
constexpr std::array<std::uint32_t,4> kTickHashes{
    4233370276u, 2447710505u, 3592689218u, 4069332669u
};
constexpr std::size_t kSwapchains = 0xCEBF48;
constexpr std::size_t kDeviceStride = 0xB0;
constexpr std::size_t kDirectQueue = 0x14104D0;
constexpr int kMaxDevice = 48;

// The same REDengine device layout and resolver hashes as the verified
// m1core/core/src/reverse/RenderContext.h + D3D12_Hooks.cpp.
// Only read from the game after checking the mapped memory region.
bool Readable(const void* address, std::size_t length) noexcept
{
    if (!address || !length) return false;
    MEMORY_BASIC_INFORMATION region{};
    if (VirtualQuery(address,&region,sizeof(region)) != sizeof(region))
        return false;
    if (region.State != MEM_COMMIT ||
        (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    const auto begin = reinterpret_cast<std::uintptr_t>(address);
    const auto limit = reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize;
    return begin < limit && length <= limit - begin;
}
template<typename T> T Read(const void* address) noexcept
{
    T value{};
    if (Readable(address,sizeof(T))) std::memcpy(&value,address,sizeof(T));
    return value;
}
bool ValidComObject(const void* object) noexcept
{
    auto* vtable = Read<const void*>(object);
    if (!vtable || !Readable(vtable,sizeof(void*)*8)) return false;
    auto* code = Read<const void*>(vtable);
    if (!code) return false;
    MEMORY_BASIC_INFORMATION region{};
    if (VirtualQuery(code,&region,sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT) return false;
    const auto protection = region.Protect & 0xFFu;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}
}

std::uintptr_t ResolveGameSymbol(std::uint32_t hash) noexcept
{
    using Resolver = std::uintptr_t (*)(std::uint32_t);
    auto* module = GetModuleHandleW(L"RED4ext.dll");
    if (!module) return 0;
    auto* fn = reinterpret_cast<Resolver>(GetProcAddress(module,"RED4ext_ResolveAddress"));
    return fn ? fn(hash) : 0;
}

GameFrame DiscoverGameFrame(std::int32_t deviceIndex) noexcept
{
    GameFrame frame{};
    if (deviceIndex < 1 || deviceIndex > kMaxDevice) return frame;
    const auto address = ResolveGameSymbol(kRenderContext);
    if (!address) return frame;
    const auto context = Read<std::uintptr_t>(reinterpret_cast<const void*>(address));
    if (!context) return frame;

    auto* queue = Read<ID3D12CommandQueue*>(
        reinterpret_cast<const void*>(context + kDirectQueue));
    auto* swapchain = Read<IDXGISwapChain4*>(
        reinterpret_cast<const void*>(context + kSwapchains +
                                     static_cast<std::size_t>(deviceIndex-1)*kDeviceStride));
    if (!ValidComObject(queue) || !ValidComObject(swapchain))
        return frame;

    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(swapchain->GetDesc(&desc)) || !IsWindow(desc.OutputWindow))
        return frame;
    frame.queue = queue;
    frame.swapchain = swapchain;
    frame.window = desc.OutputWindow;
    return frame;
}

Renderer& Renderer::Get() noexcept
{
    static Renderer instance;
    return instance;
}

void Renderer::ReleaseDrawData(ImDrawData& draw) noexcept
{
    for (int i = 0; i < draw.CmdListsCount; ++i)
        IM_DELETE(draw.CmdLists[i]);
    draw.Clear();
}

void Renderer::Reset(bool shutdown) noexcept
{
    InputRouter::Get().SetRendererReady(false);
    std::lock_guard lock(mutex_);
    for (auto& draw : staging_) ReleaseDrawData(draw);
    if (dx12Ready_) {
        ImGui_ImplDX12_Shutdown();
        dx12Ready_ = false;
    }
    if (win32Ready_) {
        ImGui_ImplWin32_Shutdown();
        win32Ready_ = false;
    }
    buffers_.clear();
    targets_.Reset();
    descriptors_.Reset();
    swapchain_.Reset();
    queue_.Reset();
    device_.Reset();
    size_ = {};
    window_ = nullptr;
    initialized_ = false;
    if (shutdown && ownContext_) {
        ImGui::DestroyContext();
        ownContext_ = false;
    }
}

bool Renderer::Initialize(const GameFrame& frame) noexcept
{
    if (initialized_) return true;
    if (!frame.queue || !frame.swapchain || !IsWindow(frame.window))
        return false;

    std::lock_guard lock(mutex_);
    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(frame.swapchain->GetDesc(&desc)) ||
        desc.BufferCount == 0 || desc.BufferCount > 4) return false;
    swapchain_ = frame.swapchain;
    queue_ = frame.queue;
    window_ = frame.window;

    if (FAILED(swapchain_->GetDevice(IID_PPV_ARGS(&device_))))
        goto fail;

    {
        Microsoft::WRL::ComPtr<ID3D12Device> queueDevice;
        if (FAILED(queue_->GetDevice(IID_PPV_ARGS(&queueDevice))) ||
            queueDevice.Get() != device_.Get()) goto fail;
    }
    size_ = {static_cast<LONG>(desc.BufferDesc.Width),
             static_cast<LONG>(desc.BufferDesc.Height)};
    if (!size_.cx || !size_.cy) goto fail;

    {
        D3D12_DESCRIPTOR_HEAP_DESC srv{};
        srv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv.NumDescriptors = 8;
        srv.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(device_->CreateDescriptorHeap(&srv,IID_PPV_ARGS(&descriptors_))))
            goto fail;
        D3D12_DESCRIPTOR_HEAP_DESC rtv{};
        rtv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv.NumDescriptors = desc.BufferCount;
        rtv.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (FAILED(device_->CreateDescriptorHeap(&rtv,IID_PPV_ARGS(&targets_))))
            goto fail;
    }
    buffers_.resize(desc.BufferCount);
    {
        const auto stride = device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        auto handle = targets_->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < desc.BufferCount; ++i) {
            auto& b = buffers_[i];
            if (FAILED(swapchain_->GetBuffer(i,IID_PPV_ARGS(&b.texture))) ||
                FAILED(device_->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&b.allocator))))
                goto fail;
            b.target = handle;
            device_->CreateRenderTargetView(b.texture.Get(),nullptr,handle);
            handle.ptr += stride;
            if (FAILED(device_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,
                b.allocator.Get(),nullptr,IID_PPV_ARGS(&b.commands))) ||
                FAILED(b.commands->Close()))
                goto fail;
        }
    }

    if (!ImGui::GetCurrentContext()) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ownContext_ = true;
        // Keep the working m1core double-buffered ImGui arrangement.
        // Only the appearance is reset to Dear ImGui's normal dark style.
        ImGui::StyleColorsDark(&style_);
        ImGui::GetIO().Fonts->AddFontDefault();
    }
    ImGui::GetStyle() = style_;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplWin32_Init(window_)) goto fail;
    win32Ready_ = true;
    if (!ImGui_ImplDX12_Init(device_.Get(),static_cast<int>(desc.BufferCount),
        DXGI_FORMAT_R8G8B8A8_UNORM,descriptors_.Get(),
        descriptors_->GetCPUDescriptorHandleForHeapStart(),
        descriptors_->GetGPUDescriptorHandleForHeapStart())) goto fail;
    dx12Ready_ = true;
    if (!ImGui_ImplDX12_CreateDeviceObjects(queue_.Get())) goto fail;
    initialized_ = true;
    RenderBridge::Get().LogInfo("m1arc: ImGui Win32/DX12 initialized from game renderer");
    return true;

fail:
    // Initialize failures are not fatal to the game. Releasing here may
    // reset the renderer; never capture any mouse/keyboard before success.
    RenderBridge::Get().LogWarn("m1arc: ImGui graphics initialization failed");
    if (dx12Ready_) { ImGui_ImplDX12_Shutdown(); dx12Ready_ = false; }
    if (win32Ready_) { ImGui_ImplWin32_Shutdown(); win32Ready_ = false; }
    buffers_.clear();
    targets_.Reset();
    descriptors_.Reset();
    swapchain_.Reset();
    queue_.Reset();
    device_.Reset();
    window_ = nullptr;
    return false;
}

void Renderer::PrepareFrame() noexcept
{
    if (!initialized_) return;
    std::lock_guard lock(mutex_);
    if (!initialized_) return;
    ImGui_ImplWin32_NewFrame(size_);
    ImGui::NewFrame();

    DrawArcUI();

    ImGui::Render();
    auto& latest = staging_[2];
    ReleaseDrawData(latest);
    latest = *ImGui::GetDrawData();

    // The proven m1core draw staging avoids calling ImGui::NewFrame()
    // from Present; clone command lists for the separate render thread.
    ImVector<ImDrawList*> cloned;
    cloned.resize(latest.CmdListsCount);
    for (int i = 0; i < latest.CmdListsCount; ++i)
        cloned[i] = latest.CmdLists[i]->CloneOutput();
    latest.CmdLists = std::move(cloned);
    std::swap(staging_[1],staging_[2]);
}

void Renderer::Present(const GameFrame& frame) noexcept
{
    if (!frame.swapchain || !frame.queue || !IsWindow(frame.window)) return;
    if (initialized_ && (swapchain_.Get()!=frame.swapchain ||
                          queue_.Get()!=frame.queue || window_!=frame.window))
        Reset();
    if (!initialized_ && !Initialize(frame)) return;

    std::lock_guard lock(mutex_);
    if (!initialized_) return;
    ImGui_ImplDX12_NewFrame(queue_.Get());
    if (staging_[1].Valid) {
        std::swap(staging_[0],staging_[1]);
        staging_[1].Valid = false;
    }
    auto& draw = staging_[0];
    if (!draw.Valid || draw.CmdListsCount == 0) return;
    const auto index = swapchain_->GetCurrentBackBufferIndex();
    if (index >= buffers_.size()) return;
    auto& b = buffers_[index];
    if (FAILED(b.allocator->Reset()) ||
        FAILED(b.commands->Reset(b.allocator.Get(),nullptr))) return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = b.texture.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    b.commands->ResourceBarrier(1,&barrier);
    ID3D12DescriptorHeap* heaps[] = {descriptors_.Get()};
    b.commands->SetDescriptorHeaps(1,heaps);
    b.commands->OMSetRenderTargets(1,&b.target,FALSE,nullptr);
    ImGui_ImplDX12_RenderDrawData(&draw,b.commands.Get());
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);
    b.commands->ResourceBarrier(1,&barrier);
    if (FAILED(b.commands->Close())) return;
    ID3D12CommandList* cmds[]{b.commands.Get()};
    queue_->ExecuteCommandLists(1,cmds);
    InputRouter::Get().SetRendererReady(true);
}

RenderBridge& RenderBridge::Get() noexcept
{
    static RenderBridge bridge;
    return bridge;
}
void RenderBridge::LogInfo(const char* text) const noexcept
{
    if (sdk_ && sdk_->logger && sdk_->logger->Info)
        sdk_->logger->Info(handle_,text);
}
void RenderBridge::LogWarn(const char* text) const noexcept
{
    if (sdk_ && sdk_->logger && sdk_->logger->Warn)
        sdk_->logger->Warn(handle_,text);
}

bool RenderBridge::Install(RED4ext::v1::PluginHandle handle,
                           const RED4ext::v1::Sdk* sdk) noexcept
{
    if (presentAddress_) return true;
    if (!sdk || !sdk->hooking || !sdk->hooking->Attach || !sdk->hooking->Detach)
        return false;
    handle_ = handle;
    sdk_ = sdk;
    presentAddress_ = reinterpret_cast<void*>(ResolveGameSymbol(kPresent));
    resizeAddress_ = reinterpret_cast<void*>(ResolveGameSymbol(kResize));
    shutdownAddress_ = reinterpret_cast<void*>(ResolveGameSymbol(kShutdown));
    for (std::size_t i = 0; i < kTickHashes.size(); ++i)
        tickAddresses_[i] = reinterpret_cast<void*>(ResolveGameSymbol(kTickHashes[i]));

    if (!presentAddress_ || !resizeAddress_ || !shutdownAddress_ ||
        std::any_of(tickAddresses_.begin(),tickAddresses_.end(),
            [](auto* address){ return !address; })) {
        LogWarn("m1arc: required RED4ext game symbols missing; overlay disabled safely");
        Uninstall();
        return false;
    }
    if (!sdk_->hooking->Attach(handle_,presentAddress_,
        reinterpret_cast<void*>(&OnPresent),
        reinterpret_cast<void**>(&originalPresent_)) ||
        !sdk_->hooking->Attach(handle_,resizeAddress_,
        reinterpret_cast<void*>(&OnResize),
        reinterpret_cast<void**>(&originalResize_)) ||
        !sdk_->hooking->Attach(handle_,shutdownAddress_,
        reinterpret_cast<void*>(&OnShutdown),
        reinterpret_cast<void**>(&originalShutdown_))) {
        Uninstall();
        return false;
    }
    for (std::size_t i = 0; i < tickAddresses_.size(); ++i)
        if (!sdk_->hooking->Attach(handle_,tickAddresses_[i],
            reinterpret_cast<void*>(&OnStateTick),
            reinterpret_cast<void**>(&originalTicks_[i]))) {
            Uninstall();
            return false;
        }
    LogInfo("m1arc: RED4ext render + game-thread ImGui hooks attached");
    return true;
}
void RenderBridge::Uninstall() noexcept
{
    Renderer::Get().Reset(true);
    InputRouter::Get().Detach();
    if (sdk_ && sdk_->hooking && sdk_->hooking->Detach) {
        for (std::size_t i=0;i<tickAddresses_.size();++i)
            if (tickAddresses_[i] && originalTicks_[i])
                sdk_->hooking->Detach(handle_,tickAddresses_[i]);
        if (shutdownAddress_ && originalShutdown_)
            sdk_->hooking->Detach(handle_,shutdownAddress_);
        if (resizeAddress_ && originalResize_)
            sdk_->hooking->Detach(handle_,resizeAddress_);
        if (presentAddress_ && originalPresent_)
            sdk_->hooking->Detach(handle_,presentAddress_);
    }
    originalTicks_.fill(nullptr);
    tickAddresses_.fill(nullptr);
    originalPresent_=nullptr;
    originalResize_=nullptr;
    originalShutdown_=nullptr;
    presentAddress_=resizeAddress_=shutdownAddress_=nullptr;
    sdk_=nullptr;
    frameFound_.store(false);
    frameMissing_.store(false);
}
void* RenderBridge::OnPresent(std::int32_t* index,std::uint8_t sync,UINT interval)
{
    auto& bridge=Get();
    if (index && bridge.originalPresent_) {
        const auto frame=DiscoverGameFrame(*index);
        if (frame.window) {
            InputRouter::Get().Attach(frame.window);
            if (!bridge.frameFound_.exchange(true))
                bridge.LogInfo("m1arc: game DX12 frame discovered");
            Renderer::Get().Present(frame);
        } else if (!bridge.frameMissing_.exchange(true))
            bridge.LogWarn("m1arc: game renderer not ready; ImGui deferred");
    }
    return bridge.originalPresent_ ?
           bridge.originalPresent_(index,sync,interval) : nullptr;
}
void* RenderBridge::OnResize(std::uint32_t a,std::uint32_t b,
                             std::uint32_t c,std::uint8_t d,std::int32_t* e)
{
    auto& bridge=Get();
    // Just as in verified m1core: invalidate graphics but keep visible UI
    // intent and shortcut across Alt+Tab/fullscreen changes.
    Renderer::Get().Reset();
    return bridge.originalResize_ ? bridge.originalResize_(a,b,c,d,e) : nullptr;
}
void* RenderBridge::OnShutdown(std::uint64_t a,std::uint64_t b,
                               std::uint64_t c,std::uint64_t d)
{
    auto& bridge=Get();
    Renderer::Get().Reset(true);
    return bridge.originalShutdown_ ?
           bridge.originalShutdown_(a,b,c,d) : nullptr;
}
bool RenderBridge::OnStateTick(RED4ext::IGameState* state,
                               RED4ext::CGameApplication* game)
{
    auto& bridge=Get();
    // This is where the working m1core prepared ImGui, NOT in Present.
    Renderer::Get().PrepareFrame();
    const auto index = state ? static_cast<std::size_t>(state->GetType()) : 4u;
    if (index < bridge.originalTicks_.size() && bridge.originalTicks_[index])
        return bridge.originalTicks_[index](state,game);
    // An unexpected engine state must not prevent the original tick.
    bridge.LogWarn("m1arc: unexpected game state in tick hook");
    return true;
}

} // namespace m1arc
