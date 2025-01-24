#ifndef __resources_h__
#define __resources_h__

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>

#include "gfx/gfx.h"
#include "mathlib.h"
#include "common.h"

#include "render_manager.h"

/*
texture.b5a0953624bca8644be523fa1c4cada7.png
mesh.a59f1c021a3d7e942ad5eeddb30a16c7.fbx
*/

class resource
{
public:
    const atomic_string &   guid() const { return m_guid;}
private:
    atomic_string           m_guid;
    atomic_string           m_name;
};


class texture : resource
{
public:
    gfx_texture_t *         texture_() { return m_texture; }
    gfx_sampler_t *         sampler_() { return m_sampler; }

private:
    gfx_texture_t *         m_texture = nullptr;
    gfx_sampler_t *         m_sampler = nullptr;
};


class mesh : resource
{
public:
    gfx_mesh_pool_t *       mesh_pool() { return m_pool; }
    gfx_mesh_t*             mesh_()     { return m_mesh; }

private:
    gfx_mesh_pool_t *       m_pool;
    gfx_mesh_t *            m_mesh;
};


class material : resource
{
private:
    // void notify_texture_changed(texture * tex);
private:
  //  material *              m_instance = nullptr; //in instance material
    gfx_uniform_t *         m_uniforms = nullptr; //in instance material
    gfx_descriptor_set_t *  m_descriptor_set = nullptr;
};

class renderer : resource
{
    std::shared_ptr<mesh>       m_mesh;
    std::shared_ptr<material>   m_material;
};

class atlas : resource
{
public:
    vec4 get_frame(const std::string_view& name);
private:
    texture*                                    m_texture;
    std::unordered_map<atomic_string, vec4>     m_frames;  // name + tileinfo(uv)
};


class resource_manager
{
public:
    resource_manager(gfx_context_t * ctx):m_ctx(ctx) { s_shared = this;}

    static void create_and_make_shader(gfx_context_t* ctx);
    static resource_manager* shared()                               { return s_shared;}

    gfx_context_t* ctx() const                                      { return m_ctx; }
    
public:
    void                                                            init();
    void                                                            mount(const std::string & dir);

    std::string                                                     find(const std::string & name);

    std::shared_ptr<gfx_mesh_t>                                     load_mesh(const char * path);
    std::shared_ptr<gfx_material_t>                                 load_material(const char * path, gfx_shader_t* shader = nullptr);
    std::shared_ptr<gfx_texture_t>                                  load_texture(const char * path);
    void                                                            load_shader(const char * path);

    static std::vector<char>                                        file_data(const std::string_view& path);
    void                                                            set_defaults(gfx_shader_t * shader, gfx_texture_t * texture);
private:
    gfx_material_instance_t *                                       load_material_instance(const std::string_view& guid);
    void                                                            directory_changed(std::filesystem::path &path);

private:
    static resource_manager*                                                    s_shared;

    gfx_context_t *                                                             m_ctx = nullptr;
    gfx_shader_t*                                                               m_default_shader = nullptr;
    gfx_pipeline_t *                                                            m_default_pipeline = nullptr;
    std::unordered_set<atomic_string>                                           m_dirs;
    std::unordered_map<atomic_string, std::filesystem::path>                    m_guid_2_path;
    std::unordered_multimap<atomic_string, std::filesystem::path>               m_file_pathes; //


    std::unordered_map<atomic_string, std::shared_ptr<gfx_mesh_t>>              m_meshes;
    std::unordered_map<atomic_string, std::shared_ptr<gfx_material_instance_t>> m_material_instances;
    std::unordered_map<atomic_string, std::shared_ptr<gfx_texture_t>>           m_textures;
    std::unordered_map<atomic_string, std::shared_ptr<gfx_shader_t>>            m_shaders;
    std::vector<std::shared_ptr<gfx_material_t>>                                m_materials;
}; 


struct vertex
{
    vec4    position;
    vec4    normal;
    vec4    tangent;
    vec4    uv;
};

struct vertex_compressed
{
    int64_t position;   // 16+16+16 - pos, 16 - color
    int64_t tbn;        // 8 quaternion
    int64_t uv01;       // 8
    int64_t payload;    // 8

    //int4  pos_tbn;    128bit:  16+16+16 - pos, 16 - color, 16+16+16+16 - tbn quat
    //int4  uv_payload; 128bit:  16+16 - uv0, 16+16 - uv1, 32+32 - payload
                                //  8+8+8+8 = bone indexes
                                //  8+8+8+8 = bone weights
};

extern size_t   read_file_data(const char* path, char** data);
extern void     create_mesh_pool(gfx_context_t* ctx, uint32_t vsize, uint32_t isize, gfx_mesh_pool_t* pool);

extern bool     create_mesh_from_file_path(gfx_context_t* ctx, const char* path, gfx_mesh_t* out_mesh);
extern void     create_mesh_from_file_data(gfx_context_t* ctx, const char* name, char* path, size_t size, gfx_mesh_t* out_mesh);

extern void     create_texture_from_file_path(gfx_context_t* ctx, const char* path, gfx_texture_t** out_texture);
extern void     create_texture_from_file_data(gfx_context_t* ctx, char* data, size_t size, gfx_texture_t** out_texture);

extern void     create_shader_from_file_path(gfx_context_t* ctx, const char* path, gfx_shader_t** out_shader);
extern void     create_shader_from_file_data(gfx_context_t* ctx, const char* name, char* data, size_t size, gfx_shader_t** out_shader);


extern void     create_material_from_file_path(gfx_context_t* ctx, const char* path, struct gfx_material_instance_t** insance);
extern void     create_material_from_file_data(gfx_context_t* ctx, char* data, size_t size, struct gfx_material_instance_t** insance);

#endif // __resources_h__
