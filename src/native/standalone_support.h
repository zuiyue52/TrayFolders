#pragma once
#define NOMINMAX
#include <windows.h>
#include <string>
#include <string_view>
#include <vector>
#include <initializer_list>
#include <atomic>
#include <mutex>
#include <algorithm>
#include "../../vendor/minhook/include/MinHook.h"
#include "adaptation_profile.h"

// No synchronous debugger output on Explorer's UI thread.
#define AppLog(...) ((void)0)
HMODULE g_dllModule;
std::wstring ModuleDirectory() {
    wchar_t path[32768];
    DWORD n = GetModuleFileNameW(g_dllModule, path, ARRAYSIZE(path));
    if (!n || n >= ARRAYSIZE(path)) return {};
    std::wstring result(path, n);
    return result.substr(0, result.find_last_of(L"\\/") + 1);
}
std::wstring ConfigString(const wchar_t* section, const wchar_t* key) {
    wchar_t value[4096];
    auto path = ModuleDirectory() + L"settings.ini";
    GetPrivateProfileStringW(section, key, L"", value, ARRAYSIZE(value), path.c_str());
    return value;
}
int ConfigInt(const wchar_t* section, const wchar_t* key, int fallback = 0) {
    auto path = ModuleDirectory() + L"settings.ini";
    return GetPrivateProfileIntW(section, key, fallback, path.c_str());
}

#include "loaded_profile.h"

struct SymbolHook {
    std::vector<std::wstring_view> names;
    void** original;
    void* detour;
    bool optional;
    template<typename T, typename U>
    SymbolHook(std::initializer_list<std::wstring_view> n, T* o, U d, bool opt = false)
        : names(n), original(reinterpret_cast<void**>(o)), detour(reinterpret_cast<void*>(d)), optional(opt) {}
    template<typename T>
    SymbolHook(std::initializer_list<std::wstring_view> n, T* o, std::nullptr_t, bool opt = false)
        : names(n), original(reinterpret_cast<void**>(o)), detour(nullptr), optional(opt) {}
};
bool HookSymbols(HMODULE module, SymbolHook* hooks, size_t count) {
    if (!IsSupportedModule(module)) return false;
    for (size_t i = 0; i < count; ++i) {
        void* target = nullptr;
        for (auto name : hooks[i].names) {
            for (size_t s = 0; s < kSymbolCount; s++) {
                if (name == kRequiredSymbols[s].name && g_adaptation.rvas[s] < g_adaptation.imageSize) {
                    target = reinterpret_cast<BYTE*>(module) + g_adaptation.rvas[s];
                    break;
                }
            }
            if (target) break;
        }
        if (!target) { if (hooks[i].optional) continue; return false; }
        if (hooks[i].detour) {
            if (MH_CreateHook(target, hooks[i].detour, hooks[i].original) != MH_OK) return false;
        } else {
            *hooks[i].original = target;
        }
    }
    return true;
}

// Keep disk I/O away from Explorer's UI thread. Only the newest snapshot is kept.
std::mutex g_inventoryMutex;
std::wstring g_inventoryPending;
HANDLE g_inventoryEvent;
std::atomic<bool> g_inventoryStarted;
DWORD WINAPI InventoryWorker(void*) {
    for (;;) {
        WaitForSingleObject(g_inventoryEvent, INFINITE);
        std::wstring value;
        { std::lock_guard<std::mutex> lock(g_inventoryMutex); value.swap(g_inventoryPending); }
        if (value.empty()) continue;
        int n = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        std::string utf8(n, '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), utf8.data(), n, nullptr, nullptr);
        auto temp = ModuleDirectory() + L"icons.tmp";
        HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) continue;
        DWORD written;
        BOOL ok = WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
        CloseHandle(file);
        if (ok && written == utf8.size()) {
            auto path = ModuleDirectory() + L"icons.tsv";
            MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
        }
    }
}
void QueueInventory(std::wstring text) {
    if (!g_inventoryEvent) return;
    { std::lock_guard<std::mutex> lock(g_inventoryMutex); g_inventoryPending = std::move(text); }
    SetEvent(g_inventoryEvent);
}
