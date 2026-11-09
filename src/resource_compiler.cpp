#include "resource_compiler.h"
#include <iomanip>
#include "gfx/gfx.h"
#include "gfx/gfx_shader_compiler.h"

#include "json_serializer.h"
#include "scene/scene.h"


// ============================================================================
// ---                  Mesh                                                ---
// ============================================================================
bool resource_compile_mesh(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
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
    static gfx_shader_compiler_context_t* compiler_context = nullptr;
    if (compiler_context == nullptr)
        gfx_shader_compiler_context_create(0, &compiler_context);

    char* data = nullptr;
    uint32_t size = read_file_data(src, &data);

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
};

ReflectObjectExternal2(texture_meta_t, guid, maxTextureSize, srgb, compress);

bool resource_compile_texture(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    std::string python_exe = "python"; // Change to "python3" depending on your OS configuration
    std::string script_path = "../tools/texture_converter.py";
    std::string src_file = std::string("\"").append(src).append("\"");
    std::string dst_file = dst;
    std::string size = "1024";

    auto meta_data = resource_manager::file_data(meta);
    auto jmat = json::from_json_string<texture_meta_t>(meta_data.size(), (char*)meta_data.data());

   // auto f = std::quoted(src_file);

    // std::string method = "astc4";
    std::string method = "bc7";

    std::string command = python_exe + " " + script_path + " " +
        src_file + " " + dst_file + " " +
        size + " " + method;

    debug::log_warning("[C++] Executing script: %s\n", command.c_str());

   
    std::system(command.c_str());

    return false;
}

// ============================================================================
// ---                  Material                                            ---
// ============================================================================

ReflectObjectExternal2(vec4, x, y, z, w);

struct material_meta_t {
    interned_string     guid;

    ReflectObject2(material_meta_t, guid);
};

template<class T>
struct shader_slot {
    interned_string     key;        // uniform name
    T                   value = {};

    ReflectObject2(shader_slot, key, value);
};

struct shader_texture_slot {
    interned_string     guid;
    interned_string     name;
    vec4                scaleOffset;

    ReflectObject2(shader_texture_slot, guid, name, scaleOffset);
};

struct material_json {
    interned_string                             shader;     // name or guid
    std::vector< shader_texture_slot>           textures;   // key - name of shader uniform, key - texture guid
    std::vector< shader_slot<vec4>  >           vectors;    // key - name of shader uniform, key - data
    std::vector< shader_slot<float> >           scalars;    // key - name of shader uniform, key - data

    ReflectObject2(material_json, shader, textures, vectors, scalars);
};

bool resource_compile_material(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    auto data = resource_manager::file_data(src);
    auto meta_data = resource_manager::file_data(meta);

    auto jmat = json::from_json_string<material_json>(data.size(), (char*)data.data());
    auto jmeta = json::from_json_string<material_meta_t>(meta_data.size(), (char*)meta_data.data());

    filestream * fs = filestream::open_wb(dst);
    fs->write(jmat.shader);

    path::copy_file(src, dst);
    return false;
}


// ============================================================================
// ---                  Scene                                               ---
// ============================================================================
bool resource_compile_scene(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    if (std::filesystem::exists(dst))
        return false;

    auto bin_path = std::string(src).append(".bin");
    auto scene = scene::create_from_json_file(src);
    scene.save(bin_path.c_str());

    path::copy_file(bin_path.c_str(), dst);
    return false;
}