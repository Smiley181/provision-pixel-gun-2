#define _CRT_SECURE_NO_WARNINGS
#include "../include/renderer.h"
#include "../include/features.h"
#include "../include/menu.h"
#include "../include/il2cpp_api.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <atomic>
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
    static HANDLE g_watchdog_thread = nullptr;
    static std::atomic<bool> g_watchdog_running{ false };
    static std::atomic<int> g_present_stage{ 0 };
    static std::atomic<unsigned long long> g_present_stage_start_ms{ 0 };
    static std::atomic<unsigned long long> g_present_frame{ 0 };
    static std::atomic<unsigned long long> g_last_present_enter_ms{ 0 };
    static std::atomic<unsigned long long> g_last_present_exit_ms{ 0 };
    static std::atomic<int> g_last_completed_stage{ 0 };
    static std::atomic<unsigned int> g_last_sync{ 0 };
    static std::atomic<unsigned int> g_last_flags{ 0 };
    static ImFont* g_esp_name_font = nullptr;
    static float g_esp_name_font_size = 12.0f;

    typedef HRESULT(__stdcall* present_fn)(IDXGISwapChain*, UINT, UINT);
    static present_fn original_present = nullptr;

    enum PresentStage {
        stage_idle = 0,
        stage_setup_imgui,
        stage_attach_il2cpp,
        stage_imgui_new_frame,
        stage_update_players,
        stage_run_chams,
        stage_run_aimbot,
        stage_render_esp,
        stage_render_crosshair,
        stage_render_menu,
        stage_imgui_render,
        stage_dx_render,
        stage_original_present
    };

    static const char* present_stage_name(int stage) {
        switch (stage) {
        case stage_setup_imgui: return "setup_imgui";
        case stage_attach_il2cpp: return "attach_il2cpp";
        case stage_imgui_new_frame: return "imgui_new_frame";
        case stage_update_players: return "update_players";
        case stage_run_chams: return "run_chams";
        case stage_run_aimbot: return "run_aimbot";
        case stage_render_esp: return "render_esp";
        case stage_render_crosshair: return "render_crosshair";
        case stage_render_menu: return "render_menu";
        case stage_imgui_render: return "imgui_render";
        case stage_dx_render: return "dx_render";
        case stage_original_present: return "original_present";
        default: return "idle";
        }
    }

    static unsigned long long slow_stage_threshold_ms(int stage) {
        return stage == stage_original_present ? 1000 : 100;
    }

    static void print_unity_activity() {
        const char* op = unity::get_debug_activity_op();
        const char* klass = unity::get_debug_activity_class();
        if (!op)
            return;

        printf(" unity=%s(%s inactive=%s %llums)",
            op,
            klass ? klass : "unknown",
            unity::get_debug_activity_include_inactive() ? "yes" : "no",
            unity::get_debug_activity_elapsed_ms());
    }

    static void begin_present_stage(int stage) {
        g_present_stage_start_ms.store(GetTickCount64());
        g_present_stage.store(stage);
    }

    static void finish_present_stage(int stage) {
        unsigned long long start = g_present_stage_start_ms.load();
        unsigned long long elapsed = start ? (GetTickCount64() - start) : 0;
        if (elapsed >= slow_stage_threshold_ms(stage)) {
            printf("[freeze] Slow Present stage: %s took %llums frame=%llu sync=%u flags=0x%X",
                present_stage_name(stage),
                elapsed,
                g_present_frame.load(),
                g_last_sync.load(),
                g_last_flags.load());
            print_unity_activity();
            printf("\n");
        }
        g_last_completed_stage.store(stage);
        g_present_stage.store(stage_idle);
        g_present_stage_start_ms.store(0);
    }

    template <typename Fn>
    static void run_present_stage(int stage, Fn fn) {
        begin_present_stage(stage);
        fn();
        finish_present_stage(stage);
    }

    static DWORD WINAPI freeze_watchdog_thread(LPVOID) {
        unsigned long long last_log_ms = 0;
        while (g_watchdog_running.load()) {
            Sleep(250);

            unsigned long long now = GetTickCount64();
            int stage = g_present_stage.load();
            unsigned long long start = g_present_stage_start_ms.load();
            if (stage == stage_idle || !start) {
                unsigned long long last_seen = g_last_present_exit_ms.load();
                unsigned long long last_enter = g_last_present_enter_ms.load();
                if (last_enter > last_seen)
                    last_seen = last_enter;

                unsigned long long elapsed = last_seen ? (now - last_seen) : 0;
                if (last_seen && elapsed >= 2000 && now - last_log_ms >= 1000) {
                    last_log_ms = now;
                    printf("[freeze] Present heartbeat stopped: no Present for %llums frame=%llu last_stage=%s sync=%u flags=0x%X",
                        elapsed,
                        g_present_frame.load(),
                        present_stage_name(g_last_completed_stage.load()),
                        g_last_sync.load(),
                        g_last_flags.load());
                    print_unity_activity();
                    printf("\n");
                }
                continue;
            }

            unsigned long long elapsed = now - start;
            if (elapsed < 1500 || now - last_log_ms < 1000)
                continue;

            last_log_ms = now;
            printf("[freeze] Present appears stuck: stage=%s elapsed=%llums frame=%llu sync=%u flags=0x%X",
                present_stage_name(stage),
                elapsed,
                g_present_frame.load(),
                g_last_sync.load(),
                g_last_flags.load());
            print_unity_activity();
            printf("\n");
        }
        return 0;
    }

    static void start_freeze_watchdog() {
        if (g_watchdog_thread)
            return;

        g_watchdog_running.store(true);
        g_watchdog_thread = CreateThread(nullptr, 0, freeze_watchdog_thread, nullptr, 0, nullptr);
        if (g_watchdog_thread)
            printf("[freeze] Watchdog enabled (logs Present stalls over 1500ms)\n");
        else
            g_watchdog_running.store(false);
    }

    static void stop_freeze_watchdog() {
        if (!g_watchdog_thread)
            return;

        g_watchdog_running.store(false);
        WaitForSingleObject(g_watchdog_thread, 1000);
        CloseHandle(g_watchdog_thread);
        g_watchdog_thread = nullptr;
    }

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
        ImFont* default_font = io.Fonts->AddFontDefault();
        io.FontDefault = default_font;
        g_esp_name_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", g_esp_name_font_size);
        if (!g_esp_name_font)
            g_esp_name_font = default_font;
        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(g_window);
        ImGui_ImplDX11_Init(g_device, g_context);

        g_original_wndproc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndproc_hook)));

        g_imgui_ready = true;
        printf("[cheat] ImGui ready on game GPU (esp_name_font=%p)\n", g_esp_name_font);
        return true;
    }

    static HRESULT __stdcall present_hook(IDXGISwapChain* swapchain, UINT sync, UINT flags) {
        g_last_present_enter_ms.store(GetTickCount64());
        g_present_frame.fetch_add(1);
        g_last_sync.store(sync);
        g_last_flags.store(flags);

        if (!g_imgui_ready) {
            run_present_stage(stage_setup_imgui, [&]() {
                setup_imgui(swapchain);
            });
        }

        if (g_imgui_ready) {
            if (!g_attached_thread && il2cpp::initialized) {
                run_present_stage(stage_attach_il2cpp, [&]() {
                    void* domain = il2cpp::get_domain();
                    if (domain) {
                        g_attached_thread = il2cpp::thread_attach(domain);
                        printf("[cheat] Render thread attached to IL2CPP domain\n");
                    }
                });
            }

            run_present_stage(stage_imgui_new_frame, [&]() {
                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
            });

            run_present_stage(stage_update_players, []() { features::update_players(); });
            run_present_stage(stage_run_chams, []() { features::run_chams(); });
            run_present_stage(stage_run_aimbot, []() { features::run_aimbot(); });

            ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
            run_present_stage(stage_render_esp, [&]() { features::render_esp(draw_list); });
            run_present_stage(stage_render_crosshair, [&]() { features::render_crosshair(draw_list); });

            run_present_stage(stage_render_menu, []() { menu::render(); });

            run_present_stage(stage_imgui_render, []() { ImGui::Render(); });
            run_present_stage(stage_dx_render, []() {
                g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            });
        }

        HRESULT result = S_OK;
        run_present_stage(stage_original_present, [&]() {
            result = original_present(swapchain, sync, flags);
        });
        g_last_present_exit_ms.store(GetTickCount64());
        return result;
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
        start_freeze_watchdog();
        printf("[cheat] Present hook installed\n");
        return true;
    }

    void shutdown() {
        stop_freeze_watchdog();
        if (g_original_wndproc && g_window) {
            SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_original_wndproc));
        }
        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
        if (g_context) { g_context->Release(); g_context = nullptr; }
        if (g_device) { g_device->Release(); g_device = nullptr; }
        g_esp_name_font = nullptr;
        g_imgui_ready = false;
        g_initialized = false;
    }

    bool is_initialized() { return g_initialized; }
    ID3D11Device* get_device() { return g_device; }
    ID3D11DeviceContext* get_context() { return g_context; }
    HWND get_window() { return g_window; }
    ImFont* get_esp_name_font() { return g_esp_name_font; }
    float get_esp_name_font_size() { return g_esp_name_font_size; }
}
