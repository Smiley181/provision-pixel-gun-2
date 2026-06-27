#pragma once
#include <cstdint>
#include <cstddef>

namespace il2cpp {
    extern bool initialized;
    extern uintptr_t module_base;
    extern size_t module_size;

    typedef void* (*get_domain_t)();
    typedef void** (*get_assemblies_t)(void* domain, size_t* count);
    typedef void* (*assembly_get_image_t)(void* assembly);
    typedef const char* (*image_get_name_t)(void* image);
    typedef size_t (*image_get_class_count_t)(void* image);
    typedef void* (*image_get_class_t)(void* image, size_t index);
    typedef const char* (*class_get_name_t)(void* klass);
    typedef const char* (*class_get_namespace_t)(void* klass);
    typedef void* (*class_get_parent_t)(void* klass);
    typedef void* (*class_get_type_t)(void* klass);
    typedef void* (*class_from_name_t)(void* image, const char* ns, const char* name);
    typedef int32_t (*class_instance_size_t)(void* klass);
    typedef void* (*class_get_method_from_name_t)(void* klass, const char* name, int argc);
    typedef void* (*runtime_invoke_t)(void* method, void* obj, void** params, void** exception);
    typedef void* (*object_get_class_t)(void* obj);
    typedef void* (*thread_attach_t)(void* domain);
    typedef void (*thread_detach_t)(void* thread);
    typedef int32_t (*array_length_t)(void* array);
    typedef int32_t (*string_length_t)(void* str);
    typedef wchar_t* (*string_chars_t)(void* str);
    typedef void* (*object_unbox_t)(void* obj);
    typedef void* (*resolve_icall_t)(const char* name);
    typedef void* (*class_get_field_from_name_t)(void* klass, const char* name);
    typedef int32_t (*field_get_offset_t)(void* field);
    typedef uint32_t (*field_get_flags_t)(void* field);
    typedef void* (*class_get_fields_t)(void* klass, void** iter);
    typedef void* (*class_get_methods_t)(void* klass, void** iter);
    typedef const char* (*method_get_name_t)(void* method);
    typedef void* (*method_get_param_t)(void* method, uint32_t index);
    typedef void* (*type_get_object_t)(void* type);
    typedef const char* (*field_get_name_t)(void* field);
    typedef void* (*string_new_t)(const char* str);

    extern get_domain_t get_domain;
    extern get_assemblies_t get_assemblies;
    extern assembly_get_image_t assembly_get_image;
    extern image_get_name_t image_get_name;
    extern image_get_class_count_t image_get_class_count;
    extern image_get_class_t image_get_class;
    extern class_get_name_t class_get_name;
    extern class_get_namespace_t class_get_namespace;
    extern class_get_parent_t class_get_parent;
    extern class_get_type_t class_get_type;
    extern class_from_name_t class_from_name;
    extern class_instance_size_t class_instance_size;
    extern class_get_method_from_name_t class_get_method_from_name;
    extern runtime_invoke_t runtime_invoke;
    extern object_get_class_t object_get_class;
    extern thread_attach_t thread_attach;
    extern thread_detach_t thread_detach;
    extern array_length_t array_length;
    extern string_length_t string_length_fn;
    extern string_chars_t string_chars;
    extern object_unbox_t object_unbox;
    extern resolve_icall_t resolve_icall;
    extern class_get_field_from_name_t class_get_field_from_name;
    extern field_get_offset_t field_get_offset;
    extern field_get_flags_t field_get_flags;
    extern class_get_fields_t class_get_fields;
    extern class_get_methods_t class_get_methods;
    extern method_get_name_t method_get_name;
    extern method_get_param_t method_get_param;
    extern type_get_object_t type_get_object;
    extern field_get_name_t field_get_name;

    bool init();
    void* get_image(const char* name);
    void* find_class(const char* image_name, const char* namesp, const char* class_name);
}
