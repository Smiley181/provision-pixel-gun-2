#define _CRT_SECURE_NO_WARNINGS
#include "../include/il2cpp_api.h"
#include <windows.h>
#include <cstdio>

namespace il2cpp {
    bool initialized = false;
    uintptr_t module_base = 0;
    size_t module_size = 0;

    get_domain_t get_domain = nullptr;
    get_assemblies_t get_assemblies = nullptr;
    assembly_get_image_t assembly_get_image = nullptr;
    image_get_name_t image_get_name = nullptr;
    image_get_class_count_t image_get_class_count = nullptr;
    image_get_class_t image_get_class = nullptr;
    class_get_name_t class_get_name = nullptr;
    class_get_namespace_t class_get_namespace = nullptr;
    class_get_parent_t class_get_parent = nullptr;
    class_get_type_t class_get_type = nullptr;
    class_from_name_t class_from_name = nullptr;
    class_instance_size_t class_instance_size = nullptr;
    class_get_method_from_name_t class_get_method_from_name = nullptr;
    runtime_invoke_t runtime_invoke = nullptr;
    object_get_class_t object_get_class = nullptr;
    thread_attach_t thread_attach = nullptr;
    thread_detach_t thread_detach = nullptr;
    array_length_t array_length = nullptr;
    string_length_t string_length_fn = nullptr;
    string_chars_t string_chars = nullptr;
    object_unbox_t object_unbox = nullptr;
    resolve_icall_t resolve_icall = nullptr;
    class_get_field_from_name_t class_get_field_from_name = nullptr;
    field_get_offset_t field_get_offset = nullptr;
    field_get_flags_t field_get_flags = nullptr;
    class_get_fields_t class_get_fields = nullptr;
    class_get_methods_t class_get_methods = nullptr;
    method_get_name_t method_get_name = nullptr;
    method_get_param_t method_get_param = nullptr;
    type_get_object_t type_get_object = nullptr;
    field_get_name_t field_get_name = nullptr;

    template<typename T>
    T get_export(const char* name) {
        return (T)GetProcAddress((HMODULE)module_base, name);
    }

    bool init() {
        if (initialized) return true;

        HMODULE gameAsm = GetModuleHandleA("GameAssembly.dll");
        if (!gameAsm) {
            printf("[il2cpp] GameAssembly.dll not loaded yet\n");
            return false;
        }
        module_base = (uintptr_t)gameAsm;

        auto dos = (IMAGE_DOS_HEADER*)gameAsm;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
            auto nt = (IMAGE_NT_HEADERS*)(module_base + dos->e_lfanew);
            if (nt->Signature == IMAGE_NT_SIGNATURE)
                module_size = nt->OptionalHeader.SizeOfImage;
        }

        get_domain = get_export<get_domain_t>("il2cpp_domain_get");
        get_assemblies = get_export<get_assemblies_t>("il2cpp_domain_get_assemblies");
        assembly_get_image = get_export<assembly_get_image_t>("il2cpp_assembly_get_image");
        image_get_name = get_export<image_get_name_t>("il2cpp_image_get_name");
        image_get_class_count = get_export<image_get_class_count_t>("il2cpp_image_get_class_count");
        image_get_class = get_export<image_get_class_t>("il2cpp_image_get_class");
        class_from_name = get_export<class_from_name_t>("il2cpp_class_from_name");
        class_get_name = get_export<class_get_name_t>("il2cpp_class_get_name");
        class_get_namespace = get_export<class_get_namespace_t>("il2cpp_class_get_namespace");
        class_get_parent = get_export<class_get_parent_t>("il2cpp_class_get_parent");
        class_get_type = get_export<class_get_type_t>("il2cpp_class_get_type");
        class_get_method_from_name = get_export<class_get_method_from_name_t>("il2cpp_class_get_method_from_name");
        runtime_invoke = get_export<runtime_invoke_t>("il2cpp_runtime_invoke");
        object_get_class = get_export<object_get_class_t>("il2cpp_object_get_class");
        thread_attach = get_export<thread_attach_t>("il2cpp_thread_attach");
        thread_detach = get_export<thread_detach_t>("il2cpp_thread_detach");
        array_length = get_export<array_length_t>("il2cpp_array_length");
        string_length_fn = get_export<string_length_t>("il2cpp_string_length");
        string_chars = get_export<string_chars_t>("il2cpp_string_chars");
        object_unbox = get_export<object_unbox_t>("il2cpp_object_unbox");
        resolve_icall = get_export<resolve_icall_t>("il2cpp_resolve_icall");
        class_get_field_from_name = get_export<class_get_field_from_name_t>("il2cpp_class_get_field_from_name");
        field_get_offset = get_export<field_get_offset_t>("il2cpp_field_get_offset");
        field_get_flags = get_export<field_get_flags_t>("il2cpp_field_get_flags");
        class_get_fields = get_export<class_get_fields_t>("il2cpp_class_get_fields");
        class_get_methods = get_export<class_get_methods_t>("il2cpp_class_get_methods");
        method_get_name = get_export<method_get_name_t>("il2cpp_method_get_name");
        method_get_param = get_export<method_get_param_t>("il2cpp_method_get_param");
        type_get_object = get_export<type_get_object_t>("il2cpp_type_get_object");
        field_get_name = get_export<field_get_name_t>("il2cpp_field_get_name");

        if (!get_domain || !get_assemblies || !class_from_name) {
            printf("[il2cpp] CRITICAL: missing core exports\n");
            module_base = 0;
            return false;
        }

        initialized = true;
        printf("[il2cpp] IL2CPP API initialized (%zu core exports resolved)\n", module_size);
        return true;
    }

    void* get_image(const char* name) {
        if (!initialized) return nullptr;
        void* domain = get_domain();
        if (!domain) return nullptr;

        size_t count = 0;
        void** assemblies = get_assemblies(domain, &count);
        if (!assemblies) return nullptr;

        for (size_t i = 0; i < count; i++) {
            void* img = assembly_get_image(assemblies[i]);
            if (img) {
                const char* img_name = image_get_name(img);
                if (img_name && strcmp(img_name, name) == 0)
                    return img;
            }
        }
        return nullptr;
    }

    void* find_class(const char* image_name, const char* namesp, const char* class_name) {
        if (!initialized) return nullptr;
        void* img = get_image(image_name);
        if (!img) {
            printf("[il2cpp] image '%s' not found\n", image_name);
            return nullptr;
        }
        void* klass = class_from_name(img, namesp, class_name);
        if (!klass)
            printf("[il2cpp] class %s.%s not found in %s\n", namesp, class_name, image_name);
        return klass;
    }
}
