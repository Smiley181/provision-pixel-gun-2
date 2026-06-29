#define _CRT_SECURE_NO_WARNINGS
#include "../include/config.h"
#include "../include/features.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#define CSIDL_PERSONAL 0x0005

namespace config {
    static std::string config_path;

    static std::string get_documents_path() {
        char buf[MAX_PATH];
        HMODULE shell32 = LoadLibraryA("shell32.dll");
        if (shell32) {
            using SHGetFolderPathA_t = HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR);
            auto fn = (SHGetFolderPathA_t)GetProcAddress(shell32, "SHGetFolderPathA");
            if (fn) {
                if (SUCCEEDED(fn(nullptr, CSIDL_PERSONAL, nullptr, 0, buf))) {
                    FreeLibrary(shell32);
                    return std::string(buf);
                }
            }
            FreeLibrary(shell32);
        }
        GetEnvironmentVariableA("USERPROFILE", buf, sizeof(buf));
        return std::string(buf) + "\\Documents";
    }

    const char* get_config_path() {
        if (config_path.empty()) {
            config_path = get_documents_path();
            config_path += "\\provision_pixelgun_2";
            CreateDirectoryA(config_path.c_str(), nullptr);
            config_path += "\\config.json";
        }
        return config_path.c_str();
    }

    // --- JSON writer helpers ---

    static void write_indent(std::string& out, int depth) {
        for (int i = 0; i < depth; i++) out += "  ";
    }

    static void write_json_string(std::string& out, const char* s) {
        out += '"';
        while (*s) {
            char c = *s++;
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;      break;
            }
        }
        out += '"';
    }

    static void write_field(std::string& out, int depth, const char* name, bool val) {
        write_indent(out, depth);
        write_json_string(out, name);
        out += ": ";
        out += val ? "true" : "false";
    }

    static void write_field(std::string& out, int depth, const char* name, float val) {
        write_indent(out, depth);
        write_json_string(out, name);
        out += ": ";
        char buf[64];
        snprintf(buf, sizeof(buf), "%.7g", (double)val);
        out += buf;
    }

    static void write_field(std::string& out, int depth, const char* name, int val) {
        write_indent(out, depth);
        write_json_string(out, name);
        out += ": ";
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", val);
        out += buf;
    }

    static void write_array_field(std::string& out, int depth, const char* name, const float* arr, int count) {
        write_indent(out, depth);
        write_json_string(out, name);
        out += ": [";
        for (int i = 0; i < count; i++) {
            if (i > 0) out += ", ";
            char buf[32];
            snprintf(buf, sizeof(buf), "%.7g", (double)arr[i]);
            out += buf;
        }
        out += ']';
    }

    // --- JSON parser ---

    struct JsonParser {
        const char* p;
        const char* end;

        JsonParser(const char* s, size_t len) : p(s), end(s + len) {}

        void skip_ws() {
            while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
                p++;
        }

        bool match(char c) {
            skip_ws();
            if (p < end && *p == c) { p++; return true; }
            return false;
        }

        bool parse_string(std::string& out) {
            skip_ws();
            if (p >= end || *p != '"') return false;
            p++;
            out.clear();
            while (p < end && *p != '"') {
                if (*p == '\\') {
                    p++;
                    if (p >= end) return false;
                    switch (*p) {
                        case '"': out += '"'; break;
                        case '\\': out += '\\'; break;
                        case 'n': out += '\n'; break;
                        case 'r': out += '\r'; break;
                        case 't': out += '\t'; break;
                        default: out += *p; break;
                    }
                } else {
                    out += *p;
                }
                p++;
            }
            if (p >= end) return false;
            p++;
            return true;
        }

        bool parse_number(double& out) {
            skip_ws();
            const char* start = p;
            if (p < end && *p == '-') p++;
            while (p < end && *p >= '0' && *p <= '9') p++;
            if (p < end && *p == '.') {
                p++;
                while (p < end && *p >= '0' && *p <= '9') p++;
            }
            if (p < end && (*p == 'e' || *p == 'E')) {
                p++;
                if (p < end && (*p == '+' || *p == '-')) p++;
                while (p < end && *p >= '0' && *p <= '9') p++;
            }
            if (p == start) return false;
            std::string num_str(start, p - start);
            out = std::stod(num_str);
            return true;
        }

        bool parse_bool(bool& out) {
            skip_ws();
            if (end - p >= 4 && memcmp(p, "true", 4) == 0) { out = true; p += 4; return true; }
            if (end - p >= 5 && memcmp(p, "false", 5) == 0) { out = false; p += 5; return true; }
            return false;
        }

        bool parse_float_array(float* arr, int count) {
            if (!match('[')) return false;
            for (int i = 0; i < count; i++) {
                if (i > 0) { if (!match(',')) return false; }
                double v;
                if (!parse_number(v)) return false;
                arr[i] = (float)v;
            }
            if (!match(']')) return false;
            return true;
        }

        void skip_value() {
            skip_ws();
            if (p >= end) return;
            char c = *p;
            if (c == '{' || c == '[') {
                int depth = 1;
                char close = (c == '{') ? '}' : ']';
                p++;
                while (p < end && depth > 0) {
                    if (*p == c) depth++;
                    else if (*p == close) depth--;
                    p++;
                }
            } else if (c == '"') {
                p++;
                while (p < end && *p != '"') {
                    if (*p == '\\') p++;
                    p++;
                }
                if (p < end) p++;
            } else {
                while (p < end && *p != ',' && *p != '}' && *p != ']' &&
                       *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
                    p++;
            }
        }
    };

    // --- Type-based field readers ---

    static bool read_field(JsonParser& parser, bool& val) {
        return parser.parse_bool(val);
    }

    static bool read_field(JsonParser& parser, float& val) {
        double d;
        if (!parser.parse_number(d)) return false;
        val = (float)d;
        return true;
    }

    static bool read_field(JsonParser& parser, int& val) {
        double d;
        if (!parser.parse_number(d)) return false;
        val = (int)d;
        return true;
    }

    // --- Save: iterate x-macros to auto-generate JSON ---

    bool save() {
        std::string json;
        json += "{\n";

        // --- ESP ---
        json += "  \"esp\": {\n";
        {
            int n = 0;
#define M(type, name, default) \
            if (n++ > 0) json += ",\n"; \
            write_field(json, 2, #name, esp_config.name);
            ESP_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) \
            if (n++ > 0) json += ",\n"; \
            write_array_field(json, 2, #name, esp_config.name, count);
            ESP_ARRAY_FIELDS(M)
#undef M
        }
        json += "\n  },\n";

        // --- Aimbot ---
        json += "  \"aimbot\": {\n";
        {
            int n = 0;
#define M(type, name, default) \
            if (n++ > 0) json += ",\n"; \
            write_field(json, 2, #name, aimbot_config.name);
            AIMBOT_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) \
            if (n++ > 0) json += ",\n"; \
            write_array_field(json, 2, #name, aimbot_config.name, count);
            AIMBOT_ARRAY_FIELDS(M)
#undef M
        }
        json += "\n  },\n";

        // --- Menu ---
        json += "  \"menu\": {\n";
        {
            int n = 0;
#define M(type, name, default) \
            if (n++ > 0) json += ",\n"; \
            write_field(json, 2, #name, menu_config.name);
            MENU_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) \
            if (n++ > 0) json += ",\n"; \
            write_array_field(json, 2, #name, menu_config.name, count);
            MENU_ARRAY_FIELDS(M)
#undef M
        }
        json += "\n  }\n";

        json += "}\n";

        const char* path = get_config_path();
        FILE* f = fopen(path, "w");
        if (!f) return false;
        fwrite(json.c_str(), 1, json.size(), f);
        fclose(f);
        return true;
    }

    // --- Load: iterate x-macros to auto-match keys ---

    bool load() {
        const char* path = get_config_path();
        FILE* f = fopen(path, "r");
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (len <= 0) { fclose(f); return false; }

        std::vector<char> data(len + 1);
        fread(data.data(), 1, len, f);
        data[len] = 0;
        fclose(f);

        JsonParser parser(data.data(), len);
        if (!parser.match('{')) return false;

        std::string key;
        while (!parser.match('}')) {
            if (!parser.parse_string(key)) break;
            if (!parser.match(':')) break;

            if (key == "esp") {
                if (!parser.match('{')) { parser.skip_value(); continue; }
                while (!parser.match('}')) {
                    std::string k;
                    if (!parser.parse_string(k)) break;
                    if (!parser.match(':')) break;

                    bool found = false;
#define M(type, name, default) \
                    if (!found && k == #name) { found = read_field(parser, esp_config.name); }
                    ESP_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) \
                    if (!found && k == #name) { found = parser.parse_float_array(esp_config.name, count); }
                    ESP_ARRAY_FIELDS(M)
#undef M
                    if (!found) parser.skip_value();
                    parser.match(',');
                }
            } else if (key == "aimbot") {
                if (!parser.match('{')) { parser.skip_value(); continue; }
                while (!parser.match('}')) {
                    std::string k;
                    if (!parser.parse_string(k)) break;
                    if (!parser.match(':')) break;

                    bool found = false;
#define M(type, name, default) \
                    if (!found && k == #name) { found = read_field(parser, aimbot_config.name); }
                    AIMBOT_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) \
                    if (!found && k == #name) { found = parser.parse_float_array(aimbot_config.name, count); }
                    AIMBOT_ARRAY_FIELDS(M)
#undef M
                    if (!found) parser.skip_value();
                    parser.match(',');
                }
            } else if (key == "menu") {
                if (!parser.match('{')) { parser.skip_value(); continue; }
                while (!parser.match('}')) {
                    std::string k;
                    if (!parser.parse_string(k)) break;
                    if (!parser.match(':')) break;

                    bool found = false;
#define M(type, name, default) \
                    if (!found && k == #name) { found = read_field(parser, menu_config.name); }
                    MENU_SCALAR_FIELDS(M)
#undef M
#define M(type, name, count, ...) \
                    if (!found && k == #name) { found = parser.parse_float_array(menu_config.name, count); }
                    MENU_ARRAY_FIELDS(M)
#undef M
                    if (!found) parser.skip_value();
                    parser.match(',');
                }
            } else {
                parser.skip_value();
            }
            parser.match(',');
        }
        return true;
    }
}
