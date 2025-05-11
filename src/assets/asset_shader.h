#ifndef __asset_shader_h__
#define __asset_shader_h__

#include <string>
#include <vector>

static constexpr char* shader_target_spirv          = "spirv";
static constexpr char* shader_target_metal          = "metal";
static constexpr char* shader_target_wsl            = "wgsl";
static constexpr char* shader_target_glsl           = "glsl";
static constexpr char* shader_target_dxil           = "dxil";

//generic
static constexpr char* pragma_vertex_name           = "vertex";
static constexpr char* pragma_fragment_name         = "fragment";
static constexpr char* pragma_compute_name          = "compute";

// exotique                                                   
static constexpr char* pragma_hull_name             = "hull";
static constexpr char* pragma_domain_name           = "domain";
static constexpr char* pragma_geometry_name         = "geometry";

// ray tracing 
static constexpr char* pragma_ray_gen_name          = "ray_gen";
static constexpr char* pragma_ray_intersect_name    = "ray_intersect";
static constexpr char* pragma_ray_anyhit_name       = "ray_anyhit";
static constexpr char* pragma_ray_closesthit_name   = "ray_closesthit";
static constexpr char* pragma_ray_miss_name         = "ray_miss";
static constexpr char* pragma_ray_callable_name     = "ray_callable";

// mesh shaders
static constexpr char* pragma_mesh_name             = "mesh";
static constexpr char* pragma_amplification_name    = "amplification";


typedef struct stage_blob_t {
    const char* stage;
    int         size;
    char*       blob;
} stage_blob_t;

extern int  asset_shader_compile(const char* name, const char* data, uint32_t size, const char* target, char** out_blobs, int* out_sizes, const char** out_stages);
extern int  asset_shader_compile(const char* name, const char* data, uint32_t size, const char* target, stage_blob_t *blobs);
extern void asset_shader_blob_free(stage_blob_t * blob);
extern void save_shader_asset(const char * path, stage_blob_t* stages, uint32_t stage_count);


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

public:
    AssetShader();

    static void export_shader(const std::string& path, const std::string& dst_path);

public:
   // static bool compile_shader(const void *data, size_t size, ShaderStage stage, char * entry, std::string *blob, std::string * error);
    static bool compile_shader(const std::string &path, ShaderStage stage, const std::string & entry, std::string *blob, std::string * error);
    static bool compile_shader_form_data(const char *data, size_t size, const std::wstring & include_path, ShaderStage stage, const std::string &entry, std::string *blob, std::string * error);
};

#endif
