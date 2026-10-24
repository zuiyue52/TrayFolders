// Rebuilding the displayed items

void SetDisplayedItems(OverflowState& state,
                       std::vector<wf::IInspectable> const& items) {
    auto& displayed = state.displayed;

    // Apply minimal edits so that unchanged icons keep their containers.
    for (uint32_t i = 0; i < items.size(); i++) {
        while (i < displayed.Size() && displayed.GetAt(i) != items[i]) {
            bool existsLater = false;
            for (uint32_t j = i + 1; j < displayed.Size(); j++) {
                if (displayed.GetAt(j) == items[i]) {
                    existsLater = true;
                    break;
                }
            }
            if (!existsLater) {
                break;
            }
            displayed.RemoveAt(i);
        }

        if (i < displayed.Size() && displayed.GetAt(i) == items[i]) {
            continue;
        }

        // Make sure the item isn't present further down before inserting it,
        // a UIElement can't be in the list twice.
        for (uint32_t j = i; j < displayed.Size(); j++) {
            if (displayed.GetAt(j) == items[i]) {
                displayed.RemoveAt(j);
                break;
            }
        }
        displayed.InsertAt(i, items[i]);
    }

    while (displayed.Size() > items.size()) {
        displayed.RemoveAtEnd();
    }
}

void EnsureItemsSource(OverflowState& state) {
    if (state.itemsSourceOverridden || state.detaching || g_unloading) {
        return;
    }
    auto itemsControl = state.itemsControl.get();
    if (!itemsControl) {
        return;
    }
    if (itemsControl.ItemsSource() != state.displayed) {
        state.settingItemsSource = true;
        auto reset =
            ScopeExit([&state] { state.settingItemsSource = false; });
        itemsControl.ItemsSource(state.displayed);
    }
}

void Rebuild(OverflowState& state, bool opening) {
    if (state.rebuilding || state.detaching || state.itemsSourceOverridden ||
        g_unloading || !state.displayed) {
        return;
    }
    state.rebuilding = true;
    auto reset = ScopeExit([&state] { state.rebuilding = false; });

    try {
        if (auto overflow = state.overflow.get()) {
            auto icons = GetOverflowIcons(overflow);
            if (icons != state.originalIcons) {
                if (!state.originalItemsSourceBinding &&
                    state.originalItemsSource == state.originalIcons) {
                    state.originalItemsSource = icons;
                }
                SubscribeOriginalIcons(state, icons);
            }
        }
        if (!state.originalIcons) {
            return;
        }

        const auto& folders = g_settings.folders;
        double tileWidth;
        double tileHeight;
        GetItemSize(state, &tileWidth, &tileHeight);
        if (state.folderTiles.size() != folders.size() ||
            state.tileWidth != tileWidth || state.tileHeight != tileHeight) {
            ClearTiles(state);
            state.folderTiles.resize(folders.size());
            state.tileWidth = tileWidth;
            state.tileHeight = tileHeight;
        }

        if (opening) {
            state.iconCache.clear();
        }
        std::unordered_map<void*, std::shared_ptr<IconInfo>> iconCache;
        std::vector<std::shared_ptr<IconInfo>> icons;
        uint32_t count = state.originalIcons.Size();
        icons.reserve(count);
        for (uint32_t i = 0; i < count; i++) {
            auto viewModel = state.originalIcons.GetAt(i);
            void* key = winrt::get_abi(viewModel);
            std::shared_ptr<IconInfo> info;
            if (auto it = state.iconCache.find(key);
                it != state.iconCache.end()) {
                info = it->second;
            } else {
                info = std::make_shared<IconInfo>();
                info->viewModel = viewModel;
                ReadIconInfo(*info);
                info->lowerAppName = ToLower(info->appName);
                info->lowerToolTip = ToLower(info->toolTip);
                info->lowerName = ToLower(info->name);
            }
            info->folder = FindFolderForIcon(
                {info->lowerAppName, info->lowerToolTip, info->lowerName});
            if (auto move = g_manualMoves.find(IconAssignmentKey(*info)); move != g_manualMoves.end()) {
                if (move->second == L"@root") info->folder = -1;
                else for (size_t k = 0; k < folders.size(); ++k) {
                    if (folders[k].id == move->second) { info->folder = static_cast<int>(k); break; }
                }
            }
            iconCache.emplace(key, info);
            icons.push_back(std::move(info));
        }
        state.iconCache = std::move(iconCache);
        std::wstring inventory;
        for (const auto& info : icons) {
            inventory += NormalizeName(info->appName) + L"\t" +
                         NormalizeName(info->toolTip) + L"\t" +
                         NormalizeName(info->name) + L"\t" + std::to_wstring(info->folder) + L"\t" +
                         (info->folder >= 0 ? folders[info->folder].id : L"") + L"\n";
        }
        QueueInventory(std::move(inventory));

        std::vector<int> folderCounts(folders.size());
        for (const auto& icon : icons) {
            if (icon->folder >= 0) {
                folderCounts[icon->folder]++;
            }
        }

        if (state.openFolder >= (int)folders.size() ||
            (state.openFolder >= 0 && folderCounts[state.openFolder] == 0 &&
             !g_settings.showEmptyFolders)) {
            state.openFolder = -1;
        }

        std::vector<wf::IInspectable> items;

        if (state.openFolder >= 0) {
            Tile& backTile = EnsureBackTile(state);
            const auto& folder = folders[state.openFolder];
            Automation::AutomationProperties::SetName(backTile.button,
                                                      L"Back");
            Controls::ToolTipService::SetToolTip(
                backTile.button, winrt::box_value(L"Back (" + folder.name + L")"));
            items.push_back(backTile.button);
            for (const auto& icon : icons) {
                if (icon->folder == state.openFolder) {
                    items.push_back(icon->viewModel);
                }
            }
        } else {
            std::vector<wf::IInspectable> folderItems;
            for (size_t k = 0; k < folders.size(); k++) {
                if (folderCounts[k] == 0 && !g_settings.showEmptyFolders) {
                    continue;
                }
                std::vector<Media::ImageSource> images;
                for (const auto& icon : icons) {
                    if (icon->folder == (int)k && icon->image) {
                        images.push_back(icon->image);
                        if (images.size() == 4) {
                            break;
                        }
                    }
                }
                Tile& tile = EnsureFolderTile(state, (int)k);
                UpdateFolderTile(state, tile, folders[k], images,
                                 folderCounts[k]);
                folderItems.push_back(tile.button);
            }

            if (!g_settings.foldersAtEnd) {
                items = folderItems;
            }
            for (const auto& icon : icons) {
                if (icon->folder < 0) {
                    items.push_back(icon->viewModel);
                }
            }
            if (g_settings.foldersAtEnd) {
                items.insert(items.end(), folderItems.begin(),
                             folderItems.end());
            }
        }

        SetDisplayedItems(state, items);
        EnsureItemsSource(state);
    } catch (winrt::hresult_error const& ex) {
        AppLog(L"Rebuild failed: %08X %s", ex.code(), ex.message().c_str());
    } catch (...) {
        AppLog(L"Rebuild failed");
    }
}

void PrepareOpening(OverflowState& state) {
    if (state.openingPrepared || state.rebuilding || state.detaching) {
        return;
    }
    // Set this before rebuilding: XAML can synchronously reenter Measure.
    state.openingPrepared = true;
    state.openFolder = -1;
    Rebuild(state, /*opening=*/true);
}

void RefreshTileSize(OverflowState& state) {
    auto overflow = state.overflow.get();
    if (!overflow) {
        return;
    }

    // Native Show has already measured the grid. Only force another layout
    // when the realized tile size differs from the pre-Measure fallback.
    double width;
    double height;
    GetItemSize(state, &width, &height);
    if (width != state.tileWidth || height != state.tileHeight) {
        Rebuild(state);
        overflow.UpdateLayout();
    }
}

void FocusFirstItem(OverflowState& state) {
    auto itemsControl = state.itemsControl.get();
    if (!itemsControl || state.displayed.Size() == 0) {
        return;
    }
    itemsControl.UpdateLayout();
    if (auto button = state.displayed.GetAt(0).try_as<Controls::Button>()) {
        button.Focus(FocusState::Programmatic);
    }
}

void OpenFolder(OverflowState& state, int folder) {
    AppLog(L"Opening folder %d", folder);
    state.openFolder = folder;
    Rebuild(state);
    FocusFirstItem(state);
}

void CloseFolder(OverflowState& state) {
    AppLog(L"Closing folder %d", state.openFolder);
    int previous = state.openFolder;
    state.openFolder = -1;
    Rebuild(state);

    if (previous >= 0 && previous < (int)state.folderTiles.size()) {
        if (auto itemsControl = state.itemsControl.get()) {
            itemsControl.UpdateLayout();
        }
        if (auto button = state.folderTiles[previous].button) {
            button.Focus(FocusState::Programmatic);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
