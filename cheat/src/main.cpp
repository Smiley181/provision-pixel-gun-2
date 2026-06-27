#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include "../include/il2cpp_api.h"
#include "../include/renderer.h"
#include "../include/unity.h"
#include "../include/features.h"
#include "../include/menu.h"

static void init_il2cpp() {
    for (int i = 0; i < 60; i++) {
        if (il2cpp::init()) {
            printf("[main] IL2CPP API ready\n");
            if (unity::initialize())
                printf("[main] Unity API ready\n");
            return;
        }
        Sleep(2000);
    }
    printf("[main] IL2CPP init FAILED after 60 retries\n");
}

static DWORD WINAPI main_thread(LPVOID) {
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitleA("Pixel Gun 2 Cheat");

    printf("[main] Waiting for game...\n");

    for (int i = 0; i < 120; i++) {
        if (GetModuleHandleA("GameAssembly.dll")) break;
        Sleep(1000);
    }

    if (!il2cpp::init()) {
        HANDLE il2cpp_thread = CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            init_il2cpp();
            return 0;
        }, nullptr, 0, nullptr);
        if (il2cpp_thread) CloseHandle(il2cpp_thread);
    } else {
        printf("[main] IL2CPP immediately ready\n");
        if (unity::initialize())
            printf("[main] Unity API ready\n");
    }

    if (!renderer::initialize()) {
        printf("[main] Present hook setup FAILED\n");
        return 1;
    }

    printf("[main] Cheat loaded. Press INSERT to toggle menu.\n");

    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        Sleep(100);
    }

    renderer::shutdown();
    if (f) fclose(f);
    FreeConsole();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, main_thread, nullptr, 0, nullptr);
    }
    return TRUE;
}
