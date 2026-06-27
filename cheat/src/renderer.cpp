#define _CRT_SECURE_NO_WARNINGS
#include "../include/renderer.h"
#include "../include/features.h"
#include "../include/menu.h"
#include "../include/il2cpp_api.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace renderer {
    static ID3D11Device* g_device = nullptr;
    static ID3D11DeviceContext* g_context = nullptr;
    static ID3D11RenderTargetView* g_rtv = nullptr;
    static HWND g_window = nullptr;
    static bool g_initialized = false;
    static bool g_imgui_ready = false;
    static WNDPROC g_original_wndproc = nullptr;
    static void* g_attached_thread = nullptr;

    typedef HRESULT(__stdcall* present_fn)(IDXGISwapChain*, UINT, UINT);
    static present_fn original_present = nullptr;

    static LRESULT CALLBACK wndproc_hook(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        if (msg == WM_KEYDOWN && wparam == menu_config.menu_key) {
            menu_config.show_menu = !menu_config.show_menu;
            return true;
        }
        if (menu_config.show_menu && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
            return true;
        return CallWindowProc(g_original_wndproc, hwnd, msg, wparam, lparam);
    }

    static bool setup_imgui(IDXGISwapChain* swapchain) {
        if (FAILED(swapchain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device)))
            return false;

        g_device->GetImmediateContext(&g_context);

        DXGI_SWAP_CHAIN_DESC desc = {};
        if (FAILED(swapchain->GetDesc(&desc)))
            return false;
        g_window = desc.OutputWindow;

        ID3D11Texture2D* backbuffer = nullptr;
        if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer))))
            return false;

        if (FAILED(g_device->CreateRenderTargetView(backbuffer, nullptr, &g_rtv))) {
            backbuffer->Release();
            return false;
        }
        backbuffer->Release();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(g_window);
        ImGui_ImplDX11_Init(g_device, g_context);

        g_original_wndproc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndproc_hook)));

        g_imgui_ready = true;
        printf("[cheat] ImGui ready on game GPU\n");
        return true;
    }

    static HRESULT __stdcall present_hook(IDXGISwapChain* swapchain, UINT sync, UINT flags) {
        if (!g_imgui_ready) {
            setup_imgui(swapchain);
        }

        if (g_imgui_ready) {
            if (!g_attached_thread && il2cpp::initialized) {
                void* domain = il2cpp::get_domain();
                if (domain) {
                    g_attached_thread = il2cpp::thread_attach(domain);
                    printf("[cheat] Render thread attached to IL2CPP domain\n");
                }
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            features::update_players();
            features::run_aimbot();

            ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
            features::render_esp(draw_list);
            features::render_crosshair(draw_list);

            menu::render();

            ImGui::Render();
            g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        return original_present(swapchain, sync, flags);
    }

    bool initialize() {
        if (g_initialized) return true;

        HMODULE d3d11 = GetModuleHandleA("d3d11.dll");
        if (!d3d11) d3d11 = LoadLibraryA("d3d11.dll");
        if (!d3d11) {
            printf("[cheat] d3d11.dll not found\n");
            return false;
        }

        auto create_fn = (HRESULT(WINAPI*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
            const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*,
            IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**))
            GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain");

        if (!create_fn) {
            printf("[cheat] D3D11CreateDeviceAndSwapChain export not found\n");
            return false;
        }

        IDXGISwapChain* temp_swapchain = nullptr;
        ID3D11Device* temp_device = nullptr;
        ID3D11DeviceContext* temp_context = nullptr;

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.Width = 1;
        sd.BufferDesc.Height = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = GetDesktopWindow();
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

        HRESULT hr = create_fn(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd,
            &temp_swapchain, &temp_device, nullptr, &temp_context);

        if (FAILED(hr) || !temp_swapchain) {
            printf("[cheat] Temp swapchain failed: 0x%08lX\n", hr);
            sd.OutputWindow = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            hr = create_fn(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                levels, 2, D3D11_SDK_VERSION, &sd,
                &temp_swapchain, &temp_device, nullptr, &temp_context);
            if (temp_swapchain && sd.OutputWindow)
                ShowWindow(sd.OutputWindow, SW_HIDE);
        }

        if (FAILED(hr) || !temp_swapchain) {
            printf("[cheat] Temp swapchain failed (retry): 0x%08lX\n", hr);
            return false;
        }

        void** vtable = *(void***)temp_swapchain;
        original_present = (present_fn)vtable[8];

        DWORD old = 0;
        if (!VirtualProtect(&vtable[8], sizeof(void*), PAGE_READWRITE, &old)) {
            printf("[cheat] VirtualProtect vtable failed\n");
            temp_context->Release();
            temp_device->Release();
            temp_swapchain->Release();
            return false;
        }
        vtable[8] = &present_hook;
        VirtualProtect(&vtable[8], sizeof(void*), old, &old);

        temp_context->Release();
        temp_device->Release();
        temp_swapchain->Release();

        g_initialized = true;
        printf("[cheat] Present hook installed\n");
        return true;
    }

    void shutdown() {
        if (g_original_wndproc && g_window) {
            SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_original_wndproc));
        }
        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
        if (g_context) { g_context->Release(); g_context = nullptr; }
        if (g_device) { g_device->Release(); g_device = nullptr; }
        g_imgui_ready = false;
        g_initialized = false;
    }

    bool is_initialized() { return g_initialized; }
    ID3D11Device* get_device() { return g_device; }
    ID3D11DeviceContext* get_context() { return g_context; }
    HWND get_window() { return g_window; }
}
