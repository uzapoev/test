#include "resource_compiler.h"
#include <iomanip>
#include "gfx/gfx.h"
#include "gfx/gfx_shader_compiler.h"

#include "json_serializer.h"
#include "binary_serializer.h"
#include "material_data.h"
#include "scene/scene.h"
#include "threadpool.h"

// ============================================================================
// ---                  Mesh                                                ---
// ============================================================================
bool resource_compile_mesh(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    (void)src; (void)dst; (void)meta; (void)type; (void)userdata;
    path::copy_file(src, dst);
    return true;
}

// ============================================================================
// ---                  Shader                                              ---
// ============================================================================
// 
//asset_compile_func_t
bool resource_compile_shader(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    (void)src; (void)dst; (void)meta; (void)type; (void)userdata;
    static gfx_shader_compiler_context_t* compiler_context = nullptr;
    if (compiler_context == nullptr)
        gfx_shader_compiler_context_create(0, &compiler_context);

    char* data = nullptr;
    uint32_t size = read_file_data(src, true, &data);

    gfx_shader_compiler_request_desc_t desc = {};
    desc.name = strrchr(src, '/');
    desc.size = size;
    desc.data = data;
    desc.target = shader_target_spirv;
    desc.options = shader_compile_option_invert_y;

    compiled_shader_program_t program = {};
    gfx_compile_shader(compiler_context, &desc, &program);
    return false;
}

// ============================================================================
// ---                  Texture                                             ---
// {
//   "guid": "7f8a9b...",
//   "maxSize": 2048,
//   "sRGB": true,
//   "mipStreaming": true,
//   "platforms": {
//     "default": { "compress":"true", "maxSize": "2048" },
//     "desktop": { "format": "BC7", "maxSize": 2048 },
//     "mobile":  { "format": "ASTC_6x6", "maxSize": 1048 },
//     "web":     { "format": "UASTC", "maxSize": 1048 }
//   }
// }
// ============================================================================

struct texture_meta_t
{
    guid_t      guid;
    uint32_t    maxTextureSize;
    bool        mipMapStreaming;
    bool        srgb;
    bool        compress;
    struct per_platform_settings {
        uint32_t    format;
        uint32_t    max_size;
    };
    //ReflectObject(texture_meta_t, ReflectObjectField(guid), ReflectObjectField(max_size), ReflectObjectField(srgb), ReflectObjectField(compress));
   // ReflectObject2(texture_meta_t, guid, maxTextureSize, srgb, compress);
};

ReflectObjectExternal2(texture_meta_t, guid, maxTextureSize, srgb, compress);

bool resource_compile_texture(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    (void)src; (void)dst; (void)meta; (void)type; (void)userdata;
    std::string python_exe = "python"; // Change to "python3" depending on your OS configuration
    std::string script_path = "../tools/texture_converter.py";
    std::string src_file = std::string("\"").append(src).append("\"");
    std::string dst_file = dst;
    std::string size = "1024";

    auto meta_data = resource_manager::file_data(meta);
    auto jmat = json::from_json_string<texture_meta_t>(meta_data.size(), (char*)meta_data.data());

    // std::string method = "astc4";
    std::string method = "bc7";
    switch (type)
    {
        case platform_type_common:
        case platform_type_web:
        case platform_type_pc:      method = "bc7";     break;
        case platform_type_mobile:  method = "astc4";   break;
    }

    std::string command = python_exe + " " + script_path + " " +
        src_file + " " + dst_file + " " +
        size + " " + method;

    debug::log_warning("[C++] Executing script: %s\n", command.c_str());

#if defined(__APPLE__) && TARGET_OS_IPHONE
    // iOS cannot spawn processes: textures must be compiled on the host and shipped with the app.
    debug::log_error("[resource_compiler] texture compilation is not available on iOS: %s\n", src_file.c_str());
    return false;
#else
    task_handle_t handle = 0;
    
    while(!thread_pool::shared()->try_allocate([command](){ std::system(command.c_str()); }, &handle))
    {
        thread_pool::shared()->yield();
    }
    thread_pool::shared()->submit(handle);

    return false;
#endif
}

// ============================================================================
// ---                  Material                                            ---
// ============================================================================

// json (source) -> binary blob (see binary_serializer.h / material_data.h). The loader reads it back with
// binary::read_blob<material_json>.
bool resource_compile_material(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    (void)src; (void)dst; (void)meta; (void)type; (void)userdata;
    auto data = resource_manager::file_data(src);
    if (data.empty()) {
        debug::log_error("[material] cannot read '%s'", src);
        return false;
    }

    // sajson works in place and asserts on a broken document: check it on a copy first
    {
        std::vector<char> copy(data);
        sajson::document doc = sajson::parse(sajson::dynamic_allocation(), sajson::mutable_string_view(copy.size(), copy.data()));
        if (!doc.is_valid() || doc.get_root().get_type() != sajson::TYPE_OBJECT) {
            debug::log_error("[material] '%s' is not a valid json object", src);
            return false;
        }
    }

    material_json jmat = json::from_json_string<material_json>(data.size(), data.data());

    binary_writer blob;
    binary::write_blob(blob, material_blob_magic, material_blob_version, jmat);

    filestream* fs = filestream::open_wb(dst);
    if (fs == nullptr) {
        debug::log_error("[material] cannot write '%s'", dst);
        return false;
    }
    const bool ok = fs->write(blob.size(), blob.data()) == blob.size();
    fs->flush();
    delete fs;
    if (!ok) {
        debug::log_error("[material] short write to '%s'", dst);
        return false;
    }
    return true;
}


// ============================================================================
// ---                  Scene                                               ---
// ============================================================================
bool resource_compile_scene(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    (void)src; (void)dst; (void)meta; (void)type; (void)userdata;
    if (std::filesystem::exists(dst))
        return false;

    auto bin_path = std::string(src).append(".bin");
    auto scene = scene::create_from_json_file(src);
    scene.save(bin_path.c_str());

    path::copy_file(bin_path.c_str(), dst);
    return false;
}
