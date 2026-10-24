// Windows symbol RVAs. Valid only for the exact module identity below.
#pragma once
#include <cstdint>
struct SymbolRva { const wchar_t* name; uint32_t rva; };
constexpr uint32_t kModuleTimestamp = 1775531627;
constexpr uint32_t kModuleImageSize = 11526144;
constexpr GUID kPdbGuid = {0x0AE0E302,0xE8E3,0x4BE2,{0x9C,0xC3,0x03,0xA7,0xA6,0x2A,0xBC,0xA7}};
constexpr SymbolRva kSymbols[] = {
    {LR"sym(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IIconConfiguration>)sym", 9692976},
    {LR"sym(public: __cdecl winrt::impl::consume_Windows_UI_Xaml_IUIElement<struct winrt::SystemTray::NotificationAreaOverflow>::Measure(struct winrt::Windows::Foundation::Size const &)const )sym", 2819264},
    {LR"sym(public: void __cdecl winrt::SystemTray::OverflowXamlIslandManager::Show(struct tagPOINT,enum winrt::WindowsUdk::UI::Shell::InputDeviceKind))sym", 2392232},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::get_IconImageSource(void * *))sym", 1377984},
    {LR"sym(const winrt::impl::produce<struct winrt::SystemTray::implementation::IconConfiguration,struct winrt::SystemTray::IIconConfiguration>::`vftable')sym", 9706504},
    {LR"sym(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IIconContentViewModel>)sym", 9680136},
    {LR"sym(const winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::`vftable')sym", 9701248},
    {LR"sym(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::INotificationAreaIconsDataModel>)sym", 9689688},
    {LR"sym(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IImageIconContentViewModel>)sym", 9657928},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::NotificationAreaIconsDataModel,struct winrt::SystemTray::INotificationAreaIconsDataModel>::get_UDKObject(void * *))sym", 644448},
    {LR"sym(const winrt::impl::produce<struct winrt::SystemTray::implementation::NotificationAreaIconsDataModel,struct winrt::SystemTray::INotificationAreaIconsDataModel>::`vftable')sym", 9652424},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::get_AutomationPropertyName(void * *))sym", 2260784},
    {LR"sym(const winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IIconContentViewModel>::`vftable')sym", 9701440},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IIconContentViewModel>::get_ToolTipText(void * *))sym", 805008},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::IconConfiguration,struct winrt::SystemTray::IIconConfiguration>::get_DataModel(void * *))sym", 1427632},
};
