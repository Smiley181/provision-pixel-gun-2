#define _CRT_SECURE_NO_WARNINGS
#include "../include/unity.h"
#include "../include/il2cpp_api.h"
#include "../include/renderer.h"
#include <windows.h>
#include <cstdio>
#include <cmath>
#include <cstring>

struct Vector4 { float x, y, z, w; };
struct Quaternion { float x, y, z, w; };

namespace unity {
    static bool g_ready = false;
    static void* g_camera_class = nullptr;
    static void* g_gameobject_class = nullptr;
    static void* g_transform_class = nullptr;
    static void* g_object_class = nullptr;
    static void* g_component_class = nullptr;
    static void* g_cinemachine_brain_class = nullptr;

    bool initialize() {
        if (!il2cpp::initialized && !il2cpp::init())
            return false;

        printf("[unity] Initialization deferred (core module may load later)\n");
        g_ready = false;
        return true;
    }

    static void try_init_classes() {
        if (g_ready) return;
        if (!il2cpp::initialized) return;

        void* domain = il2cpp::get_domain();
        if (!domain) return;

        size_t asm_count = 0;
        void** assemblies = il2cpp::get_assemblies(domain, &asm_count);
        if (!assemblies || asm_count == 0) return;

        static bool dumped = false;
        if (!dumped) {
            dumped = true;
            printf("[unity] Assemblies loaded: %zu\n", asm_count);
            for (size_t i = 0; i < asm_count; i++) {
                if (!assemblies[i]) continue;
                void* img = il2cpp::assembly_get_image(assemblies[i]);
                if (img) {
                    const char* name = il2cpp::image_get_name(img);
                    printf("[unity]   Assembly[%zu]: %s\n", i, name ? name : "(null)");
                }
            }
        }

        const char* image_names[] = {
            "UnityEngine.CoreModule",
            "UnityEngine",
            "UnityEngine.dll",
            "UnityEngine.CoreModule.dll",
        };
        void* ue_core = nullptr;
        for (auto iname : image_names) {
            ue_core = il2cpp::get_image(iname);
            if (ue_core) {
                printf("[unity] Found image: %s\n", iname);
                break;
            }
        }
        if (!ue_core) return;

        g_gameobject_class = il2cpp::class_from_name(ue_core, "UnityEngine", "GameObject");
        g_transform_class = il2cpp::class_from_name(ue_core, "UnityEngine", "Transform");
        g_object_class = il2cpp::class_from_name(ue_core, "UnityEngine", "Object");
        g_camera_class = il2cpp::class_from_name(ue_core, "UnityEngine", "Camera");
        g_component_class = il2cpp::class_from_name(ue_core, "UnityEngine", "Component");

        printf("[unity] GameObject=%p Transform=%p Object=%p Camera=%p Component=%p\n",
            g_gameobject_class, g_transform_class, g_object_class, g_camera_class, g_component_class);

        // CinemachineBrain class (Unity.Cinemachine namespace)
        __try {
            const char* nss[] = {"Unity.Cinemachine", "Cinemachine", "UnityEngine.Cinemachine", "UnityEngine"};
            for (size_t i = 0; i < asm_count; i++) {
                if (!assemblies[i]) continue;
                void* img = il2cpp::assembly_get_image(assemblies[i]);
                if (!img) continue;
                for (auto ns : nss) {
                    g_cinemachine_brain_class = il2cpp::class_from_name(img, ns, "CinemachineBrain");
                    if (g_cinemachine_brain_class) break;
                }
                if (g_cinemachine_brain_class) break;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {}

        g_ready = (g_camera_class != nullptr);
        if (g_ready) printf("[unity] Unity classes ready (CB=%p)\n", g_cinemachine_brain_class);
    }

    bool is_ready() {
        try_init_classes();
        return g_ready;
    }

    static void* g_cached_pno_class = nullptr;
    static int32_t g_nickname_off = 0;
    static int32_t g_player_count = 0;
    static bool g_camera_found = false;
    static void* g_cached_view_method = nullptr;
    static void* g_cached_proj_method = nullptr;
    static void* g_cached_wts_method = nullptr;
    static void* g_cached_get_main_method = nullptr;
    static void* g_cached_get_current_method = nullptr;
    static void* g_cached_get_output_camera_method = nullptr;
    static void* g_cached_get_brain_state_method = nullptr;
    static void* g_cached_get_active_brain_count_method = nullptr;
    static void* g_cached_get_active_brain_method = nullptr;
    static void* g_cached_fallback_camera = nullptr;
    static void* g_player_camera_component_class = nullptr;
    static void* g_player_mob_class = nullptr;
    static void* g_move_component_class = nullptr;
    static void* g_game_mode_class = nullptr;
    static void* g_scene_camera_class = nullptr;
    static void* g_recoil_producer_component_class = nullptr;
    static void* g_spread_use_component_class = nullptr;
    static void* g_dispersion_component_class = nullptr;
    static void* g_item_use_request_container_class = nullptr;
    static void* g_kinematic_projectile_view_class = nullptr;
    static void* g_cached_get_camera_position_method = nullptr;
    static void* g_cached_get_first_person_camera_method = nullptr;
    static void* g_cached_get_player_avatar_method = nullptr;
    static void* g_cached_game_mode_get_player_avatar_method = nullptr;
    static void* g_cached_get_local_player_mob_method = nullptr;
    static void* g_cached_get_mobs_method = nullptr;
    static bool g_view_method_found = false;
    static bool g_proj_method_found = false;
    static bool g_wts_method_found = false;
    static const char* g_camera_source = "N";
    static int g_view_invoke_ok = 0;
    static int g_proj_invoke_ok = 0;
    static DWORD g_last_brain_scan_ms = 0;
    static DWORD g_last_camera_scan_ms = 0;
    static int g_brain_scan_count = 0;
    static int g_camera_scan_count = 0;

    static void* g_brain_ptr = nullptr;
    static Vector3 g_brain_pos = {0,0,0};
    static bool g_brain_pos_valid = false;
    static Vector3 g_local_player_pos = {0,0,0};
    static bool g_local_player_valid = false;
    static void* g_local_pno_ptr = nullptr;
    static void* g_local_player_avatar = nullptr;
    static void* g_local_player_camera_component = nullptr;
    static void* g_cached_has_input_auth = nullptr;
    static int g_pcc_avatar_count = 0;
    static int g_pcc_component_count = 0;
    static int g_pcc_auth_count = 0;
    static int g_pcc_position_count = 0;
    static int g_game_mode_count = 0;
    static int g_game_mode_local_mob_count = 0;
    static int g_scene_camera_count = 0;
    static int g_pno_object_count = 0;
    static int g_player_mob_count = 0;
    static int g_player_mob_position_count = 0;
    static int g_move_component_count = 0;
    static int g_move_component_position_count = 0;
    static int g_game_mode_mob_count = 0;
    static int g_game_mode_mob_position_count = 0;
    static int g_game_mode_team_count = 0;
    static int g_game_mode_team_player_count = 0;
    static int g_game_mode_team_mob_count = 0;
    static int g_grenade_count = 0;
    static std::vector<PlayerInfo> g_cached_players;
    static std::vector<uintptr_t> g_tracked_mobs;

    static constexpr uintptr_t RVA_HealthComponent_get_State = 0x1187720;
    static constexpr uintptr_t RVA_DeathComponent_get_DeathState = 0x11BCC20;
    static constexpr uintptr_t RVA_AvatarHpBar_get_IsDead = 0xFB3690;
    static constexpr uintptr_t RVA_MoveComponent_get_ViewRotation = 0x1156DD0;
    static constexpr uintptr_t RVA_MoveComponent_SetViewRotation = 0x11556B0;
    static constexpr uintptr_t RVA_MoveComponent_SetRecoilView = 0x11556A0;
    static constexpr uintptr_t RVA_RecoilComponent_get_State = 0x1007970;
    static constexpr uintptr_t RVA_RecoilComponent_SetRecoilAxis = 0x10078F0;
    static constexpr uintptr_t RVA_RecoilProducerComponent_get_State = 0x1007E70;
    static constexpr uintptr_t RVA_SpreadUseComponent_get_State = 0x10088B0;
    static constexpr uintptr_t RVA_DispersionComponent_get_State = 0xFFCC30;
    static constexpr uintptr_t RVA_ProjectileViewBase_Kinematic_get_IsFinished = 0xBBCEA0;

    int get_debug_player_count() { return g_player_count; }
    bool get_debug_camera_found() { return g_camera_found; }
    bool get_debug_view_method() { return g_view_method_found; }
    bool get_debug_proj_method() { return g_proj_method_found; }
    bool get_debug_wts_method() { return g_wts_method_found; }
    const char* get_debug_camera_source() { return g_camera_source; }
    int get_debug_view_invoke() { return g_view_invoke_ok; }
    int get_debug_proj_invoke() { return g_proj_invoke_ok; }
    int get_debug_player_camera_avatar_count() { return g_pcc_avatar_count; }
    int get_debug_player_camera_component_count() { return g_pcc_component_count; }
    int get_debug_player_camera_authority_count() { return g_pcc_auth_count; }
    int get_debug_player_camera_position_count() { return g_pcc_position_count; }
    bool get_debug_player_camera_cached() { return g_local_player_camera_component != nullptr; }
    int get_debug_game_mode_count() { return g_game_mode_count; }
    int get_debug_game_mode_local_mob_count() { return g_game_mode_local_mob_count; }
    int get_debug_scene_camera_count() { return g_scene_camera_count; }
    int get_debug_pno_object_count() { return g_pno_object_count; }
    int get_debug_player_mob_count() { return g_player_mob_count; }
    int get_debug_player_mob_position_count() { return g_player_mob_position_count; }
    int get_debug_move_component_count() { return g_move_component_count; }
    int get_debug_move_component_position_count() { return g_move_component_position_count; }
    int get_debug_game_mode_mob_count() { return g_game_mode_mob_count; }
    int get_debug_game_mode_mob_position_count() { return g_game_mode_mob_position_count; }
    int get_debug_game_mode_team_count() { return g_game_mode_team_count; }
    int get_debug_game_mode_team_player_count() { return g_game_mode_team_player_count; }
    int get_debug_game_mode_team_mob_count() { return g_game_mode_team_mob_count; }
    int get_debug_brain_scan_count() { return g_brain_scan_count; }
    int get_debug_camera_scan_count() { return g_camera_scan_count; }
    int get_debug_grenade_count() { return g_grenade_count; }

    static void publish_player_snapshot(const std::vector<PlayerInfo>& players) {
        g_player_count = (int32_t)players.size();
        g_cached_players = players;
    }

    static void track_mob_pointer(void* mob) {
        if (!mob)
            return;

        uintptr_t ptr = (uintptr_t)mob;
        for (uintptr_t tracked : g_tracked_mobs) {
            if (tracked == ptr)
                return;
        }

        if (g_tracked_mobs.size() < 128)
            g_tracked_mobs.push_back(ptr);
    }

    static bool contains_rendered_ptr(uintptr_t* rendered, int rendered_count, uintptr_t ptr) {
        for (int i = 0; i < rendered_count; i++) {
            if (rendered[i] == ptr)
                return true;
        }
        return false;
    }

    static void mark_rendered_ptr(uintptr_t* rendered, int& rendered_count, uintptr_t ptr) {
        if (!ptr || rendered_count >= 128 || contains_rendered_ptr(rendered, rendered_count, ptr))
            return;
        rendered[rendered_count++] = ptr;
    }

    std::vector<PlayerInfo> get_cached_players() {
        return g_cached_players;
    }

    static bool scan_interval_elapsed(DWORD& last_scan_ms, DWORD interval_ms) {
        DWORD now = GetTickCount();
        if (last_scan_ms != 0 && now - last_scan_ms < interval_ms)
            return false;
        last_scan_ms = now;
        return true;
    }

    static bool get_raw_matrix_from_method(void* obj, void* method, Matrix4x4* out_raw) {
        if (!out_raw) return false;
        *out_raw = Matrix4x4();
        if (!obj || !method) return false;
        void* exception = nullptr;
        void* boxed = il2cpp::runtime_invoke(method, obj, nullptr, &exception);
        if (!boxed) return false;
        void* unboxed = il2cpp::object_unbox(boxed);
        if (!unboxed) return false;
        memcpy(out_raw, unboxed, sizeof(Matrix4x4));
        return true;
    }

    static Matrix4x4 transpose_matrix(const Matrix4x4& raw) {
        Matrix4x4 mat = {};
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                mat[i * 4 + j] = raw[j * 4 + i];
        return mat;
    }

    static Matrix4x4 get_matrix_from_method(void* obj, void* method, bool* out_ok) {
        Matrix4x4 raw = {}, mat = {};
        if (out_ok) *out_ok = false;
        if (!get_raw_matrix_from_method(obj, method, &raw))
            return mat;
        mat = transpose_matrix(raw);
        if (out_ok) *out_ok = true;
        return mat;
    }

    Matrix4x4 get_view_matrix(void* camera) {
        if (!camera || !g_camera_class) return Matrix4x4();
        if (!g_cached_view_method)
            g_cached_view_method = il2cpp::class_get_method_from_name(g_camera_class, "get_worldToCameraMatrix", 0);
        g_view_method_found = (g_cached_view_method != nullptr);
        bool ok = false;
        Matrix4x4 r = get_matrix_from_method(camera, g_cached_view_method, &ok);
        if (ok) g_view_invoke_ok++;
        return r;
    }

    Matrix4x4 get_projection_matrix(void* camera) {
        if (!camera || !g_camera_class) return Matrix4x4();
        if (!g_cached_proj_method)
            g_cached_proj_method = il2cpp::class_get_method_from_name(g_camera_class, "get_projectionMatrix", 0);
        g_proj_method_found = (g_cached_proj_method != nullptr);
        bool ok = false;
        Matrix4x4 r = get_matrix_from_method(camera, g_cached_proj_method, &ok);
        if (ok) g_proj_invoke_ok++;
        return r;
    }

    static void* find_objects_of_type(void* klass, int32_t& out_len, bool include_inactive = false) {
        out_len = 0;
        if (!klass) return nullptr;

        void* method = nullptr;
        bool use_include_param = false;
        if (include_inactive) {
            method = il2cpp::class_get_method_from_name(g_object_class, "FindObjectsOfType", 2);
            use_include_param = (method != nullptr);
        }
        if (!method) {
            method = il2cpp::class_get_method_from_name(g_object_class, "FindObjectsOfType", 1);
            use_include_param = false;
        }
        if (!method) return nullptr;

        void* type = il2cpp::class_get_type(klass);
        if (!type) return nullptr;
        void* type_obj = il2cpp::type_get_object(type);
        if (!type_obj) return nullptr;

        bool include = include_inactive;
        void* params[2] = { type_obj, &include };
        void* exception = nullptr;
        void* arr = il2cpp::runtime_invoke(method, nullptr, use_include_param ? params : &params[0], &exception);
        if (!arr) return nullptr;

        out_len = il2cpp::array_length(arr);
        return arr;
    }

    static void** managed_object_array_items(void* arr) {
        if (!arr)
            return nullptr;
        return (void**)((uintptr_t)arr + 0x20);
    }

    static int managed_array_length_safe(void* arr) {
        if (!arr)
            return 0;
        int len = 0;
        __try { len = il2cpp::array_length(arr); }
        __except(EXCEPTION_EXECUTE_HANDLER) { len = 0; }
        return len;
    }

    static bool is_finite_vec3(const Vector3& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    static bool has_position_value(const Vector3& v) {
        return is_finite_vec3(v) && (fabs(v.x) > 0.01f || fabs(v.y) > 0.01f || fabs(v.z) > 0.01f);
    }

    static float clamp_angle(float value, float min_value, float max_value) {
        if (value < min_value) return min_value;
        if (value > max_value) return max_value;
        return value;
    }

    static float normalize_angle(float value) {
        while (value > 180.0f) value -= 360.0f;
        while (value < -180.0f) value += 360.0f;
        return value;
    }

    static bool is_valid_angle_pair(const Vector2& angles) {
        return std::isfinite(angles.x) && std::isfinite(angles.y) &&
            angles.x > -10000.0f && angles.x < 10000.0f &&
            angles.y > -1000.0f && angles.y < 1000.0f;
    }

    static bool is_reasonable_position(const Vector3& v) {
        return has_position_value(v) && fabs(v.x) < 100000.0f && fabs(v.y) < 100000.0f && fabs(v.z) < 100000.0f;
    }

    static bool get_camera_position_from_view_matrix(void* camera, Vector3* out_pos) {
        if (!camera || !out_pos)
            return false;

        if (!g_cached_view_method && g_camera_class)
            g_cached_view_method = il2cpp::class_get_method_from_name(g_camera_class, "get_worldToCameraMatrix", 0);
        g_view_method_found = (g_cached_view_method != nullptr);

        Matrix4x4 raw = {};
        if (!get_raw_matrix_from_method(camera, g_cached_view_method, &raw))
            return false;

        auto try_column_translation = [](const Matrix4x4& m, Vector3* out) -> bool {
            Vector3 t = { m[3], m[7], m[11] };
            Vector3 pos = {};
            pos.x = -(m[0] * t.x + m[4] * t.y + m[8] * t.z);
            pos.y = -(m[1] * t.x + m[5] * t.y + m[9] * t.z);
            pos.z = -(m[2] * t.x + m[6] * t.y + m[10] * t.z);
            if (!is_reasonable_position(pos))
                return false;
            *out = pos;
            return true;
        };

        auto try_row_translation = [](const Matrix4x4& m, Vector3* out) -> bool {
            Vector3 t = { m[12], m[13], m[14] };
            Vector3 pos = {};
            pos.x = -(m[0] * t.x + m[1] * t.y + m[2] * t.z);
            pos.y = -(m[4] * t.x + m[5] * t.y + m[6] * t.z);
            pos.z = -(m[8] * t.x + m[9] * t.y + m[10] * t.z);
            if (!is_reasonable_position(pos))
                return false;
            *out = pos;
            return true;
        };

        Matrix4x4 transposed = transpose_matrix(raw);
        return try_column_translation(raw, out_pos)
            || try_row_translation(raw, out_pos)
            || try_column_translation(transposed, out_pos)
            || try_row_translation(transposed, out_pos);
    }

    static void* class_get_method_recursive(void* klass, const char* name, int argc) {
        for (void* cur = klass; cur; cur = il2cpp::class_get_parent ? il2cpp::class_get_parent(cur) : nullptr) {
            void* method = il2cpp::class_get_method_from_name(cur, name, argc);
            if (method)
                return method;
        }
        return nullptr;
    }

    static void* find_class_anywhere(const char* namesp, const char* class_name) {
        if (!il2cpp::initialized) return nullptr;
        void* domain = il2cpp::get_domain();
        if (!domain) return nullptr;

        size_t asm_count = 0;
        void** assemblies = il2cpp::get_assemblies(domain, &asm_count);
        if (!assemblies) return nullptr;

        for (size_t i = 0; i < asm_count; i++) {
            if (!assemblies[i]) continue;
            void* img = il2cpp::assembly_get_image(assemblies[i]);
            if (!img) continue;
            void* klass = il2cpp::class_from_name(img, namesp, class_name);
            if (klass)
                return klass;
        }
        return nullptr;
    }

    static bool is_valid_quat(const Quaternion& q) {
        if (!std::isfinite(q.x) || !std::isfinite(q.y) || !std::isfinite(q.z) || !std::isfinite(q.w))
            return false;
        float len_sq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
        return len_sq > 0.01f && len_sq < 4.0f;
    }

    static Quaternion normalize_quat(Quaternion q) {
        float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (len <= 0.001f) return {0, 0, 0, 1};
        float inv = 1.0f / len;
        return {q.x * inv, q.y * inv, q.z * inv, q.w * inv};
    }

    static Quaternion multiply_quat(const Quaternion& a, const Quaternion& b) {
        return {
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
        };
    }

    static void basis_from_quat(const Quaternion& q_in, Vector3& fwd, Vector3& up, Vector3& right) {
        Quaternion q = normalize_quat(q_in);
        float qx = q.x, qy = q.y, qz = q.z, qw = q.w;

        fwd.x = 2.0f * (qx * qz + qw * qy);
        fwd.y = 2.0f * (qy * qz - qw * qx);
        fwd.z = 1.0f - 2.0f * (qx * qx + qy * qy);

        up.x = 2.0f * (qx * qy - qw * qz);
        up.y = 1.0f - 2.0f * (qx * qx + qz * qz);
        up.z = 2.0f * (qy * qz + qw * qx);

        right.x = 1.0f - 2.0f * (qy * qy + qz * qz);
        right.y = 2.0f * (qx * qy + qw * qz);
        right.z = 2.0f * (qx * qz - qw * qy);
    }

    static int invoke_int_method(void* obj, void* method);

    static bool read_camera_state_blob(uintptr_t state, Vector3* out_pos, Quaternion* out_rot,
        float* out_fov = nullptr, float* out_near = nullptr, float* out_far = nullptr, float* out_aspect = nullptr) {
        bool ok = false;
        __try {
            Vector3 raw_pos = *(Vector3*)(state + 0x70);        // CameraState.RawPosition
            Vector3 pos_correction = *(Vector3*)(state + 0xA0); // CameraState.PositionCorrection
            Vector3 final_pos = raw_pos + pos_correction;
            if (!has_position_value(final_pos) && has_position_value(raw_pos))
                final_pos = raw_pos;

            Quaternion raw_rot = *(Quaternion*)(state + 0x7C);        // CameraState.RawOrientation
            Quaternion rot_correction = *(Quaternion*)(state + 0xAC); // CameraState.OrientationCorrection
            Quaternion final_rot = raw_rot;
            if (is_valid_quat(raw_rot) && is_valid_quat(rot_correction))
                final_rot = multiply_quat(raw_rot, rot_correction);
            final_rot = normalize_quat(final_rot);

            if (has_position_value(final_pos)) {
                if (out_pos) *out_pos = final_pos;
                g_brain_pos = final_pos;
                g_brain_pos_valid = true;
                ok = true;
            }

            if (out_rot && is_valid_quat(final_rot))
                *out_rot = final_rot;

            if (out_fov) {
                float fov = *(float*)(state + 0x0); // LensSettings.FieldOfView
                if (std::isfinite(fov) && fov > 1.0f && fov <= 179.0f)
                    *out_fov = fov;
            }
            if (out_near) {
                float near_clip = *(float*)(state + 0x8); // LensSettings.NearClipPlane
                if (std::isfinite(near_clip) && near_clip > 0.0f)
                    *out_near = near_clip;
            }
            if (out_far) {
                float far_clip = *(float*)(state + 0xC); // LensSettings.FarClipPlane
                if (std::isfinite(far_clip) && far_clip > 0.0f)
                    *out_far = far_clip;
            }
            if (out_aspect) {
                float aspect = *(float*)(state + 0x54); // LensSettings.m_AspectFromCamera
                if (std::isfinite(aspect) && aspect > 0.0f)
                    *out_aspect = aspect;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            ok = false;
        }
        return ok;
    }

    static bool read_cinemachine_state(void* brain, Vector3* out_pos, Quaternion* out_rot,
        float* out_fov = nullptr, float* out_near = nullptr, float* out_far = nullptr, float* out_aspect = nullptr) {
        if (!brain) return false;

        bool ok = false;
        if (!g_cached_get_brain_state_method && g_cinemachine_brain_class)
            g_cached_get_brain_state_method = class_get_method_recursive(g_cinemachine_brain_class, "get_State", 0);

        if (g_cached_get_brain_state_method) {
            __try {
                void* exception = nullptr;
                void* boxed = il2cpp::runtime_invoke(g_cached_get_brain_state_method, brain, nullptr, &exception);
                void* state = boxed ? il2cpp::object_unbox(boxed) : nullptr;
                if (state)
                    ok = read_camera_state_blob((uintptr_t)state, out_pos, out_rot, out_fov, out_near, out_far, out_aspect);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                ok = false;
            }
            if (ok)
                return true;
        }

        __try {
            ok = read_camera_state_blob((uintptr_t)brain + 0x90, out_pos, out_rot, out_fov, out_near, out_far, out_aspect);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            if (brain == g_brain_ptr)
                g_brain_ptr = nullptr;
        }
        return ok;
    }

    static bool read_cinemachine_camera_state(void* vcam, Vector3* out_pos, Quaternion* out_rot,
        float* out_fov = nullptr, float* out_near = nullptr, float* out_far = nullptr, float* out_aspect = nullptr) {
        if (!vcam) return false;
        // dump.cs: CinemachineCamera.m_State is at 0x118.
        return read_camera_state_blob((uintptr_t)vcam + 0x118, out_pos, out_rot, out_fov, out_near, out_far, out_aspect);
    }

    static void* get_brain_output_camera(void* brain) {
        if (!brain) return nullptr;

        void* output_cam = nullptr;
        if (!g_cached_get_output_camera_method && g_cinemachine_brain_class)
            g_cached_get_output_camera_method = il2cpp::class_get_method_from_name(g_cinemachine_brain_class, "get_OutputCamera", 0);

        if (g_cached_get_output_camera_method) {
            __try {
                void* exception = nullptr;
                output_cam = il2cpp::runtime_invoke(g_cached_get_output_camera_method, brain, nullptr, &exception);
            } __except(EXCEPTION_EXECUTE_HANDLER) { output_cam = nullptr; }
        }

        if (!output_cam) {
            __try { output_cam = *(void**)((uintptr_t)brain + 0x60); } // CinemachineBrain.m_OutputCamera
            __except(EXCEPTION_EXECUTE_HANDLER) { output_cam = nullptr; }
        }

        return output_cam;
    }

    static bool refresh_cinemachine_brain() {
        if (!g_cinemachine_brain_class)
            return false;

        if (g_brain_ptr && read_cinemachine_state(g_brain_ptr, nullptr, nullptr))
            return true;

        if (!g_cached_get_active_brain_count_method)
            g_cached_get_active_brain_count_method = class_get_method_recursive(g_cinemachine_brain_class, "get_ActiveBrainCount", 0);
        if (!g_cached_get_active_brain_method)
            g_cached_get_active_brain_method = class_get_method_recursive(g_cinemachine_brain_class, "GetActiveBrain", 1);

        int active_brains = invoke_int_method(nullptr, g_cached_get_active_brain_count_method);
        if (active_brains > 0 && g_cached_get_active_brain_method) {
            void* fallback = nullptr;
            int max_brains = active_brains > 8 ? 8 : active_brains;
            for (int i = 0; i < max_brains; i++) {
                void* brain = nullptr;
                __try {
                    void* exception = nullptr;
                    void* params[1] = { &i };
                    brain = il2cpp::runtime_invoke(g_cached_get_active_brain_method, nullptr, params, &exception);
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    brain = nullptr;
                }

                if (!brain) continue;
                if (!fallback) fallback = brain;
                if (read_cinemachine_state(brain, nullptr, nullptr)) {
                    g_brain_ptr = brain;
                    return true;
                }
            }

            if (fallback) {
                g_brain_ptr = fallback;
                return true;
            }
        }

        if (!scan_interval_elapsed(g_last_brain_scan_ms, 1000))
            return false;
        g_brain_scan_count++;

        int32_t brain_count = 0;
        void* brain_arr = nullptr;
        __try { brain_arr = find_objects_of_type(g_cinemachine_brain_class, brain_count, true); }
        __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
        if (!brain_arr || brain_count <= 0)
            return false;

        void** brain_elements = nullptr;
        __try { brain_elements = managed_object_array_items(brain_arr); }
        __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
        if (!brain_elements)
            return false;

        void* fallback = nullptr;
        for (int32_t i = 0; i < brain_count; i++) {
            void* brain = brain_elements[i];
            if (!brain) continue;
            if (!fallback) fallback = brain;
            if (read_cinemachine_state(brain, nullptr, nullptr)) {
                g_brain_ptr = brain;
                return true;
            }
        }

        g_brain_ptr = fallback;
        return g_brain_ptr != nullptr;
    }

    static bool get_active_cinemachine_state(Vector3* out_pos, Quaternion* out_rot,
        float* out_fov = nullptr, float* out_near = nullptr, float* out_far = nullptr, float* out_aspect = nullptr) {
        if (!refresh_cinemachine_brain())
            return false;
        return read_cinemachine_state(g_brain_ptr, out_pos, out_rot, out_fov, out_near, out_far, out_aspect);
    }

    static void* get_active_cinemachine_camera() {
        if (!refresh_cinemachine_brain())
            return nullptr;
        return get_brain_output_camera(g_brain_ptr);
    }

    static Vector3 invoke_vector3_method(void* obj, void* method, bool* out_ok = nullptr) {
        Vector3 v = {};
        if (out_ok) *out_ok = false;
        if (!obj || !method) return v;

        void* exception = nullptr;
        void* result = nullptr;
        __try {
            result = il2cpp::runtime_invoke(method, obj, nullptr, &exception);
            if (result) {
                void* unboxed = il2cpp::object_unbox(result);
                if (unboxed) {
                    memcpy(&v, unboxed, sizeof(Vector3));
                    if (out_ok) *out_ok = true;
                }
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
        return v;
    }

    static bool invoke_bool_method(void* obj, void* method) {
        if (!obj || !method) return false;

        void* exception = nullptr;
        void* result = nullptr;
        __try {
            result = il2cpp::runtime_invoke(method, obj, nullptr, &exception);
            if (result) {
                void* unboxed = il2cpp::object_unbox(result);
                if (unboxed)
                    return *(bool*)unboxed;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    static void* invoke_ref_return_rva(void* obj, uintptr_t rva) {
        if (!obj || !il2cpp::module_base || !rva || (il2cpp::module_size && rva >= il2cpp::module_size))
            return nullptr;

        using fn_t = void* (*)(void*, void*);
        void* result = nullptr;
        __try {
            auto fn = (fn_t)(il2cpp::module_base + rva);
            result = fn(obj, nullptr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            result = nullptr;
        }
        return result;
    }

    static bool invoke_bool_rva(void* obj, uintptr_t rva, bool* out_value) {
        if (out_value)
            *out_value = false;
        if (!obj || !il2cpp::module_base || !rva || (il2cpp::module_size && rva >= il2cpp::module_size))
            return false;

        using fn_t = bool (*)(void*, void*);
        bool value = false;
        __try {
            auto fn = (fn_t)(il2cpp::module_base + rva);
            value = fn(obj, nullptr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        if (out_value)
            *out_value = value;
        return true;
    }

    static bool invoke_vector2_rva(void* obj, uintptr_t rva, Vector2* out_value) {
        if (out_value)
            *out_value = {};
        if (!obj || !out_value || !il2cpp::module_base || !rva || (il2cpp::module_size && rva >= il2cpp::module_size))
            return false;

        using fn_t = Vector2 (*)(void*, void*);
        Vector2 value = {};
        __try {
            auto fn = (fn_t)(il2cpp::module_base + rva);
            value = fn(obj, nullptr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        *out_value = value;
        return is_valid_angle_pair(value);
    }

    static bool invoke_void_vector2_rva(void* obj, uintptr_t rva, const Vector2& value) {
        if (!obj || !il2cpp::module_base || !rva || (il2cpp::module_size && rva >= il2cpp::module_size))
            return false;

        using fn_t = void (*)(void*, Vector2, void*);
        __try {
            auto fn = (fn_t)(il2cpp::module_base + rva);
            fn(obj, value, nullptr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
        return true;
    }

    static bool invoke_void_vector2_float_rva(void* obj, uintptr_t rva, const Vector2& value, float factor) {
        if (!obj || !il2cpp::module_base || !rva || (il2cpp::module_size && rva >= il2cpp::module_size))
            return false;

        using fn_t = void (*)(void*, Vector2, float, void*);
        __try {
            auto fn = (fn_t)(il2cpp::module_base + rva);
            fn(obj, value, factor, nullptr);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
        return true;
    }

    static bool patch_weapon_prototype_ballistics(void* weapon_prototype, bool patch_recoil, bool patch_spread) {
        if (!weapon_prototype)
            return false;

        bool wrote = false;

        void* scatter_aspect = nullptr;
        __try { scatter_aspect = *(void**)((uintptr_t)weapon_prototype + 0x70); } // WeaponPrototype._aspectWeaponScatter
        __except(EXCEPTION_EXECUTE_HANDLER) { scatter_aspect = nullptr; }

        if (scatter_aspect) {
            void* scatter_proto = nullptr;
            __try { scatter_proto = *(void**)((uintptr_t)scatter_aspect + 0x10); } // AspectBase<WeaponScatter>._protoData
            __except(EXCEPTION_EXECUTE_HANDLER) { scatter_proto = nullptr; }

            if (scatter_proto) {
                __try {
                    int32_t* has_bits = (int32_t*)((uintptr_t)scatter_proto + 0x18);
                    if (patch_spread) {
                        constexpr int32_t spread_mask =
                            (1 << 3) |  // minDespersion_
                            (1 << 4) |  // maxDispersion_
                            (1 << 5) |  // minFireDispersion_
                            (1 << 6) |  // maxFireDispersion_
                            (1 << 7) |  // dispersionIncFirstShot_
                            (1 << 8) |  // dispersionIncPerShot_
                            (1 << 11) | // runDispersionMult_
                            (1 << 12) | // airDispersionMult_
                            (1 << 13) | // aimDispersionMult_
                            (1 << 21);  // maxScatterMult_

                        *has_bits |= spread_mask;
                        *(float*)((uintptr_t)scatter_proto + 0x28) = 0.0f; // minDespersion_
                        *(float*)((uintptr_t)scatter_proto + 0x2C) = 0.0f; // maxDispersion_
                        *(float*)((uintptr_t)scatter_proto + 0x30) = 0.0f; // minFireDispersion_
                        *(float*)((uintptr_t)scatter_proto + 0x34) = 0.0f; // maxFireDispersion_
                        *(float*)((uintptr_t)scatter_proto + 0x38) = 0.0f; // dispersionIncFirstShot_
                        *(float*)((uintptr_t)scatter_proto + 0x3C) = 0.0f; // dispersionIncPerShot_
                        *(float*)((uintptr_t)scatter_proto + 0x48) = 0.0f; // runDispersionMult_
                        *(float*)((uintptr_t)scatter_proto + 0x4C) = 0.0f; // airDispersionMult_
                        *(float*)((uintptr_t)scatter_proto + 0x50) = 0.0f; // aimDispersionMult_
                        *(float*)((uintptr_t)scatter_proto + 0x70) = 0.0f; // maxScatterMult_
                        wrote = true;
                    }

                    if (patch_recoil) {
                        constexpr int32_t recoil_mask =
                            (1 << 14) | // recoilPatternId_
                            (1 << 15) | // recoilAimMult_
                            (1 << 16) | // recoilDecreaseMult_
                            (1 << 17) | // recoilMaxAngleX_
                            (1 << 18) | // recoilMaxAngleY_
                            (1 << 19) | // weaponRecoilShotCoeff_
                            (1 << 20) | // weaponRecoilEndSpeed_
                            (1 << 22);  // recoilScopeMult_

                        *has_bits |= recoil_mask;
                        *(int32_t*)((uintptr_t)scatter_proto + 0x54) = 0;  // recoilPatternId_
                        *(float*)((uintptr_t)scatter_proto + 0x58) = 0.0f; // recoilAimMult_
                        *(float*)((uintptr_t)scatter_proto + 0x5C) = 0.0f; // recoilDecreaseMult_
                        *(float*)((uintptr_t)scatter_proto + 0x60) = 0.0f; // recoilMaxAngleX_
                        *(float*)((uintptr_t)scatter_proto + 0x64) = 0.0f; // recoilMaxAngleY_
                        *(float*)((uintptr_t)scatter_proto + 0x68) = 0.0f; // weaponRecoilShotCoeff_
                        *(float*)((uintptr_t)scatter_proto + 0x6C) = 0.0f; // weaponRecoilEndSpeed_
                        *(float*)((uintptr_t)scatter_proto + 0x74) = 0.0f; // recoilScopeMult_
                        wrote = true;
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
        }

        if (patch_spread) {
            void* spread_aspect = nullptr;
            __try { spread_aspect = *(void**)((uintptr_t)weapon_prototype + 0x98); } // WeaponPrototype._aspectItemAttackSpread
            __except(EXCEPTION_EXECUTE_HANDLER) { spread_aspect = nullptr; }

            if (spread_aspect) {
                void* spread_proto = nullptr;
                __try { spread_proto = *(void**)((uintptr_t)spread_aspect + 0x10); } // AspectBase<ItemAttackSpread>._protoData
                __except(EXCEPTION_EXECUTE_HANDLER) { spread_proto = nullptr; }

                if (spread_proto) {
                    __try {
                        *(int32_t*)((uintptr_t)spread_proto + 0x18) |= (1 << 1); // spreadAngle_ is present
                        *(float*)((uintptr_t)spread_proto + 0x20) = 0.0f;        // spreadAngle_
                        wrote = true;
                    } __except(EXCEPTION_EXECUTE_HANDLER) {}
                }
            }
        }

        return wrote;
    }

    static int invoke_int_method(void* obj, void* method) {
        if (!method) return 0;

        void* exception = nullptr;
        void* result = nullptr;
        __try {
            result = il2cpp::runtime_invoke(method, obj, nullptr, &exception);
            if (result) {
                void* unboxed = il2cpp::object_unbox(result);
                if (unboxed)
                    return *(int*)unboxed;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    static void* invoke_object_method(void* obj, void* method) {
        if (!obj || !method) return nullptr;

        void* exception = nullptr;
        void* result = nullptr;
        __try {
            result = il2cpp::runtime_invoke(method, obj, nullptr, &exception);
        } __except(EXCEPTION_EXECUTE_HANDLER) { result = nullptr; }
        return result;
    }

    static void* ensure_player_network_object_class() {
        if (g_cached_pno_class)
            return g_cached_pno_class;

        g_cached_pno_class = find_class_anywhere("Psa.Core.Infrastructure.Spawners", "PlayerNetworkObject");
        if (g_cached_pno_class) {
            void* nick_field = il2cpp::class_get_field_from_name(g_cached_pno_class, "cache_Nickname");
            if (!nick_field)
                nick_field = il2cpp::class_get_field_from_name(g_cached_pno_class, "_Nickname");
            if (nick_field)
                g_nickname_off = il2cpp::field_get_offset(nick_field);
        }
        return g_cached_pno_class;
    }

    static void* get_object_runtime_class(void* obj, void* fallback_class = nullptr) {
        if (obj && il2cpp::object_get_class) {
            void* klass = nullptr;
            __try { klass = il2cpp::object_get_class(obj); }
            __except(EXCEPTION_EXECUTE_HANDLER) { klass = nullptr; }
            if (klass)
                return klass;
        }
        return fallback_class;
    }

    static void* get_player_camera_component_class() {
        if (!g_player_camera_component_class) {
            g_player_camera_component_class = find_class_anywhere(
                "Psa.Core.Modules.Camera.Services.Components.Implementation",
                "PlayerCameraComponent");
        }
        return g_player_camera_component_class;
    }

    static void* get_player_mob_class() {
        if (!g_player_mob_class) {
            g_player_mob_class = find_class_anywhere(
                "Psa.Core.Modules.Player.Components.Implementation",
                "PlayerMob");
        }
        return g_player_mob_class;
    }

    static void* get_move_component_class() {
        if (!g_move_component_class) {
            g_move_component_class = find_class_anywhere(
                "Psa.Core.Modules.Movements.Services.Components.Implementation",
                "MoveComponent");
        }
        return g_move_component_class;
    }

    static void* get_game_mode_class() {
        if (!g_game_mode_class) {
            g_game_mode_class = find_class_anywhere(
                "Psa.Core.Modules.RoundFlow.GameModes",
                "GameMode");
        }
        return g_game_mode_class;
    }

    static void* get_scene_camera_class() {
        if (!g_scene_camera_class) {
            g_scene_camera_class = find_class_anywhere(
                "Psa.Core.Modules.Shooting.SceneContext",
                "SceneCamera");
        }
        return g_scene_camera_class;
    }

    static void* get_recoil_producer_component_class() {
        if (!g_recoil_producer_component_class) {
            g_recoil_producer_component_class = find_class_anywhere(
                "Psa.Core.Modules.Shooting.Weapons.Components",
                "RecoilProducerComponent");
        }
        return g_recoil_producer_component_class;
    }

    static void* get_spread_use_component_class() {
        if (!g_spread_use_component_class) {
            g_spread_use_component_class = find_class_anywhere(
                "Psa.Core.Modules.Shooting.Weapons.Components",
                "SpreadUseComponent");
        }
        return g_spread_use_component_class;
    }

    static void* get_dispersion_component_class() {
        if (!g_dispersion_component_class) {
            g_dispersion_component_class = find_class_anywhere(
                "Psa.Core.Modules.Shooting.Weapons.Components",
                "DispersionComponent");
        }
        return g_dispersion_component_class;
    }

    static void* get_item_use_request_container_class() {
        if (!g_item_use_request_container_class) {
            g_item_use_request_container_class = find_class_anywhere(
                "Psa.Core.Modules.Shooting.Weapons.Components",
                "ItemUseRequestContainerComponent");
        }
        return g_item_use_request_container_class;
    }

    static void* get_kinematic_projectile_view_class() {
        if (!g_kinematic_projectile_view_class) {
            g_kinematic_projectile_view_class = find_class_anywhere(
                "Psa.Core.Modules.Shooting.Projectiles.Kinematic.Views",
                "KinematicProjectileView");
        }
        return g_kinematic_projectile_view_class;
    }

    static bool patch_weapon_prototypes_from_dispersion(bool patch_recoil, bool patch_spread) {
        void* dispersion_class = get_dispersion_component_class();
        int32_t count = 0;
        void* arr = nullptr;
        __try { arr = find_objects_of_type(dispersion_class, count, true); }
        __except(EXCEPTION_EXECUTE_HANDLER) { arr = nullptr; count = 0; }

        void** elements = nullptr;
        if (arr && count > 0) {
            __try { elements = managed_object_array_items(arr); }
            __except(EXCEPTION_EXECUTE_HANDLER) { elements = nullptr; }
        }
        if (count > 64)
            count = 64;

        bool wrote = false;
        for (int32_t i = 0; elements && i < count; i++) {
            void* dispersion = nullptr;
            __try { dispersion = elements[i]; }
            __except(EXCEPTION_EXECUTE_HANDLER) { dispersion = nullptr; }
            if (!dispersion)
                continue;

            void* weapon_prototype = nullptr;
            __try { weapon_prototype = *(void**)((uintptr_t)dispersion + 0x260); } // DispersionComponent.WeaponPrototype
            __except(EXCEPTION_EXECUTE_HANDLER) { weapon_prototype = nullptr; }

            if (patch_weapon_prototype_ballistics(weapon_prototype, patch_recoil, patch_spread))
                wrote = true;
        }

        return wrote;
    }

    static bool clear_single_request_container_offsets(void* container) {
        if (!container)
            return false;

        Vector3 zero = {};
        bool wrote = false;

        __try {
            *(Vector3*)((uintptr_t)container + 0x248) = zero; // ItemUseRequestContainerComponent.DirectionOffset
            wrote = true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {}

        void* requests = nullptr;
        __try { requests = *(void**)((uintptr_t)container + 0x240); } // ItemUseRequestContainerComponent.Requests
        __except(EXCEPTION_EXECUTE_HANDLER) { requests = nullptr; }
        if (!requests)
            return wrote;

        void* items = nullptr;
        int32_t size = 0;
        __try {
            items = *(void**)((uintptr_t)requests + 0x10); // List<ItemUseRequest>._items
            size = *(int32_t*)((uintptr_t)requests + 0x18); // List<ItemUseRequest>._size
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            items = nullptr;
            size = 0;
        }

        int32_t length = managed_array_length_safe(items);
        if (size > length)
            size = length;
        if (size > 16)
            size = 16;

        for (int32_t i = 0; items && i < size; i++) {
            uint8_t* request = (uint8_t*)items + 0x20 + ((uintptr_t)i * 0x68); // sizeof(ItemUseRequest)
            __try {
                *(Vector3*)(request + 0x18) = zero; // ItemUseRequest.ModifierDirection
                wrote = true;
            } __except(EXCEPTION_EXECUTE_HANDLER) {}
        }

        return wrote;
    }

    static bool clear_item_use_request_offsets() {
        static DWORD last_scan = 0;
        static std::vector<uintptr_t> cached_containers;

        if (scan_interval_elapsed(last_scan, 100)) {
            cached_containers.clear();

            void* container_class = get_item_use_request_container_class();
            int32_t count = 0;
            void* arr = nullptr;
            __try { arr = find_objects_of_type(container_class, count, false); }
            __except(EXCEPTION_EXECUTE_HANDLER) { arr = nullptr; count = 0; }

            void** elements = nullptr;
            if (arr && count > 0) {
                __try { elements = managed_object_array_items(arr); }
                __except(EXCEPTION_EXECUTE_HANDLER) { elements = nullptr; }
            }

            if (count > 64)
                count = 64;

            for (int32_t i = 0; elements && i < count; i++) {
                void* container = nullptr;
                __try { container = elements[i]; }
                __except(EXCEPTION_EXECUTE_HANDLER) { container = nullptr; }
                if (container)
                    cached_containers.push_back((uintptr_t)container);
            }
        }

        bool wrote = false;
        for (uintptr_t ptr : cached_containers) {
            if (clear_single_request_container_offsets((void*)ptr))
                wrote = true;
        }

        return wrote;
    }

    static void* get_player_avatar_from_pno(void* pno) {
        if (!pno) return nullptr;

        void* pno_class = get_object_runtime_class(pno, ensure_player_network_object_class());
        if (!g_cached_get_player_avatar_method && pno_class)
            g_cached_get_player_avatar_method = class_get_method_recursive(pno_class, "get_PlayerAvatar", 0);

        void* avatar = invoke_object_method(pno, g_cached_get_player_avatar_method);
        if (!avatar) {
            __try { avatar = *(void**)((uintptr_t)pno + 0x80); } // PlayerNetworkObject._PlayerAvatar
            __except(EXCEPTION_EXECUTE_HANDLER) { avatar = nullptr; }
        }
        return avatar;
    }

    static void* get_camera_component_from_avatar(void* avatar) {
        if (!avatar) return nullptr;

        void* comp = nullptr;
        __try { comp = *(void**)((uintptr_t)avatar + 0x788); } // PlayerMob.<PlayerCameraComponent>k__BackingField
        __except(EXCEPTION_EXECUTE_HANDLER) { comp = nullptr; }
        if (!comp) {
            __try { comp = *(void**)((uintptr_t)avatar + 0x7A8); } // PlayerMob._palyerCamera
            __except(EXCEPTION_EXECUTE_HANDLER) { comp = nullptr; }
        }
        if (!comp) {
            void* hp_bar = nullptr;
            __try { hp_bar = *(void**)((uintptr_t)avatar + 0x7C8); } // PlayerMob.HpBar
            __except(EXCEPTION_EXECUTE_HANDLER) { hp_bar = nullptr; }
            if (hp_bar) {
                __try { comp = *(void**)((uintptr_t)hp_bar + 0x178); } // AvatarHpBar._localPlayerCameraComponent
                __except(EXCEPTION_EXECUTE_HANDLER) { comp = nullptr; }
                if (!comp) {
                    __try { comp = *(void**)((uintptr_t)hp_bar + 0x160); } // AvatarHpBar._playercamera
                    __except(EXCEPTION_EXECUTE_HANDLER) { comp = nullptr; }
                }
            }
        }
        return comp;
    }

    static void* get_move_component_from_avatar(void* avatar) {
        if (!avatar) return nullptr;

        void* move = nullptr;
        __try { move = *(void**)((uintptr_t)avatar + 0x7A0); } // PlayerMob._moveComponent
        __except(EXCEPTION_EXECUTE_HANDLER) { move = nullptr; }
        return move;
    }

    static void* get_local_move_component() {
        return get_move_component_from_avatar(g_local_player_avatar);
    }

    Vector3 get_object_position(void* obj);

    static void cache_local_player_from_mob(void* mob) {
        if (!mob) return;

        g_local_player_avatar = mob;
        g_local_player_camera_component = get_camera_component_from_avatar(mob);
        if (!g_local_player_valid)
            g_local_player_pos = get_object_position(mob);
        if (has_position_value(g_local_player_pos))
            g_local_player_valid = true;
    }

    static void* get_local_player_mob_from_game_mode(void* game_mode) {
        if (!game_mode) return nullptr;

        void* klass = get_object_runtime_class(game_mode, get_game_mode_class());
        if (!g_cached_get_local_player_mob_method && klass)
            g_cached_get_local_player_mob_method = class_get_method_recursive(klass, "get_LocalPlayerMob", 0);

        void* mob = invoke_object_method(game_mode, g_cached_get_local_player_mob_method);
        if (!mob) {
            __try { mob = *(void**)((uintptr_t)game_mode + 0xA60); } // GameMode._localPlayerMob
            __except(EXCEPTION_EXECUTE_HANDLER) { mob = nullptr; }
        }
        return mob;
    }

    static void refresh_scene_camera_cache() {
        void* scene_camera_class = get_scene_camera_class();
        if (!scene_camera_class)
            return;

        int32_t count = 0;
        void* arr = nullptr;
        __try { arr = find_objects_of_type(scene_camera_class, count, true); }
        __except(EXCEPTION_EXECUTE_HANDLER) { return; }
        if (!arr || count <= 0)
            return;

        void** elements = nullptr;
        __try { elements = managed_object_array_items(arr); }
        __except(EXCEPTION_EXECUTE_HANDLER) { return; }
        if (!elements)
            return;

        for (int32_t i = 0; i < count; i++) {
            void* scene_camera = elements[i];
            if (!scene_camera) continue;
            g_scene_camera_count++;

            void* camera = nullptr;
            void* brain = nullptr;
            __try {
                camera = *(void**)((uintptr_t)scene_camera + 0x20); // SceneCamera._camera
                brain = *(void**)((uintptr_t)scene_camera + 0x28);   // SceneCamera._brain
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                camera = nullptr;
                brain = nullptr;
            }

            if (camera)
                g_cached_fallback_camera = camera;
            if (brain)
                g_brain_ptr = brain;
        }
    }

    static void refresh_game_mode_local_player_cache() {
        void* game_mode_class = get_game_mode_class();
        if (!game_mode_class)
            return;

        int32_t count = 0;
        void* arr = nullptr;
        __try { arr = find_objects_of_type(game_mode_class, count, true); }
        __except(EXCEPTION_EXECUTE_HANDLER) { return; }
        if (!arr || count <= 0)
            return;

        void** elements = nullptr;
        __try { elements = managed_object_array_items(arr); }
        __except(EXCEPTION_EXECUTE_HANDLER) { return; }
        if (!elements)
            return;

        for (int32_t i = 0; i < count; i++) {
            void* game_mode = elements[i];
            if (!game_mode) continue;
            g_game_mode_count++;

            void* mob = get_local_player_mob_from_game_mode(game_mode);
            if (!mob)
                continue;

            g_game_mode_local_mob_count++;
            cache_local_player_from_mob(mob);
            if (g_local_player_camera_component)
                break;
        }
    }

    static void cache_local_player_from_pno(void* pno) {
        if (!pno) return;
        g_local_pno_ptr = pno;
        g_local_player_avatar = get_player_avatar_from_pno(pno);
        g_local_player_camera_component = get_camera_component_from_avatar(g_local_player_avatar);
    }

    static bool get_player_camera_component_position(void* camera_component, Vector3* out_pos);

    static bool pno_has_input_authority(void* pno) {
        if (!pno) return false;

        void* klass = get_object_runtime_class(pno, ensure_player_network_object_class());
        if (!g_cached_has_input_auth && klass)
            g_cached_has_input_auth = class_get_method_recursive(klass, "get_HasInputAuthority", 0);
        return invoke_bool_method(pno, g_cached_has_input_auth);
    }

    static void* find_local_player_camera_component() {
        try_init_classes();
        return g_local_player_camera_component;
    }

    static void* get_first_person_camera_from_component(void* camera_component) {
        if (!camera_component) return nullptr;

        void* vcam = nullptr;
        void* comp_class = get_object_runtime_class(camera_component, get_player_camera_component_class());
        if (!g_cached_get_first_person_camera_method && comp_class)
            g_cached_get_first_person_camera_method = class_get_method_recursive(comp_class, "get_FirstPersonCamera", 0);

        vcam = invoke_object_method(camera_component, g_cached_get_first_person_camera_method);
        if (!vcam) {
            __try { vcam = *(void**)((uintptr_t)camera_component + 0x68); } // PlayerCameraComponent._firstPersonCamera
            __except(EXCEPTION_EXECUTE_HANDLER) { vcam = nullptr; }
        }
        return vcam;
    }

    static bool get_player_camera_component_position(void* camera_component, Vector3* out_pos) {
        if (!camera_component || !out_pos) return false;

        void* pivot = nullptr;
        __try { pivot = *(void**)((uintptr_t)camera_component + 0x60); } // PlayerCameraComponent._cameraPivot
        __except(EXCEPTION_EXECUTE_HANDLER) { pivot = nullptr; }
        if (pivot) {
            Vector3 pos = get_transform_position(pivot);
            if (has_position_value(pos)) {
                *out_pos = pos;
                return true;
            }
        }

        void* vcam = get_first_person_camera_from_component(camera_component);
        Vector3 pos = {};
        if (vcam && read_cinemachine_camera_state(vcam, &pos, nullptr) && has_position_value(pos)) {
            *out_pos = pos;
            return true;
        }

        void* comp_class = get_object_runtime_class(camera_component, get_player_camera_component_class());
        if (!g_cached_get_camera_position_method && comp_class)
            g_cached_get_camera_position_method = class_get_method_recursive(comp_class, "get_CameraPosition", 0);

        bool ok = false;
        pos = invoke_vector3_method(camera_component, g_cached_get_camera_position_method, &ok);
        if (ok && has_position_value(pos)) {
            *out_pos = pos;
            return true;
        }

        return false;
    }

    static bool get_active_player_camera_state(Vector3* out_pos, Quaternion* out_rot,
        float* out_fov = nullptr, float* out_near = nullptr, float* out_far = nullptr, float* out_aspect = nullptr) {
        void* camera_component = find_local_player_camera_component();
        if (!camera_component)
            return false;

        bool got_any = false;
        Vector3 comp_pos = {};
        if (get_player_camera_component_position(camera_component, &comp_pos)) {
            if (out_pos) *out_pos = comp_pos;
            got_any = true;
        }

        void* vcam = get_first_person_camera_from_component(camera_component);
        Vector3 state_pos = {};
        Quaternion state_rot = {};
        bool got_state = read_cinemachine_camera_state(vcam, &state_pos, &state_rot, out_fov, out_near, out_far, out_aspect);
        if (got_state) {
            if (out_pos && !has_position_value(comp_pos))
                *out_pos = state_pos;
            if (out_rot && is_valid_quat(state_rot))
                *out_rot = state_rot;
            got_any = true;
        }

        return got_any;
    }

    void* get_main_camera() {
        try_init_classes();
        if (!g_camera_class || !il2cpp::initialized) return nullptr;

        if (!g_cached_get_main_method)
            g_cached_get_main_method = il2cpp::class_get_method_from_name(g_camera_class, "get_main", 0);
        if (!g_cached_get_current_method)
            g_cached_get_current_method = il2cpp::class_get_method_from_name(g_camera_class, "get_current", 0);

        auto position_valid = [](void* c) -> bool {
            if (!c) return false;
            __try {
                void* t = get_transform(c);
                if (t) {
                    Vector3 p = get_transform_position(t);
                    if (has_position_value(p))
                        return true;
                }

                Vector3 matrix_pos = {};
                return get_camera_position_from_view_matrix(c, &matrix_pos);
            } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
        };

        void* best_cam = nullptr;

        // Gameplay uses Cinemachine; prefer its live output camera when the brain has a real pose.
        best_cam = get_active_cinemachine_camera();
        if (best_cam && position_valid(best_cam))
            goto done;
        if (best_cam) {
            g_cached_fallback_camera = best_cam;
            best_cam = nullptr;
        }

        // Camera.current is often the render camera while Unity is drawing a frame.
        if (g_cached_get_current_method) {
            void* exception = nullptr;
            __try {
                best_cam = il2cpp::runtime_invoke(g_cached_get_current_method, nullptr, nullptr, &exception);
            } __except(EXCEPTION_EXECUTE_HANDLER) { best_cam = nullptr; }
            if (best_cam && position_valid(best_cam))
                goto done;
        }

        // Then try Camera.main
        if (g_cached_get_main_method) {
            void* exception = nullptr;
            __try {
                best_cam = il2cpp::runtime_invoke(g_cached_get_main_method, nullptr, nullptr, &exception);
            } __except(EXCEPTION_EXECUTE_HANDLER) { best_cam = nullptr; }
            if (best_cam && position_valid(best_cam))
                goto done;
        }

        if (g_cached_fallback_camera) {
            best_cam = g_cached_fallback_camera;
            if (position_valid(best_cam))
                goto done;
        }

        // Camera.main was null or invalid — enumerate all cameras
        if (scan_interval_elapsed(g_last_camera_scan_ms, 1000)) {
            g_camera_scan_count++;
            int32_t count = 0;
            void* arr = nullptr;
            __try { arr = find_objects_of_type(g_camera_class, count, true); } __except(EXCEPTION_EXECUTE_HANDLER) {}
            if (arr && count > 0) {
                void** elements = managed_object_array_items(arr);
                if (elements) {
                    for (int32_t i = 0; i < count; i++) {
                        if (elements[i] && position_valid(elements[i])) {
                            best_cam = elements[i];
                            goto done;
                        }
                    }
                    best_cam = elements[0];
                }
            }
            if (best_cam)
                g_cached_fallback_camera = best_cam;
        }

        if (!best_cam && g_cached_fallback_camera)
            best_cam = g_cached_fallback_camera;

        // Last resort: accept Cinemachine's output camera even if the state was not readable yet.
        if (!best_cam && refresh_cinemachine_brain())
            best_cam = get_brain_output_camera(g_brain_ptr);

    done:
        g_camera_found = (best_cam != nullptr);

        if (!g_cached_wts_method)
            g_cached_wts_method = il2cpp::class_get_method_from_name(g_camera_class, "WorldToScreenPoint", 1);
        g_wts_method_found = (g_cached_wts_method != nullptr);

        return best_cam;
    }

    bool world_to_screen(const Vector3& world, Vector2& screen, const Matrix4x4& view, const Matrix4x4& proj) {
        Matrix4x4 vp;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                vp[i * 4 + j] = 0;
                for (int k = 0; k < 4; k++)
                    vp[i * 4 + j] += view[i * 4 + k] * proj[k * 4 + j];
            }
        }

        Vector4 transform;
        transform.x = world.x * vp[0] + world.y * vp[4] + world.z * vp[8] + vp[12];
        transform.y = world.x * vp[1] + world.y * vp[5] + world.z * vp[9] + vp[13];
        transform.z = world.x * vp[2] + world.y * vp[6] + world.z * vp[10] + vp[14];
        transform.w = world.x * vp[3] + world.y * vp[7] + world.z * vp[11] + vp[15];

        if (transform.w < 0.01f) return false;

        float inv_w = 1.0f / transform.w;
        float ndc_x = transform.x * inv_w;
        float ndc_y = transform.y * inv_w;

        HWND game_hwnd = renderer::get_window();
        if (!game_hwnd) game_hwnd = GetDesktopWindow();
        RECT rect;
        if (!GetWindowRect(game_hwnd, &rect)) return false;
        int sw = rect.right - rect.left;
        int sh = rect.bottom - rect.top;

        screen.x = (ndc_x * 0.5f + 0.5f) * sw;
        screen.y = (-ndc_y * 0.5f + 0.5f) * sh;
        return true;
    }

    static Matrix4x4 build_view_matrix(const Vector3& pos, const Vector3& forward, const Vector3& up, const Vector3& right) {
        // view = [R^T | -R^T * pos; 0 0 0 1] where R = [right | up | -forward]
        Matrix4x4 m = {};
        m[0] = right.x;   m[4] = right.y;   m[8]  = right.z;   m[12] = -(right.x * pos.x + right.y * pos.y + right.z * pos.z);
        m[1] = up.x;      m[5] = up.y;      m[9]  = up.z;      m[13] = -(up.x * pos.x + up.y * pos.y + up.z * pos.z);
        m[2] = -forward.x; m[6] = -forward.y; m[10] = -forward.z; m[14] = forward.x * pos.x + forward.y * pos.y + forward.z * pos.z;
        m[3] = 0;         m[7] = 0;         m[11] = 0;          m[15] = 1;
        return m;
    }

    static Matrix4x4 build_projection_matrix(float fov, float aspect, float near_, float far_) {
        float f = 1.0f / tanf(fov * 0.5f * 3.14159265f / 180.0f);
        float fn = far_ - near_;
        Matrix4x4 m = {};
        m[0] = f / aspect; m[4] = 0;   m[8]  = 0;                      m[12] = 0;
        m[1] = 0;          m[5] = f;   m[9]  = 0;                      m[13] = 0;
        m[2] = 0;          m[6] = 0;   m[10] = -(far_ + near_) / fn;   m[14] = -2.0f * far_ * near_ / fn;
        m[3] = 0;          m[7] = 0;   m[11] = -1.0f;                   m[15] = 0;
        return m;
    }

    bool world_to_screen(const Vector3& world, Vector2& screen) {
        void* cam = get_main_camera();
        if (!cam) return false;

        __try {
            Vector3 active_pos = {};
            Quaternion active_rot = {};
            float active_fov = 75.0f, active_near = 0.3f, active_far = 1000.0f, active_aspect = 0.0f;
            bool have_active_pose = get_active_player_camera_state(
                &active_pos, &active_rot, &active_fov, &active_near, &active_far, &active_aspect);
            if (!have_active_pose) {
                have_active_pose = get_active_cinemachine_state(
                    &active_pos, &active_rot, &active_fov, &active_near, &active_far, &active_aspect);
            }

            if (active_aspect <= 0.0f) {
                HWND game_hwnd = renderer::get_window();
                RECT rect = {};
                if (game_hwnd && GetWindowRect(game_hwnd, &rect)) {
                    float width = (float)(rect.right - rect.left);
                    float height = (float)(rect.bottom - rect.top);
                    if (width > 0.0f && height > 0.0f)
                        active_aspect = width / height;
                }
                if (active_aspect <= 0.0f)
                    active_aspect = 16.0f / 9.0f;
            }

            if (have_active_pose && has_position_value(active_pos) && is_valid_quat(active_rot)) {
                Vector3 fwd = {}, up = {}, right = {};
                basis_from_quat(active_rot, fwd, up, right);
                Matrix4x4 view_mat = build_view_matrix(active_pos, fwd, up, right);
                Matrix4x4 proj_mat = build_projection_matrix(active_fov, active_aspect, active_near, active_far);
                return world_to_screen(world, screen, view_mat, proj_mat);
            }

            // Try WorldToScreenPoint first
            if (g_cached_wts_method) {
                void* exception = nullptr;
                void* params[1] = { const_cast<Vector3*>(&world) };
                void* result = nullptr;
                __try {
                    result = il2cpp::runtime_invoke(g_cached_wts_method, cam, params, &exception);
                } __except(EXCEPTION_EXECUTE_HANDLER) { result = nullptr; }
                if (result) {
                    void* unboxed = nullptr;
                    __try { unboxed = il2cpp::object_unbox(result); } __except(EXCEPTION_EXECUTE_HANDLER) {}
                    if (unboxed) {
                        float* vec3 = (float*)unboxed;
                        screen.x = vec3[0];
                        screen.y = vec3[1];
                        if (vec3[2] >= 0.0f)
                            return true;
                    }
                }
            }

            // Try get_worldToCameraMatrix + get_projectionMatrix
            if (!g_cached_view_method)
                g_cached_view_method = il2cpp::class_get_method_from_name(g_camera_class, "get_worldToCameraMatrix", 0);
            if (!g_cached_proj_method)
                g_cached_proj_method = il2cpp::class_get_method_from_name(g_camera_class, "get_projectionMatrix", 0);
            g_view_method_found = (g_cached_view_method != nullptr);
            g_proj_method_found = (g_cached_proj_method != nullptr);

            bool ok_v = false, ok_p = false;
            Matrix4x4 view_mat = get_matrix_from_method(cam, g_cached_view_method, &ok_v);
            Matrix4x4 proj_mat = get_matrix_from_method(cam, g_cached_proj_method, &ok_p);
            if (ok_v && ok_p)
                return world_to_screen(world, screen, view_mat, proj_mat);

            // Fallback: build matrices from Transform data only (avoids broken camera icalls)
            void* transform = get_transform(cam);
            if (!transform) return false;
            Vector3 pos = get_transform_position(transform);

            if (!has_position_value(pos) && have_active_pose)
                pos = active_pos;

            // Still zero — use local player position as last resort
            if (!has_position_value(pos)) {
                if (g_local_player_valid) {
                    pos = g_local_player_pos;
                }
            }

            Vector3 fwd = {}, up = {}, right = {};
            if (have_active_pose && is_valid_quat(active_rot))
                basis_from_quat(active_rot, fwd, up, right);

            void* get_fwd_m = il2cpp::class_get_method_from_name(g_transform_class, "get_forward", 0);
            void* get_up_m = il2cpp::class_get_method_from_name(g_transform_class, "get_up", 0);
            void* get_right_m = il2cpp::class_get_method_from_name(g_transform_class, "get_right", 0);

            auto invoke_v3 = [](void* m, void* obj) -> Vector3 {
                Vector3 v = {0,0,0};
                if (!m || !obj) return v;
                void* exc = nullptr;
                __try {
                    void* r = il2cpp::runtime_invoke(m, obj, nullptr, &exc);
                    if (r) { void* u = il2cpp::object_unbox(r); if (u) memcpy(&v, u, sizeof(Vector3)); }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
                return v;
            };

            if (fwd.x == 0 && fwd.y == 0 && fwd.z == 0) {
                fwd = invoke_v3(get_fwd_m, transform);
                up = invoke_v3(get_up_m, transform);
                right = invoke_v3(get_right_m, transform);
            }

            float fov = 75.0f, near_ = 0.3f, far_ = 1000.0f;
            float aspect = 16.0f / 9.0f;
            if (active_fov > 1.0f && active_fov <= 179.0f) fov = active_fov;
            if (active_near > 0.0f) near_ = active_near;
            if (active_far > 0.0f) far_ = active_far;
            if (active_aspect > 0.0f) aspect = active_aspect;
            {
                void* get_fov_m = il2cpp::class_get_method_from_name(g_camera_class, "get_fieldOfView", 0);
                void* get_aspect_m = il2cpp::class_get_method_from_name(g_camera_class, "get_aspect", 0);
                void* get_near_m = il2cpp::class_get_method_from_name(g_camera_class, "get_nearClipPlane", 0);
                void* get_far_m = il2cpp::class_get_method_from_name(g_camera_class, "get_farClipPlane", 0);
                auto invoke_f = [](void* m, void* obj) -> float {
                    if (!m || !obj) return 0;
                    void* exc = nullptr;
                    __try {
                        void* r = il2cpp::runtime_invoke(m, obj, nullptr, &exc);
                        if (r) { void* u = il2cpp::object_unbox(r); if (u) return *(float*)u; }
                    } __except(EXCEPTION_EXECUTE_HANDLER) {}
                    return 0;
                };
                float f = invoke_f(get_fov_m, cam);
                float a = invoke_f(get_aspect_m, cam);
                float n = invoke_f(get_near_m, cam);
                float f2 = invoke_f(get_far_m, cam);
                if (f > 0 && f <= 180) fov = f;
                if (a > 0) aspect = a;
                if (n > 0) near_ = n;
                if (f2 > 0) far_ = f2;
            }

            if (fwd.x == 0 && fwd.y == 0 && fwd.z == 0) {
                void* get_euler_m = il2cpp::class_get_method_from_name(g_transform_class, "get_eulerAngles", 0);
                Vector3 euler = invoke_v3(get_euler_m, transform);
                float yaw = euler.y * 3.14159265f / 180.0f;
                float pitch = euler.x * 3.14159265f / 180.0f;
                fwd.x = cosf(pitch) * sinf(yaw);
                fwd.y = sinf(pitch);
                fwd.z = cosf(pitch) * cosf(yaw);
                right.x = fwd.z; right.y = 0; right.z = -fwd.x;
                float rl = sqrtf(right.x * right.x + right.z * right.z);
                if (rl > 0.001f) { right.x /= rl; right.z /= rl; }
                up.x = fwd.y * right.z - fwd.z * right.y;
                up.y = fwd.z * right.x - fwd.x * right.z;
                up.z = fwd.x * right.y - fwd.y * right.x;
            }

            if (fwd.x == 0 && fwd.y == 0 && fwd.z == 0) return false;

            view_mat = build_view_matrix(pos, fwd, up, right);
            proj_mat = build_projection_matrix(fov, aspect, near_, far_);
            return world_to_screen(world, screen, view_mat, proj_mat);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    std::string get_object_name(void* obj) {
        if (!obj) return "null";
        void* get_name = il2cpp::class_get_method_from_name(g_object_class, "get_name", 0);
        if (!get_name) return "unknown";

        void* exception = nullptr;
        void* str = il2cpp::runtime_invoke(get_name, obj, nullptr, &exception);
        if (!str) return "unknown";

        int len = il2cpp::string_length_fn(str);
        wchar_t* chars = il2cpp::string_chars(str);
        if (!chars || len <= 0) return "unknown";

        int size = WideCharToMultiByte(CP_UTF8, 0, chars, len, nullptr, 0, nullptr, nullptr);
        std::string result(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, chars, len, &result[0], size, nullptr, nullptr);
        return result;
    }

    Vector3 get_transform_position(void* transform) {
        Vector3 pos = {};
        if (!transform || !il2cpp::initialized) return pos;

        void* method = il2cpp::class_get_method_from_name(g_transform_class, "get_position", 0);
        if (!method) return pos;

        void* exception = nullptr;
        void* result = il2cpp::runtime_invoke(method, transform, nullptr, &exception);
        if (result) {
            void* unboxed = il2cpp::object_unbox(result);
            if (unboxed)
                memcpy(&pos, unboxed, sizeof(Vector3));
        }
        return pos;
    }

    void* get_transform(void* obj) {
        if (!obj || !g_component_class || !il2cpp::initialized) return nullptr;

        void* method = il2cpp::class_get_method_from_name(g_component_class, "get_transform", 0);
        if (!method) return nullptr;

        void* exception = nullptr;
        void* result = nullptr;
        __try {
            result = il2cpp::runtime_invoke(method, obj, nullptr, &exception);
        } __except(EXCEPTION_EXECUTE_HANDLER) { result = nullptr; }
        return result;
    }

    Vector3 get_object_position(void* obj) {
        if (!obj) return Vector3();
        void* transform = get_transform(obj);
        if (!transform) return Vector3();
        return get_transform_position(transform);
    }

    Vector3 get_camera_position(void* camera) {
        Vector3 pos = {};
        __try {
            if (get_active_player_camera_state(&pos, nullptr) && has_position_value(pos)) {
                g_camera_source = "PCC";
                return pos;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }

        __try {
            if (camera)
                pos = get_object_position(camera);
        } __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
        if (has_position_value(pos))
            g_camera_source = "U";

        if (!has_position_value(pos)) {
            __try {
                if (get_camera_position_from_view_matrix(camera, &pos))
                    g_camera_source = "VM";
            } __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
        }

        if (!has_position_value(pos)) {
            Vector3 brain_pos = {};
            if (get_active_cinemachine_state(&brain_pos, nullptr)) {
                pos = brain_pos;
                g_camera_source = "CB";
            }
        }

        if (!has_position_value(pos) && g_local_player_valid) {
            pos = g_local_player_pos;
            g_camera_source = "LP";
        }

        if (!has_position_value(pos))
            g_camera_source = "N";

        return pos;
    }

    bool aim_at_world(const Vector3& target, float smooth) {
        try_init_classes();
        if (!il2cpp::initialized)
            return false;

        void* move = get_local_move_component();
        if (!move)
            return false;

        Vector3 cam_pos = {};
        void* cam = nullptr;
        __try { cam = get_main_camera(); }
        __except(EXCEPTION_EXECUTE_HANDLER) { cam = nullptr; }

        if (cam) {
            __try { cam_pos = get_camera_position(cam); }
            __except(EXCEPTION_EXECUTE_HANDLER) { cam_pos = {}; }
        }
        if (!has_position_value(cam_pos) && g_local_player_valid)
            cam_pos = g_local_player_pos;
        if (!has_position_value(cam_pos))
            return false;

        Vector3 dir = target - cam_pos;
        float flat = sqrtf(dir.x * dir.x + dir.z * dir.z);
        if (!std::isfinite(flat) || flat < 0.001f)
            return false;

        float yaw = normalize_angle(atan2f(dir.x, dir.z) * (180.0f / 3.14159265f));
        float pitch = clamp_angle(-atan2f(dir.y, flat) * (180.0f / 3.14159265f), -65.0f, 80.0f);

        Vector2 target_angles;
        target_angles.x = pitch;
        target_angles.y = yaw;

        Vector2 write_angles = target_angles;
        Vector2 current_angles = {};
        if (smooth > 1.0f && invoke_vector2_rva(move, RVA_MoveComponent_get_ViewRotation, &current_angles)) {
            float step = 1.0f / smooth;
            if (step < 0.01f) step = 0.01f;
            if (step > 1.0f) step = 1.0f;

            write_angles.x = clamp_angle(current_angles.x + normalize_angle(target_angles.x - current_angles.x) * step, -65.0f, 80.0f);
            write_angles.y = normalize_angle(current_angles.y + normalize_angle(target_angles.y - current_angles.y) * step);
        }

        if (!is_valid_angle_pair(write_angles))
            return false;

        return invoke_void_vector2_rva(move, RVA_MoveComponent_SetViewRotation, write_angles);
    }

    bool compensate_recoil() {
        try_init_classes();
        if (!il2cpp::initialized)
            return false;

        bool wrote = false;
        if (clear_item_use_request_offsets())
            wrote = true;

        void* move = get_local_move_component();
        if (!move)
            return wrote;

        Vector2 zero_axis = {};
        Vector3 zero_euler = {};

        if (invoke_void_vector2_float_rva(move, RVA_MoveComponent_SetRecoilView, zero_axis, 0.0f))
            wrote = true;

        __try {
            *(Vector3*)((uintptr_t)move + 0xD8) = zero_euler; // MoveComponent._recoilViewCurrentEuler
            *(Vector3*)((uintptr_t)move + 0xE4) = zero_euler; // MoveComponent._recoilViewTargetEuler
            *(Vector2*)((uintptr_t)move + 0xF0) = zero_axis;  // MoveComponent._recoilAxisTarget
            *(float*)((uintptr_t)move + 0xF8) = 0.0f;         // MoveComponent._recoilViewFactor
            wrote = true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {}

        void* recoil = nullptr;
        __try { recoil = *(void**)((uintptr_t)move + 0xB8); } // MoveComponent._recoil
        __except(EXCEPTION_EXECUTE_HANDLER) { recoil = nullptr; }

        if (recoil) {
            if (invoke_void_vector2_rva(recoil, RVA_RecoilComponent_SetRecoilAxis, zero_axis))
                wrote = true;

            void* state = invoke_ref_return_rva(recoil, RVA_RecoilComponent_get_State);
            if (state) {
                __try {
                    *(Vector2*)((uintptr_t)state + 0x0) = zero_axis; // RecoilState.Recoil
                    *(Vector2*)((uintptr_t)state + 0x8) = zero_axis; // RecoilState.PreviousRecoil
                    *(int32_t*)((uintptr_t)state + 0x10) = 0;         // RecoilState.RecoilAimEffectMult
                    wrote = true;
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
        }

        static DWORD last_recoil_producer_scan = 0;
        static std::vector<uintptr_t> cached_recoil_producers;
        if (scan_interval_elapsed(last_recoil_producer_scan, 100)) {
            cached_recoil_producers.clear();

            void* recoil_producer_class = get_recoil_producer_component_class();
            int32_t count = 0;
            void* arr = nullptr;
            __try { arr = find_objects_of_type(recoil_producer_class, count, true); }
            __except(EXCEPTION_EXECUTE_HANDLER) { arr = nullptr; count = 0; }

            void** elements = nullptr;
            if (arr && count > 0) {
                __try { elements = managed_object_array_items(arr); }
                __except(EXCEPTION_EXECUTE_HANDLER) { elements = nullptr; }
            }

            if (count > 128)
                count = 128;

            for (int32_t i = 0; elements && i < count; i++) {
                void* producer = nullptr;
                __try { producer = elements[i]; }
                __except(EXCEPTION_EXECUTE_HANDLER) { producer = nullptr; }
                if (producer)
                    cached_recoil_producers.push_back((uintptr_t)producer);
            }

            if (patch_weapon_prototypes_from_dispersion(true, false))
                wrote = true;
        }

        for (uintptr_t producer_ptr : cached_recoil_producers) {
            void* producer = (void*)producer_ptr;
            if (!producer)
                continue;

            __try {
                *(float*)((uintptr_t)producer + 0x258) = 0.0f; // RecoilConfig.RecoilAimMultiplier
                *(float*)((uintptr_t)producer + 0x25C) = 0.0f; // RecoilConfig.RecoilDecreaseSpeed
                *(float*)((uintptr_t)producer + 0x260) = 0.0f; // RecoilConfig.RecoilDecreaseDelay
                *(float*)((uintptr_t)producer + 0x278) = 0.0f; // RecoilIncreaseTimeMultiplier
                *(float*)((uintptr_t)producer + 0x27C) = 0.0f; // RecoilScopeMult
                *(int*)((uintptr_t)producer + 0x280) = 0;      // FireTicks
                *(int*)((uintptr_t)producer + 0x284) = 0;      // RecoilTicks
                *(int*)((uintptr_t)producer + 0x288) = 0;      // RecoilDecreaseDelayTicks
                wrote = true;
            } __except(EXCEPTION_EXECUTE_HANDLER) {}

            void* producer_state = invoke_ref_return_rva(producer, RVA_RecoilProducerComponent_get_State);
            if (producer_state) {
                __try {
                    memset(producer_state, 0, 0x18); // RecoilProducerComponent.RecoilState
                    wrote = true;
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
        }

        return wrote;
    }

    bool remove_spread() {
        try_init_classes();
        if (!il2cpp::initialized)
            return false;

        bool wrote = false;
        if (clear_item_use_request_offsets())
            wrote = true;

        static DWORD last_spread_scan = 0;
        static bool last_spread_success = false;
        if (!scan_interval_elapsed(last_spread_scan, 100)) {
            last_spread_success = wrote || last_spread_success;
            return last_spread_success;
        }

        void* spread_class = get_spread_use_component_class();
        int32_t spread_count = 0;
        void* spread_arr = nullptr;
        __try { spread_arr = find_objects_of_type(spread_class, spread_count, true); }
        __except(EXCEPTION_EXECUTE_HANDLER) { spread_arr = nullptr; spread_count = 0; }

        void** spread_elements = nullptr;
        if (spread_arr && spread_count > 0) {
            __try { spread_elements = managed_object_array_items(spread_arr); }
            __except(EXCEPTION_EXECUTE_HANDLER) { spread_elements = nullptr; }
        }
        if (spread_count > 128)
            spread_count = 128;

        for (int32_t i = 0; spread_elements && i < spread_count; i++) {
            void* spread = nullptr;
            __try { spread = spread_elements[i]; }
            __except(EXCEPTION_EXECUTE_HANDLER) { spread = nullptr; }
            if (!spread)
                continue;

            void* state = invoke_ref_return_rva(spread, RVA_SpreadUseComponent_get_State);
            if (state) {
                __try {
                    *(int32_t*)((uintptr_t)state + 0x0) = 0; // SpreadUseState.Angle
                    wrote = true;
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
        }

        void* dispersion_class = get_dispersion_component_class();
        int32_t dispersion_count = 0;
        void* dispersion_arr = nullptr;
        __try { dispersion_arr = find_objects_of_type(dispersion_class, dispersion_count, true); }
        __except(EXCEPTION_EXECUTE_HANDLER) { dispersion_arr = nullptr; dispersion_count = 0; }

        void** dispersion_elements = nullptr;
        if (dispersion_arr && dispersion_count > 0) {
            __try { dispersion_elements = managed_object_array_items(dispersion_arr); }
            __except(EXCEPTION_EXECUTE_HANDLER) { dispersion_elements = nullptr; }
        }
        if (dispersion_count > 128)
            dispersion_count = 128;

        for (int32_t i = 0; dispersion_elements && i < dispersion_count; i++) {
            void* dispersion = nullptr;
            __try { dispersion = dispersion_elements[i]; }
            __except(EXCEPTION_EXECUTE_HANDLER) { dispersion = nullptr; }
            if (!dispersion)
                continue;

            void* state = invoke_ref_return_rva(dispersion, RVA_DispersionComponent_get_State);
            if (state) {
                __try {
                    *(float*)((uintptr_t)state + 0x0) = 0.0f; // DispersionChangeTime
                    *(int32_t*)((uintptr_t)state + 0xC) = 0;  // Dispersion
                    *(int32_t*)((uintptr_t)state + 0x10) = 0; // DispersionMult
                    wrote = true;
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }

            __try {
                *(float*)((uintptr_t)dispersion + 0x258) = 0.0f; // ChangeDuration
                *(float*)((uintptr_t)dispersion + 0x268) = 0.0f; // MinCalculateDispersion
                wrote = true;
            } __except(EXCEPTION_EXECUTE_HANDLER) {}

            void* weapon_prototype = nullptr;
            __try { weapon_prototype = *(void**)((uintptr_t)dispersion + 0x260); } // DispersionComponent.WeaponPrototype
            __except(EXCEPTION_EXECUTE_HANDLER) { weapon_prototype = nullptr; }

            if (patch_weapon_prototype_ballistics(weapon_prototype, false, true))
                wrote = true;
        }

        last_spread_success = wrote;
        return wrote;
    }

    std::vector<WorldObjectInfo> get_grenades() {
        std::vector<WorldObjectInfo> grenades;
        g_grenade_count = 0;

        try_init_classes();
        if (!il2cpp::initialized || !g_ready)
            return grenades;

        void* projectile_view_class = get_kinematic_projectile_view_class();
        int32_t count = 0;
        void* arr = nullptr;
        __try { arr = find_objects_of_type(projectile_view_class, count, false); }
        __except(EXCEPTION_EXECUTE_HANDLER) { arr = nullptr; count = 0; }

        void** elements = nullptr;
        if (arr && count > 0) {
            __try { elements = managed_object_array_items(arr); }
            __except(EXCEPTION_EXECUTE_HANDLER) { elements = nullptr; }
        }
        if (count > 64)
            count = 64;

        for (int32_t i = 0; elements && i < count; i++) {
            void* view = nullptr;
            __try { view = elements[i]; }
            __except(EXCEPTION_EXECUTE_HANDLER) { view = nullptr; }
            if (!view)
                continue;

            bool finished = false;
            if (invoke_bool_rva(view, RVA_ProjectileViewBase_Kinematic_get_IsFinished, &finished) && finished)
                continue;

            Vector3 pos = {};
            __try { pos = get_object_position(view); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }

            if (!has_position_value(pos)) {
                void* transform = nullptr;
                __try { transform = *(void**)((uintptr_t)view + 0x178); } // KinematicProjectileView._transform
                __except(EXCEPTION_EXECUTE_HANDLER) { transform = nullptr; }
                if (transform) {
                    __try { pos = get_transform_position(transform); }
                    __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
                }
            }

            if (!is_reasonable_position(pos))
                continue;

            std::string name;
            __try { name = get_object_name(view); }
            __except(EXCEPTION_EXECUTE_HANDLER) { name.clear(); }
            if (name.empty() || name == "unknown" || name == "null")
                name = "Grenade";
            if (name.find("Kinematic") != std::string::npos || name.find("Projectile") != std::string::npos)
                name = "Grenade";

            WorldObjectInfo info;
            info.name = name;
            info.position = pos;
            info.is_valid = true;
            info.object_ptr = (uintptr_t)view;
            info.source_type = 1;
            grenades.push_back(info);
        }

        g_grenade_count = (int)grenades.size();
        return grenades;
    }

    void* find_object_of_type(const char* image_name, const char* namesp, const char* class_name) {
        void* klass = il2cpp::find_class(image_name, namesp, class_name);
        if (!klass) return nullptr;

        int32_t len = 0;
        void* arr = find_objects_of_type(klass, len, true);
        if (!arr) return nullptr;
        if (len <= 0) return nullptr;

        void** elements = managed_object_array_items(arr);
        if (!elements) return nullptr;

        return elements[0];
    }

    static std::string read_managed_string(void* str_obj) {
        if (!str_obj) return "";
        int len = il2cpp::string_length_fn(str_obj);
        if (len <= 0) return "";
        wchar_t* chars = il2cpp::string_chars(str_obj);
        if (!chars) return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, chars, len, nullptr, 0, nullptr, nullptr);
        if (size <= 0) return "";
        std::string result(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, chars, len, &result[0], size, nullptr, nullptr);
        return result;
    }

    static Vector3 get_position_from_transform_field(void* obj, uintptr_t offset) {
        Vector3 pos = {};
        if (!obj)
            return pos;

        void* transform = nullptr;
        __try { transform = *(void**)((uintptr_t)obj + offset); }
        __except(EXCEPTION_EXECUTE_HANDLER) { transform = nullptr; }

        if (transform) {
            __try { pos = get_transform_position(transform); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
        }

        return pos;
    }

    static Vector3 get_move_component_position(void* move) {
        Vector3 pos = {};
        if (!move)
            return pos;

        __try { pos = get_object_position(move); }
        __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
        if (has_position_value(pos))
            return pos;

        pos = get_position_from_transform_field(move, 0x120); // MoveComponent.<UpperBodyHitboxPivot>k__BackingField
        if (has_position_value(pos))
            return pos;

        pos = get_position_from_transform_field(move, 0x70); // MoveComponent.cameraPivot
        if (has_position_value(pos))
            return pos;

        pos = get_position_from_transform_field(move, 0x78); // MoveComponent.cameraRecoil
        if (has_position_value(pos))
            return pos;

        pos = get_position_from_transform_field(move, 0x100); // MoveComponent.<ExplosionApplyingPoint>k__BackingField
        if (has_position_value(pos))
            return pos;

        void* controller = nullptr;
        __try { controller = *(void**)((uintptr_t)move + 0xC0); } // MoveComponent._psaCC
        __except(EXCEPTION_EXECUTE_HANDLER) { controller = nullptr; }
        if (controller) {
            __try { pos = get_object_position(controller); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
            if (has_position_value(pos))
                return pos;
        }

        void* entity = nullptr;
        __try { entity = *(void**)((uintptr_t)move + 0x20); } // EcsComponentBeh._entity
        __except(EXCEPTION_EXECUTE_HANDLER) { entity = nullptr; }
        if (entity) {
            __try { pos = get_object_position(entity); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
        }

        return pos;
    }

    static Vector3 get_player_mob_position(void* mob) {
        Vector3 pos = {};
        if (!mob) return pos;

        __try { pos = get_object_position(mob); }
        __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
        if (has_position_value(pos))
            return pos;

        void* move = nullptr;
        __try { move = *(void**)((uintptr_t)mob + 0x7A0); } // PlayerMob._moveComponent
        __except(EXCEPTION_EXECUTE_HANDLER) { move = nullptr; }
        if (move) {
            pos = get_move_component_position(move);
            if (has_position_value(pos))
                return pos;
        }

        void* entity = nullptr;
        __try { entity = *(void**)((uintptr_t)mob + 0x98); } // EcsComponentNet.<Entity>
        __except(EXCEPTION_EXECUTE_HANDLER) { entity = nullptr; }
        if (entity) {
            __try { pos = get_object_position(entity); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
            if (has_position_value(pos))
                return pos;
        }

        void* health_component = nullptr;
        __try { health_component = *(void**)((uintptr_t)mob + 0x760); } // PlayerMob.<HealthComponent>k__BackingField
        __except(EXCEPTION_EXECUTE_HANDLER) { health_component = nullptr; }
        if (health_component) {
            __try { pos = get_object_position(health_component); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
            if (has_position_value(pos))
                return pos;
        }

        void* hit_target = nullptr;
        __try { hit_target = *(void**)((uintptr_t)mob + 0x770); } // PlayerMob.<HitTargetComponent>k__BackingField
        __except(EXCEPTION_EXECUTE_HANDLER) { hit_target = nullptr; }
        if (hit_target) {
            __try { pos = get_object_position(hit_target); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
            if (has_position_value(pos))
                return pos;
        }

        void* hp_bar = nullptr;
        __try { hp_bar = *(void**)((uintptr_t)mob + 0x7C8); } // PlayerMob.HpBar
        __except(EXCEPTION_EXECUTE_HANDLER) { hp_bar = nullptr; }
        if (hp_bar) {
            __try { pos = get_object_position(hp_bar); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
        }

        return pos;
    }

    static bool get_player_mob_team_info(void* mob, bool* is_teammate) {
        if (is_teammate)
            *is_teammate = false;
        if (!mob)
            return false;

        void* hp_bar = nullptr;
        __try { hp_bar = *(void**)((uintptr_t)mob + 0x7C8); } // PlayerMob.HpBar
        __except(EXCEPTION_EXECUTE_HANDLER) { hp_bar = nullptr; }
        if (!hp_bar)
            return false;

        bool ally = false;
        __try { ally = *(bool*)((uintptr_t)hp_bar + 0x1C1); } // AvatarHpBar._isAlly
        __except(EXCEPTION_EXECUTE_HANDLER) { return false; }

        if (is_teammate)
            *is_teammate = ally;
        return true;
    }

    static void* get_player_mob_hp_bar(void* mob) {
        if (!mob)
            return nullptr;

        void* hp_bar = nullptr;
        __try { hp_bar = *(void**)((uintptr_t)mob + 0x7C8); } // PlayerMob.HpBar
        __except(EXCEPTION_EXECUTE_HANDLER) { hp_bar = nullptr; }
        return hp_bar;
    }

    static bool get_player_mob_hp_bar_fill(void* mob, float* out_fill) {
        if (out_fill)
            *out_fill = 1.0f;

        void* hp_bar = get_player_mob_hp_bar(mob);
        if (!hp_bar)
            return false;

        float fill = 1.0f;
        __try { fill = *(float*)((uintptr_t)hp_bar + 0x19C); } // AvatarHpBar._currentFill
        __except(EXCEPTION_EXECUTE_HANDLER) { return false; }

        if (!std::isfinite(fill) || fill < -0.01f || fill > 1.01f)
            return false;

        if (fill < 0.0f)
            fill = 0.0f;
        if (fill > 1.0f)
            fill = 1.0f;

        if (out_fill)
            *out_fill = fill;
        return true;
    }

    static bool get_death_component_is_dead(void* death_component, bool* out_dead) {
        if (out_dead)
            *out_dead = false;
        if (!death_component)
            return false;

        uint8_t state = 0;
        __try { state = *(uint8_t*)((uintptr_t)death_component + 0xC8); } // DeathComponent._DeathState.State
        __except(EXCEPTION_EXECUTE_HANDLER) { return false; }

        if (out_dead)
            *out_dead = (state & 0x1) != 0;
        return true;
    }

    static bool get_player_mob_hp_bar_is_dead(void* mob, bool* out_dead) {
        if (out_dead)
            *out_dead = false;

        void* hp_bar = get_player_mob_hp_bar(mob);
        if (!hp_bar)
            return false;

        void* death_component = nullptr;
        __try { death_component = *(void**)((uintptr_t)hp_bar + 0x168); } // AvatarHpBar._selfDeathComponent
        __except(EXCEPTION_EXECUTE_HANDLER) { death_component = nullptr; }
        return get_death_component_is_dead(death_component, out_dead);
    }

    static bool get_player_mob_death_state_flag(void* mob, bool* out_dead) {
        if (out_dead)
            *out_dead = false;
        if (!mob)
            return false;

        void* death_component = nullptr;
        __try { death_component = *(void**)((uintptr_t)mob + 0x758); } // PlayerMob.<DeathComponent>
        __except(EXCEPTION_EXECUTE_HANDLER) { death_component = nullptr; }
        if (!death_component)
            return false;

        return get_death_component_is_dead(death_component, out_dead);
    }

    static float hp_amount_to_float(int raw_hp) {
        return (float)raw_hp * 0.001f;
    }

    static bool read_health_state_values(void* state, int initial_health, float* out_health, float* out_max_health) {
        if (out_health)
            *out_health = 100.0f;
        if (out_max_health)
            *out_max_health = 100.0f;
        if (!state)
            return false;

        int health_raw = 0;
        int max_raw = 0;
        float max_mult = 1.0f;
        float boost_mult = 1.0f;
        __try {
            health_raw = *(int*)((uintptr_t)state + 0x4);   // HealthState.Health.Value
            max_mult = *(float*)((uintptr_t)state + 0xC);   // HealthState.MaxHealthMultiplier
            boost_mult = *(float*)((uintptr_t)state + 0x10); // HealthState.HpBoostOnKillMultiplier
            max_raw = *(int*)((uintptr_t)state + 0x18);     // HealthState._maxHealth.Value
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        float health = hp_amount_to_float(health_raw);
        float max_health = hp_amount_to_float(max_raw);
        if (!std::isfinite(max_mult) || max_mult <= 0.0f || max_mult > 20.0f)
            max_mult = 1.0f;
        if (!std::isfinite(boost_mult) || boost_mult <= 0.0f || boost_mult > 20.0f)
            boost_mult = 1.0f;
        max_health *= max_mult * boost_mult;

        if (!std::isfinite(max_health) || max_health <= 0.0f) {
            if (initial_health > 0 && initial_health < 100000)
                max_health = (float)initial_health;
            else
                max_health = 100.0f;
        }
        if (!std::isfinite(health))
            health = max_health;
        if (health < 0.0f)
            health = 0.0f;
        if (health > max_health)
            health = max_health;

        if (out_health)
            *out_health = health;
        if (out_max_health)
            *out_max_health = max_health;
        return true;
    }

    static bool get_health_component_values(void* health_component, float* out_health, float* out_max_health) {
        if (out_health)
            *out_health = 100.0f;
        if (out_max_health)
            *out_max_health = 100.0f;
        if (!health_component)
            return false;

        int initial_health = 0;
        __try { initial_health = *(int*)((uintptr_t)health_component + 0xA8); } // HealthComponent.InitialHealth
        __except(EXCEPTION_EXECUTE_HANDLER) { initial_health = 0; }

        void* state_ref = invoke_ref_return_rva(health_component, RVA_HealthComponent_get_State);
        if (read_health_state_values(state_ref, initial_health, out_health, out_max_health))
            return true;

        return read_health_state_values((void*)((uintptr_t)health_component + 0xAC),
            initial_health, out_health, out_max_health);
    }

    static bool get_player_mob_dead_state_only(void* mob) {
        bool is_dead = false;
        if (get_player_mob_hp_bar_is_dead(mob, &is_dead) && is_dead)
            return true;
        if (get_player_mob_death_state_flag(mob, &is_dead) && is_dead)
            return true;
        return false;
    }

    static bool get_player_mob_health_info(void* mob, float* out_health, float* out_max_health) {
        if (out_health)
            *out_health = 100.0f;
        if (out_max_health)
            *out_max_health = 100.0f;
        if (!mob)
            return false;

        float component_health = 100.0f;
        float component_max_health = 100.0f;
        bool component_ok = false;
        void* health_component = nullptr;
        __try { health_component = *(void**)((uintptr_t)mob + 0x760); } // PlayerMob.<HealthComponent>
        __except(EXCEPTION_EXECUTE_HANDLER) { health_component = nullptr; }
        if (health_component)
            component_ok = get_health_component_values(health_component, &component_health, &component_max_health);

        bool dead_state = get_player_mob_dead_state_only(mob);
        if (component_ok) {
            if (component_health <= 0.01f && !dead_state) {
                float fill = 1.0f;
                if (get_player_mob_hp_bar_fill(mob, &fill) && fill > 0.01f)
                    component_health = fill * component_max_health;
                else
                    component_health = component_max_health;
            }
            if (out_health)
                *out_health = component_health;
            if (out_max_health)
                *out_max_health = component_max_health;
            return true;
        }

        float fill = 1.0f;
        bool fill_ok = get_player_mob_hp_bar_fill(mob, &fill);
        if (fill_ok) {
            if (fill <= 0.01f && !dead_state)
                fill = 1.0f;
            if (out_health)
                *out_health = fill * 100.0f;
            if (out_max_health)
                *out_max_health = 100.0f;
            return true;
        }

        return false;
    }

    static bool get_player_mob_is_dead(void* mob) {
        if (!mob)
            return true;

        return get_player_mob_dead_state_only(mob);
    }

    static PlayerInfo make_player_info(const std::string& name, const Vector3& pos, uintptr_t object_ptr,
        uintptr_t avatar_ptr, int source_type) {
        Vector3 head = pos;
        head.y += 1.75f;

        PlayerInfo pi;
        pi.name = name;
        pi.position = pos;
        pi.head_position = head;
        pi.health = 100.0f;
        pi.max_health = 100.0f;
        pi.is_valid = true;
        pi.object_ptr = object_ptr;
        pi.avatar_ptr = avatar_ptr;
        pi.source_type = source_type;
        pi.team_known = false;
        pi.is_teammate = false;
        pi.is_local = false;
        pi.is_dead = false;

        void* mob = avatar_ptr ? (void*)avatar_ptr : (source_type == 2 ? (void*)object_ptr : nullptr);
        if (mob) {
            pi.is_dead = get_player_mob_is_dead(mob);
            __try { get_player_mob_health_info(mob, &pi.health, &pi.max_health); }
            __except(EXCEPTION_EXECUTE_HANDLER) {}
            __try { pi.team_known = get_player_mob_team_info(mob, &pi.is_teammate); }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                pi.team_known = false;
                pi.is_teammate = false;
            }
            pi.is_local = (mob == g_local_player_avatar);
        }
        return pi;
    }

    struct DictPtrEntry {
        int hash_code;
        int next;
        int key;
        int pad;
        void* value;
    };

    static void* get_game_mode_mobs_dictionary(void* game_mode) {
        if (!game_mode)
            return nullptr;

        void* klass = get_object_runtime_class(game_mode, get_game_mode_class());
        if (!g_cached_get_mobs_method && klass)
            g_cached_get_mobs_method = class_get_method_recursive(klass, "GetMobs", 0);

        void* dict = invoke_object_method(game_mode, g_cached_get_mobs_method);
        if (!dict) {
            __try { dict = *(void**)((uintptr_t)game_mode + 0x130); } // GameMode._playerMobs
            __except(EXCEPTION_EXECUTE_HANDLER) { dict = nullptr; }
        }
        return dict;
    }

    static int collect_ptr_values_from_dictionary(void* dict, void** out_values, int max_values) {
        if (!dict || !out_values || max_values <= 0)
            return 0;

        void* entries_arr = nullptr;
        int dict_count = 0;
        __try {
            entries_arr = *(void**)((uintptr_t)dict + 0x18);
            dict_count = *(int*)((uintptr_t)dict + 0x20);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }

        if (!entries_arr || dict_count <= 0)
            return 0;
        if (dict_count > 256)
            return 0;

        int arr_len = managed_array_length_safe(entries_arr);
        if (arr_len <= 0)
            return 0;
        if (arr_len > 256)
            arr_len = 256;

        int limit = dict_count < arr_len ? dict_count : arr_len;
        if (limit > 64)
            limit = 64;

        auto entries = (DictPtrEntry*)((uintptr_t)entries_arr + 0x20);
        int found = 0;
        for (int i = 0; i < limit && found < max_values; i++) {
            DictPtrEntry entry = {};
            __try { entry = entries[i]; }
            __except(EXCEPTION_EXECUTE_HANDLER) { continue; }

            if (entry.hash_code < 0 || !entry.value)
                continue;

            bool dup = false;
            for (int j = 0; j < found; j++) {
                if (out_values[j] == entry.value) {
                    dup = true;
                    break;
                }
            }
            if (dup)
                continue;

            out_values[found++] = entry.value;
        }

        return found;
    }

    static int collect_player_mobs_from_dictionary(void* dict, void** out_mobs, int max_mobs) {
        return collect_ptr_values_from_dictionary(dict, out_mobs, max_mobs);
    }

    static void* get_game_mode_team_models_dictionary(void* game_mode) {
        if (!game_mode)
            return nullptr;

        void* dict = nullptr;
        __try { dict = *(void**)((uintptr_t)game_mode + 0x140); } // GameMode._teamModels
        __except(EXCEPTION_EXECUTE_HANDLER) { dict = nullptr; }
        return dict;
    }

    static int get_game_mode_team_number(void* team) {
        int team_number = -1;
        if (!team)
            return team_number;
        __try { team_number = *(int*)((uintptr_t)team + 0x10); } // GameModeTeam.<Team>
        __except(EXCEPTION_EXECUTE_HANDLER) { team_number = -1; }
        return team_number;
    }

    static int collect_game_mode_team_players(void* team, void** out_players, int max_players) {
        if (!team || !out_players || max_players <= 0)
            return 0;

        void* list = nullptr;
        __try { list = *(void**)((uintptr_t)team + 0x18); } // GameModeTeam._players
        __except(EXCEPTION_EXECUTE_HANDLER) { list = nullptr; }
        if (!list)
            return 0;

        void* items_arr = nullptr;
        int size = 0;
        __try {
            items_arr = *(void**)((uintptr_t)list + 0x10); // List<T>._items
            size = *(int*)((uintptr_t)list + 0x18);        // List<T>._size
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
        if (!items_arr || size <= 0)
            return 0;
        if (size > 64)
            size = 64;

        int arr_len = managed_array_length_safe(items_arr);
        if (arr_len <= 0)
            return 0;
        int limit = size < arr_len ? size : arr_len;
        if (limit > max_players)
            limit = max_players;

        void** items = managed_object_array_items(items_arr);
        if (!items)
            return 0;

        int found = 0;
        for (int i = 0; i < limit; i++) {
            void* player = nullptr;
            __try { player = items[i]; }
            __except(EXCEPTION_EXECUTE_HANDLER) { player = nullptr; }
            if (!player)
                continue;

            bool dup = false;
            for (int j = 0; j < found; j++) {
                if (out_players[j] == player) {
                    dup = true;
                    break;
                }
            }
            if (dup)
                continue;

            out_players[found++] = player;
        }
        return found;
    }

    static int get_game_mode_player_ref(void* player_data) {
        int player_ref = 0;
        if (!player_data)
            return player_ref;
        __try { player_ref = *(int*)((uintptr_t)player_data + 0x40); } // GameModePlayer.<PlayerRef>
        __except(EXCEPTION_EXECUTE_HANDLER) { player_ref = 0; }
        return player_ref;
    }

    static bool is_game_mode_player_mine(void* player_data) {
        bool is_mine = false;
        if (!player_data)
            return false;
        __try { is_mine = *(bool*)((uintptr_t)player_data + 0x48); } // GameModePlayer.<IsMine>
        __except(EXCEPTION_EXECUTE_HANDLER) { is_mine = false; }
        return is_mine;
    }

    static std::string get_game_mode_player_name(void* player_data) {
        if (!player_data)
            return "";
        void* str = nullptr;
        __try { str = *(void**)((uintptr_t)player_data + 0x30); } // GameModePlayer.<Name>
        __except(EXCEPTION_EXECUTE_HANDLER) { str = nullptr; }
        return read_managed_string(str);
    }

    static void* get_player_avatar_from_game_mode_ref(void* game_mode, int player_ref) {
        if (!game_mode)
            return nullptr;

        void* klass = get_object_runtime_class(game_mode, get_game_mode_class());
        if (!g_cached_game_mode_get_player_avatar_method && klass)
            g_cached_game_mode_get_player_avatar_method = class_get_method_recursive(klass, "GetPlayerAvatar", 1);
        if (!g_cached_game_mode_get_player_avatar_method)
            return nullptr;

        void* mob = nullptr;
        __try {
            void* exception = nullptr;
            void* params[1] = { &player_ref };
            mob = il2cpp::runtime_invoke(g_cached_game_mode_get_player_avatar_method, game_mode, params, &exception);
            if (exception)
                mob = nullptr;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            mob = nullptr;
        }
        return mob;
    }

    static PlayerInfo* find_player_info_for_mob(std::vector<PlayerInfo>& players, uintptr_t mob_ptr) {
        if (!mob_ptr)
            return nullptr;
        for (auto& player : players) {
            if (player.avatar_ptr == mob_ptr || player.object_ptr == mob_ptr)
                return &player;
        }
        return nullptr;
    }

    static void apply_game_mode_team_info(PlayerInfo& player, const std::string& name,
        bool team_known, bool is_teammate, bool is_local) {
        if (!name.empty())
            player.name = name;
        if (team_known) {
            player.team_known = true;
            player.is_teammate = is_teammate;
        }
        if (is_local)
            player.is_local = true;
    }

    static void add_game_mode_team_players(void* game_mode, std::vector<PlayerInfo>& players,
        uintptr_t* rendered, int& rendered_count) {
        void* team_dict = get_game_mode_team_models_dictionary(game_mode);
        if (!team_dict)
            return;

        void* teams[16] = {};
        int team_count = 0;
        __try { team_count = collect_ptr_values_from_dictionary(team_dict, teams, 16); }
        __except(EXCEPTION_EXECUTE_HANDLER) { team_count = 0; }
        if (team_count <= 0)
            return;

        g_game_mode_team_count += team_count;

        int local_team = -1;
        for (int ti = 0; ti < team_count; ti++) {
            void* team = teams[ti];
            int team_number = get_game_mode_team_number(team);
            void* team_players[32] = {};
            int team_player_count = collect_game_mode_team_players(team, team_players, 32);
            for (int pi = 0; pi < team_player_count; pi++) {
                if (is_game_mode_player_mine(team_players[pi])) {
                    local_team = team_number;
                    break;
                }
            }
            if (local_team >= 0)
                break;
        }

        for (int ti = 0; ti < team_count; ti++) {
            void* team = teams[ti];
            int team_number = get_game_mode_team_number(team);
            void* team_players[32] = {};
            int team_player_count = collect_game_mode_team_players(team, team_players, 32);
            g_game_mode_team_player_count += team_player_count;

            for (int pi = 0; pi < team_player_count; pi++) {
                void* player_data = team_players[pi];
                if (!player_data)
                    continue;

                int player_ref = get_game_mode_player_ref(player_data);
                bool is_mine = is_game_mode_player_mine(player_data);
                std::string name = get_game_mode_player_name(player_data);

                void* mob = nullptr;
                __try { mob = get_player_avatar_from_game_mode_ref(game_mode, player_ref); }
                __except(EXCEPTION_EXECUTE_HANDLER) { mob = nullptr; }
                if (!mob)
                    continue;

                g_game_mode_team_mob_count++;
                g_game_mode_mob_count++;
                track_mob_pointer(mob);

                uintptr_t mob_ptr = (uintptr_t)mob;
                bool team_known = local_team >= 0 && team_number >= 0;
                bool is_teammate = team_known && team_number == local_team;
                bool is_local = is_mine || mob == g_local_player_avatar;

                if (is_local) {
                    g_local_player_avatar = mob;
                    cache_local_player_from_mob(mob);
                }

                PlayerInfo* existing = find_player_info_for_mob(players, mob_ptr);
                if (existing) {
                    apply_game_mode_team_info(*existing, name, team_known, is_teammate, is_local);
                    continue;
                }

                if (contains_rendered_ptr(rendered, rendered_count, mob_ptr))
                    continue;

                Vector3 pos = {};
                __try { pos = get_player_mob_position(mob); }
                __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
                if (!has_position_value(pos))
                    continue;

                g_game_mode_mob_position_count++;

                PlayerInfo info = make_player_info(
                    name.empty() ? (std::string("GPlayer_") + std::to_string(player_ref)) : name,
                    pos,
                    mob_ptr,
                    mob_ptr,
                    4);
                if (!info.is_valid)
                    continue;
                apply_game_mode_team_info(info, name, team_known, is_teammate, is_local);
                players.push_back(info);
                mark_rendered_ptr(rendered, rendered_count, mob_ptr);
                publish_player_snapshot(players);
            }
        }
    }

    std::vector<PlayerInfo> refresh_cached_players() {
        std::vector<PlayerInfo> refreshed;
        if (!il2cpp::initialized || !g_ready)
            return refreshed;

        uintptr_t rendered[128] = {};
        int rendered_count = 0;

        for (PlayerInfo pi : g_cached_players) {
            void* mob = nullptr;
            if (pi.avatar_ptr)
                mob = (void*)pi.avatar_ptr;
            else if (pi.source_type == 2)
                mob = (void*)pi.object_ptr;
            else if (pi.source_type == 1) {
                __try { mob = get_player_avatar_from_pno((void*)pi.object_ptr); }
                __except(EXCEPTION_EXECUTE_HANDLER) { mob = nullptr; }
            }

            Vector3 pos = {};
            if (mob) {
                __try { pos = get_player_mob_position(mob); }
                __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
            }

            if (!has_position_value(pos) && pi.source_type == 3) {
                __try { pos = get_move_component_position((void*)pi.object_ptr); }
                __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
            }

            if (!has_position_value(pos)) {
                void* obj = (void*)pi.object_ptr;
                __try { pos = get_object_position(obj); }
                __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
            }

            if (has_position_value(pos)) {
                pi.position = pos;
                pi.head_position = pos;
                pi.head_position.y += 1.75f;
            }
            pi.is_valid = true;
            if (mob) {
                __try { pi.is_dead = get_player_mob_is_dead(mob); }
                __except(EXCEPTION_EXECUTE_HANDLER) {}
                __try { get_player_mob_health_info(mob, &pi.health, &pi.max_health); }
                __except(EXCEPTION_EXECUTE_HANDLER) {}
                bool teammate = false;
                bool team_known = false;
                __try { team_known = get_player_mob_team_info(mob, &teammate); }
                __except(EXCEPTION_EXECUTE_HANDLER) {}
                if (team_known) {
                    pi.team_known = true;
                    pi.is_teammate = teammate;
                }
                pi.is_local = pi.is_local || (mob == g_local_player_avatar);
            }
            refreshed.push_back(pi);
            mark_rendered_ptr(rendered, rendered_count, pi.avatar_ptr ? pi.avatar_ptr : pi.object_ptr);
        }

        publish_player_snapshot(refreshed);
        return refreshed;
    }

    struct ClassCache {
        void* klass;
        int32_t nick_offset;
    };

    static ClassCache g_cached_classes[3] = {};

    std::vector<PlayerInfo> get_players() {
        std::vector<PlayerInfo> players;
        g_player_count = 0;
        g_pcc_avatar_count = 0;
        g_pcc_component_count = 0;
        g_pcc_auth_count = 0;
        g_pcc_position_count = 0;
        g_game_mode_count = 0;
        g_game_mode_local_mob_count = 0;
        g_scene_camera_count = 0;
        g_pno_object_count = 0;
        g_player_mob_count = 0;
        g_player_mob_position_count = 0;
        g_move_component_count = 0;
        g_move_component_position_count = 0;
        g_game_mode_mob_count = 0;
        g_game_mode_mob_position_count = 0;
        g_game_mode_team_count = 0;
        g_game_mode_team_player_count = 0;
        g_game_mode_team_mob_count = 0;
        if (!il2cpp::initialized || !g_ready) return players;

        const char* pno_namespace = "Psa.Core.Infrastructure.Spawners";
        const char* pno_classname = "PlayerNetworkObject";
        const char* nick_fields[] = {"cache_Nickname", "_Nickname", "_nickname", "nickname", "_Name", "name"};

        if (!g_cached_classes[0].klass) {
            void* domain = il2cpp::get_domain();
            size_t asm_count = 0;
            void** assemblies = il2cpp::get_assemblies(domain, &asm_count);
            for (size_t i = 0; i < asm_count; i++) {
                if (!assemblies[i]) continue;
                void* img = il2cpp::assembly_get_image(assemblies[i]);
                if (!img) continue;
                if (g_cached_classes[0].klass) continue;
                g_cached_classes[0].klass = il2cpp::class_from_name(img, pno_namespace, pno_classname);
                if (g_cached_classes[0].klass) {
                    g_cached_pno_class = g_cached_classes[0].klass;
                    printf("[unity] Found %s in %s\n", pno_classname, il2cpp::image_get_name(img));
                    for (auto nf : nick_fields) {
                        void* f = il2cpp::class_get_field_from_name(g_cached_classes[0].klass, nf);
                        if (f) { g_cached_classes[0].nick_offset = il2cpp::field_get_offset(f); break; }
                    }
                    g_nickname_off = g_cached_classes[0].nick_offset;
                    printf("[unity] PNO nick_offset=%d (0x%X)\n", g_cached_classes[0].nick_offset, g_cached_classes[0].nick_offset);
                }
            }
        }

        if (!g_cached_classes[0].klass) {
            static bool printed = false;
            if (!printed) { printed = true; printf("[unity] No PNO class found\n"); }
        }

        uintptr_t rendered[128] = {};
        int rendered_count = 0;

        int32_t count = 0;
        void* arr = nullptr;
        if (g_cached_classes[0].klass) {
            __try { arr = find_objects_of_type(g_cached_classes[0].klass, count); }
            __except(EXCEPTION_EXECUTE_HANDLER) { arr = nullptr; count = 0; }
        }

        void** elements = nullptr;
        if (arr && count > 0) {
            __try { elements = managed_object_array_items(arr); }
            __except(EXCEPTION_EXECUTE_HANDLER) { elements = nullptr; }
        }

        g_local_pno_ptr = nullptr;
        g_local_player_avatar = nullptr;
        g_local_player_camera_component = nullptr;
        g_local_player_valid = false;

        for (int32_t i = 0; elements && i < count; i++) {
            void* obj = elements[i];
            if (!obj) continue;

            uintptr_t ptr = (uintptr_t)obj;
            g_pno_object_count++;

            void* avatar = nullptr;
            void* camera_component = nullptr;
            Vector3 camera_pos = {};
            bool camera_pos_ok = false;
            __try {
                avatar = get_player_avatar_from_pno(obj);
                if (avatar) {
                    g_pcc_avatar_count++;
                    track_mob_pointer(avatar);
                }

                camera_component = get_camera_component_from_avatar(avatar);
                if (camera_component)
                    g_pcc_component_count++;
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                avatar = nullptr;
                camera_component = nullptr;
            }

            if (camera_component) {
                __try {
                    camera_pos_ok = get_player_camera_component_position(camera_component, &camera_pos);
                    if (camera_pos_ok)
                        g_pcc_position_count++;
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    camera_pos_ok = false;
                }
            }

            std::string name;
            __try {
                if (g_cached_classes[0].nick_offset > 0) {
                    void* str_obj = *(void**)(ptr + g_cached_classes[0].nick_offset);
                    name = read_managed_string(str_obj);
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {}

            Vector3 pos = {};
            __try { pos = get_object_position(obj); }
            __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }

            if (!has_position_value(pos) && camera_pos_ok)
                pos = camera_pos;

            // Check if this is the local player
            bool has_input_auth = false;
            if (!g_cached_has_input_auth) {
                __try {
                    g_cached_has_input_auth = class_get_method_recursive(g_cached_classes[0].klass, "get_HasInputAuthority", 0);
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
            if (g_cached_has_input_auth) {
                __try {
                    if (invoke_bool_method(obj, g_cached_has_input_auth)) {
                        has_input_auth = true;
                        g_pcc_auth_count++;
                        g_local_player_pos = pos;
                        g_local_player_valid = has_position_value(pos);
                        g_local_pno_ptr = obj;
                        g_local_player_avatar = avatar;
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }

            if (camera_component && (has_input_auth || !g_local_player_camera_component)) {
                g_local_pno_ptr = obj;
                g_local_player_avatar = avatar;
                g_local_player_camera_component = camera_component;
                if (camera_pos_ok) {
                    g_local_player_pos = camera_pos;
                    g_local_player_valid = true;
                }
            }

            if (!has_position_value(pos))
                continue;

            uintptr_t render_key = avatar ? (uintptr_t)avatar : ptr;
            if (contains_rendered_ptr(rendered, rendered_count, render_key))
                continue;

            PlayerInfo pi = make_player_info(
                name.empty() ? (std::string("Player_") + std::to_string(i)) : name,
                pos,
                ptr,
                (uintptr_t)avatar,
                1);
            if (!pi.is_valid)
                continue;
            players.push_back(pi);
            mark_rendered_ptr(rendered, rendered_count, render_key);
            publish_player_snapshot(players);
        }

        void* game_mode_class = get_game_mode_class();
        if (game_mode_class) {
            int32_t game_mode_count = 0;
            void* game_mode_arr = nullptr;
            __try { game_mode_arr = find_objects_of_type(game_mode_class, game_mode_count, true); }
            __except(EXCEPTION_EXECUTE_HANDLER) { game_mode_arr = nullptr; game_mode_count = 0; }

            void** game_mode_elements = nullptr;
            if (game_mode_arr && game_mode_count > 0) {
                __try { game_mode_elements = managed_object_array_items(game_mode_arr); }
                __except(EXCEPTION_EXECUTE_HANDLER) { game_mode_elements = nullptr; }
            }

            for (int32_t gi = 0; game_mode_elements && gi < game_mode_count; gi++) {
                void* game_mode = game_mode_elements[gi];
                if (!game_mode)
                    continue;

                g_game_mode_count++;

                void* local_mob = nullptr;
                __try { local_mob = get_local_player_mob_from_game_mode(game_mode); }
                __except(EXCEPTION_EXECUTE_HANDLER) { local_mob = nullptr; }
                if (local_mob) {
                    g_game_mode_local_mob_count++;
                    cache_local_player_from_mob(local_mob);
                    track_mob_pointer(local_mob);
                }

                __try { add_game_mode_team_players(game_mode, players, rendered, rendered_count); }
                __except(EXCEPTION_EXECUTE_HANDLER) {}

                void* mobs[64] = {};
                int mob_count = 0;
                void* mob_dict = nullptr;
                __try { mob_dict = get_game_mode_mobs_dictionary(game_mode); }
                __except(EXCEPTION_EXECUTE_HANDLER) { mob_dict = nullptr; }
                __try { mob_count = collect_player_mobs_from_dictionary(mob_dict, mobs, 64); }
                __except(EXCEPTION_EXECUTE_HANDLER) { mob_count = 0; }

                for (int mi = 0; mi < mob_count; mi++) {
                    void* mob = mobs[mi];
                    if (!mob)
                        continue;

                    uintptr_t ptr = (uintptr_t)mob;
                    if (contains_rendered_ptr(rendered, rendered_count, ptr))
                        continue;

                    g_game_mode_mob_count++;
                    track_mob_pointer(mob);

                    Vector3 pos = {};
                    __try { pos = get_player_mob_position(mob); }
                    __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
                    if (!has_position_value(pos))
                        continue;

                    g_game_mode_mob_position_count++;

                    bool has_input_auth = (mob == g_local_player_avatar);
                    if (!has_input_auth) {
                        __try { has_input_auth = pno_has_input_authority(mob); }
                        __except(EXCEPTION_EXECUTE_HANDLER) { has_input_auth = false; }
                    }

                    PlayerInfo pi = make_player_info(std::string("GMob_") + std::to_string(mi), pos, ptr, ptr, 4);
                    pi.is_local = has_input_auth || pi.is_local;
                    if (!pi.is_valid)
                        continue;
                    players.push_back(pi);
                    mark_rendered_ptr(rendered, rendered_count, ptr);
                    publish_player_snapshot(players);
                }
            }
        }

        void* mob_class = get_player_mob_class();
        if (mob_class) {
            int32_t mob_count = 0;
            void* mob_arr = nullptr;
            __try { mob_arr = find_objects_of_type(mob_class, mob_count); }
            __except(EXCEPTION_EXECUTE_HANDLER) { mob_arr = nullptr; mob_count = 0; }

            void** mob_elements = nullptr;
            if (mob_arr && mob_count > 0) {
                __try { mob_elements = managed_object_array_items(mob_arr); }
                __except(EXCEPTION_EXECUTE_HANDLER) { mob_elements = nullptr; }
            }

            for (int32_t i = 0; mob_elements && i < mob_count; i++) {
                void* mob = mob_elements[i];
                if (!mob)
                    continue;

                uintptr_t ptr = (uintptr_t)mob;
                if (contains_rendered_ptr(rendered, rendered_count, ptr))
                    continue;

                g_player_mob_count++;
                track_mob_pointer(mob);

                Vector3 pos = {};
                __try { pos = get_player_mob_position(mob); }
                __except(EXCEPTION_EXECUTE_HANDLER) { pos = {}; }
                if (!has_position_value(pos))
                    continue;

                g_player_mob_position_count++;

                bool has_input_auth = false;
                __try { has_input_auth = pno_has_input_authority(mob); }
                __except(EXCEPTION_EXECUTE_HANDLER) { has_input_auth = false; }
                if (has_input_auth) {
                    g_local_player_avatar = mob;
                    g_local_player_pos = pos;
                    g_local_player_valid = true;
                    if (!g_local_player_camera_component) {
                        __try { g_local_player_camera_component = get_camera_component_from_avatar(mob); }
                        __except(EXCEPTION_EXECUTE_HANDLER) {}
                    }
                }

                PlayerInfo pi = make_player_info(std::string("Mob_") + std::to_string(i), pos, ptr, ptr, 2);
                pi.is_local = has_input_auth || pi.is_local;
                if (!pi.is_valid)
                    continue;
                players.push_back(pi);
                mark_rendered_ptr(rendered, rendered_count, ptr);
                publish_player_snapshot(players);
            }
        }

        // Direct MoveComponent scene scans are intentionally disabled here:
        // during map joins Unity is constructing/despawning these components and
        // FindObjectsOfType(MoveComponent) can stall the render thread. We still
        // use MoveComponent offsets through PlayerMob._moveComponent above.
        g_move_component_count = 0;
        g_move_component_position_count = 0;

        __try { refresh_scene_camera_cache(); }
        __except(EXCEPTION_EXECUTE_HANDLER) {}

        publish_player_snapshot(players);
        return players;
    }

    Vector3 get_bone_position(void* transform) {
        if (!transform) return Vector3();
        return get_transform_position(transform);
    }

    float get_field_float(void* obj, int32_t offset) {
        if (!obj) return 0;
        return *(float*)((uintptr_t)obj + offset);
    }

    void set_field_float(void* obj, int32_t offset, float value) {
        if (!obj) return;
        *(float*)((uintptr_t)obj + offset) = value;
    }

    void set_field_bool(void* obj, int32_t offset, bool value) {
        if (!obj) return;
        *(bool*)((uintptr_t)obj + offset) = value;
    }
}
