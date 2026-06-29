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

    static bool is_unclassified_bot_entry(const PlayerInfo& player) {
        if (player.team_known)
            return false;
        return player.source_type == 3 || player.source_type == 5 || player.name == "BOT";
    }

    struct Vec2 { float x, y; };

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

    static void* safe_get_main_camera() {
        void* cam = nullptr;
        __try { cam = unity::get_main_camera(); }
        __except(EXCEPTION_EXECUTE_HANDLER) { cam = nullptr; }
        return cam;
    }

    static bool safe_get_camera_position(void* cam, Vector3& out_pos) {
        __try {
            out_pos = unity::get_camera_position(cam);
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            out_pos = {};
            return false;
        }
    }

    static bool safe_world_to_screen(const Vector3& world, Vector2& screen) {
        bool ok = false;
        __try { ok = unity::world_to_screen(world, screen); }
        __except(EXCEPTION_EXECUTE_HANDLER) { ok = false; }
        return ok;
    }

    static bool safe_world_to_screen(const Vector3& world, Vector2& screen, void* cam) {
        bool ok = false;
        __try { ok = unity::world_to_screen(world, screen, cam); }
        __except(EXCEPTION_EXECUTE_HANDLER) { ok = false; }
        return ok;
    }

    static bool safe_aim_at_world(const Vector3& target) {
        bool ok = false;
        __try { ok = unity::aim_at_world(target, aimbot_config.smooth, aimbot_config.recoil_compensation); }
        __except(EXCEPTION_EXECUTE_HANDLER) { ok = false; }
        return ok;
    }

    static void safe_set_chams_enabled(bool enabled, bool xray, bool glow, const float* color) {
        __try { unity::set_chams_enabled(enabled, xray, glow, color); }
        __except(EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void safe_ensure_main_thread_tracking() {
        __try { unity::ensure_main_thread_tracking(); }
        __except(EXCEPTION_EXECUTE_HANDLER) {}
    }

    void update_players() {
        static DWORD last_scan = 0;
        static DWORD last_refresh = 0;
        static DWORD scan_interval = 2500;
        static int empty_scan_count = 0;
        static bool bootstrap_delay_started = false;
        static DWORD first_scene_scan_after = 0;
        DWORD now = GetTickCount();

        if (!bootstrap_delay_started) {
            bootstrap_delay_started = true;
            first_scene_scan_after = now + 2500;
            last_scan = now;
        }

        if (unity::is_ready())
            safe_ensure_main_thread_tracking();

        // Refresh live player positions every frame so the ESP tracks smoothly
        // instead of stepping at a fixed cadence. This path only reads cached
        // transform positions (no scene-wide FindObjectsOfType scans), so it is
        // cheap enough to run per Present. The heavy scan below is still throttled.
        last_refresh = now;
        auto refreshed = unity::refresh_cached_players();
        if (!refreshed.empty()) {
            g_players = refreshed;
            empty_scan_count = 0;
            scan_interval = 5000;
        }

        if (unity::has_tracked_scene_state() || unity::has_recent_main_thread_tracking())
            return;

        if (now - first_scene_scan_after > 0x70000000u)
            return;

        if (now - last_scan < scan_interval)
            return;

        last_scan = now;
        auto scanned = unity::get_players();
        if (!scanned.empty()) {
            g_players = scanned;
            empty_scan_count = 0;
            scan_interval = 5000;
        } else {
            if (g_players.empty())
                g_players.clear();
            empty_scan_count++;
            scan_interval = empty_scan_count >= 3 ? 10000 : (empty_scan_count == 2 ? 5000 : 2500);
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
            safe_set_chams_enabled(true, esp_config.chams_xray, esp_config.chams_glow, esp_config.chams_color);
        } else {
            safe_set_chams_enabled(false, false, false, nullptr);
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

        void* cam = safe_get_main_camera();
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
            if (!aimbot_config.aim_teammates &&
                ((p.team_known && p.is_teammate) || is_unclassified_bot_entry(p)))
                continue;

            Vector3 target = p.head_position;
            if (target.x == 0.0f && target.y == 0.0f && target.z == 0.0f) {
                target = p.position;
                target.y += 1.45f;
            }

            Vector2 screen;
            if (!safe_world_to_screen(target, screen, cam))
                continue;
            float dx = screen.x - cx, dy = screen.y - cy;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < closest_dist) {
                closest_dist = dist;
                best_target = target;
                found_target = true;
            }
        }

        if (found_target) {
            g_aim_last_success = safe_aim_at_world(best_target);
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
        int bot_objects = unity::get_debug_bot_mob_count();
        int bot_positions = unity::get_debug_bot_mob_position_count();
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
            cam = safe_get_main_camera();
        }
        Vector3 cpos;
        if (cam)
            safe_get_camera_position(cam, cpos);

        snprintf(debug_buf, sizeof(debug_buf),
            "CHEAT|C34 P:%d/%zu Cam:%s R:%s AH:%s A:%s G:%d/%zu"
            " VM:%s PM:%s WM:%s B:%d/%d",
            player_count, g_players.size(), camera_ok ? "Y" : "N", unity_ready ? "Y" : "N",
            g_aim_hold_active ? "Y" : "N",
            g_aim_last_success ? "Y" : "N",
            grenade_count, g_grenades.size(),
            vm ? "Y" : "N", pm ? "Y" : "N", wm ? "Y" : "N",
            bot_objects, bot_positions);

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
        static DWORD grenade_cooldown = 1500;
        if (esp_config.show_grenades && !g_players.empty()) {
            if (now - last_grenade_scan > grenade_cooldown) {
                last_grenade_scan = now;
                g_grenades = unity::get_grenades();
                grenade_cooldown = g_grenades.empty() ? 3000 : 1500;
            }
        } else if (!g_grenades.empty()) {
            g_grenades.clear();
        }

        for (auto& player : g_players) {
            if (!player.is_valid) continue;
            if (player.is_dead) continue;
            if (player.is_local) continue;
            if (!esp_config.show_teammates &&
                ((player.team_known && player.is_teammate) || is_unclassified_bot_entry(player)))
                continue;
            if (!is_reasonable_world_point(player.position))
                continue;

            // Distance sanity: skip if too far from camera (2x configured max distance)
            float dist_to_cam = (player.position - cpos).magnitude();
            if (!std::isfinite(dist_to_cam) || dist_to_cam <= 0.0f || dist_to_cam > esp_config.esp_distance * 2.0f)
                continue;

            Vector3 aim_head_world = player.head_position;
            if (!is_reasonable_world_point(aim_head_world)) {
                aim_head_world = player.position;
                aim_head_world.y += 1.7f;
            }
            if (!is_reasonable_world_point(aim_head_world))
                continue;

            Vector2 foot_screen, head_screen;
            bool foot_ok = false;
            bool head_ok = false;
            wts_try++;
            foot_ok = safe_world_to_screen(player.position, foot_screen, cam);
            if (!foot_ok)
                continue;
            w2s_ok++;

            wts_try++;
            Vector3 box_top_world = player.position;
            box_top_world.y += 2.05f;
            if (aim_head_world.y > box_top_world.y)
                box_top_world = aim_head_world;

            head_ok = safe_world_to_screen(box_top_world, head_screen, cam);
            if (head_ok) {
                w2s_ok++;
            } else {
                head_screen = foot_screen;
                head_screen.y -= 80.0f;
            }

            if (!valid_esp_projection(player.position, box_top_world, foot_screen, head_screen, sw, sh))
                continue;

            // No screen-space smoothing: positions are refreshed every frame and
            // projected through this frame's camera matrix, so the box stays
            // locked to the player during mouse movement instead of trailing it.

            float body_height = fabsf(foot_screen.y - head_screen.y);
            if (body_height < 18.0f)
                body_height = 18.0f;
            if (body_height > sh * 0.9f)
                body_height = sh * 0.9f;

            // This top padding is purely for the ESP box. The aimbot still uses
            // PlayerInfo::head_position directly in run_aimbot().
            float top_pad = body_height * 0.05f;
            if (top_pad < 2.0f)
                top_pad = 2.0f;
            if (top_pad > 8.0f)
                top_pad = 8.0f;

            float top = (head_screen.y < foot_screen.y ? head_screen.y : foot_screen.y) - top_pad;
            float bottom = (head_screen.y > foot_screen.y ? head_screen.y : foot_screen.y);
            float height = bottom - top;
            float width = body_height * 0.52f;
            float center_x = (head_screen.x + foot_screen.x) * 0.5f;
            ImVec2 center(center_x, top);
            float box_left = center.x - width * 0.5f;
            float box_right = center.x + width * 0.5f;
            float box_top = center.y;
            float box_bottom = center.y + height;
            if (box_right < 0.0f || box_left > (float)sw ||
                box_bottom < 0.0f || box_top > (float)sh)
                continue;

            ImU32 color = (player.team_known && player.is_teammate)
                ? IM_COL32(0, 200, 255, 255)
                : IM_COL32(255, 0, 0, 255);

            if (esp_config.show_boxes) {
                draw_list->AddRect(ImVec2(box_left, box_top),
                    ImVec2(box_right, box_bottom), color, 0.0f, 0, 1.5f);
            }

            if (esp_config.show_lines) {
                draw_list->AddLine(ImVec2(sw / 2.0f, (float)sh),
                    ImVec2(center.x, box_bottom), color, 1.0f);
            }

            if (esp_config.show_names) {
                ImFont* name_font = renderer::get_esp_name_font();
                float name_size_px = renderer::get_esp_name_font_size();
                ImVec2 name_size = name_font
                    ? name_font->CalcTextSizeA(name_size_px, FLT_MAX, 0.0f, player.name.c_str())
                    : ImGui::CalcTextSize(player.name.c_str());
                ImVec2 name_pos(center.x - name_size.x * 0.5f, box_top - name_size.y - 3.0f);
                if (name_font) {
                    draw_list->AddText(name_font, name_size_px, ImVec2(name_pos.x + 1.0f, name_pos.y + 1.0f),
                        IM_COL32(0, 0, 0, 180), player.name.c_str());
                    draw_list->AddText(name_font, name_size_px, name_pos, color, player.name.c_str());
                } else {
                    draw_list->AddText(ImVec2(name_pos.x + 1.0f, name_pos.y + 1.0f),
                        IM_COL32(0, 0, 0, 180), player.name.c_str());
                    draw_list->AddText(name_pos, color, player.name.c_str());
                }
            }

            if (esp_config.show_distance) {
                float dist = (player.position - cpos).magnitude();
                char buf[32];
                snprintf(buf, sizeof(buf), "%.0fm", dist);
                draw_list->AddText(ImVec2(center.x - 15, box_bottom), IM_COL32(255, 255, 255, 255), buf);
            }

            if (esp_config.show_health && player.max_health > 0) {
                float hp_pct = player.health / player.max_health;
                if (!std::isfinite(hp_pct))
                    hp_pct = 1.0f;
                if (hp_pct < 0.0f)
                    hp_pct = 0.0f;
                if (hp_pct > 1.0f)
                    hp_pct = 1.0f;
                float bar_w = 3.0f;
                float bar_gap = 3.0f;
                float bar_x = box_left - bar_gap - bar_w;
                float fill_top = box_bottom - height * hp_pct;
                draw_list->AddRectFilled(ImVec2(bar_x - 1.0f, box_top - 1.0f),
                    ImVec2(bar_x + bar_w + 1.0f, box_bottom + 1.0f), IM_COL32(0, 0, 0, 190));
                draw_list->AddRectFilled(ImVec2(bar_x, box_top),
                    ImVec2(bar_x + bar_w, box_bottom), IM_COL32(25, 25, 25, 210));
                ImU32 hp_color = IM_COL32((int)((1.0f - hp_pct) * 220.0f), (int)(80.0f + hp_pct * 155.0f), 45, 255);
                draw_list->AddRectFilled(ImVec2(bar_x, fill_top),
                    ImVec2(bar_x + bar_w, box_bottom), hp_color);
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
                wts_try++;
                screen_ok = safe_world_to_screen(grenade.position, screen, cam);
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
