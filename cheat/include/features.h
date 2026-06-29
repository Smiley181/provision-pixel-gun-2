#pragma once
#include "unity.h"
#include "config_fields.h"
#include <imgui.h>

struct ESPConfig {
#define M(type, name, default) type name = default;
    ESP_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) type name[count] = __VA_ARGS__;
    ESP_ARRAY_FIELDS(M)
#undef M
};

struct AimbotConfig {
#define M(type, name, default) type name = default;
    AIMBOT_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) type name[count] = __VA_ARGS__;
    AIMBOT_ARRAY_FIELDS(M)
#undef M
};

struct MenuConfig {
#define M(type, name, default) type name = default;
    MENU_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) type name[count] = __VA_ARGS__;
    MENU_ARRAY_FIELDS(M)
#undef M
};

extern ESPConfig esp_config;
extern AimbotConfig aimbot_config;
extern MenuConfig menu_config;

namespace features {
    void update_players();
    void run_chams();
    void run_aimbot();
    void render_esp(ImDrawList* draw_list);
    void render_crosshair(ImDrawList* draw_list);
}
