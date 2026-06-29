#pragma once

// Single source of truth for all config fields.
// Add a new field here in ONE place and the struct definition,
// serialization, and deserialization all update automatically.
//
// Usage:
//   SCALAR(type, name, default_value)  -- for bool, float, int
//   ARRAY(type, name, count, default)  -- for float[4] etc.

// --- ESP ---

#define ESP_SCALAR_FIELDS(M) \
    M(bool, enabled, true) \
    M(bool, show_boxes, true) \
    M(bool, show_names, true) \
    M(bool, show_health, true) \
    M(bool, show_distance, true) \
    M(bool, show_lines, false) \
    M(bool, show_teammates, false) \
    M(bool, show_grenades, false) \
    M(bool, show_refill_stations, false) \
    M(bool, show_chams, false) \
    M(float, esp_distance, 200.0f)

#define ESP_ARRAY_FIELDS(M) \
    M(float, chams_color, 4, {1.0f, 0.0f, 0.0f, 0.6f}) \
    M(float, teammate_color, 4, {0.0f, 1.0f, 0.0f, 1.0f})

// --- Aimbot ---

#define AIMBOT_SCALAR_FIELDS(M) \
    M(bool, enabled, true) \
    M(bool, aim_on_mouse5_hold, true) \
    M(bool, show_fov_circle, true) \
    M(bool, aim_teammates, false) \
    M(bool, aim_only_visible, false) \
    M(bool, recoil_compensation, false) \
    M(float, fov, 15.0f) \
    M(float, smooth, 5.0f)

#define AIMBOT_ARRAY_FIELDS(M)

// --- Menu ---

#define MENU_SCALAR_FIELDS(M) \
    M(bool, show_menu, true) \
    M(int, menu_key, VK_INSERT)

#define MENU_ARRAY_FIELDS(M)
