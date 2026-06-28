#define _CRT_SECURE_NO_WARNINGS
#include "../include/features.h"
#include "../include/renderer.h"
#include "../include/il2cpp_api.h"
#include <cmath>

ESPConfig esp_config;
AimbotConfig aimbot_config;
MenuConfig menu_config;

namespace features {
    static std::vector<PlayerInfo> g_players;
    static std::vector<WorldObjectInfo> g_grenades;
    static bool g_aim_last_success = false;
    static bool g_aim_hold_active = false;

    struct Vec2 { float x, y; };
    struct EspSmoothState {
        uintptr_t key;
        Vector2 foot;
        Vector2 head;
        DWORD last_seen;
        bool initialized;
    };

    static std::vector<EspSmoothState> g_esp_smooth;

    static float clamp_float(float value, float min_value, float max_value) {
        if (value < min_value)
            return min_value;
        if (value > max_value)
            return max_value;
        return value;
    }

    static bool is_finite_screen_point(const Vector2& point) {
        return std::isfinite(point.x) && std::isfinite(point.y) &&
            fabsf(point.x) < 100000.0f && fabsf(point.y) < 100000.0f;
    }

    static bool is_reasonable_world_point(const Vector3& point) {
        return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z) &&
            (fabsf(point.x) > 0.01f || fabsf(point.y) > 0.01f || fabsf(point.z) > 0.01f) &&
            fabsf(point.x) < 2000.0f && fabsf(point.y) < 2000.0f && fabsf(point.z) < 2000.0f;
    }

    static bool screen_point_near_view(const Vector2& point, int sw, int sh) {
        float margin_x = (float)sw * 0.35f + 80.0f;
        float margin_y = (float)sh * 0.35f + 80.0f;
        return point.x >= -margin_x && point.x <= (float)sw + margin_x &&
            point.y >= -margin_y && point.y <= (float)sh + margin_y;
    }

    static bool valid_esp_projection(const Vector3& foot_world, const Vector3& head_world,
        const Vector2& foot_screen, const Vector2& head_screen, int sw, int sh) {
        if (!is_reasonable_world_point(foot_world) || !is_reasonable_world_point(head_world))
            return false;
        if (!is_finite_screen_point(foot_screen) || !is_finite_screen_point(head_screen))
            return false;
        if (!screen_point_near_view(foot_screen, sw, sh) && !screen_point_near_view(head_screen, sw, sh))
            return false;

        float raw_height = fabsf(foot_screen.y - head_screen.y);
        if (!std::isfinite(raw_height) || raw_height < 4.0f || raw_height > (float)sh * 1.6f)
            return false;

        float raw_width = raw_height * 0.5f;
        if (!std::isfinite(raw_width) || raw_width > (float)sw * 0.75f)
            return false;

        float x_delta = fabsf(foot_screen.x - head_screen.x);
        if (x_delta > raw_height * 1.25f + 120.0f)
            return false;

        return true;
    }

    static void smooth_screen_pair(uintptr_t key, Vector2& foot, Vector2& head, DWORD now) {
        if (!key)
            return;

        EspSmoothState* state = nullptr;
        for (auto& entry : g_esp_smooth) {
            if (entry.key == key) {
                state = &entry;
                break;
            }
        }

        if (!state) {
            g_esp_smooth.push_back({ key, foot, head, now, true });
            return;
        }

        DWORD elapsed = now - state->last_seen;
        float foot_dx = foot.x - state->foot.x;
        float foot_dy = foot.y - state->foot.y;
        float head_dx = head.x - state->head.x;
        float head_dy = head.y - state->head.y;
        bool snap = elapsed > 200 ||
            (foot_dx * foot_dx + foot_dy * foot_dy) > 40000.0f ||
            (head_dx * head_dx + head_dy * head_dy) > 40000.0f;

        if (!state->initialized || snap) {
            state->foot = foot;
            state->head = head;
            state->initialized = true;
        } else {
            float alpha = clamp_float((float)elapsed / 25.0f, 0.55f, 1.0f);
            state->foot.x += (foot.x - state->foot.x) * alpha;
            state->foot.y += (foot.y - state->foot.y) * alpha;
            state->head.x += (head.x - state->head.x) * alpha;
            state->head.y += (head.y - state->head.y) * alpha;
        }

        state->last_seen = now;
        foot = state->foot;
        head = state->head;
    }

    static void prune_esp_smoothing(DWORD now) {
        for (auto it = g_esp_smooth.begin(); it != g_esp_smooth.end();) {
            if (now - it->last_seen > 1000)
                it = g_esp_smooth.erase(it);
            else
                ++it;
        }
    }

    void update_players() {
        static DWORD last_scan = 0;
        static DWORD last_refresh = 0;
        static DWORD scan_interval = 500;
        static int empty_scan_count = 0;
        DWORD now = GetTickCount();

        if (!g_players.empty() && now - last_refresh >= 33) {
            last_refresh = now;
            __try {
                auto refreshed = unity::refresh_cached_players();
                for (auto& rp : refreshed) {
                    bool found = false;
                    for (auto& gp : g_players) {
                        if ((rp.object_ptr && gp.object_ptr == rp.object_ptr) ||
                            (rp.avatar_ptr && gp.avatar_ptr == rp.avatar_ptr)) {
                            gp.position = rp.position;
                            gp.head_position = rp.head_position;
                            gp.health = rp.health;
                            gp.max_health = rp.max_health;
                            gp.is_dead = rp.is_dead;
                            gp.is_valid = rp.is_valid;
                            gp.team_known = rp.team_known;
                            gp.is_teammate = rp.is_teammate;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        g_players.push_back(rp);
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (now - last_scan < scan_interval)
            return;

        last_scan = now;
        __try {
            auto scanned = unity::get_players();
            if (!scanned.empty()) {
                g_players = scanned;
                empty_scan_count = 0;
                scan_interval = 500;
            } else {
                if (g_players.empty())
                    g_players.clear();
                empty_scan_count++;
                scan_interval = empty_scan_count >= 3 ? 2000 : (empty_scan_count == 2 ? 1500 : (empty_scan_count == 1 ? 1000 : 500));
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            auto cached = unity::get_cached_players();
            if (!cached.empty())
                g_players = cached;

            empty_scan_count++;
            scan_interval = empty_scan_count >= 3 ? 2000 : (empty_scan_count == 2 ? 1500 : (empty_scan_count == 1 ? 1000 : 500));

            static DWORD last_exception_log = 0;
            if (now - last_exception_log > 2000) {
                last_exception_log = now;
                printf("[features] Exception in update_players (%d), using cached: %zu\n", empty_scan_count, g_players.size());
            }
        }
    }

    void run_chams() {
        static bool prev_enabled = false;
        static bool prev_xray = false;
        static bool prev_glow = false;
        static float prev_color[4] = {};

        bool changed = prev_enabled != esp_config.show_chams ||
            prev_xray != esp_config.chams_xray ||
            prev_glow != esp_config.chams_glow ||
            prev_color[0] != esp_config.chams_color[0] ||
            prev_color[1] != esp_config.chams_color[1] ||
            prev_color[2] != esp_config.chams_color[2] ||
            prev_color[3] != esp_config.chams_color[3];

        if (!changed) return;

        prev_enabled = esp_config.show_chams;
        prev_xray = esp_config.chams_xray;
        prev_glow = esp_config.chams_glow;
        prev_color[0] = esp_config.chams_color[0];
        prev_color[1] = esp_config.chams_color[1];
        prev_color[2] = esp_config.chams_color[2];
        prev_color[3] = esp_config.chams_color[3];

        bool unity_ready = unity::is_ready();
        if (!unity_ready) return;

        if (esp_config.show_chams) {
            __try { unity::set_chams_enabled(true, esp_config.chams_xray, esp_config.chams_glow, esp_config.chams_color); }
            __except(EXCEPTION_EXECUTE_HANDLER) {}
        } else {
            __try { unity::set_chams_enabled(false, false, false, nullptr); }
            __except(EXCEPTION_EXECUTE_HANDLER) {}
        }
    }

    void run_aimbot() {
        g_aim_last_success = false;
        g_aim_hold_active = false;

        if (!aimbot_config.enabled) {
            return;
        }

        bool mouse5_down = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
        if (aimbot_config.aim_on_mouse5_hold && !mouse5_down)
            return;
        g_aim_hold_active = true;

        if (!unity::is_ready()) {
            g_aim_hold_active = false;
            return;
        }
        if (!unity::get_debug_player_camera_cached()) {
            g_aim_hold_active = false;
            return;
        }
        if (g_players.empty()) return;

        void* cam = nullptr;
        __try { cam = unity::get_main_camera(); } __except(EXCEPTION_EXECUTE_HANDLER) { return; }
        if (!cam) return;

        HWND hwnd = renderer::get_window();
        if (!hwnd) return;
        RECT rect;
        if (!GetWindowRect(hwnd, &rect)) return;
        float sw = (float)(rect.right - rect.left);
        float sh = (float)(rect.bottom - rect.top);
        float cx = sw * 0.5f, cy = sh * 0.5f;

        float closest_dist = aimbot_config.fov * 4.0f;
        Vector3 best_target;
        bool found_target = false;

        for (auto& p : g_players) {
            if (!p.is_valid) continue;
            if (p.is_dead) continue;
            if (p.is_local) continue;
            if (!aimbot_config.aim_teammates && p.team_known && p.is_teammate)
                continue;

            Vector3 target = p.head_position;
            if (target.x == 0.0f && target.y == 0.0f && target.z == 0.0f) {
                target = p.position;
                target.y += 1.45f;
            }

            Vector2 screen;
            __try {
                if (!unity::world_to_screen(target, screen, cam))
                    continue;
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                continue;
            }
            float dx = screen.x - cx, dy = screen.y - cy;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < closest_dist) {
                closest_dist = dist;
                best_target = target;
                found_target = true;
            }
        }

        if (found_target) {
            bool aim_ok = false;
            __try { aim_ok = unity::aim_at_world(best_target, aimbot_config.smooth, aimbot_config.recoil_compensation); }
            __except(EXCEPTION_EXECUTE_HANDLER) {}
            g_aim_last_success = aim_ok;
        }
    }

    void render_esp(ImDrawList* draw_list) {
        DWORD now = GetTickCount();
        int player_count = unity::get_debug_player_count();
        bool camera_ok = unity::get_debug_camera_found();
        bool unity_ready = unity::is_ready();

        char debug_buf[512];
        int w2s_ok = 0, wts_try = 0;

        bool vm = unity::get_debug_view_method();
        bool pm = unity::get_debug_proj_method();
        bool wm = unity::get_debug_wts_method();
        int pc_avatars = unity::get_debug_player_camera_avatar_count();
        int pc_components = unity::get_debug_player_camera_component_count();
        int pc_auth = unity::get_debug_player_camera_authority_count();
        int pc_positions = unity::get_debug_player_camera_position_count();
        bool pc_cached = unity::get_debug_player_camera_cached();
        int gm_count = unity::get_debug_game_mode_count();
        int gm_local_mob_count = unity::get_debug_game_mode_local_mob_count();
        int scene_camera_count = unity::get_debug_scene_camera_count();
        int pno_objects = unity::get_debug_pno_object_count();
        int mob_objects = unity::get_debug_player_mob_count();
        int mob_positions = unity::get_debug_player_mob_position_count();
        int move_objects = unity::get_debug_move_component_count();
        int move_positions = unity::get_debug_move_component_position_count();
        int game_mode_mobs = unity::get_debug_game_mode_mob_count();
        int game_mode_mob_positions = unity::get_debug_game_mode_mob_position_count();
        int game_mode_teams = unity::get_debug_game_mode_team_count();
        int game_mode_team_players = unity::get_debug_game_mode_team_player_count();
        int game_mode_team_mobs = unity::get_debug_game_mode_team_mob_count();
        int brain_scans = unity::get_debug_brain_scan_count();
        int camera_scans = unity::get_debug_camera_scan_count();
        int grenade_count = unity::get_debug_grenade_count();

        void* cam = nullptr;
        if (esp_config.enabled && unity_ready) {
            __try { cam = unity::get_main_camera(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        }
        Vector3 cpos;
        if (cam) { __try { cpos = unity::get_camera_position(cam); } __except(EXCEPTION_EXECUTE_HANDLER) {} }

        snprintf(debug_buf, sizeof(debug_buf),
            "CHEAT|C34 P:%d/%zu Cam:%s R:%s AH:%s A:%s G:%d/%zu"
            " VM:%s PM:%s WM:%s",
            player_count, g_players.size(), camera_ok ? "Y" : "N", unity_ready ? "Y" : "N",
            g_aim_hold_active ? "Y" : "N",
            g_aim_last_success ? "Y" : "N",
            grenade_count, g_grenades.size(),
            vm ? "Y" : "N", pm ? "Y" : "N", wm ? "Y" : "N");

        draw_list->AddRectFilled(ImVec2(5, 5), ImVec2(720, 30), IM_COL32(0, 0, 0, 180), 4.0f);
        draw_list->AddText(ImVec2(10, 8), IM_COL32(0, 255, 255, 255), debug_buf);

        if (!esp_config.enabled || !unity_ready)
            goto draw_cam_overlay;

        if (!cam) goto draw_cam_overlay;

        {
        RECT rect;
        if (!GetWindowRect(renderer::get_window(), &rect)) goto draw_cam_overlay;
        int sw = rect.right - rect.left;
        int sh = rect.bottom - rect.top;

        static DWORD last_grenade_scan = 0;
        static DWORD grenade_cooldown = 250;
        if (esp_config.show_grenades && !g_players.empty()) {
            if (now - last_grenade_scan > grenade_cooldown) {
                last_grenade_scan = now;
                __try { g_grenades = unity::get_grenades(); grenade_cooldown = 250; }
                __except(EXCEPTION_EXECUTE_HANDLER) { g_grenades.clear(); grenade_cooldown = 1000; }
            }
        } else if (!g_grenades.empty()) {
            g_grenades.clear();
        }

        for (auto& player : g_players) {
            if (!player.is_valid) continue;
            if (player.is_dead) continue;
            if (player.is_local) continue;
            if (!esp_config.show_teammates && player.team_known && player.is_teammate)
                continue;
            if (!is_reasonable_world_point(player.position))
                continue;

            // Distance sanity: skip if too far from camera (2x configured max distance)
            float dist_to_cam = (player.position - cpos).magnitude();
            if (!std::isfinite(dist_to_cam) || dist_to_cam <= 0.0f || dist_to_cam > esp_config.esp_distance * 2.0f)
                continue;

            Vector3 head_world = player.head_position;
            if (!is_reasonable_world_point(head_world)) {
                head_world = player.position;
                head_world.y += 1.7f;
            }
            if (!is_reasonable_world_point(head_world))
                continue;

            Vector2 foot_screen, head_screen;
            bool foot_ok = false;
            bool head_ok = false;
            __try {
                wts_try++;
                foot_ok = unity::world_to_screen(player.position, foot_screen);
            } __except(EXCEPTION_EXECUTE_HANDLER) { continue; }
            if (!foot_ok)
                continue;
            w2s_ok++;

            __try {
                wts_try++;
                head_ok = unity::world_to_screen(head_world, head_screen);
            } __except(EXCEPTION_EXECUTE_HANDLER) { continue; }
            if (head_ok) {
                w2s_ok++;
            } else {
                head_screen = foot_screen;
                head_screen.y -= 80.0f;
            }

            if (!valid_esp_projection(player.position, head_world, foot_screen, head_screen, sw, sh))
                continue;

            smooth_screen_pair(player.avatar_ptr ? player.avatar_ptr : player.object_ptr, foot_screen, head_screen, now);
            if (!valid_esp_projection(player.position, head_world, foot_screen, head_screen, sw, sh))
                continue;

            float height = fabsf(foot_screen.y - head_screen.y);
            if (height < 18.0f)
                height = 18.0f;
            if (height > sh * 0.9f)
                height = sh * 0.9f;
            float width = height * 0.5f;
            ImVec2 center(head_screen.x, head_screen.y);
            if (center.x + width * 0.5f < 0.0f || center.x - width * 0.5f > (float)sw ||
                center.y + height < 0.0f || center.y > (float)sh)
                continue;

            ImU32 color = (player.team_known && player.is_teammate)
                ? IM_COL32(0, 200, 255, 255)
                : IM_COL32(255, 0, 0, 255);

            if (esp_config.show_boxes) {
                draw_list->AddRect(ImVec2(center.x - width / 2, center.y),
                    ImVec2(center.x + width / 2, center.y + height), color, 0.0f, 0, 1.5f);
            }

            if (esp_config.show_lines) {
                draw_list->AddLine(ImVec2(sw / 2.0f, (float)sh),
                    ImVec2(center.x, center.y + height), color, 1.0f);
            }

            if (esp_config.show_names) {
                draw_list->AddText(ImVec2(center.x - 30, center.y - 15), color, player.name.c_str());
            }

            if (esp_config.show_distance) {
                float dist = (player.position - unity::get_camera_position(cam)).magnitude();
                char buf[32];
                snprintf(buf, sizeof(buf), "%.0fm", dist);
                draw_list->AddText(ImVec2(center.x - 15, center.y + height), IM_COL32(255, 255, 255, 255), buf);
            }

            if (esp_config.show_health && player.max_health > 0) {
                float hp_pct = player.health / player.max_health;
                if (!std::isfinite(hp_pct))
                    hp_pct = 1.0f;
                if (hp_pct < 0.0f)
                    hp_pct = 0.0f;
                if (hp_pct > 1.0f)
                    hp_pct = 1.0f;
                float bar_h = height * 0.8f;
                float bar_w = 4.0f;
                float bar_x = center.x + width / 2 + 2;
                float bar_y = center.y + height * 0.1f;
                draw_list->AddRectFilled(ImVec2(bar_x, bar_y), ImVec2(bar_x + bar_w, bar_y + bar_h), IM_COL32(0, 0, 0, 180));
                ImU32 hp_color = IM_COL32((int)((1 - hp_pct) * 255), (int)(hp_pct * 255), 0, 255);
                draw_list->AddRectFilled(ImVec2(bar_x, bar_y + bar_h * (1 - hp_pct)), ImVec2(bar_x + bar_w, bar_y + bar_h), hp_color);
            }
        }

        if (esp_config.show_grenades) {
            ImU32 grenade_color = IM_COL32(255, 180, 0, 255);
            for (auto& grenade : g_grenades) {
                if (!grenade.is_valid)
                    continue;
                if (!is_reasonable_world_point(grenade.position))
                    continue;

                Vector2 screen;
                bool screen_ok = false;
                __try {
                    wts_try++;
                    screen_ok = unity::world_to_screen(grenade.position, screen);
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    screen_ok = false;
                }
                if (!screen_ok || !is_finite_screen_point(screen) || !screen_point_near_view(screen, sw, sh))
                    continue;

                w2s_ok++;
                float size = 16.0f;
                if (screen.x + size < 0.0f || screen.x - size > (float)sw ||
                    screen.y + size < 0.0f || screen.y - size > (float)sh)
                    continue;

                draw_list->AddRect(ImVec2(screen.x - size * 0.5f, screen.y - size * 0.5f),
                    ImVec2(screen.x + size * 0.5f, screen.y + size * 0.5f), grenade_color, 0.0f, 0, 1.5f);
                draw_list->AddText(ImVec2(screen.x - 24.0f, screen.y - size - 11.0f),
                    grenade_color, grenade.name.c_str());
            }
        }
        }
        prune_esp_smoothing(now);

draw_cam_overlay:
        if (cam) {
            char cam_buf[512];
            snprintf(cam_buf, sizeof(cam_buf), "Cpos:%s %.1f %.1f %.1f W2S:%d/%d LC:%s SRC:%d/%d/%d GD:%d/%d TM:%d/%d/%d MV:%d/%d PC:%d/%d/%d/%d GM:%d/%d SC:%d S:%d/%d",
                unity::get_debug_camera_source(), cpos.x, cpos.y, cpos.z, w2s_ok, wts_try,
                pc_cached ? "Y" : "N", pno_objects, mob_objects, mob_positions,
                game_mode_mobs, game_mode_mob_positions,
                game_mode_teams, game_mode_team_players, game_mode_team_mobs,
                move_objects, move_positions,
                pc_avatars, pc_components, pc_auth, pc_positions,
                gm_count, gm_local_mob_count, scene_camera_count,
                brain_scans, camera_scans);
            draw_list->AddRectFilled(ImVec2(5, 22), ImVec2(1080, 42), IM_COL32(0, 0, 0, 180), 4.0f);
            draw_list->AddText(ImVec2(10, 25), IM_COL32(0, 255, 255, 255), cam_buf);
        }
    }

    void render_crosshair(ImDrawList* draw_list) {
        RECT rect;
        if (!GetWindowRect(renderer::get_window(), &rect)) return;
        float sw = (float)(rect.right - rect.left);
        float sh = (float)(rect.bottom - rect.top);
        float cx = sw * 0.5f, cy = sh * 0.5f;

        draw_list->AddLine(ImVec2(cx - 10, cy), ImVec2(cx + 10, cy), IM_COL32(255, 255, 255, 200), 1.0f);
        draw_list->AddLine(ImVec2(cx, cy - 10), ImVec2(cx, cy + 10), IM_COL32(255, 255, 255, 200), 1.0f);

        if (aimbot_config.enabled && aimbot_config.show_fov_circle) {
            draw_list->AddCircle(ImVec2(cx, cy), aimbot_config.fov * 4.0f, IM_COL32(255, 255, 255, 100), 64, 1.0f);
        }
    }
}
