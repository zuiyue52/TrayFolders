// Preserve native tray dragging; only folder/back tiles consume local drops.
std::wstring IconAssignmentKey(const IconInfo& info) {
    std::wstring identity;
    if (!info.lowerAppName.empty() && info.lowerAppName != L"windows 资源管理器" && info.lowerAppName != L"windows explorer")
        identity = L"A:" + info.lowerAppName;
    else identity = L"T:" + (info.lowerToolTip.empty() ? info.lowerName : info.lowerToolTip);
    std::wstring key;
    constexpr wchar_t digits[] = L"0123456789abcdef";
    for (wchar_t c : identity) for (int shift = 12; shift >= 0; shift -= 4) key.push_back(digits[(c >> shift) & 15]);
    return key;
}
bool CanDropIcon(OverflowState& state) {
    return state.attached && !state.detaching && !g_unloading && !state.dragKey.empty() &&
           GetTickCount64() - state.dragStarted < 30000;
}
bool AssignIconToFolder(OverflowState& state, std::wstring const& key, int folder) {
    if (key.empty() || folder >= static_cast<int>(g_settings.folders.size())) return false;
    std::wstring destination = folder < 0 ? L"@root" : g_settings.folders[folder].id;
    auto path = ModuleDirectory() + L"assignments.ini";
    if (!WritePrivateProfileStringW(L"Moves", key.c_str(), destination.c_str(), path.c_str())) return false;
    g_manualMoves[key] = destination;
    auto weak = state.weak_from_this();
    if (auto items = state.itemsControl.get()) {
        items.Dispatcher().RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Low, [weak] {
            if (auto state = weak.lock(); state && state->attached && !state->detaching && !g_unloading) Rebuild(*state);
        });
    }
    return true;
}
void ConfigureDropTarget(OverflowState& state, Tile& tile, int folder) {
    tile.button.AllowDrop(true);
    auto weak = state.weak_from_this();
    tile.dragOverToken = tile.button.DragOver([weak, folder](wf::IInspectable const&, DragEventArgs const& args) {
        auto state = weak.lock();
        if (!state || !CanDropIcon(*state)) return;
        state->dragActive = true;
        args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Move);
        args.DragUIOverride().Caption(folder < 0 ? L"移回主面板" : L"移入文件夹");
        args.Handled(true);
    });
    tile.dropToken = tile.button.Drop([weak, folder](wf::IInspectable const&, DragEventArgs const& args) {
        auto state = weak.lock();
        if (!state || !CanDropIcon(*state) || folder >= static_cast<int>(g_settings.folders.size())) return;
        auto key = std::exchange(state->dragKey, {});
        state->dragActive = false;
        if (!AssignIconToFolder(*state, key, folder)) return;
        args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Move);
        args.Handled(true);
    });
}
void AttachDragSource(OverflowState& state) {
    auto items = state.itemsControl.get();
    if (!items || state.dragPressed) return;
    auto weak = state.weak_from_this();
    state.dragPressed = winrt::box_value(Input::PointerEventHandler([weak](wf::IInspectable const&, Input::PointerRoutedEventArgs const& args) {
        auto state = weak.lock();
        if (!state || !state->attached || state->detaching || g_unloading) return;
        state->dragKey.clear(); state->dragActive = false;
        if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) return;
        auto items = state->itemsControl.get();
        if (!items) return;
        auto source = args.OriginalSource().try_as<DependencyObject>();
        for (auto& [unused, info] : state->iconCache) {
            auto container = items.ContainerFromItem(info->viewModel);
            if (!container) continue;
            auto node = source;
            for (int depth = 0; node && depth < 32; ++depth) {
                if (node == container) {
                    state->dragKey = IconAssignmentKey(*info);
                    state->dragStarted = GetTickCount64();
                    return;
                }
                node = Media::VisualTreeHelper::GetParent(node);
            }
        }
    }));
    state.dragReleased = winrt::box_value(Input::PointerEventHandler([weak](wf::IInspectable const&, Input::PointerRoutedEventArgs const&) {
        if (auto state = weak.lock(); state && !state->dragActive) state->dragKey.clear();
    }));
    items.AddHandler(UIElement::PointerPressedEvent(), state.dragPressed, true);
    items.AddHandler(UIElement::PointerReleasedEvent(), state.dragReleased, true);
}
void DetachDragSource(OverflowState& state) {
    if (auto items = state.itemsControl.get()) {
        if (state.dragPressed) items.RemoveHandler(UIElement::PointerPressedEvent(), state.dragPressed);
        if (state.dragReleased) items.RemoveHandler(UIElement::PointerReleasedEvent(), state.dragReleased);
    }
    state.dragPressed = nullptr; state.dragReleased = nullptr; state.dragKey.clear();
}

// Some shell builds use a native drag loop instead of XAML Drop events.
// Observe only this tray UI thread and only gestures starting on its real icons.
HHOOK g_trayMouseHook;
struct MouseGesture {
    std::weak_ptr<OverflowState> state;
    std::wstring key;
    POINT start{};
    ULONGLONG tick = 0;
    bool dragging = false;
    Controls::Button highlight{nullptr};
};
MouseGesture g_mouseGesture;
bool ElementContains(OverflowState& state, FrameworkElement const& element, POINT point) {
    if (!element || !state.islandWindow || !IsWindowVisible(state.islandWindow)) return false;
    auto bounds = element.TransformToVisual(nullptr).TransformBounds(
        {0, 0, static_cast<float>(element.ActualWidth()), static_cast<float>(element.ActualHeight())});
    double scale = element.XamlRoot() ? element.XamlRoot().RasterizationScale() : GetDpiForWindow(state.islandWindow) / 96.0;
    POINT origin{}; ClientToScreen(state.islandWindow, &origin);
    return point.x >= origin.x + bounds.X * scale && point.x < origin.x + (bounds.X + bounds.Width) * scale &&
           point.y >= origin.y + bounds.Y * scale && point.y < origin.y + (bounds.Y + bounds.Height) * scale;
}
int FolderAtPoint(OverflowState& state, POINT point, Controls::Button* button = nullptr) {
    if (state.openFolder >= 0) {
        if (state.backTile.button && ElementContains(state, state.backTile.button, point)) {
            if (button) *button = state.backTile.button;
            return -1;
        }
    } else for (size_t i = 0; i < state.folderTiles.size(); ++i) {
        auto tile = state.folderTiles[i].button;
        if (tile && ElementContains(state, tile, point)) { if (button) *button = tile; return static_cast<int>(i); }
    }
    return -2;
}
void ClearMouseGesture() {
    if (g_mouseGesture.highlight) g_mouseGesture.highlight.Opacity(1.0);
    g_mouseGesture = {};
}
LRESULT CALLBACK TrayMouseHook(int code, WPARAM message, LPARAM param) {
    if (code >= 0 && !g_unloading && g_states) try {
        POINT point = reinterpret_cast<MOUSEHOOKSTRUCT*>(param)->pt;
        if (message == WM_LBUTTONDOWN) {
            ClearMouseGesture();
            for (auto& state : *g_states) {
                if (!state->attached || state->detaching || !state->islandWindow || !IsWindowVisible(state->islandWindow)) continue;
                auto items = state->itemsControl.get();
                if (!items) continue;
                for (auto& [unused, info] : state->iconCache) {
                    auto container = items.ContainerFromItem(info->viewModel).try_as<FrameworkElement>();
                    if (container && ElementContains(*state, container, point)) {
                        g_mouseGesture.state = state; g_mouseGesture.key = IconAssignmentKey(*info);
                        g_mouseGesture.start = point; g_mouseGesture.tick = GetTickCount64();
                        break;
                    }
                }
            }
        } else if (message == WM_MOUSEMOVE || message == WM_LBUTTONUP) {
            auto state = g_mouseGesture.state.lock();
            if (state && state->attached && !state->detaching && GetTickCount64() - g_mouseGesture.tick < 30000) {
                if (abs(point.x - g_mouseGesture.start.x) >= GetSystemMetrics(SM_CXDRAG) ||
                    abs(point.y - g_mouseGesture.start.y) >= GetSystemMetrics(SM_CYDRAG)) g_mouseGesture.dragging = true;
                if (g_mouseGesture.dragging) {
                    Controls::Button target{nullptr};
                    int folder = FolderAtPoint(*state, point, &target);
                    if (g_mouseGesture.highlight != target) {
                        if (g_mouseGesture.highlight) g_mouseGesture.highlight.Opacity(1.0);
                        g_mouseGesture.highlight = target;
                        if (target) target.Opacity(0.55);
                    }
                    if (message == WM_LBUTTONUP && folder >= -1) AssignIconToFolder(*state, g_mouseGesture.key, folder);
                }
            }
            if (message == WM_LBUTTONUP) ClearMouseGesture();
        }
    } catch (...) { g_mouseGesture.key.clear(); }
    return CallNextHookEx(g_trayMouseHook, code, message, param);
}
void EnsureMouseTracking() {
    if (!g_trayMouseHook) g_trayMouseHook = SetWindowsHookExW(WH_MOUSE, TrayMouseHook, nullptr, GetCurrentThreadId());
}
