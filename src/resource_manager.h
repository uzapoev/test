#ifndef __resource_manager_h__
#define __resource_manager_h__

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>

#include "gfx/gfx.h"
#include "mathlib.h"
#include "common.h"
#include "resources.h"

#include "scene/render_system.h"
#include "resource_loader.h"

extern size_t   read_file_data(const char* path, char** data);
extern void     create_mesh_pool(gfx_context_t* ctx, uint32_t vsize, uint32_t isize, mesh_pool_t* pool);


/*
texture.b5a0953624bca8644be523fa1c4cada7.png
mesh.a59f1c021a3d7e942ad5eeddb30a16c7.fbx

typedef struct bundle_header_t {
    uint64_t            version;            // 1.0.0.a
    uint64_t            compressiontype;    // 'zip', 'fastlz', 'lzma'... etc
} bundle_header_t;

typedef struct file_info_t {
    char                path[MAX_PATH];
    uint64_t            crc32;      // checksum
    uint64_t            offset;     // offset from file begin
    uint64_t            size;       // compressed size
    uint64_t            orig_size;  // uncompressed size
} file_info_t;


class bundle {   
    bundle_header_t     header;
    uint32_t            file_count;         //
    file_info_t *       files;              // path, offset, size, orig_size, mtime
};
*/

class resource
{
public:
    resource(interned_string guid, interned_string name) :m_guid(guid), m_name(name) {}

    const interned_string&  guid() const    { return m_guid; }
    const interned_string&  name() const    { return m_name; }

    inline void retain()                    { m_ref_counter.fetch_add(1);   }
    inline void release()                   { m_ref_counter.fetch_sub(1);   }
    inline int  refcount()                  { return m_ref_counter.load();  }

private:
    interned_string                         m_guid;
    interned_string                         m_name;
    std::atomic_int                         m_ref_counter; 

private:
    resource(const resource&) = delete;
    resource(resource&&) = delete;
    resource& operator=(const resource&) = delete;
    resource& operator=(resource&&) = delete;
};


class texture_manager_prototype;
class texture : public resource
{
    friend class texture_manager_prototype;
public:
    texture(gfx_texture_t* tex, interned_string guid = "", interned_string name = "");

    gfx_texture_t *         texture_handle() { return m_texture; }
    gfx_sampler_t *         sampler_handle() { return m_sampler; }

    void                    set_mip(uint16_t mip, bool unload) {

     /*   texture_load_desc_t option;
            option.name = limit_mip_levels;
            option.value = mip
        m_manager->load(guid(), true, &option, [&](gfx_texture_t* new_handle){
            auto old = m_texture;
            update_handle(new_handle);
            gfx_texture_destroy(m_manager->ctx(), old);
        }*/
    }

protected:
    void                    update_handle(gfx_texture_t * handle) {
        m_texture = handle;
        //foreach(auto material in m_materials)
        //  materials.texture_handle_changed(this, m_texture);
    }// mipmap streaming 

private:
    texture_manager_prototype*  m_manager = nullptr;
    gfx_texture_t *             m_texture = nullptr;
    gfx_sampler_t *             m_sampler = nullptr;
    gfx_texture_desc_t          m_create_info = {};
//  gfx_list_t *                m_materials = nullptr; // 
};


class rendermesh : public resource
{
public:
    rendermesh(interned_string guid, interned_string name, render_mesh_t mesh);

    mesh_pool_t *       mesh_pool() { return m_pool; }
    render_mesh_t*      mesh()      { return &m_mesh; }

private:
    mesh_pool_t *       m_pool = nullptr;
    render_mesh_t       m_mesh = {};
};


class material : resource
{
public:
    void set_texture(const char * slot, texture * _texture)    { replace_texture(slot,  _texture); }

    void clear()
    {
       /* for(auto _texture in m_textures)
        {
            uint64_t uniform_handle = 0;//find_slot(_texture);
            gfx_descriptor_set_write_texture(m_descriptor_set, uniform_handle, nullptr);
            _texture->release();
            texture->unsubscribe(this);
        }*/
    }

private:
    void replace_texture(const char * slot, texture * _texture)
    {
        assert(slot);
        auto uniform_handle = gfx_uniform_location(m_instance->shader, slot);
        if(uniform_handle == -1) {
            debug::log_warning("no texture slot in material: %s", name());
            return;
        }
        
        auto texture_handle = _texture ? _texture->texture_handle() : nullptr;
        gfx_descriptor_set_write_texture(m_descriptor_set, uniform_handle, texture_handle);

     //   if(_texture != nullptr)
     //       m_material_manager->on_texture_changed(this, _texture);
    }
    
    void texture_handle_changed(texture * _texture)
    {
        uint64_t uniform_handle = 0;//find_slot(tex);
        gfx_descriptor_set_write_texture(m_descriptor_set, uniform_handle, _texture->texture_handle());
    }
private:
    gfx_material_instance_t *   m_instance = nullptr;
    gfx_uniform_t *             m_uniforms = nullptr; //in instance material
    gfx_descriptor_set_t *      m_descriptor_set = nullptr;

    class material_manager *    m_material_manager = nullptr;
};


class atlas : resource
{
public:
    vec4 get_frame(const std::string_view& name) {
        auto frame = m_frames.find(name);
        return frame != m_frames.end() ? frame->second : vec4();
    }
private:
    texture*                                    m_texture;
    std::unordered_map<interned_string, vec4>   m_frames;  // name + tileinfo(uv)
};



class texture_manager_prototype
{
public:
    typedef enum texture_tag {
        texture_tag_notag       = 0,
        texture_tag_albedo      = 1 << 0,   // albedo or diffuse
        texture_tag_normal      = 1 << 1,   // normal map
        texture_tag_mrao        = 1 << 2,   // metal,rafnes,acclusion
        texture_tag_sprite      = 1 << 3,   // ui
        texture_tag_mask        = 1 << 4,   // 
        texture_tag_lightmap    = 1 << 5,   // lightmap
        texture_tag_all         = 0xFFFF    // 
    } texture_tag;

    typedef struct texture_load_desc_t {
        uint32_t    mem_limit   = 0;        //
        uint32_t    mip_limit   = 0;        // limit miplevels
        uint32_t    dim_limit   = 0;        // limit with dimension
        uint32_t    tag_mask    = 0;        // lightmap, normalmap, mrao, albedo, sprites, mask, etc
        uint32_t    priority    = 0;        // 
    } texture_load_desc_t;

    texture_manager_prototype(gfx_context_t *, class resource_manager * rm);

    texture *                                       load(const char * guid, bool async, texture_load_desc_t * desc);

    void                                            set_memory_limit(uint32_t target_memory);

    void                                            clear();

    void                                            purge();

private:
    std::recursive_mutex                            m_mutex;
    gfx_context_t *                                 m_ctx = nullptr;
    texture *                                       m_default_texture = nullptr;
    class resource_manager *                        m_filesystem = nullptr;

    gfx_allocator_t *                               m_allocator = nullptr;
    std::unordered_map<interned_string, texture*>   m_textures;
    std::vector<texture*>                           m_erase_list;
    std::unordered_set<interned_string>             m_load_queue;
};


class mesh_manager_prototype
{
public:
    mesh_manager_prototype(gfx_context_t*, class resource_manager* rm);

    rendermesh * load(const char* guid, bool async);
private:
    std::recursive_mutex                                m_mutex;
    gfx_context_t *                                     m_ctx = nullptr;
    class resource_manager*                             m_filesystem = nullptr;

    std::unordered_map<interned_string, rendermesh*>    m_meshes;
    std::vector<mesh_pool_t*>                           m_mesh_pools;
};


class material_manager_prototype
{
public:
    material * load(const char* guid, bool async);

protected:
    void on_texture_removed(texture * _texture);
    void on_texture_changed(material * _material, texture * _texture);
private:
    std::unordered_map<texture*, std::vector<material*>> m_texture_consumers;
};


class resource_manager
{
public:
    static void                                                     create_and_make_shader(gfx_context_t* ctx);
    static resource_manager *                                       shared()    { return s_shared;}

public:
    resource_manager(gfx_context_t* ctx) :m_ctx(ctx), 
    m_texture_manager(ctx, this){ s_shared = this; }

    gfx_context_t *                                                 ctx() const { return m_ctx; }
    
public:
    void                                                            init();
    void                                                            mount(const std::string & dir);

    std::string                                                     find(const std::string & name);

    texture *                                                       load_texture(const char* path); // options = e_async|e_auto_gc|miplimits|mem limits etc
    rendermesh *                                                    load_mesh(const char* path);

    std::shared_ptr<gfx_material_t>                                 load_material(const char * path, gfx_shader_t* shader = nullptr);
    void                                                            load_shader(const char * path);

    static std::vector<char>                                        file_data(const std::string_view& path);
   // void                                                          set_defaults(gfx_shader_t * shader, gfx_texture_t * texture);
private:
    gfx_material_instance_t *                                       load_material_instance(const std::string_view& guid);
    void                                                            directory_changed(std::filesystem::path &path);

private:
  /*  
    struct info     { uint32_t size, compressed_size, offset;   };
    struct mem      {};
    struct blob     {char * data; size_t size;};

    info * get_info(const char * guid)      { return nullptr; }

    blob * load_blob(info * ptr, mem*)      
    {
        return nullptr; 
    }

    mem  * allocate_stage_mem(size_t size)  { return nullptr; }

    texture load_texture2(const char* guid)
    {
        auto fileinfo = get_info(guid);
        if (fileinfo == nullptr)
        {
            debug::log_error("failed to load texture: %s", guid);
            return m_default_texture;
        }

        auto mem  = allocate_stage_mem(fileinfo->size);
        auto blob = load_blob(fileinfo, mem);

        create_texture_from_file_data(m_ctx, blob->data, blob->size, nullptr);

        release_stage_mem(mem);
    }*/

private:
    static resource_manager*                                        s_shared;

    gfx_context_t *                                                 m_ctx = nullptr;
    gfx_shader_t*                                                   m_default_shader = nullptr;
    gfx_pipeline_t *                                                m_default_pipeline = nullptr;

    texture_manager_prototype                                       m_texture_manager;

    mesh_pool_t                                                     m_mesh_pool = {};

    std::unordered_set<interned_string>                             m_dirs;
    std::unordered_map<interned_string, std::filesystem::path>      m_guid_2_path;

    std::unordered_map<interned_string, texture*>                   m_textures;
    std::unordered_map<interned_string, rendermesh*>                m_meshes;

    struct aligned_allocator *                                      m_allocator = nullptr;
    struct temp_allocator *                                         m_temp_allocator = nullptr;
    struct paged_pool_allocator *                                   m_texture_allocator = nullptr;
    struct paged_pool_allocator *                                   m_material_allocator = nullptr;
    struct paged_pool_allocator *                                   m_rendermesh_allocator = nullptr;


    std::unordered_map<interned_string, std::shared_ptr<gfx_material_instance_t>>   m_material_instances;
    std::unordered_map<interned_string, std::shared_ptr<gfx_shader_t>>              m_shaders;
    std::vector<std::shared_ptr<gfx_material_t>>                                    m_materials;
}; 

#endif // __resource_manager_h__
