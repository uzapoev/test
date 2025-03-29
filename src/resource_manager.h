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


class bundle
{   
    bundle_header_t     header;
    uint32_t            file_count;         //
    file_info_t *       files;              // path, offset, size, orig_size, mtime
};
*/

struct filestream
{
    static filestream* open(const char* path, const char* mode);
    static filestream* open_rb(const char* path);   // binary read only 
    static filestream* open_wb(const char* path);   // binary write

    uint32_t    read(uint32_t size, void * out_data);
    uint32_t    write(uint32_t size, const void * in_data);
    void        seek(uint32_t offset, int whence);
    void        flush();

    template<class T> 
    inline T    read()        { T res = {}; read(sizeof(T), &res); return res;   }

    inline char* read_string(char* data) {
        data[0] = '\0';
        uint16_t len = read<uint16_t>();
        read(len, data);
        return data;
    }

    template<class T> 
    inline void write(T v)    { write(sizeof(T), &v);    }

    inline void write_chunk_info(uint16_t type, uint16_t size) { 
        write(type);
        write(size);
    }

    inline void write_chunk(uint16_t type, uint16_t size, const char * data) { 
        write(type);
        write(size);
        write(size, data);
    }

    inline void write_string(uint16_t size, const char* data) {
        write(size);
        write(size, data);
    }

    struct stream_impl * m_impl;
};


class resource
{
private:
    resource(const resource&) = delete;
    resource(resource&&) = delete;
    resource& operator=(const resource&) = delete;
    resource& operator=(resource&&) = delete;
public:
    resource(interned_string guid, interned_string name) :m_guid(guid), m_name(name) {}

    const interned_string&  guid() const    { return m_guid; }
    const interned_string&  name() const    { return m_name; }

    inline void retain()                    { m_ref_counter.fetch_add(1);   }
    inline void release()                   { m_ref_counter.fetch_sub(1);   }
    inline int  refcount()                  { return m_ref_counter.load();  }

private:
    interned_string         m_guid;
    interned_string         m_name;
    std::atomic_int         m_ref_counter; 
};


class texture : public resource
{
public:
    texture(interned_string guid, interned_string name, gfx_texture_t* tex);

    gfx_texture_t *         texture_() { return m_texture; }
    gfx_sampler_t *         sampler_() { return m_sampler; }

protected:
    void                    update_handle(gfx_texture_t * handle); // mipmap streaming 

private:
    gfx_texture_t *         m_texture = nullptr;
    gfx_sampler_t *         m_sampler = nullptr;
//  gfx_list_t *            m_materials = nullptr; // 
};


class rendermesh : public resource
{
public:
    rendermesh(interned_string guid, interned_string name, gfx_mesh_t mesh);

    gfx_mesh_pool_t *       mesh_pool() { return m_pool; }
    gfx_mesh_t*             mesh()      { return &m_mesh; }

private:
    gfx_mesh_pool_t *       m_pool = nullptr;
    gfx_mesh_t              m_mesh = {};
};


class material : resource
{
private:
    // void notify_texture_changed(texture * tex);
private:
    material *              m_instance = nullptr; //in instance material
    gfx_uniform_t *         m_uniforms = nullptr; //in instance material
    gfx_descriptor_set_t *  m_descriptor_set = nullptr;
};


class atlas : resource
{
public:
    vec4 get_frame(const std::string_view& name);
private:
    texture*                                    m_texture;
    std::unordered_map<interned_string, vec4>   m_frames;  // name + tileinfo(uv)
};


class resource_manager
{
public:
    static void                                                     create_and_make_shader(gfx_context_t* ctx);
    static resource_manager *                                       shared()    { return s_shared;}

public:
    resource_manager(gfx_context_t* ctx) :m_ctx(ctx)                { s_shared = this; }

    gfx_context_t *                                                 ctx() const { return m_ctx; }
    
public:
    void                                                            init();
    void                                                            mount(const std::string & dir);

    std::string                                                     find(const std::string & name);

    filestream *                                                    open_stream(const std::string& name, int mode); // e_read|e_write, e_buffered|e_mapped

    texture *                                                       load_texture(const char* path); // options = e_async|e_auto_gc
    rendermesh *                                                    load_mesh(const char* path);

    std::shared_ptr<gfx_material_t>                                 load_material(const char * path, gfx_shader_t* shader = nullptr);
    void                                                            load_shader(const char * path);

    static std::vector<char>                                        file_data(const std::string_view& path);
   // void                                                            set_defaults(gfx_shader_t * shader, gfx_texture_t * texture);
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

    gfx_mesh_pool_t                                                 m_mesh_pool;

    std::unordered_set<interned_string>                             m_dirs;
    std::unordered_map<interned_string, std::filesystem::path>      m_guid_2_path;

    std::unordered_map<interned_string, texture*>                   m_textures;
    std::unordered_map<interned_string, rendermesh*>                m_meshes;


    std::unordered_map<interned_string, std::shared_ptr<gfx_material_instance_t>>   m_material_instances;
    std::unordered_map<interned_string, std::shared_ptr<gfx_shader_t>>              m_shaders;
    std::vector<std::shared_ptr<gfx_material_t>>                                    m_materials;
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
    int64_t position;   // 16+16+16 - pos, 16 - color?
    int64_t uv01;       // 8
    int64_t tbn;        // 8 quaternion
    int64_t payload;    // 8

    //int4  pos_tbn;    128bit:  16+16+16 - pos, 16 - color, 16+16+16+16 - tbn quat
    //int4  uv_payload; 128bit:  16+16 - uv0, 16+16 - uv1, 32+32 - payload
                                //  8+8+8+8 = bone indexes
                                //  8+8+8+8 = bone weights
};

extern size_t   read_file_data(const char* path, char** data);
extern void     create_mesh_pool(gfx_context_t* ctx, uint32_t vsize, uint32_t isize, gfx_mesh_pool_t* pool);

extern bool     create_mesh_from_file_path(gfx_context_t* ctx, gfx_mesh_pool_t* pool, const char* path, gfx_mesh_t* out_mesh);
extern void     create_mesh_from_file_data(gfx_context_t* ctx, gfx_mesh_pool_t* pool, const char* name, char* data, size_t size, gfx_mesh_t* out_mesh);

extern void     create_texture_from_file_path(gfx_context_t* ctx, const char* path, gfx_texture_t** out_texture);
extern void     create_texture_from_file_data(gfx_context_t* ctx, char* data, size_t size, gfx_texture_t** out_texture);

extern void     create_shader_from_file_path(gfx_context_t* ctx, const char* path, gfx_shader_t** out_shader);
extern void     create_shader_from_file_data(gfx_context_t* ctx, const char* name, char* data, size_t size, gfx_shader_t** out_shader);

extern void     create_material_from_file_path(gfx_context_t* ctx, const char* path, struct gfx_material_instance_t** insance);
extern void     create_material_from_file_data(gfx_context_t* ctx, char* data, size_t size, struct gfx_material_instance_t** insance);

#endif // __resources_h__
