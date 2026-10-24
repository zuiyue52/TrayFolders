////////////////////////////////////////////////////////////////////////////////
// Standalone lifecycle. The controller targets only the shell taskbar thread.
// A process-lifetime module reference prevents callbacks into an unloaded DLL.
// Stopping restores the original XAML source and disables every function hook.
HHOOK g_controllerHook;
HWND g_controllerTray;
bool g_active;
bool g_hooksCreated;
constexpr wchar_t kStatusProperty[] = L"TrayFolders.Standalone.Status.v2";
UINT CommandMessage() { return RegisterWindowMessageW(L"TrayFolders.Standalone.Command.v2"); }

bool StopEngine() {
    g_unloading = true;
    bool clean = true;
    auto cleanup = [&] {
        if (g_trayMouseHook) { UnhookWindowsHookEx(g_trayMouseHook); g_trayMouseHook = nullptr; }
        ClearMouseGesture();
        if (!g_states) return;
        for (auto& state : *g_states) {
            try { Detach(*state); } catch (...) { clean = false; }
        }
        if (clean) {
            g_states->clear();
            g_contentVmMember = nullptr;
            g_configurationMember = nullptr;
            g_overflowIconsMember = nullptr;
        }
    };
    if (g_overflowThreadId) {
        if (!RunFromOverflowThread(cleanup)) clean = false;
    } else cleanup();
    if (g_hooksCreated && MH_DisableHook(MH_ALL_HOOKS) != MH_OK) clean = false;
    // Do not free trampolines: another thread may be completing an original call.
    g_active = false;
    return clean;
}

int StartEngine() {
    if (g_active) return 1;
    auto module = GetSystemTrayModuleHandle();
    if (!IsSupportedModule(module)) return -2;
    g_runtimeValidated = false; g_runtimeRejected = false;
    g_settings = LoadSettings();
    LoadManualMoves();
    if (g_settings.folders.empty()) return -3;
    if (!g_hooksCreated) {
        auto result = MH_Initialize();
        if (result != MH_OK && result != MH_ERROR_ALREADY_INITIALIZED) return -4;
        if (!HookSystemTraySymbols(module)) { MH_RemoveHook(MH_ALL_HOOKS); return -4; }
        g_hooksCreated = true;
    }
    if (!g_states) g_states.emplace();
    g_unloading = false;
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) { g_unloading = true; return -4; }
    g_active = true;
    if (!g_inventoryStarted.exchange(true)) {
        g_inventoryEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (g_inventoryEvent) {
            if (HANDLE thread = CreateThread(nullptr, 0, InventoryWorker, nullptr, 0, nullptr)) CloseHandle(thread);
        }
    }
    return 1;
}

extern "C" __declspec(dllexport) LRESULT CALLBACK TrayFoldersMessageHook(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        auto& message = *reinterpret_cast<CWPSTRUCT*>(lParam);
        if (message.message == CommandMessage()) {
            HMODULE pinned;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                              reinterpret_cast<LPCWSTR>(&TrayFoldersMessageHook), &pinned);
            int status = g_active ? 1 : 0;
            try {
                switch (message.wParam) {
                case 1: status = StartEngine(); break;
                case 2:
                    if (g_active) {
                        auto settings = LoadSettings();
                        AcquireSRWLockExclusive(&g_pendingSettingsLock);
                        g_pendingSettings = std::move(settings);
                        ReleaseSRWLockExclusive(&g_pendingSettingsLock);
                        if (g_overflowThreadId) RunFromOverflowThread(ApplyPendingSettings);
                        status = 1;
                    }
                    break;
                case 3: status = StopEngine() ? 0 : -5; break;
                case 6: status = !g_active ? 0 : g_runtimeRejected ? -13 : g_runtimeValidated ? 1 : 2; break;
                case 5:
                    if (g_active) {
                        status = 2; // No overflow state until the panel has opened once.
                        if (g_overflowThreadId) {
                            bool refreshed = false;
                            bool ran = RunFromOverflowThread([&] {
                                ApplyPendingSettings();
                                LoadManualMoves();
                                if (!g_states) return;
                                for (auto& state : *g_states) {
                                    if (state->attached && !state->detaching) {
                                        Rebuild(*state, true);
                                        refreshed = true;
                                    }
                                }
                            });
                            if (ran && refreshed) status = 1;
                        }
                    }
                    break;
                }
            } catch (...) { status = -6; }
            SetPropW(message.hwnd, kStatusProperty, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(status + 100)));
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

extern "C" __declspec(dllexport) int WINAPI TF_Command(int command) {
    HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!tray) return -1;
    if (g_controllerTray != tray && g_controllerHook) {
        UnhookWindowsHookEx(g_controllerHook); g_controllerHook = nullptr;
    }
    if (!g_controllerHook) {
        DWORD pid;
        DWORD tid = GetWindowThreadProcessId(tray, &pid);
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        wchar_t path[32768]; DWORD length = ARRAYSIZE(path);
        bool shell = process && QueryFullProcessImageNameW(process, 0, path, &length);
        if (process) CloseHandle(process);
        if (!shell || _wcsicmp(wcsrchr(path, L'\\') ? wcsrchr(path, L'\\') + 1 : path, L"explorer.exe") != 0) return -7;
        g_controllerHook = SetWindowsHookExW(WH_CALLWNDPROC, TrayFoldersMessageHook, g_dllModule, tid);
        if (!g_controllerHook) return -7;
        g_controllerTray = tray;
    }
    DWORD_PTR result;
    if (!SendMessageTimeoutW(tray, CommandMessage(), command, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 4000, &result)) return -8;
    auto status = reinterpret_cast<INT_PTR>(GetPropW(tray, kStatusProperty));
    return status ? static_cast<int>(status - 100) : -9;
}
extern "C" __declspec(dllexport) void WINAPI TF_Disconnect() {
    if (g_controllerHook) { UnhookWindowsHookEx(g_controllerHook); g_controllerHook = nullptr; }
    g_controllerTray = nullptr;
}
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { g_dllModule = module; DisableThreadLibraryCalls(module); }
    return TRUE;
}
