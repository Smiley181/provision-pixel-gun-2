#pragma once
#include <d3d11.h>
#include <vector>
#include <string>

struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3 operator-(const Vector3& o) const { return Vector3(x - o.x, y - o.y, z - o.z); }
    Vector3 operator+(const Vector3& o) const { return Vector3(x + o.x, y + o.y, z + o.z); }
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
    float dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    float magnitude() const { return sqrtf(x * x + y * y + z * z); }
    Vector3 normalized() const { float m = magnitude(); return m > 0 ? *this * (1.0f / m) : Vector3(0, 0, 0); }
};

struct Matrix4x4 {
    float m[16];
    float& operator[](int i) { return m[i]; }
    const float& operator[](int i) const { return m[i]; }
};

struct PlayerInfo {
    std::string name;
    Vector3 position;
    Vector3 head_position;
    float health;
    float max_health;
    bool is_valid;
    uintptr_t object_ptr;
    uintptr_t avatar_ptr;
    int source_type;
    bool team_known;
    bool is_teammate;
    bool is_local;
    bool is_dead;
};

struct WorldObjectInfo {
    std::string name;
    Vector3 position;
    bool is_valid;
    uintptr_t object_ptr;
    int source_type;
};

namespace unity {
    bool initialize();
    bool is_ready();

    int get_debug_player_count();
    bool get_debug_camera_found();
    bool get_debug_view_method();
    bool get_debug_proj_method();
    bool get_debug_wts_method();
    const char* get_debug_camera_source();
    int get_debug_view_invoke();
    int get_debug_proj_invoke();
    int get_debug_player_camera_avatar_count();
    int get_debug_player_camera_component_count();
    int get_debug_player_camera_authority_count();
    int get_debug_player_camera_position_count();
    bool get_debug_player_camera_cached();
    int get_debug_game_mode_count();
    int get_debug_game_mode_local_mob_count();
    int get_debug_scene_camera_count();
    int get_debug_pno_object_count();
    int get_debug_player_mob_count();
    int get_debug_player_mob_position_count();
    int get_debug_bot_mob_count();
    int get_debug_bot_mob_position_count();
    int get_debug_move_component_count();
    int get_debug_move_component_position_count();
    int get_debug_game_mode_mob_count();
    int get_debug_game_mode_mob_position_count();
    int get_debug_game_mode_team_count();
    int get_debug_game_mode_team_player_count();
    int get_debug_game_mode_team_mob_count();
    int get_debug_brain_scan_count();
    int get_debug_camera_scan_count();
    int get_debug_grenade_count();
    int get_debug_refill_station_count();
    const char* get_debug_activity_op();
    const char* get_debug_activity_class();
    bool get_debug_activity_include_inactive();
    unsigned long long get_debug_activity_elapsed_ms();
    bool has_tracked_scene_state();
    bool has_recent_main_thread_tracking();
    void ensure_main_thread_tracking();
    void* get_main_camera();
    Matrix4x4 get_view_matrix(void* camera);
    Matrix4x4 get_projection_matrix(void* camera);

    bool world_to_screen(const Vector3& world, Vector2& screen, const Matrix4x4& view, const Matrix4x4& proj);
    bool world_to_screen(const Vector3& world, Vector2& screen);
    bool world_to_screen(const Vector3& world, Vector2& screen, void* camera);

    std::vector<PlayerInfo> get_players();
    std::vector<PlayerInfo> get_cached_players();
    std::vector<PlayerInfo> refresh_cached_players();
    std::vector<WorldObjectInfo> get_grenades();
    std::vector<WorldObjectInfo> get_refill_stations();
    std::string get_object_name(void* obj);
    Vector3 get_camera_position(void* camera);
    Vector3 get_object_position(void* transform);
    Vector3 get_bone_position(void* transform);
    bool aim_at_world(const Vector3& target, float smooth, bool recoil_compensation = false);
    // Returns true if there is a clear line of sight between `from` and `to`
    // (i.e. no world geometry blocks the segment). `layer_mask` selects which
    // physics layers count as occluders; pass -1 to test against every layer.
    bool is_visible(const Vector3& from, const Vector3& to, int layer_mask = -1);

    void* find_object_of_type(const char* image_name, const char* namesp, const char* class_name);
    void* get_transform(void* obj);
    Vector3 get_transform_position(void* transform);
    void* get_component(void* obj, const char* image_name, const char* namesp, const char* class_name);
    void set_chams_enabled(bool enabled, bool xray, bool glow, const float color[4]);

    float get_field_float(void* obj, int32_t offset);
    bool get_field_bool(void* obj, int32_t offset);
    int32_t get_field_int(void* obj, int32_t offset);
    float get_field_float(void* obj, const char* field_name);
    void set_field_float(void* obj, int32_t offset, float value);
    void set_field_bool(void* obj, int32_t offset, bool value);
}
