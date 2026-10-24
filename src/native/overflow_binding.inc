// Attaching to the overflow

void SubscribeOriginalIcons(OverflowState& state,
                            wfc::IObservableVector<wf::IInspectable> icons) {
    if (state.originalIcons == icons) {
        return;
    }

    if (state.originalIcons && state.originalIconsChangedToken) {
        state.originalIcons.VectorChanged(state.originalIconsChangedToken);
        state.originalIconsChangedToken = {};
    }

    state.originalIcons = icons;
    if (!icons) {
        return;
    }

    state.originalIconsChangedToken = icons.VectorChanged(
        [weakState = state.weak_from_this()](
            wfc::IObservableVector<wf::IInspectable> const&,
            wfc::IVectorChangedEventArgs const&) {
            auto state = weakState.lock();
            if (!state || !state->attached || state->detaching || g_unloading) {
                return;
            }
            if (state->islandWindow && !IsWindowVisible(state->islandWindow)) {
                return;
            }
            Rebuild(*state);
        });
}

void OnItemsSourceChanged(OverflowState& state) {
    if (state.settingItemsSource || state.detaching || !state.attached ||
        g_unloading) {
        return;
    }
    auto itemsControl = state.itemsControl.get();
    auto overflow = state.overflow.get();
    if (!itemsControl || !overflow ||
        itemsControl.ItemsSource() == state.displayed) {
        return;
    }

    // Adopt external property changes and only replace the shell's own list.
    AppLog(L"ItemsSource replaced externally");
    auto property = Controls::ItemsControl::ItemsSourceProperty();
    auto binding = itemsControl.GetBindingExpression(property);
    state.originalItemsSourceBinding =
        binding ? binding.ParentBinding() : nullptr;
    state.originalItemsSource = itemsControl.ReadLocalValue(property);
    auto icons = GetOverflowIcons(overflow);
    state.itemsSourceOverridden = !icons || itemsControl.ItemsSource() != icons;
    SubscribeOriginalIcons(state, icons);
    Rebuild(state);
    EnsureItemsSource(state);
}

void Detach(OverflowState& state);

bool ValidateOverflow(Controls::Control const& overflow, Controls::ItemsControl const& itemsControl) {
    // Inspect first; never replace a different list or an unknown view model.
    auto icons = GetOverflowIcons(overflow);
    if (!icons || itemsControl.ItemsSource() != icons ||
        !GetXamlMember(L"SystemTray.IconViewModel", L"Configuration") ||
        !GetXamlMember(L"SystemTray.IconViewModel", L"ContentVM")) return false;
    bool readable = icons.Size() == 0;
    for (uint32_t i = 0; i < icons.Size() && i < 64; i++) {
        IconInfo info; info.viewModel = icons.GetAt(i);
        ReadIconInfo(info);
        if (!info.appName.empty() || !info.toolTip.empty() || !info.name.empty()) { readable = true; break; }
    }
    return readable;
}

void Attach(OverflowState& state,
            Controls::Control const& overflow,
            Controls::ItemsControl const& itemsControl) try {
    AppLog(L"Attaching to overflow");

    if (g_runtimeRejected) return;
    if (!ValidateOverflow(overflow, itemsControl)) {
        g_runtimeRejected = true;
        return;
    }

    state.overflow = overflow;
    state.itemsControl = itemsControl;
    auto property = Controls::ItemsControl::ItemsSourceProperty();
    auto binding = itemsControl.GetBindingExpression(property);
    state.originalItemsSourceBinding =
        binding ? binding.ParentBinding() : nullptr;
    state.originalItemsSource = itemsControl.ReadLocalValue(property);
    state.displayed = winrt::single_threaded_observable_vector<wf::IInspectable>();
    state.detaching = false;

    state.itemsSourceCallbackToken = itemsControl.RegisterPropertyChangedCallback(
        Controls::ItemsControl::ItemsSourceProperty(),
        [weakState = state.weak_from_this()](
            DependencyObject const&, DependencyProperty const&) {
            if (auto state = weakState.lock()) {
                OnItemsSourceChanged(*state);
            }
        });
    state.itemsSourceCallbackRegistered = true;

    auto icons = GetOverflowIcons(overflow);
    state.itemsSourceOverridden = itemsControl.ItemsSource() != icons;
    SubscribeOriginalIcons(state, icons);
    state.attached = true;
    AttachDragSource(state);
    state.openingPrepared = false;
    PrepareOpening(state);
    g_runtimeValidated = true;
} catch (...) {
    Detach(state);
    throw;
}

void Detach(OverflowState& state) {
    DetachDragSource(state);
    AppLog(L"Detaching from overflow");
    state.detaching = true;
    auto resetDetaching = ScopeExit([&state] { state.detaching = false; });
    state.attached = false;

    auto overflow = state.overflow.get();
    auto itemsControl = state.itemsControl.get();
    auto originalIcons = state.originalIcons;

    // Cleanup steps that can fail don't stop the ones after them; the first
    // failure is reported at the end. (Removing an event handler doesn't
    // throw, the C++/WinRT event removers are noexcept.)
    std::exception_ptr error;
    auto step = [&error](auto&& function) {
        try {
            function();
            return true;
        } catch (...) {
            if (!error) {
                error = std::current_exception();
            }
            return false;
        }
    };

    if (itemsControl && state.itemsSourceCallbackRegistered) {
        step([&] {
            itemsControl.UnregisterPropertyChangedCallback(
                Controls::ItemsControl::ItemsSourceProperty(),
                state.itemsSourceCallbackToken);
            state.itemsSourceCallbackRegistered = false;
        });
    }

    if (auto token = std::exchange(state.loadedToken, {}); token && overflow) {
        overflow.Loaded(token);
    }

    if (state.originalIcons && state.originalIconsChangedToken) {
        state.originalIcons.VectorChanged(
            std::exchange(state.originalIconsChangedToken, {}));
    }
    ClearTiles(state);
    state.openFolder = -1;

    // Try to give the shell its own list back before reporting any failure. If
    // the original value can't be put back, fall back to the shell's current
    // list, then to no value, so that the mod's list isn't left installed. The
    // ItemsSource callback ignores these changes while detaching.
    if (itemsControl && state.displayed) {
        auto property = Controls::ItemsControl::ItemsSourceProperty();
        auto replaceItemsSource = [&](auto&& apply) {
            step([&] {
                if (itemsControl.ItemsSource() != state.displayed) {
                    return;
                }
                state.settingItemsSource = true;
                auto reset =
                    ScopeExit([&state] { state.settingItemsSource = false; });
                apply();
                itemsControl.UpdateLayout();
            });
        };

        replaceItemsSource([&] {
            if (state.originalItemsSourceBinding) {
                itemsControl.SetBinding(property,
                                        state.originalItemsSourceBinding);
            } else if (state.originalItemsSource ==
                       DependencyProperty::UnsetValue()) {
                itemsControl.ClearValue(property);
            } else {
                if (overflow && state.originalItemsSource == originalIcons) {
                    if (auto icons = GetOverflowIcons(overflow)) {
                        state.originalItemsSource = icons;
                    }
                }
                itemsControl.SetValue(property, state.originalItemsSource);
            }
        });

        replaceItemsSource([&] {
            if (auto icons = overflow ? GetOverflowIcons(overflow) : nullptr) {
                itemsControl.SetValue(property, icons);
            } else {
                itemsControl.ClearValue(property);
            }
        });
    }

    // If the mod's list is still installed anyway, show all original icons in
    // it so navigation isn't needed to reach them.
    step([&] {
        if (!state.displayed) {
            return;
        }
        if (itemsControl && itemsControl.ItemsSource() == state.displayed) {
            std::vector<wf::IInspectable> icons;
            if (originalIcons) {
                for (auto const& icon : originalIcons) {
                    icons.push_back(icon);
                }
            }
            SetDisplayedItems(state, icons);
        } else {
            state.displayed.Clear();
        }
    });

    state.overflow = nullptr;
    state.itemsControl = nullptr;
    state.displayed = nullptr;
    state.originalIcons = nullptr;
    state.originalItemsSource = nullptr;
    state.originalItemsSourceBinding = nullptr;
    state.itemsSourceCallbackRegistered = false;
    state.itemsSourceOverridden = false;
    state.itemsSourceCallbackToken = 0;
    state.openFolder = -1;
    state.islandWindow = nullptr;
    state.iconCache.clear();
    state.tileWidth = 0;
    state.tileHeight = 0;

    if (error) {
        std::rethrow_exception(error);
    }
}

std::shared_ptr<OverflowState> FindState(void* manager) {
    for (auto& state : *g_states) {
        if (state->manager == manager) {
            return state;
        }
    }
    return nullptr;
}

std::shared_ptr<OverflowState> EnsureAttached(
    void* manager,
    Controls::Control const& overflow) {
    if (!manager || !overflow || g_unloading) {
        return nullptr;
    }

    g_overflowThreadId = GetCurrentThreadId();
    if (g_settings.folders.empty()) {
        return nullptr;
    }

    auto state = FindState(manager);
    if (state && state->detaching) {
        return nullptr;
    }
    if (state && state->attached && state->overflow.get() == overflow &&
        state->itemsControl.get()) {
        return state;
    }

    auto states = *g_states;
    for (auto& other : states) {
        if (other->manager != manager && !other->detaching &&
            !other->overflow.get()) {
            std::erase(*g_states, other);
            Detach(*other);
        }
    }

    overflow.ApplyTemplate();
    auto itemsControl = FindOverflowItemsControl(overflow);
    if (state && state->attached && state->overflow.get() == overflow &&
        itemsControl && state->itemsControl.get() == itemsControl) {
        return state;
    }
    if (state) {
        if (!itemsControl && state->overflow.get() == overflow &&
            state->loadedToken) {
            return nullptr;
        }
        Detach(*state);
    } else {
        state = std::make_shared<OverflowState>();
        state->manager = manager;
        g_states->push_back(state);
    }
    if (!itemsControl) {
        if (!state->diagnosticTreeLogged) {
            state->diagnosticTreeLogged = true;
            int remaining = 40;
            LogOverflowTree(overflow, remaining);
        }
        if (!state->loadedToken) {
            AppLog(L"Overflow template not ready, waiting for Loaded");
            state->overflow = overflow;
            state->detaching = false;
            state->loadedToken = overflow.Loaded(
                [weakState = state->weak_from_this()](
                    wf::IInspectable const& sender, RoutedEventArgs const&) {
                    auto state = weakState.lock();
                    if (!state || state->detaching || g_unloading) {
                        return;
                    }
                    if (auto overflow = state->overflow.get()) {
                        overflow.Loaded(state->loadedToken);
                    }
                    state->loadedToken = {};
                    try {
                        auto overflow = sender.as<Controls::Control>();
                        if (state->overflow.get() != overflow) {
                            return;
                        }
                        auto itemsControl = FindOverflowItemsControl(overflow);
                        if (itemsControl) {
                            Attach(*state, overflow, itemsControl);
                        } else {
                            AppLog(L"OverflowItemsControl not found");
                        }
                    } catch (...) {
                        AppLog(L"Loaded: attach failed");
                    }
                });
        }
        return nullptr;
    }

    Attach(*state, overflow, itemsControl);
    return state;
}

SRWLOCK g_pendingSettingsLock = SRWLOCK_INIT;
std::optional<Settings> g_pendingSettings;

void ApplyPendingSettings() {
    std::optional<Settings> settings;
    AcquireSRWLockExclusive(&g_pendingSettingsLock);
    settings.swap(g_pendingSettings);
    ReleaseSRWLockExclusive(&g_pendingSettingsLock);
    if (!settings || g_unloading) {
        return;
    }

    g_settings = std::move(*settings);
    LoadManualMoves();
    auto states = *g_states;
    for (auto& state : states) {
        if (state->detaching) {
            continue;
        }
        try {
            if (g_settings.folders.empty()) {
                std::erase(*g_states, state);
                Detach(*state);
            } else if (state->attached) {
                state->openFolder = -1;
                Rebuild(*state);
            }
        } catch (...) {
            AppLog(L"Applying settings failed");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
