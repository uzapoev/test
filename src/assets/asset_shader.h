#ifndef __asset_shader_h__
#define __asset_shader_h__

#include <string>
#include <vector>

class AssetShader
{
public:
    enum ShaderStage : uint8_t
    {
        Vertex,
        Hull,
        Domain,
        Geometry,
        Fragment,

        Compute,

        Count,
        /*
        RayGeneration,  // = 0x0100,
        AnyHit,         // = 0x0200,
        ClosestHit,     // = 0x0400,
        Miss,           // = 0x0800,
        Intersection,   // = 0x1000,
        Callable,       // = 0x2000,
        AllRayTracing,  // = 0x3F00,*/
    };

    enum CompileOptions
    {
        spirv,
        fvk_invert_y
    };

    struct ShaderEntry
    {
        ShaderStage stage;
        std::string entry;
        std::string blob;
        std::string error;
    };

public:
    AssetShader();

    static const char* find_pragma_entry(const char* data, const char* stage_name, char* entry_buffer = nullptr, size_t buffer_size = 0);
    static void export_shader(const std::string& path, const std::string& dst_path);

public:
   // static bool compile_shader(const void *data, size_t size, ShaderStage stage, char * entry, std::string *blob, std::string * error);
    static bool compile_shader(const std::string &path, ShaderStage stage, const std::string & entry, std::string *blob, std::string * error);
    static bool compile_shader_form_data(const char *data, size_t size, const std::wstring & include_path, ShaderStage stage, const std::string &entry, std::string *blob, std::string * error);
 //   static bool compile_shader2(const std::string &path, std::vector<ShaderEntry> &shaders);
};

#endif
