// Module loading

VS_FIXEDFILEINFO* GetModuleVersionInfo(HMODULE hModule, UINT* puPtrLen) {
    void* pFixedFileInfo = nullptr;
    UINT uPtrLen = 0;

    HRSRC hResource =
        FindResource(hModule, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    if (hResource) {
        HGLOBAL hGlobal = LoadResource(hModule, hResource);
        if (hGlobal) {
            void* pData = LockResource(hGlobal);
            if (pData) {
                if (!VerQueryValue(pData, L"\\", &pFixedFileInfo, &uPtrLen) ||
                    uPtrLen == 0) {
                    pFixedFileInfo = nullptr;
                    uPtrLen = 0;
                }
            }
        }
    }

    if (puPtrLen) {
        *puPtrLen = uPtrLen;
    }

    return (VS_FIXEDFILEINFO*)pFixedFileInfo;
}

HMODULE GetSystemTrayModuleHandle() {
    auto module = GetModuleHandleW(L"SystemTray.dll");
    return module ? module : GetModuleHandleW(L"Taskbar.View.dll");
}
std::atomic<bool> g_systemTrayModuleHooked;

bool HookSystemTraySymbols(HMODULE module) {
    // SystemTray.dll, Taskbar.View.dll
    SymbolHook symbolHooks[] = {
        {
            {LR"(public: void __cdecl winrt::SystemTray::OverflowXamlIslandManager::Show(struct tagPOINT,enum winrt::WindowsUdk::UI::Shell::InputDeviceKind))"},
            &OverflowXamlIslandManager_Show_Original,
            OverflowXamlIslandManager_Show_Hook,
        },
        {
            {LR"(public: __cdecl winrt::impl::consume_Windows_UI_Xaml_IUIElement<struct winrt::SystemTray::NotificationAreaOverflow>::Measure(struct winrt::Windows::Foundation::Size const &)const )"},
            &NotificationAreaOverflow_Measure_Original,
            NotificationAreaOverflow_Measure_Hook,
        },
        {
            {LR"(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IIconConfiguration>)"},
            &g_iconConfigurationIid,
            nullptr,
            true,
        },
        {
            {LR"(const winrt::impl::produce<struct winrt::SystemTray::implementation::IconConfiguration,struct winrt::SystemTray::IIconConfiguration>::`vftable')"},
            &g_iconConfigurationVftable,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::IconConfiguration,struct winrt::SystemTray::IIconConfiguration>::get_DataModel(void * *))"},
            &g_iconConfiguration_get_DataModel,
            nullptr,
            true,
        },
        {
            {LR"(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::INotificationAreaIconsDataModel>)"},
            &g_notificationAreaIconsDataModelIid,
            nullptr,
            true,
        },
        {
            {LR"(const winrt::impl::produce<struct winrt::SystemTray::implementation::NotificationAreaIconsDataModel,struct winrt::SystemTray::INotificationAreaIconsDataModel>::`vftable')"},
            &g_notificationAreaIconsDataModelVftable,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::NotificationAreaIconsDataModel,struct winrt::SystemTray::INotificationAreaIconsDataModel>::get_UDKObject(void * *))"},
            &g_notificationAreaIconsDataModel_get_UDKObject,
            nullptr,
            true,
        },
        {
            {LR"(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IIconContentViewModel>)"},
            &g_iconContentViewModelIid,
            nullptr,
            true,
        },
        {
            {LR"(const winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IIconContentViewModel>::`vftable')"},
            &g_imageContentViewModelBaseVftable,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IIconContentViewModel>::get_ToolTipText(void * *))"},
            &g_imageContentViewModel_get_ToolTipText,
            nullptr,
            true,
        },
        {
            {LR"(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IImageIconContentViewModel>)"},
            &g_imageContentViewModelIid,
            nullptr,
            true,
        },
        {
            {LR"(const winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::`vftable')"},
            &g_imageContentViewModelVftable,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::get_AutomationPropertyName(void * *))"},
            &g_imageContentViewModel_get_AutomationPropertyName,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::get_IconImageSource(void * *))"},
            &g_imageContentViewModel_get_IconImageSource,
            nullptr,
            true,
        },
    };

    if (!HookSymbols(module, symbolHooks,
                                    ARRAYSIZE(symbolHooks))) {
        AppLog(L"HookSymbols failed");
        return false;
    }

    bool appNames = g_iconConfigurationIid && g_iconConfigurationVftable &&
                    g_iconConfiguration_get_DataModel &&
                    g_notificationAreaIconsDataModelIid &&
                    g_notificationAreaIconsDataModelVftable &&
                    g_notificationAreaIconsDataModel_get_UDKObject;
    bool toolTips = g_iconContentViewModelIid &&
                    g_imageContentViewModelBaseVftable &&
                    g_imageContentViewModel_get_ToolTipText;
    bool names = g_imageContentViewModelIid && g_imageContentViewModelVftable &&
                 g_imageContentViewModel_get_AutomationPropertyName;
    AppLog(L"Icon names: app=%d tooltip=%d name=%d", appNames, toolTips,
           names);

    if (!appNames) {
        AppLog(L"App name matching unavailable");
    }

    if (!toolTips && !names) {
        AppLog(L"Couldn't find the icon tooltip getters");
        return false;
    }

    return true;
}
