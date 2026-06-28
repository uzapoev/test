#include "resource_compiler.h"

#include "gfx/gfx.h"
#include "gfx/gfx_shader_compiler.h"

#include "json_serializer.h"
#include "scene/scene.h"

/*
{
  "guid": "7f8a9b...",
  "maxSize": 2048,
  "sRGB": true,
  "platforms": {
    "default": { "compress":"true", "maxSize": "2048" },
    "desktop": { "format": "BC7", "maxSize": 2048 },
    "mobile":  { "format": "ASTC_6x6", "maxSize": 1048 },
    "web":     { "format": "UASTC", "maxSize": 1048 }
  }
}*/
struct texture_meta_t
{
    guid_t      guid;
    uint32_t    max_size;
    bool        srgb;
    bool        compress;
    struct per_platform_settings {
        uint32_t    format;
        uint32_t    max_size;
    };
};

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
// ============================================================================
bool resource_compile_texture(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    std::string python_exe = "python"; // Change to "python3" depending on your OS configuration
    std::string script_path = "../tools/texture_converter.py";
    std::string src_file = src;
    std::string dst_file = dst;
    std::string size = "1024";

    // std::string method = "astc4";
    std::string method = "bc7";

    std::string command = python_exe + " " + script_path + " " +
        src_file + " " + dst_file + " " +
        size + " " + method;

    debug::log_warning("[C++] Executing script: %s", command.c_str());

    std::system(command.c_str());

    return false;
}

// ============================================================================
// ---                  Material                                            ---
// ============================================================================
struct material_json
{
    interned_string                             shader;
    std::vector< shader_slot<interned_string>>  textures;   // key - name of shader uniform, key - texture guid
    std::vector< shader_slot<vec4>  >           vectors;    // key - name of shader uniform, key - data
    std::vector< shader_slot<float> >           scalars;    // key - name of shader uniform, key - data

    ReflectObject(material_json, ReflectObjectField(shader),
        ReflectObjectField(textures),
        ReflectObjectField(vectors),
        ReflectObjectField(scalars));
};

bool resource_compile_material(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    auto data = resource_manager::file_data(src);
    auto jmat = json::from_json_string<material_json>(data.size(), (char*)data.data());

    return false;
}


// ============================================================================
// ---                  Scene                                               ---
// ============================================================================
bool resource_compile_scene(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
{
    if (std::filesystem::exists(dst))
        return false;

    std::string bin_path = src;
    bin_path.append(".bin");

    auto scene = scene::create_from_json_file(src);
    scene.save(bin_path.c_str());

    path::copy_file(bin_path.c_str(), dst);
    return false;
}