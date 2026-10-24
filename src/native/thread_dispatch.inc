// Threading helpers

struct RunFromWindowThreadParam {
    std::function<void()> proc;
    bool succeeded = false;
};

bool RunFromWindowThread(HWND hWnd, std::function<void()> proc) {
    static const UINT message =
        RegisterWindowMessage(L"TrayFolders.RunOnUiThread.v1");

    DWORD threadId = GetWindowThreadProcessId(hWnd, nullptr);
    if (!threadId || !message) {
        return false;
    }
    if (threadId == GetCurrentThreadId()) {
        try {
            proc();
            return true;
        } catch (...) {
            AppLog(L"Window thread operation failed");
            return false;
        }
    }

    RunFromWindowThreadParam param{std::move(proc)};
    HHOOK hook = SetWindowsHookEx(
        WH_CALLWNDPROC,
        [](int nCode, WPARAM wParam, LPARAM lParam) -> LRESULT {
            if (nCode == HC_ACTION) {
                const auto& cwp = *reinterpret_cast<const CWPSTRUCT*>(lParam);
                if (cwp.message == message) {
                    auto& param =
                        *reinterpret_cast<RunFromWindowThreadParam*>(
                            cwp.lParam);
                    try {
                        param.proc();
                        param.succeeded = true;
                    } catch (...) {
                        AppLog(L"Window thread operation failed");
                    }
                }
            }
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        },
        nullptr, threadId);
    if (!hook) {
        return false;
    }

    SendMessage(hWnd, message, 0, (LPARAM)&param);
    UnhookWindowsHookEx(hook);
    return param.succeeded;
}

std::vector<HWND> GetOverflowThreadWindows() {
    std::vector<HWND> windows;
    if (DWORD threadId = g_overflowThreadId) {
        EnumThreadWindows(
            threadId,
            [](HWND hWnd, LPARAM lParam) -> BOOL {
                ((std::vector<HWND>*)lParam)->push_back(hWnd);
                return TRUE;
            },
            (LPARAM)&windows);
    }

    EnumWindows(
        [](HWND hWnd, LPARAM lParam) -> BOOL {
            DWORD processId;
            WCHAR className[32];
            if (GetWindowThreadProcessId(hWnd, &processId) &&
                processId == GetCurrentProcessId() &&
                GetClassName(hWnd, className, ARRAYSIZE(className)) &&
                _wcsicmp(className, L"Shell_TrayWnd") == 0) {
                ((std::vector<HWND>*)lParam)->push_back(hWnd);
                return FALSE;
            }
            return TRUE;
        },
        (LPARAM)&windows);

    return windows;
}

bool RunFromOverflowThread(std::function<void()> const& proc) {
    for (HWND hWnd : GetOverflowThreadWindows()) {
        bool ran = false;
        bool succeeded = RunFromWindowThread(hWnd, [&ran, &proc] {
            ran = true;
            proc();
        });
        if (ran) {
            return succeeded;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
