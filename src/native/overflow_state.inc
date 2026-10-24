// Overflow state

struct Tile {
    Controls::Button button{nullptr};
    winrt::event_token clickToken{};
    winrt::event_token dragOverToken{};
    winrt::event_token dropToken{};
    std::optional<std::wstring> contentGlyph;
};

struct OverflowState : std::enable_shared_from_this<OverflowState> {
    void* manager = nullptr;
    winrt::weak_ref<Controls::Control> overflow;
    winrt::weak_ref<Controls::ItemsControl> itemsControl;

    wfc::IObservableVector<wf::IInspectable> originalIcons{nullptr};
    winrt::event_token originalIconsChangedToken{};
    int64_t itemsSourceCallbackToken = 0;
    bool itemsSourceCallbackRegistered = false;
    winrt::event_token loadedToken{};
    HWND islandWindow = nullptr;

    wfc::IObservableVector<wf::IInspectable> displayed{nullptr};
    wf::IInspectable originalItemsSource{nullptr};
    Data::Binding originalItemsSourceBinding{nullptr};

    std::vector<Tile> folderTiles;
    Tile backTile;
    double tileWidth = 0;
    double tileHeight = 0;

    std::unordered_map<void*, std::shared_ptr<IconInfo>> iconCache;

    int openFolder = -1;
    bool settingItemsSource = false;
    bool rebuilding = false;
    bool attached = false;
    bool detaching = false;
    bool itemsSourceOverridden = false;
    bool diagnosticTreeLogged = false;
    bool openingPrepared = false;
    wf::IInspectable dragPressed{nullptr};
    wf::IInspectable dragReleased{nullptr};
    std::wstring dragKey;
    ULONGLONG dragStarted = 0;
    bool dragActive = false;
};

[[clang::no_destroy]] std::optional<std::vector<std::shared_ptr<OverflowState>>>
    g_states{std::in_place};
std::atomic<DWORD> g_overflowThreadId;

void Rebuild(OverflowState& state, bool opening = false);
void ConfigureDropTarget(OverflowState& state, Tile& tile, int folder);
void AttachDragSource(OverflowState& state);
void DetachDragSource(OverflowState& state);
void EnsureMouseTracking();
std::wstring IconAssignmentKey(const IconInfo& info);
std::unordered_map<std::wstring, std::wstring> g_manualMoves;
void LoadManualMoves() {
    g_manualMoves.clear();
    wchar_t buffer[65536];
    auto path = ModuleDirectory() + L"assignments.ini";
    DWORD length = GetPrivateProfileSectionW(L"Moves", buffer, ARRAYSIZE(buffer), path.c_str());
    if (length >= ARRAYSIZE(buffer) - 2) return;
    for (const wchar_t* entry = buffer; *entry; entry += wcslen(entry) + 1) {
        std::wstring value(entry);
        auto at = value.find(L'=');
        if (at != std::wstring::npos) g_manualMoves.emplace(value.substr(0, at), value.substr(at + 1));
    }
}
void SubscribeOriginalIcons(OverflowState& state,
                            wfc::IObservableVector<wf::IInspectable> icons);

[[clang::no_destroy]] Markup::IXamlMember g_overflowIconsMember{nullptr};

wfc::IObservableVector<wf::IInspectable> GetOverflowIcons(
    Controls::Control const& overflow) {
    if (!g_overflowIconsMember) {
        g_overflowIconsMember = GetXamlMember(
            L"SystemTray.NotificationAreaOverflow", L"OverflowIcons");
        if (!g_overflowIconsMember) {
            return nullptr;
        }
    }
    try {
        return g_overflowIconsMember.GetValue(overflow)
            .try_as<wfc::IObservableVector<wf::IInspectable>>();
    } catch (...) {
        return nullptr;
    }
}

FrameworkElement FindDescendantByName(DependencyObject const& element,
                                      std::wstring_view name) {
    int count = Media::VisualTreeHelper::GetChildrenCount(element);
    for (int i = 0; i < count; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i);
        if (auto frameworkElement = child.try_as<FrameworkElement>()) {
            if (frameworkElement.Name() == name) {
                return frameworkElement;
            }
        }
        if (auto found = FindDescendantByName(child, name)) {
            return found;
        }
    }
    return nullptr;
}

// Identify the tray's list by its backing collection, not by an arbitrary
// ItemsControl: templates can contain other lists that must stay untouched.
Controls::ItemsControl FindItemsControlBySource(
    DependencyObject const& root,
    wf::IInspectable const& source,
    unsigned depth = 0) {
    if (!root || !source || depth > 32) {
        return nullptr;
    }
    if (auto items = root.try_as<Controls::ItemsControl>()) {
        if (items.ItemsSource() == source) {
            return items;
        }
    }
    int count = Media::VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; i++) {
        if (auto items = FindItemsControlBySource(
                Media::VisualTreeHelper::GetChild(root, i), source, depth + 1)) {
            return items;
        }
    }
    return nullptr;
}

Controls::ItemsControl FindOverflowItemsControl(Controls::Control const& overflow) {
    // Older Taskbar.View templates often rename the list; bind by source first.
    if (g_adaptation.adapter == 1) {
        if (auto icons = GetOverflowIcons(overflow)) {
            if (auto items = FindItemsControlBySource(overflow, icons)) return items;
        }
    }
    if (auto items = overflow.FindName(L"OverflowItemsControl")
                         .try_as<Controls::ItemsControl>()) {
        return items;
    }
    if (auto items = FindDescendantByName(overflow, L"OverflowItemsControl")
                         .try_as<Controls::ItemsControl>()) {
        return items;
    }
    if (auto icons = GetOverflowIcons(overflow)) {
        if (auto items = FindItemsControlBySource(overflow, icons)) {
            AppLog(L"Compatibility: found icon list by ItemsSource; name=%s",
                   items.Name().c_str());
            return items;
        }
    }
    return nullptr;
}

void LogOverflowTree(DependencyObject const& root, int& remaining, unsigned depth = 0) {
    if (!root || remaining-- <= 0 || depth > 12) {
        return;
    }
    auto element = root.try_as<FrameworkElement>();
    auto name = element ? element.Name() : winrt::hstring{};
    AppLog(L"Compatibility tree: depth=%u class=%s name=%s loaded=%d children=%d",
           depth, winrt::get_class_name(root).c_str(), name.c_str(),
           element ? element.IsLoaded() : false,
           Media::VisualTreeHelper::GetChildrenCount(root));
    int count = Media::VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count && remaining > 0; i++) {
        LogOverflowTree(Media::VisualTreeHelper::GetChild(root, i), remaining, depth + 1);
    }
}

////////////////////////////////////////////////////////////////////////////////
