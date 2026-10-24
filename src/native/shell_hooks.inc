// Hooks

void* g_showingManager;

using OverflowXamlIslandManager_Show_t =
    void(WINAPI*)(void* pThis, POINT pt, int inputDeviceKind);
OverflowXamlIslandManager_Show_t OverflowXamlIslandManager_Show_Original;
void WINAPI OverflowXamlIslandManager_Show_Hook(void* pThis,
                                                POINT pt,
                                                int inputDeviceKind) {
    AppLog(L"Show %d,%d kind=%d", pt.x, pt.y, inputDeviceKind);

    const bool nestedShow = g_showingManager == pThis;
    if (!nestedShow && g_states) {
        if (auto state = FindState(pThis)) {
            state->openingPrepared = false;
        }
    }

    try {
        ApplyPendingSettings();
    } catch (...) {
        AppLog(L"Show: applying settings failed");
    }

    {
        auto previousManager = std::exchange(g_showingManager, pThis);
        auto reset = ScopeExit([previousManager] { g_showingManager = previousManager; });
        OverflowXamlIslandManager_Show_Original(pThis, pt, inputDeviceKind);
    }

    if (nestedShow || g_unloading || !g_states) {
        return;
    }
    auto state = FindState(pThis);
    // Loaded can already have fired when the pre-Measure probe ran. Always
    // retry synchronously once the native Show has finished realizing its UI.
    if (state && !state->attached) {
        if (auto overflow = state->overflow.get()) {
            try {
                state = EnsureAttached(pThis, overflow);
                if (state && state->attached) {
                    AppLog(L"Compatibility: attached after native Show");
                }
            } catch (winrt::hresult_error const& error) {
                AppLog(L"Compatibility: post-Show failed %08X %s",
                       error.code(), error.message().c_str());
            }
        }
    }
    if (!state || !state->attached) {
        return;
    }

    try {
        PrepareOpening(*state);
        RefreshTileSize(*state);
    } catch (...) {
        AppLog(L"Show: update failed");
    }

    struct Param {
        HWND window;
        int count;
    } param{};
    EnumThreadWindows(
        GetCurrentThreadId(),
        [](HWND hWnd, LPARAM lParam) -> BOOL {
            auto& param = *(Param*)lParam;
            WCHAR className[64];
            if (IsWindowVisible(hWnd) &&
                GetClassName(hWnd, className, ARRAYSIZE(className)) &&
                _wcsicmp(className, L"TopLevelWindowForOverflowXamlIsland") ==
                    0) {
                param.window = hWnd;
                param.count++;
            }
            return TRUE;
        },
        (LPARAM)&param);
    state->islandWindow = param.count == 1 ? param.window : nullptr;
    EnsureMouseTracking();
}

// While Show positions the flyout, the manager measures its
// NotificationAreaOverflow member through this function, before the window is
// shown. Its `this` is that member, a C++/WinRT projected type, which holds
// nothing but the control's interface pointer, so the control is taken from
// there instead of from the manager's layout, which isn't part of any contract.
using NotificationAreaOverflow_Measure_t =
    void(WINAPI*)(void* pThis,
                  winrt::Windows::Foundation::Size const* availableSize);
NotificationAreaOverflow_Measure_t NotificationAreaOverflow_Measure_Original;
void WINAPI NotificationAreaOverflow_Measure_Hook(
    void* pThis,
    winrt::Windows::Foundation::Size const* availableSize) {
    if (void* manager = g_showingManager) {
        try {
            auto overflow = reinterpret_cast<wf::IInspectable const*>(pThis)
                                ->try_as<Controls::Control>();
            if (auto state = EnsureAttached(manager, overflow)) {
                // Reset once per Show, even if the shell measures repeatedly.
                // Let the native Measure below perform the pending layout.
                PrepareOpening(*state);
            }
        } catch (...) {
            AppLog(L"Show: update failed");
        }
    }

    NotificationAreaOverflow_Measure_Original(pThis, availableSize);

    // Older tray templates can materialize their children during Measure,
    // after the first lookup and without another Loaded event.
    if (void* manager = g_showingManager) {
        auto state = FindState(manager);
        if (state && !state->attached) {
            try {
                if (auto overflow = state->overflow.get()) {
                    state = EnsureAttached(manager, overflow);
                    if (state && state->attached) {
                        AppLog(L"Compatibility: attached after native Measure");
                    }
                }
            } catch (winrt::hresult_error const& error) {
                AppLog(L"Compatibility: post-Measure failed %08X %s",
                       error.code(), error.message().c_str());
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
