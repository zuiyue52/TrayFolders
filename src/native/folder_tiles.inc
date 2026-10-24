// Tiles

void GetItemSize(OverflowState& state, double* width, double* height) {
    *width = 40;
    *height = 40;
    if (auto itemsControl = state.itemsControl.get()) {
        if (auto wrapGrid =
                itemsControl.ItemsPanelRoot().try_as<Controls::WrapGrid>()) {
            double w = wrapGrid.ItemWidth();
            double h = wrapGrid.ItemHeight();
            if (!std::isnan(w) && w > 0) {
                *width = w;
            }
            if (!std::isnan(h) && h > 0) {
                *height = h;
            }
        }
    }
}

Controls::Button CreateTileButton(double width, double height) {
    std::wstring xaml = LR"(
<Button xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Padding="0" BorderThickness="0" Background="Transparent"
        UseSystemFocusVisuals="True" IsTabStop="True">
  <Button.Template>
    <ControlTemplate TargetType="Button">
      <Grid Background="Transparent">
        <VisualStateManager.VisualStateGroups>
          <VisualStateGroup x:Name="CommonStates">
            <VisualState x:Name="Normal"/>
            <VisualState x:Name="PointerOver">
              <VisualState.Setters>
                <Setter Target="HoverBorder.Background" Value="{ThemeResource SubtleFillColorSecondaryBrush}"/>
              </VisualState.Setters>
            </VisualState>
            <VisualState x:Name="Pressed">
              <VisualState.Setters>
                <Setter Target="HoverBorder.Background" Value="{ThemeResource SubtleFillColorTertiaryBrush}"/>
                <Setter Target="Presenter.Opacity" Value="0.786"/>
              </VisualState.Setters>
            </VisualState>
            <VisualState x:Name="Disabled"/>
          </VisualStateGroup>
        </VisualStateManager.VisualStateGroups>
        <Border x:Name="HoverBorder" CornerRadius="4" Background="Transparent"
                Width="{HOVERWIDTH}" Height="{HOVERHEIGHT}"
                HorizontalAlignment="Center" VerticalAlignment="Center"/>
        <ContentPresenter x:Name="Presenter" Content="{TemplateBinding Content}"
                          HorizontalAlignment="Center"
                          VerticalAlignment="Center"/>
      </Grid>
    </ControlTemplate>
  </Button.Template>
</Button>)";

    auto replace = [&xaml](std::wstring_view from, std::wstring const& to) {
        size_t pos = xaml.find(from);
        if (pos != std::wstring::npos) {
            xaml.replace(pos, from.size(), to);
        }
    };
    replace(L"{HOVERWIDTH}", std::to_wstring((int)std::max(width - 4, 16.0)));
    replace(L"{HOVERHEIGHT}",
            std::to_wstring((int)std::max(height - 10, 16.0)));

    auto button = Markup::XamlReader::Load(xaml).as<Controls::Button>();
    button.Width(width);
    button.Height(height);
    return button;
}

Controls::TextBlock CreateGlyph(std::wstring const& glyph, double fontSize) {
    Controls::TextBlock textBlock;
    textBlock.Text(glyph);
    textBlock.FontFamily(Media::FontFamily(L"Segoe Fluent Icons"));
    textBlock.FontSize(fontSize);
    textBlock.HorizontalAlignment(HorizontalAlignment::Center);
    textBlock.VerticalAlignment(VerticalAlignment::Center);
    textBlock.TextLineBounds(TextLineBounds::Tight);
    textBlock.IsHitTestVisible(false);
    return textBlock;
}

UIElement CreateFolderPreview(double itemSize) {
    std::wstring xaml = LR"(
<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
      Width="{PLATE}" Height="{PLATE}" CornerRadius="{RADIUS}"
      BorderThickness="1" Padding="1"
      Background="{ThemeResource ControlFillColorDefaultBrush}"
      BorderBrush="{ThemeResource SubtleFillColorTertiaryBrush}"
      IsHitTestVisible="False">
  <Grid.RowDefinitions>
    <RowDefinition Height="*"/>
    <RowDefinition Height="*"/>
  </Grid.RowDefinitions>
  <Grid.ColumnDefinitions>
    <ColumnDefinition Width="*"/>
    <ColumnDefinition Width="*"/>
  </Grid.ColumnDefinitions>
  <Image Grid.Row="0" Grid.Column="0" Width="{ICON}" Height="{ICON}" Stretch="Uniform"
         HorizontalAlignment="Center" VerticalAlignment="Center"/>
  <Image Grid.Row="0" Grid.Column="1" Width="{ICON}" Height="{ICON}" Stretch="Uniform"
         HorizontalAlignment="Center" VerticalAlignment="Center"/>
  <Image Grid.Row="1" Grid.Column="0" Width="{ICON}" Height="{ICON}" Stretch="Uniform"
         HorizontalAlignment="Center" VerticalAlignment="Center"/>
  <Image Grid.Row="1" Grid.Column="1" Width="{ICON}" Height="{ICON}" Stretch="Uniform"
         HorizontalAlignment="Center" VerticalAlignment="Center"/>
</Grid>)";

    auto replace = [&xaml](std::wstring_view from, std::wstring const& to) {
        for (size_t pos = xaml.find(from); pos != std::wstring::npos;
             pos = xaml.find(from, pos + to.size())) {
            xaml.replace(pos, from.size(), to);
        }
    };
    replace(L"{PLATE}", std::to_wstring((int)std::max(itemSize * 0.55, 12.0)));
    replace(L"{RADIUS}", std::to_wstring((int)std::max(itemSize * 0.125, 3.0)));
    replace(L"{ICON}", std::to_wstring((int)std::max(itemSize * 0.2, 5.0)));

    return Markup::XamlReader::Load(xaml).as<UIElement>();
}

void OpenFolder(OverflowState& state, int folder);
void CloseFolder(OverflowState& state);

void ClearTile(Tile& tile) {
    if (tile.button) {
        tile.button.Click(tile.clickToken);
        if (tile.dragOverToken) tile.button.DragOver(tile.dragOverToken);
        if (tile.dropToken) tile.button.Drop(tile.dropToken);
    }
    tile = {};
}

void ClearTiles(OverflowState& state) {
    for (auto& tile : state.folderTiles) {
        ClearTile(tile);
    }
    state.folderTiles.clear();
    ClearTile(state.backTile);
}

Tile& EnsureFolderTile(OverflowState& state, int folder) {
    Tile& tile = state.folderTiles[folder];
    if (!tile.button) {
        tile.button = CreateTileButton(state.tileWidth, state.tileHeight);
        ConfigureDropTarget(state, tile, folder);
        tile.clickToken = tile.button.Click(
            [weakState = state.weak_from_this(), folder](
                wf::IInspectable const&, RoutedEventArgs const&) {
                if (auto state = weakState.lock();
                    state && state->attached && !state->detaching &&
                    !g_unloading) {
                    OpenFolder(*state, folder);
                }
            });
    }
    return tile;
}

Tile& EnsureBackTile(OverflowState& state) {
    Tile& tile = state.backTile;
    if (!tile.button) {
        tile.button = CreateTileButton(state.tileWidth, state.tileHeight);
        ConfigureDropTarget(state, tile, -1);
        tile.button.Content(CreateGlyph(
            L"\uE72B",
            std::max(std::min(state.tileWidth, state.tileHeight) * 0.35, 8.0)));
        tile.clickToken = tile.button.Click(
            [weakState = state.weak_from_this()](
                wf::IInspectable const&, RoutedEventArgs const&) {
                if (auto state = weakState.lock();
                    state && state->attached && !state->detaching &&
                    !g_unloading) {
                    CloseFolder(*state);
                }
            });
    }
    return tile;
}

void UpdateFolderTile(OverflowState& state,
                      Tile& tile,
                      FolderConfig const& folder,
                      std::vector<Media::ImageSource> const& images,
                      int iconCount) {
    std::wstring glyph = folder.glyph;
    if (glyph.empty() && images.empty()) {
        glyph = L"\uE8B7";
    }

    if (tile.contentGlyph != glyph) {
        double itemSize = std::min(state.tileWidth, state.tileHeight);
        if (glyph.empty()) {
            tile.button.Content(CreateFolderPreview(itemSize));
        } else {
            tile.button.Content(CreateGlyph(glyph, std::max(itemSize * 0.4, 8.0)));
        }
        tile.contentGlyph = glyph;
    }

    if (glyph.empty()) {
        auto previews = tile.button.Content().as<Controls::Panel>().Children();
        for (uint32_t i = 0; i < previews.Size(); i++) {
            previews.GetAt(i).as<Controls::Image>().Source(
                i < images.size() ? images[i] : nullptr);
        }
    }

    std::wstring label = folder.name;
    Automation::AutomationProperties::SetName(tile.button, label);
    Controls::ToolTipService::SetToolTip(
        tile.button,
        winrt::box_value(label + L" (" + std::to_wstring(iconCount) + L")"));
}

////////////////////////////////////////////////////////////////////////////////
