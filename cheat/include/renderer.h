#pragma once
#include <d3d11.h>
#include <dxgi.h>

struct ImFont;

namespace renderer {
    bool initialize();
    void shutdown();
    bool is_initialized();

    ID3D11Device* get_device();
    ID3D11DeviceContext* get_context();
    HWND get_window();
    ImFont* get_esp_name_font();
    float get_esp_name_font_size();
}
