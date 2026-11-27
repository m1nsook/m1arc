#pragma once

#include <Windows.h>
#include "menu_shortcut.hpp"
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
    void FlushWheel() noexcept; // called on the ImGui frame thread
    void Toggle() noexcept;
    void BeginRebind() noexcept;
    void CancelRebind() noexcept;
    void StartCapture() noexcept;
    bool IsCapturing() const noexcept;
    bool HasCandidate() const noexcept;
    std::string CandidateLabel() const;
    bool ConfirmCandidate() noexcept;
    void Close() noexcept;
    // Only suppress the normal menu binding while the search field is active.
    void SetTextEntryActive(bool active) noexcept { textEntryActive_.store(active); }
    void SetSearchShortcut(int packed) noexcept { searchShortcut_.store(packed); }
    int SearchShortcut() const noexcept { return searchShortcut_.load(); }
    bool IsMenuShortcut(int packed) const noexcept { return packed!=0 && packed==shortcut_.load(); }
    bool ConsumeSearchRequest() noexcept { return searchRequested_.exchange(false); }
    bool IsOpen() const noexcept { return open_.load(); }
    bool IsRecording() const noexcept { return recording_.load(); }
    bool HasKey() const noexcept { return shortcut_.load() != 0; }
    // Mod binding capture reuses the existing Win32 input router.
    bool BeginModRebind(std::string modId, std::string settingId) noexcept;
    void CancelModRebind() noexcept;
    bool IsModRecording() const noexcept { return modRecording_.load(); }
    bool RecordingModKey(const std::string& modId, const std::string& settingId) const noexcept;
    static std::string DescribeKey(int virtualKey);
    int Key() const noexcept { return MenuKey(shortcut_.load()); }
    bool ShouldDraw() const noexcept { return open_.load() && rendererReady_.load(); }
    std::string KeyLabel() const;
    HWND Window() const noexcept { return window_.load(); }
private:
    InputRouter() = default;
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Process(HWND, UINT, WPARAM, LPARAM) noexcept;
    void ReadKeyFromSettings() noexcept;
    void OpenRebindLocked() noexcept;
    int ModifiersLocked() const noexcept;
    bool BlockedModifiersLocked() const noexcept;
    static std::string DescribeShortcut(int shortcut);
    void KeyEdge(unsigned code, bool down) noexcept;
    void HandleRawInput(HRAWINPUT raw) noexcept;
    void HandleRawMouse(HWND hwnd, const RAWMOUSE& mouse) noexcept;
    void ClearWheel() noexcept;
    std::atomic<int> rawWheelX_{0}, rawWheelY_{0}, legacyWheelX_{0}, legacyWheelY_{0};
    void SyncCursorOnWindowThread() noexcept;
    bool WantsCursor() const noexcept;
    bool CanCapture() const noexcept;
    std::atomic<HWND> window_{nullptr};
    WNDPROC oldProc_ = nullptr;
    std::atomic_bool open_{false};
    std::atomic_bool textEntryActive_{false};
    std::atomic<int> searchShortcut_{0};
    std::atomic_bool searchRequested_{false};
    std::atomic_bool recording_{false};
    std::atomic_bool modRecording_{false};
    std::string modCaptureId_;
    std::string modCaptureSetting_;
    std::atomic_bool rendererReady_{false};
    std::atomic_bool baseReady_{false};
    std::atomic_bool cursorOwned_{false};
    std::atomic<int> shortcut_{0};
    MenuCapture menuCapture_;
    MenuPress menuPress_;
    bool closeAfterBinding_ = false;
    std::array<bool, 256> held_{};
    bool skipAssignedKeyRelease_ = false;
    int justAssigned_ = 0;
    mutable std::mutex stateMutex_;
};

} // namespace m1arc
