// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <windows.h>
#include <cstdint>
enum class SymbolKind { Function, Guid, Vtable };
struct RequiredSymbol { const wchar_t* name; SymbolKind kind; };
constexpr RequiredSymbol kRequiredSymbols[] = {
    {LR"sym(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IIconConfiguration>)sym", SymbolKind::Guid},
    {LR"sym(public: __cdecl winrt::impl::consume_Windows_UI_Xaml_IUIElement<struct winrt::SystemTray::NotificationAreaOverflow>::Measure(struct winrt::Windows::Foundation::Size const &)const )sym", SymbolKind::Function},
    {LR"sym(public: void __cdecl winrt::SystemTray::OverflowXamlIslandManager::Show(struct tagPOINT,enum winrt::WindowsUdk::UI::Shell::InputDeviceKind))sym", SymbolKind::Function},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::get_IconImageSource(void * *))sym", SymbolKind::Function},
    {LR"sym(const winrt::impl::produce<struct winrt::SystemTray::implementation::IconConfiguration,struct winrt::SystemTray::IIconConfiguration>::`vftable')sym", SymbolKind::Vtable},
    {LR"sym(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IIconContentViewModel>)sym", SymbolKind::Guid},
    {LR"sym(const winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::`vftable')sym", SymbolKind::Vtable},
    {LR"sym(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::INotificationAreaIconsDataModel>)sym", SymbolKind::Guid},
    {LR"sym(struct guid::guid const winrt::impl::guid_v<struct winrt::SystemTray::IImageIconContentViewModel>)sym", SymbolKind::Guid},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::NotificationAreaIconsDataModel,struct winrt::SystemTray::INotificationAreaIconsDataModel>::get_UDKObject(void * *))sym", SymbolKind::Function},
    {LR"sym(const winrt::impl::produce<struct winrt::SystemTray::implementation::NotificationAreaIconsDataModel,struct winrt::SystemTray::INotificationAreaIconsDataModel>::`vftable')sym", SymbolKind::Vtable},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IImageIconContentViewModel>::get_AutomationPropertyName(void * *))sym", SymbolKind::Function},
    {LR"sym(const winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IIconContentViewModel>::`vftable')sym", SymbolKind::Vtable},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::ImageIconContentViewModel,struct winrt::SystemTray::IIconContentViewModel>::get_ToolTipText(void * *))sym", SymbolKind::Function},
    {LR"sym(public: virtual int __cdecl winrt::impl::produce<struct winrt::SystemTray::implementation::IconConfiguration,struct winrt::SystemTray::IIconConfiguration>::get_DataModel(void * *))sym", SymbolKind::Function},
};
constexpr size_t kSymbolCount = ARRAYSIZE(kRequiredSymbols);
