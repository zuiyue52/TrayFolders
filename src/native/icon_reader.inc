// SystemTray.dll internals

// WindowsUdk.UI.Shell.INotificationAreaIcon6, from the public WinRT metadata in
// windowsudk.winmd, which is also where the IID and the member order come from.
constexpr GUID IID_INotificationAreaIcon6 = {
    0xD76CAD80,
    0x3103,
    0x54F4,
    {0x9A, 0xBE, 0x5F, 0x04, 0x7C, 0xBF, 0xE8, 0x5C}};
constexpr int kNotificationAreaIcon6_get_AppFriendlyName = 6;

// Resolved from symbols.
GUID* g_iconConfigurationIid;
void* g_iconConfigurationVftable;
void* g_iconConfiguration_get_DataModel;
GUID* g_notificationAreaIconsDataModelIid;
void* g_notificationAreaIconsDataModelVftable;
void* g_notificationAreaIconsDataModel_get_UDKObject;
GUID* g_iconContentViewModelIid;
void* g_imageContentViewModelBaseVftable;
void* g_imageContentViewModel_get_ToolTipText;
GUID* g_imageContentViewModelIid;
void* g_imageContentViewModelVftable;
void* g_imageContentViewModel_get_AutomationPropertyName;
void* g_imageContentViewModel_get_IconImageSource;

Markup::IXamlMember GetXamlMember(PCWSTR typeName, PCWSTR memberName) {
    auto provider =
        Application::Current().try_as<Markup::IXamlMetadataProvider>();
    if (!provider) {
        return nullptr;
    }
    auto xamlType = provider.GetXamlType(typeName);
    if (!xamlType) {
        return nullptr;
    }
    return xamlType.GetMember(memberName);
}

struct IconInfo {
    wf::IInspectable viewModel{nullptr};
    std::wstring appName;
    std::wstring toolTip;
    std::wstring name;
    std::wstring lowerAppName;
    std::wstring lowerToolTip;
    std::wstring lowerName;
    Media::ImageSource image{nullptr};
    int folder = -1;
};

[[clang::no_destroy]] Markup::IXamlMember g_contentVmMember{nullptr};
[[clang::no_destroy]] Markup::IXamlMember g_configurationMember{nullptr};

// Queries an interface of an internal SystemTray.dll type. The result is only
// returned if it's backed by the expected implementation vtable, which makes
// calling that implementation's methods, resolved from symbols, on it safe.
winrt::com_ptr<IUnknown> QueryVerifiedInterface(wf::IInspectable const& object,
                                                GUID const* iid,
                                                void* expectedVftable) {
    winrt::com_ptr<IUnknown> result;
    if (!object || !iid || !expectedVftable) {
        return result;
    }
    ((IUnknown*)winrt::get_abi(object))->QueryInterface(*iid, result.put_void());
    if (result && *(void**)result.get() != expectedVftable) {
        result = nullptr;
    }
    return result;
}

void* CallAbiGetter(IUnknown* object, void* getter) {
    void* value = nullptr;
    HRESULT hr = ((HRESULT(WINAPI*)(void*, void**))getter)(object, &value);
    return SUCCEEDED(hr) ? value : nullptr;
}

std::wstring CallStringGetter(IUnknown* object, void* getter) {
    winrt::hstring value;
    winrt::attach_abi(value, CallAbiGetter(object, getter));
    return NormalizeName(value);
}

// The app name comes from the executable, so it stays the same regardless of
// what the app puts in its tooltip.
void ReadAppName(IconInfo& info) {
    if (!g_iconConfiguration_get_DataModel ||
        !g_notificationAreaIconsDataModel_get_UDKObject) {
        return;
    }

    if (!g_configurationMember) {
        g_configurationMember =
            GetXamlMember(L"SystemTray.IconViewModel", L"Configuration");
        if (!g_configurationMember) {
            return;
        }
    }

    wf::IInspectable configuration{nullptr};
    try {
        configuration = g_configurationMember.GetValue(info.viewModel);
    } catch (...) {
        return;
    }

    auto configurationInterface = QueryVerifiedInterface(
        configuration, g_iconConfigurationIid, g_iconConfigurationVftable);
    if (!configurationInterface) {
        return;
    }

    wf::IInspectable dataModel{nullptr};
    winrt::attach_abi(dataModel,
                      CallAbiGetter(configurationInterface.get(),
                                    g_iconConfiguration_get_DataModel));

    auto dataModelInterface =
        QueryVerifiedInterface(dataModel, g_notificationAreaIconsDataModelIid,
                               g_notificationAreaIconsDataModelVftable);
    if (!dataModelInterface) {
        return;
    }

    wf::IInspectable udkIcon{nullptr};
    winrt::attach_abi(
        udkIcon, CallAbiGetter(dataModelInterface.get(),
                               g_notificationAreaIconsDataModel_get_UDKObject));
    if (!udkIcon) {
        return;
    }

    // Public interface, no vtable verification needed.
    winrt::com_ptr<IUnknown> icon6;
    ((IUnknown*)winrt::get_abi(udkIcon))
        ->QueryInterface(IID_INotificationAreaIcon6, icon6.put_void());
    if (icon6) {
        void* getter =
            (*(void***)icon6.get())[kNotificationAreaIcon6_get_AppFriendlyName];
        info.appName = CallStringGetter(icon6.get(), getter);
    }
}

void ReadIconInfo(IconInfo& info) {
    ReadAppName(info);

    if (!g_contentVmMember) {
        g_contentVmMember =
            GetXamlMember(L"SystemTray.IconViewModel", L"ContentVM");
        if (!g_contentVmMember) {
            return;
        }
    }

    wf::IInspectable contentViewModel{nullptr};
    try {
        contentViewModel = g_contentVmMember.GetValue(info.viewModel);
    } catch (...) {
        return;
    }

    if (g_imageContentViewModel_get_ToolTipText) {
        if (auto baseInterface = QueryVerifiedInterface(
                contentViewModel, g_iconContentViewModelIid,
                g_imageContentViewModelBaseVftable)) {
            info.toolTip =
                CallStringGetter(baseInterface.get(),
                                 g_imageContentViewModel_get_ToolTipText);
        }
    }

    // Only image icons (regular app icons) implement this interface.
    auto imageInterface =
        QueryVerifiedInterface(contentViewModel, g_imageContentViewModelIid,
                               g_imageContentViewModelVftable);
    if (!imageInterface) {
        return;
    }

    if (g_imageContentViewModel_get_AutomationPropertyName) {
        info.name = CallStringGetter(
            imageInterface.get(),
            g_imageContentViewModel_get_AutomationPropertyName);
    }

    if (g_imageContentViewModel_get_IconImageSource) {
        winrt::attach_abi(
            info.image,
            CallAbiGetter(imageInterface.get(),
                          g_imageContentViewModel_get_IconImageSource));
    }
}

////////////////////////////////////////////////////////////////////////////////
