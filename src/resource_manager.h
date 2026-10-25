#ifndef __resource_manager_h__
#define __resource_manager_h__

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <stack>

#include "gfx/gfx.h"
#include "mathlib.h"
#include "common.h"
#include "memmgr.h"

//          textures        shaders
// pc       dxt1..dxt7      spirv
// mobile   astc            sriprv(android), msl(ios)
// web      dxt1..dxt7      wgsl
typedef enum platform_type:uint8_t {
    platform_type_common,   // common for all platrorm f.e. scene, mesh, materials
    platform_type_pc,       // x86/apple silicon, 
    platform_type_mobile,   // ios/android
    platform_type_web,      // wasm
    count
} platform_type;


typedef enum asset_type : uint8_t {
    asset_type_unknown,
    asset_type_mesh,
    asset_type_texture,
    asset_type_material,
    asset_type_shader,
    asset_type_font,
    asset_type_animation,
    asset_type_atlas,

    asset_type_cinematic,
    asset_type_prefab,
    asset_type_scene,
    asset_type_script,

    asset_type_raw_mesh,    // render mesh
    asset_type_raw_image,   // raw image bytes in rgba8 fromat
    asset_type_raw_data,    // raw image bytes in rgba8 fromat
    asset_type_count
} asset_type;


typedef enum asset_state : uint8_t {
    empty = 0,      // Slot is empty or resource was garbage collected
    unloaded,       // Slot is empty or resource was garbage collected
    queued,         // Load request placed, waiting for a worker thread
    loading,        // Worker thread is reading disk / decompressing to RAM
    uploading,      // CPU data ready in staging RAM, transferring to VRAM
    ready           // Resource fully uploaded and ready for rendering/use
} asset_state;


struct asset_handle_t {
    union {
        uint64_t handle;
        struct {
            uint16_t slot_index;    // index
            uint16_t generation;    // generation 
            uint32_t instance_id;   // unique variation/override index for this asset. 0 uses the base asset directly
        };
    };
};
const asset_handle_t INVALID_ASSET_HANDLE = { 0 };

inline bool operator == (asset_handle_t a, nullptr_t) { return a.handle == 0; }
inline bool operator != (asset_handle_t a, nullptr_t) { return a.handle != 0; }
inline bool operator == (asset_handle_t a, asset_handle_t b) { return a.handle == b.handle; }


struct file_info_t {
    uint64_t    guid_rt;                // 64-bit virtual path hash
    guid_t      guid;                   // 64-bit virtual path hash
    uint32_t    size;                   // Unpacked size in RAM after loading
    uint32_t    compressed_size;        // Size on disk (0 if the file is uncompressed)
    uint32_t    offset;                 // Offset within the .pack container
    uint8_t     bundle_id;              // 0 - raw disk (DEV_MODE), >0 - index of the mounted .pack
    uint8_t     flags;                  // Compression algorithm (0 - none, 1 - LZ4, 2 - Zstd)
    asset_type  type;
    const char* path = nullptr;         // Only available in debug/editor
};



struct asset_slot_t {
    uint64_t                guid_rt;            // runtime guid
    //guid_t                  guid;               // runtime guid
    asset_handle_t          handle;             // 8 bytes(index(16) + generation(16) + instance_id(32))
    uint16_t                index_in_pool;      // 4 bytes
    uint16_t                ref_count;          // 2 bytes — Reference counter (up to 65535, more than enough)

    asset_type              type;               // 1 byte — asset type
    asset_state             state;              // 1 byte — Loading status

    uint8_t                 _padding[8];
};

static_assert(sizeof(asset_slot_t) == 32, "asset_slot_t must be exactly 32 bytes for Cache-line alignment!");

struct asset_io_blob_t {
    char *                  asset_name = "";
    uint32_t                slot_index;
    void*                   cpu_data;
    uint32_t                data_size;
    uint16_t                requested_mip;
};


typedef struct load_params_t {
    uint8_t                 initial_mip = 0;    // For textures: which mip to start with (0 - full)
    uint8_t                 priority = 1;       // Queue priority: 0 - highest (critical), 255 - background
    bool                    async = true;       // Asynchronous or blocking loading
    bool                    persistent = false; // Prevent collect_garbage from unloading this asset, even if ref_count == 0
} load_params_t;


typedef struct image_t {
    uint32_t                width;
    uint32_t                height;
    uint32_t                depth;
    uint32_t                bpp;
    int8_t*                 data;
} image_t;

struct texture_t {
    gfx_texture_t*          texture_handle;
    gfx_sampler_t*          sampler_handle;
    uint16_t                requested_mip;
    uint16_t                current_mip;
    gfx_texture_desc_t      desc;
};

typedef struct shader_program_t {
    int8_t                  keyword_count;
    char**                  keywords;
    int8_t                  stage_count;

    struct {
        int16_t size;
        char* data;
    } stage_data;
} shader_program_t;

struct shader_slot_data
{
    gfx_format format;
    union {
        float   f32_1;
        vec2    f32_2;
        vec3    f32_3;
        vec4    f32_4;
    };
};

// uniform values: scalar, vector, sampler, textures
// raster states: blend, culling, stencil, depth
// render queue / priority
// shader keywords 
typedef struct material_t {
    class resource_manager*     manager = nullptr;
    gfx_shader_t*               shader = nullptr;
    gfx_pipeline_t*             pipeline = nullptr;

    uint32_t                    uniform_count;
    gfx_uniform_t*              uniforms;

    uint16_t                    texture_count;
    texture_t *                 textures[8];

    uint32_t                    slot_count;
    char **                     slot_names;
    shader_slot_data *          slot_datas;
} material_t;


typedef struct material_instance_t  {
    material_t*                 instance = nullptr;
    gfx_descriptor_set_t*       descriptor_set = nullptr;
    asset_handle_t              textures[6];
} material_instance_t;


typedef struct cinematic_t {
    struct cinematic_curve_t    {};
    struct cinematic_clip_t     {uint32_t curve_id;};
    struct cinematic_action_t   {interned_string func, args;};
    struct cinematic_track_t    {interned_string target; };
} cinematic_t;


typedef struct mesh_pool_t {
    gfx_offset_allocator_t*     vertex_buffer_allocator;
    gfx_offset_allocator_t*     index_buffer_allocator;

    gfx_index_format            index_format;

    int32_t                     vertex_buffer_size;
    gfx_buffer_t*               vertex_buffer;

    int32_t                     index_buffer_size;
    gfx_buffer_t*               index_buffer;
} mesh_pool_t;


typedef struct submesh_t {
    uint32_t                    index_start;
    uint32_t                    index_count;
} submesh_t;

typedef struct raw_mesh_t {
    uint32_t                    vertex_stride;
    uint32_t                    vertex_count;
    void*                       vertex_data;

    gfx_index_format            index_format;
    uint32_t                    index_count;
    void*                       index_data;


    uint32_t                    vertex_attribute_count;
    gfx_vertex_attribute        vertex_attributes[16];

    uint32_t                    submesh_count;
    submesh_t                   submeshes[16]; // start index

    aabbox                      bounds;
} raw_mesh_t;


typedef struct render_mesh_t {
    //vertex layout
    uint32_t                    vertex_stride;
    uint32_t                    vertex_count;
    gfx_buffer_t*               vertex_buffer;
    uint32_t                    vertex_buffer_offset;

    gfx_index_format            index_format;
    uint32_t                    index_count;
    gfx_buffer_t*               index_buffer;
    uint32_t                    index_buffer_offset;

    uint32_t                    submesh_count;
    submesh_t                   submeshes[16]; // start index

    aabbox                      bounds;
} render_mesh_t;


extern uint32_t read_file_data(const char* path, char** data);
extern void     create_mesh_pool(gfx_context_t* ctx, uint32_t vsize, uint32_t isize, mesh_pool_t* pool);


/*
b5a0953624bca8644be523fa1c4cada7.name.png
a59f1c021a3d7e942ad5eeddb30a16c7.name.fbx

typedef struct bundle_header_t {
    uint64_t            version;            // 1.0.0.a
    uint64_t            compressiontype;    // 'zip', 'fastlz', 'lzma'... etc
} bundle_header_t;

class bundle {   
    bundle_header_t     header;
    uint32_t            file_count;         //
    file_info_t *       files;              // path, offset, size, orig_size, mtime
};
*/


typedef bool (*asset_compile_func_t)(const char* src_path, const char* out_bin_path, const char* meta_path, platform_type target_platform, void* userdata);

typedef bool (*asset_load_func_t)(struct resource_manager & mgr, asset_io_blob_t & blob, asset_slot_t & slot);

struct base_manager {
    friend class resource_manager;
    class texture_streaming_manager_impl* m_impl;
    class resource_manager* m_resource_manager;
};

struct texture_streaming_manager : base_manager
{
    void set_gpu_ram_limit();
};

class mesh_pool_manager : base_manager
{
    void add_mesh();
    void remove_mesh();
};


class material_manager : base_manager
{
public:
    asset_handle_t  create_instance(class resource_manager &m, asset_handle_t);
    void            destroy_instance(asset_handle_t instance_handle);

    void set_global_texture(uint64_t id, asset_handle_t texture_handle);
    void set_global_texture(const char* name, asset_handle_t texture_handle);

    void set_global_data(uint64_t id, void * data, uint32_t size);
    void set_global_data(const char* name, void* data, uint32_t size);

    void set_texture(asset_handle_t material, uint64_t id, asset_handle_t texture_handle);
    void set_texture(asset_handle_t material, const char* name, asset_handle_t texture_handle);

    void set_data(asset_handle_t material, uint64_t id, const void* data, uint32_t size);
    void set_data(asset_handle_t material, const char* name, const void* data, uint32_t size);

protected:
    void trigger_resource_changed(guid_t guid, asset_handle_t handle);

    void trigger_material_changed(guid_t guid, material_t * material);
    void trigger_shader_changed(guid_t guid, gfx_shader_t * shader);
    void trigger_texture_changed(guid_t guid, texture_t * texture);

private:
    class material_manager_impl *                                       m_impl;
    material_instance_t *                                               m_instances;
    std::unordered_map<uint64_t,  std::vector<asset_handle_t>>          m_unform_2_material;
    std::unordered_map<asset_handle_t, std::vector<asset_handle_t>>     m_material_instances;
};



class resource_manager
{
    friend material_manager;
public:
    static resource_manager *                                       create_and_make_shared(gfx_context_t* ctx, iallocator* memory_resource);
    static resource_manager *                                       shared()    { return s_shared;}

public:
    void                                                            init(platform_type type, const char * cash_path = nullptr);

    std::string                                                     find(const std::string & name);

    static std::vector<char>                                        file_data(const std::string_view& path);
private:
    resource_manager(gfx_context_t* ctx, iallocator* memory_resource);

    material_instance_t *                                           load_material_instance(const std::string_view& guid);
    void                                                            directory_changed(std::filesystem::path &path);

private:
    static resource_manager*                                        s_shared;

public:
    void                            set_compiler(asset_type, asset_compile_func_t func);

    void                            set_assets_path(const std::string& dir);
    void                            set_cash_path(const char* path);
    void                            set_dlc_path(const char* path);

    asset_handle_t                  load(guid_t guid, load_params_t* param);
    asset_handle_t                  load(const char* path, load_params_t* param);
    void                            release(asset_handle_t);
    void                            wait(asset_handle_t handle);

    inline const render_mesh_t*     get_mesh(asset_handle_t handle) noexcept;
    inline const gfx_shader_t*      get_shader(asset_handle_t handle) noexcept;
    inline const texture_t*         get_texture(asset_handle_t handle) noexcept;
    inline const material_t*        get_material(asset_handle_t handle) noexcept;
    inline const material_instance_t* get_material_instance(asset_handle_t handle) noexcept;

    asset_slot_t *                  get_slot(uint64_t runtime_guid); // private 
    file_info_t *                   get_file_info(uint64_t runtime_guid); // private

    void                            update();


    void    gc_process();
    void    perform_resource_uploading();
    //void    job_push_io_request(guid_t guid);     // todo
    //void    job_steal_request(guid_t guid);       // todo
    gfx_context_t* ctx() { return m_ctx; }
private:
    // slot manipulations
    uint32_t       m_first_free_slot = 0;
    uint32_t       m_allocated_count = 0;

    asset_handle_t          slot_allocate(uint64_t guid, asset_type type);
    void                    slot_free(asset_handle_t handle);
    inline asset_slot_t*    slot_validate_and_get(asset_handle_t handle) noexcept {
        if (handle.slot_index >= m_slots_capacity) return nullptr;

        asset_slot_t* slot = &m_slots[handle.slot_index];
        return (slot->handle.generation == handle.generation /*|| slot->state != asset_state::empty*/) ? slot : nullptr;
    }

private:
    friend bool asset_load_mesh(struct resource_manager& mgr, asset_io_blob_t& blob, asset_slot_t& slot);
    //
    asset_io_blob_t                         read_asset_blob(file_info_t * info);
    void                                    free_asset_blob(asset_io_blob_t* blob);

    asset_handle_t                          create_material_instance(asset_handle_t handle);
    asset_handle_t                          load_material(asset_io_blob_t&);

    guid_t                                  get_or_create_guid(const char* path, const char* meta_path);

private:
    gfx_context_t*                                      m_ctx = nullptr;
    gfx_shader_t*                                       m_default_shader = nullptr;
    gfx_pipeline_t*                                     m_default_pipeline = nullptr;

    mesh_pool_t                                         m_mesh_pool = {};

    std::unordered_set<interned_string>                 m_dirs;
    std::unordered_map<uint64_t, file_info_t>           m_file_infos;   // 

    std::unordered_map<guid_t, std::filesystem::path>   m_guid_2_path;
    std::unordered_map<interned_string, guid_t>         m_path_2_guid;  //
    std::unordered_map<uint64_t, asset_slot_t*>         m_guid_2_slots; // loaded resource slots


    asset_compile_func_t                                m_compilers[asset_type_count] = {};
    asset_load_func_t                                   m_loaders[asset_type_count] = {};

    std::string                                         m_cash_path;
    platform_type                                       m_platform_type = platform_type_pc;

    asset_slot_t*                                       m_slots = nullptr;
    uint32_t                                            m_slots_capacity = 16384;

    iallocator*                                         m_allocator = nullptr;
    staging_allocator*                                  m_staging_allocator = nullptr;

    paged_pool_allocator_t<texture_t>                   m_textures;
    paged_pool_allocator_t<render_mesh_t>               m_render_meshes;
    paged_pool_allocator_t<material_t>                  m_materials;
    paged_pool_allocator_t<material_instance_t>         m_material_instances;
    paged_pool_allocator_t<gfx_shader_t>                m_shaders;

    std::stack<asset_handle_t>                          m_gc_handles;
}; 



inline const render_mesh_t* resource_manager::get_mesh(asset_handle_t handle) noexcept
{
    const uint32_t idx = handle.slot_index;

    if (idx >= m_slots_capacity)
        return nullptr;

    const asset_slot_t& slot = m_slots[idx];

    const bool valid = slot.handle.handle == handle.handle;
    return valid ? m_render_meshes.get_by_index(slot.index_in_pool) : nullptr;
}


const gfx_shader_t* resource_manager::get_shader(asset_handle_t handle) noexcept
{
    const uint32_t idx = handle.slot_index;

    if (idx >= m_slots_capacity)
        return nullptr;

    const asset_slot_t& slot = m_slots[idx];
    const bool valid = slot.handle.handle == handle.handle;
    return valid ? m_shaders.get_by_index(slot.index_in_pool) : nullptr;
}

const texture_t* resource_manager::get_texture(asset_handle_t handle) noexcept
{
    const uint32_t idx = handle.slot_index;

    if (idx >= m_slots_capacity)
        return nullptr;

    const asset_slot_t& slot = m_slots[idx];

    const bool valid = slot.handle.handle == handle.handle;
    return valid ? m_textures.get_by_index(slot.index_in_pool) : nullptr;
}

inline const material_t* resource_manager::get_material(asset_handle_t handle) noexcept
{
    const uint32_t idx = handle.slot_index;
    if (idx >= m_slots_capacity)
        return nullptr;

    const asset_slot_t& slot = m_slots[idx];
    const bool valid = slot.handle.handle == handle.handle;
    return valid ? m_materials.get_by_index(slot.index_in_pool) : nullptr;
}

inline const material_instance_t* resource_manager::get_material_instance(asset_handle_t handle) noexcept
{
    const uint32_t idx = handle.slot_index;
    if (idx >= m_slots_capacity)
        return nullptr;

    const asset_slot_t& slot = m_slots[idx];
    const bool valid = (slot.handle.generation == handle.generation) && (slot.handle.slot_index == slot.handle.slot_index);
    return valid ? m_material_instances.get_by_index(slot.index_in_pool) : nullptr;
}

#endif // __resource_manager_h__
