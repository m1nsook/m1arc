#pragma once

#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace m1arc {

// The Win32 / RAWINPUT path follows the input order in the working m1core
// core/src/window/window.cpp + core/src/VKBindings.cpp. It does not call
// into CET and does not intercept its hotkey or its ImGui context.
class InputRouter final {
public:
    static InputRouter& Get() noexcept;
    bool Attach(HWND window) noexcept;
    void Detach() noexcept;
    void ArmFirstRun() noexcept;
    void SetRendererReady(bool ready) noexcept;
    void Toggle() noexcept;
    void BeginRebind() noexcept;
    void CancelRebind() noexcept;
    void Close() noexcept;
    bool IsOpen() const noexcept { return open_.load(); }
    bool IsRecording() const noexcept { return recording_.load(); }
    bool HasKey() const noexcept { return hotkey_.load() != 0; }
    // Mod binding capture reuses the existing Win32 input router.
    bool BeginModRebind(std::string modId, std::string settingId) noexcept;
    void CancelModRebind() noexcept;
    bool IsModRecording() const noexcept { return modRecording_.load(); }
    bool RecordingModKey(const std::string& modId, const std::string& settingId) const noexcept;
    static std::string DescribeKey(int virtualKey);
    int Key() const noexcept { return hotkey_.load(); }
    bool ShouldDraw() const noexcept { return open_.load() && rendererReady_.load(); }
    std::string KeyLabel() const;
    HWND Window() const noexcept { return window_.load(); }
private:
    InputRouter() = default;
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Process(HWND, UINT, WPARAM, LPARAM) noexcept;
    void ReadKeyFromSettings() noexcept;
    void CommitKey(int key) noexcept;
    void KeyEdge(unsigned code, bool down) noexcept;
    void HandleRawInput(HRAWINPUT raw) noexcept;
    void HandleRawMouse(HWND hwnd, const RAWMOUSE& mouse) noexcept;
    void SyncCursorOnWindowThread() noexcept;
    bool WantsCursor() const noexcept;
    bool CanCapture() const noexcept;
    std::atomic<HWND> window_{nullptr};
    WNDPROC oldProc_ = nullptr;
    std::atomic_bool open_{false};
    std::atomic_bool recording_{false};
    std::atomic_bool modRecording_{false};
    std::string modCaptureId_;
    std::string modCaptureSetting_;
    std::atomic_bool rendererReady_{false};
    std::atomic_bool baseReady_{false};
    std::atomic_bool cursorOwned_{false};
    std::atomic<int> hotkey_{0};
    std::array<bool, 256> held_{};
    bool skipAssignedKeyRelease_ = false;
    int justAssigned_ = 0;
    mutable std::mutex stateMutex_;
};

} // namespace m1arc
