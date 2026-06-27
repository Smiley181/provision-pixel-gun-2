#pragma once
#include "unity.h"
#include <imgui.h>

struct ESPConfig {
    bool enabled = true;
    bool show_boxes = true;
    bool show_names = true;
    bool show_health = true;
    bool show_distance = true;
    bool show_lines = false;
    bool show_teammates = false;
    bool show_grenades = false;
    float glow_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    float teammate_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    float esp_distance = 200.0f;
};

struct MiscConfig {
    bool rapid_fire = false;
    bool no_spread = false;
    bool infinite_ammo = false;
    bool no_fall_damage = false;
    bool speed_hack = false;
    float speed_multiplier = 2.0f;
};

struct AimbotConfig {
    bool enabled = true;
    bool aim_on_mouse5_hold = true;
    float fov = 15.0f;
    float smooth = 5.0f;
    bool show_fov_circle = true;
    bool aim_teammates = false;
    bool recoil_compensation = false;
};

struct MenuConfig {
    bool show_menu = true;
    int menu_key = VK_INSERT;
};

extern ESPConfig esp_config;
extern AimbotConfig aimbot_config;
extern MiscConfig misc_config;
extern MenuConfig menu_config;

namespace features {
    void update_players();
    void run_aimbot();
    void render_esp(ImDrawList* draw_list);
    void render_crosshair(ImDrawList* draw_list);
}
