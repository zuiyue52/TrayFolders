// SPDX-License-Identifier: GPL-3.0-only
// Derived from System Tray Folders by Ris Peng.
// https://github.com/ramensoftware/windhawk-mods/blob/main/mods/system-tray-folders.wh.cpp
// Standalone adaptation, 2026-10-23: independent settings, symbol validation,
// MinHook integration and controller lifecycle. No Windhawk runtime is used.
// Module-loading and UI-thread helpers derive in part from m417z's tray mods,
// also licensed under GPL-3.0-only, as credited by the original module.
#include "standalone_support.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <cwctype>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#undef GetCurrentTime

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Data.h>
#include <winrt/Windows.UI.Xaml.Markup.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.h>
#include <winrt/base.h>

using namespace winrt::Windows::UI::Xaml;
namespace wf = winrt::Windows::Foundation;
namespace wfc = winrt::Windows::Foundation::Collections;

std::atomic<bool> g_unloading;

// Runs a function when leaving the enclosing scope.
template <typename Function>
class ScopeExit {
   public:
    explicit ScopeExit(Function function) : m_function(std::move(function)) {}
    ~ScopeExit() { m_function(); }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;

   private:
    Function m_function;
};

////////////////////////////////////////////////////////////////////////////////
#include "configuration.inc"
#include "icon_reader.inc"
#include "overflow_state.inc"
#include "folder_tiles.inc"
#include "display_items.inc"
#include "overflow_binding.inc"
#include "shell_hooks.inc"
#include "thread_dispatch.inc"
#include "module_adapter.inc"
#include "drag_drop.inc"
#include "lifecycle.inc"
