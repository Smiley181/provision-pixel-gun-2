#define _CRT_SECURE_NO_WARNINGS
#include "../include/menu.h"
#include "../include/features.h"
#include "../include/unity.h"
#include "../include/il2cpp_api.h"
#include "../include/renderer.h"
#include <cstdio>

namespace menu {
    static int current_tab = 0;
    static const char* tabs[] = { "Aimbot", "ESP", "Settings" };

    void render_aimbot_tab() {
        ImGui::Checkbox("Enable Aimbot", &aimbot_config.enabled);
        ImGui::Checkbox("Mouse5 Hold", &aimbot_config.aim_on_mouse5_hold);
        ImGui::Checkbox("Show FOV Circle", &aimbot_config.show_fov_circle);
        ImGui::Checkbox("Aim Teammates", &aimbot_config.aim_teammates);
        ImGui::Checkbox("No Recoil", &aimbot_config.recoil_compensation);
        ImGui::Checkbox("No Spread", &misc_config.no_spread);
        ImGui::SliderFloat("FOV", &aimbot_config.fov, 1.0f, 90.0f, "%.1f");
        ImGui::SliderFloat("Smoothing", &aimbot_config.smooth, 1.0f, 20.0f, "%.1f");
    }

    void render_esp_tab() {
        ImGui::Checkbox("Enable ESP", &esp_config.enabled);
        ImGui::Checkbox("Show Boxes", &esp_config.show_boxes);
        ImGui::Checkbox("Show Names", &esp_config.show_names);
        ImGui::Checkbox("Show Health", &esp_config.show_health);
        ImGui::Checkbox("Show Distance", &esp_config.show_distance);
        ImGui::Checkbox("Show Lines", &esp_config.show_lines);
        ImGui::Checkbox("Show Teammates", &esp_config.show_teammates);
        ImGui::Checkbox("Grenades", &esp_config.show_grenades);
        ImGui::Separator();
        ImGui::Text("Player Chams");
        ImGui::Checkbox("Enable Chams", &esp_config.show_chams);
        if (esp_config.show_chams) {
            ImGui::Checkbox("Xray Fill", &esp_config.chams_xray);
            ImGui::Checkbox("Glow", &esp_config.chams_glow);
            ImGui::ColorEdit4("Chams Color", esp_config.chams_color, ImGuiColorEditFlags_NoInputs);
        }
    }

    void render_settings_tab() {
        ImGui::Text("Settings");
        ImGui::Separator();
        ImGui::Text("Menu Key: INSERT");
        ImGui::Text("IL2CPP Status: %s", il2cpp::initialized ? "Connected" : "Not Connected");
        ImGui::Text("Module Base: 0x%llX", (unsigned long long)il2cpp::module_base);

        if (il2cpp::initialized) {
            void* domain = il2cpp::get_domain();
            if (domain) {
                size_t count = 0;
                void** assemblies = il2cpp::get_assemblies(domain, &count);
                ImGui::Text("Assemblies Loaded: %zu", count);
                for (size_t i = 0; i < count && i < 10; i++) {
                    if (assemblies[i]) {
                        void* img = il2cpp::assembly_get_image(assemblies[i]);
                        if (img)
                            ImGui::BulletText("%s", il2cpp::image_get_name(img));
                    }
                }
                if (count > 10) ImGui::Text("... and %zu more", count - 10);
            }
        }

        ImGui::Separator();
        ImGui::Text("Debug Info:");
        ImGui::Text("Player Count: %d", unity::get_debug_player_count());
        ImGui::Text("Camera Found: %s", unity::get_debug_camera_found() ? "Yes" : "No");
        ImGui::Text("Unity Ready: %s", unity::is_ready() ? "Yes" : "No");
    }

    void render() {
        if (!menu_config.show_menu) return;

        ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Pixel Gun 2 Demo", nullptr, ImGuiWindowFlags_NoCollapse);

        ImGui::BeginChild("sidebar", ImVec2(120, 0), true);
        for (int i = 0; i < IM_ARRAYSIZE(tabs); i++) {
            if (ImGui::Selectable(tabs[i], current_tab == i))
                current_tab = i;
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("content", ImVec2(0, 0), true);

        switch (current_tab) {
        case 0: render_aimbot_tab(); break;
        case 1: render_esp_tab(); break;
        case 2: render_settings_tab(); break;
        }

        ImGui::EndChild();
        ImGui::End();
    }
}
