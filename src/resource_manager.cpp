#include "resource_manager.h"

#include <algorithm>
#include <set>

#include "render_manager.h"
#include "json_serializer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"


static const std::filesystem::path mesh_extension(".mesh");

size_t filesize(FILE* file)
{
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

struct stream_impl
{
    stream_impl(FILE * file) : m_file(file)
    {
        m_write_buffer_size = 1024*4;
        m_write_buffer = new char [m_write_buffer_size]();

        m_read_buffer_size = m_write_buffer_size;
        m_read_buffer = new char[m_write_buffer_size]();

        refill_buffer();
    }

    size_t read(uint32_t size, char* data)
    {
        size_t bytes_read = 0;
        while (bytes_read < size) {
            if (m_read_pos == m_read_buffer_size) {
                refill_buffer();
                if (m_read_pos == m_read_buffer_size) {
                    break; // End of file
                }
            }
            data[bytes_read++] = m_read_buffer[m_read_pos++];
        }
        return bytes_read;
    }

    size_t write(uint32_t size, const char * data)
    {
        size_t bytes_written = 0;
        while (bytes_written < size) {
            if (m_write_pos == m_write_buffer_size) {
                write_buffer();
            }
            m_write_buffer[m_write_pos++] = data[bytes_written++];
        }
        return bytes_written;
    }

    size_t seek(size_t offset, int whence) 
    {
        flush(); // Flush the write buffer before seeking

        int result = fseek(m_file, (long)offset, whence);

        m_file_pos = result;
        m_read_pos = m_read_buffer_size; // Invalidate the read buffer

        return result;
    }

    void flush()
    {
        write_buffer();
    }

private:
    void refill_buffer()
    {
        size_t result = fread(m_read_buffer, 1, m_read_buffer_size, m_file);
        m_read_pos = 0;
        m_file_pos += result;
    }

    void write_buffer()
    {
        size_t result = fwrite(m_write_buffer, 1, m_write_pos, m_file);
        fflush(m_file);
        m_write_pos = 0;
        m_file_pos += result;
    }

private:
    FILE *      m_file = nullptr;
    size_t      m_file_pos = 0;

    char *      m_read_buffer = nullptr;
    size_t      m_read_buffer_size = 0;
    size_t      m_read_pos = 0;

    char *      m_write_buffer = nullptr;
    size_t      m_write_buffer_size = 0;
    size_t      m_write_pos = 0;
};



filestream* filestream::open(const char* path, const char* mode)
{
    FILE* file = fopen(path, mode);
    if (file == nullptr)
        return nullptr;

    filestream * fstream = new filestream();
    fstream->m_impl = new stream_impl(file);

    return fstream;
}

filestream* filestream::open_rb(const char* path)
{
    return filestream::open(path, "rb");
}

filestream* filestream::open_wb(const char* path)
{
    return filestream::open(path, "wb+");
}



uint32_t  filestream::read(uint32_t size, void* out_data)
{
    return (uint32_t)m_impl->read(size, (char*)out_data);
}

uint32_t  filestream::write(uint32_t size, const void* in_data)
{
    return (uint32_t)m_impl->write(size, (char*)in_data);
}

void filestream::flush()
{
    m_impl->flush();
}

void filestream::seek(uint32_t offset, int whence)
{
    m_impl->seek(offset, whence);
}



texture::texture(interned_string guid, interned_string name, gfx_texture_t* tex)
    : resource(guid, name)
    , m_texture(tex)
{   
}

rendermesh::rendermesh(interned_string guid, interned_string name, gfx_mesh_t mesh)
    : resource(guid, name)
    , m_mesh(mesh)
{
}



size_t filedata2(const char* path, char** buff)
{
    static void* g_ptr = nullptr;
    static size_t g_size = 0;

    size_t size = 0;
    FILE* file = fopen(path, "rb");
    if (file != NULL)
    {
        size = filesize(file);

        if (size > g_size)
            g_ptr = realloc(g_ptr, size + 1);
        g_size = size + 1;

        if (g_ptr != nullptr)
            fread(g_ptr, 1, size, file);
        fclose(file);
    }
    *buff = (char*)g_ptr;
    return size;
}


resource_manager * resource_manager::s_shared = nullptr;

void resource_manager::create_and_make_shader(gfx_context_t* ctx) 
{
    if (s_shared == nullptr) 
    {
        s_shared = new resource_manager(ctx);
        s_shared->init();
    }
}

static bool has_guid_in_name(const std::filesystem::path & filename, std::string * out_guid)
{
    if (filename.native().find('.') != std::wstring::npos)
    {
        auto guid = filename.extension().u8string();
        guid.erase(0, 1);
        if (!guid.empty() && guid.length() == 32)
            out_guid->assign(guid);
    }
    return !out_guid->empty();
}


gfx_mesh_pool_t create_mesh_pool(gfx_context_t * ctx, uint32_t vb_size, uint32_t ib_size)
{
    gfx_mesh_pool_t result = {};

    result.index_buffer_size = 16 * 1024 * 1024;

    gfx_buffer_desc_t vb_desc = {};
        vb_desc.label = "mesh_pool_vertex_buffer";
        vb_desc.usage = gfx_buffer_usage_vertex;
        vb_desc.size = vb_size;
    result.vertex_buffer = gfx_create_buffer2(ctx, &vb_desc);
    result.vertex_buffer_size = vb_size;

    gfx_buffer_desc_t ib_desc = {};
        ib_desc.label = "mesh_pool_index_buffer";
        ib_desc.usage = gfx_buffer_usage_index;
        ib_desc.size = ib_size;
    result.index_buffer = gfx_create_buffer2(ctx, &ib_desc);
    result.index_buffer_size = ib_size;
    //result.index_allocator = 
    return result;
}

void resource_manager::init()
{
    create_shader_from_file_path(m_ctx, "../data/shaders/simple.hlsl", &m_default_shader);

    m_mesh_pool = create_mesh_pool(m_ctx, 256*1024*1024, 16*1024*1024);

    gfx_vertex_attribute attributes[] = {
        { 0, 0, gfx_vertex_format_float4,   0                   },
        { 3, 0, gfx_vertex_format_float4,   sizeof(vec4) * 1    },  // uv
        { 2, 0, gfx_vertex_format_float4,   sizeof(vec4) * 2    },  // normal
        { 1, 0, gfx_vertex_format_float4,   sizeof(vec4) * 3    },  // color
    };

    gfx_vertex_slot_t slots[] = {
        {0, sizeof(vec4) * _countof(attributes), gfx_vertex_rate_vertex},
        //     {1, sizeof(instance_data), gfx_vertex_rate_instance}
    };

    gfx_pipeline_desc_t piplene_desc = {};
        piplene_desc.shader = m_default_shader;
        piplene_desc.assembly.topology = gfx_topology_triangles;
        piplene_desc.assembly.attributes = attributes;
        piplene_desc.assembly.attributes_count = _countof(attributes);
        piplene_desc.assembly.slots = slots;
        piplene_desc.assembly.slot_count = _countof(slots);
        piplene_desc.render_states.blend.enable = false;
        piplene_desc.render_states.blend.color_src = gfx_blend_mode_src_alpha;// VK_BLEND_FACTOR_SRC_ALPHA;
        piplene_desc.render_states.blend.color_dst = gfx_blend_mode_inv_src_alpha;// VK_BLEND_FACTOR_SRC_ALPHA;
    m_default_pipeline = gfx_create_pipeline2(m_ctx, &piplene_desc);

    m_meshes.reserve(65536);
    m_textures.reserve(65536);
}

void resource_manager::mount(const std::string & dir)
{
    if(!std::filesystem::exists(dir))
        return;
    
    if(m_dirs.find(dir) != m_dirs.end())
        return;

    std::filesystem::recursive_directory_iterator  it(dir);
    for(; it != std::filesystem::end(it); it++)
    {
        auto &path = it->path();
        if(std::filesystem::is_directory(path))
        {
            m_dirs.insert(path.u8string());
            continue;
        }

        auto filename = path.stem();

        std::string guid;
        if (has_guid_in_name(filename, &guid))
        {
            m_guid_2_path.emplace(&guid[0], path);
        }
    }
}

std::string resource_manager::find(const std::string & name)
{
    auto lowername(name);
    std::transform(lowername.begin(), lowername.end(), lowername.begin(), [](unsigned char c){ return std::tolower(c); });
    
    static std::string empty;

    auto it2 = m_guid_2_path.find(name);
    if (it2 != m_guid_2_path.end())
    {
        return it2->second.u8string();
    }
    
    return empty;
}


/*
void resource_manager::set_defaults(gfx_shader_t* shader, gfx_texture_t* texture)
{
    m_default_shader = shader;
}*/

std::vector<char> resource_manager::file_data(const std::string_view & path)
{
    std::vector<char> data;
    
    if(std::filesystem::exists(path))
    {
        FILE * file = fopen(path.data(), "rb");
        if(file == nullptr)
            return data;

        size_t size = filesize(file);
        data.resize(size);
        
        fread(data.data(), size, sizeof(char), file);
        fclose(file);
    }
    
    return data;
}


std::shared_ptr<gfx_material_t> resource_manager::load_material(const char * name, gfx_shader_t* shader)
{
    auto instance = load_material_instance(name);
    if(instance == nullptr)
        return nullptr;

    auto material = std::make_shared<gfx_material_t>();
    material->instance = instance;
    material->descriptor_set = gfx_create_descriptor_set2(m_ctx, instance->shader);
    
    for(size_t i = 0; i < _countof(instance->textures); ++i)
    {
        uint64_t handle = gfx_uniform_location(instance->shader, instance->textures[i].key.c_str());
        if(handle && instance->textures[i].value)
            gfx_uniform_set_texture(material->descriptor_set, handle, instance->textures[i].value);
    }

    for (size_t i = 0; i < _countof(instance->vectorsf); ++i)
    {
        uint64_t handle = gfx_uniform_location(instance->shader, instance->vectorsf[i].key.c_str());
        if (handle)
            gfx_uniform_set_buffer_data(material->descriptor_set, handle, &instance->vectorsf[i].value, sizeof(vec4));
    }

    m_materials.push_back(material);
    
    return material;
}

texture* resource_manager::load_texture(const char* name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end())
    {
        return it->second;
    }

    auto path = find(name);
    if (!path.empty())
    {
        gfx_texture_t* handle = nullptr;
        create_texture_from_file_path(m_ctx, path.c_str(), &handle);

        texture * result = new texture(name, path, handle);
        m_textures.emplace(name, result);
        return result;
    }
    return nullptr;
}

rendermesh* resource_manager::load_mesh(const char* name)
{
    if(name == nullptr)
        return nullptr;

    auto it = m_meshes.find(name);
    if (it != m_meshes.end())
    {
        return it->second;
    }

    auto path = find(name);
    if (!path.empty())
    {
        gfx_mesh_t handle = {};

        if (!create_mesh_from_file_path(m_ctx, &m_mesh_pool, path.data(), &handle))
            return nullptr;

        rendermesh * mesh = new rendermesh(name, path, handle);
        m_meshes.emplace(name, mesh);
        return mesh;
    }
    return nullptr;
}
/*
std::shared_ptr<gfx_texture_t> resource_manager::load_texture(const char * name)
{
    auto it = m_textures.find(name);
    if(it != m_textures.end())
    {
        return it->second;
    }

    auto path = find(name);
    if (!path.empty())
    {
        gfx_texture_t * texture = nullptr;
        create_texture_from_file_path(m_ctx, path.c_str(), &texture);
        if(texture == nullptr)
        {
            printf("\nerror while textureloading %s", name);
            return nullptr;
        }

        std::shared_ptr<gfx_texture_t> result(texture);
        m_textures.emplace(name, result);

        return result;
    }

    return nullptr;
}*/

void resource_manager::load_shader(const char* path)
{
}

struct material_descriptor
{
    interned_string                               shader;
    std::vector< shader_slot<interned_string>>    textures;
    std::vector< shader_slot<vec4>  >           vectors;
    std::vector< shader_slot<float> >           scalars;
    
        JsonSerialize(material_descriptor,
            SerializeFieldWithKey("shader", shader),
            SerializeFieldWithKey("textures", textures),
            SerializeFieldWithKey("vectors", vectors),
            SerializeFieldWithKey("scalars", scalars)
        );
};

JsonSerializeExternal(shader_slot<vec4>, SerializeFieldWithKey("key", key), SerializeFieldWithKey("value", value));
JsonSerializeExternal(shader_slot<float>, SerializeFieldWithKey("key", key), SerializeFieldWithKey("value", value));

JsonSerializeExternal(shader_slot<interned_string>,
    SerializeFieldWithKey("name", key),
    SerializeFieldWithKey("guid", value)
    //SerializeFieldWithKey("scaleOffset", value)
);


gfx_material_instance_t* resource_manager::load_material_instance(const std::string_view& guid)
{
    auto it = m_material_instances.find(guid);
    if (it != m_material_instances.end())
        return it->second.get();

    auto path = find(guid.data());

    if (path.empty())
        return nullptr;

    char *data = nullptr;
    size_t size = read_file_data(path.c_str(), &data);
    auto descriptor = json::from_json_string<material_descriptor>(data);
    free(data);

    auto material = std::make_shared<gfx_material_instance_t>();

    if(descriptor.shader.empty())
    {
        material->shader = m_default_shader;
        material->pipeline = m_default_pipeline;
    }

    for(size_t i = 0; i < descriptor.textures.size(); ++i)
    {
        auto tex = load_texture(descriptor.textures[i].value.c_str());
        material->textures[i].key = descriptor.textures[i].key;
        material->textures[i].value = tex  ? tex->texture_() : nullptr;
    }

    for (size_t i = 0; i < descriptor.vectors.size(); ++i)
    {
        material->vectorsf[i].key = descriptor.vectors[i].key;
        material->vectorsf[i].value = math::make_float4(descriptor.vectors[i].value);
    }

    for (size_t i = 0; i < descriptor.scalars.size(); ++i)
    {
        material->scalarsf[i].key = descriptor.scalars[i].key;
        material->scalarsf[i].value = descriptor.scalars[i].value;
    }

    m_material_instances.emplace(interned_string(guid), material);
    return material.get();
}

void resource_manager::directory_changed(std::filesystem::path &path)
{
}


/*
texture.b5a0953624bca8644be523fa1c4cada7.png
mesh.a59f1c021a3d7e942ad5eeddb30a16c7.fbx
*/
#pragma pack(push, 1)
struct mesh_header_t
{
    uint32_t   magick;
    uint32_t   submesh_count;

    int32_t    vertex_stride;
    int32_t    index_stride;

    int32_t    vertex_count;
    int32_t    index_count;

 //   float      bbox_max[4];
 //   float      bbox_min[4];
};
#pragma pack(pop)


size_t read_file_data2(const char* path, char** data_out)
{
    static char * s_ptr = nullptr;
    static size_t s_size = 0;

    FILE* file = fopen(path, "rb");
    if (file == nullptr)
        return 0;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if(s_size < size)
    {
        s_ptr = (char*)realloc(s_ptr, size);
        s_size = size;
    }

    fread(s_ptr, size, sizeof(char), file);
    fclose(file);

    *data_out = s_ptr;

    return (uint32_t)size;
}


size_t read_file_data(const char* path, char** data_out)
{
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
        return 0;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *data_out = (char*)calloc(1, size);
    fread(*data_out, size, sizeof(char), file);
    fclose(file);

    return (uint32_t)size;
}

void create_mesh_pool(gfx_context_t* ctx, uint32_t vertex_buffer_size, uint32_t index_buffer_size, gfx_mesh_pool_t* pool)
{
 //   pool->vertex_allocator = offset_alocator_create(vertex_buffer_size);
 //   pool->index_allocator = offset_alocator_create(index_buffer_size);

    gfx_buffer_desc_t vb = {};
        vb.data = nullptr;
        vb.mapped = false;
        vb.size = vertex_buffer_size;
        vb.usage = gfx_buffer_usage_vertex;
    pool->vertex_buffer = gfx_create_buffer2(ctx, &vb);

    gfx_buffer_desc_t ib = {};
        vb.data = nullptr;
        vb.mapped = false;
        vb.size = index_buffer_size;
        vb.usage = gfx_buffer_usage_index;
    pool->index_buffer = gfx_create_buffer2(ctx, &ib);
}

bool create_mesh_from_file_path(gfx_context_t* ctx, gfx_mesh_pool_t * pool, const char * path, gfx_mesh_t* out_mesh)
{
    char* data = nullptr;
    size_t size = read_file_data2(path, &data);
    if(size != 0)
    {
        create_mesh_from_file_data(ctx, pool, strrchr(path, '/'), data, size, out_mesh);
    }
    else
    {
        printf("\nNo file at path or file is empty: %s", path);
        return false;
    }
    
    //free(data);
    return true;
}

void create_mesh_from_file_data(gfx_context_t * ctx, gfx_mesh_pool_t* pool, const char *name, char * data, size_t size, gfx_mesh_t*out_mesh)
{
    char* curent_ptr = data;

    mesh_header_t* header = (mesh_header_t*)curent_ptr;
    curent_ptr += sizeof(mesh_header_t);

    char* vertex_data_ptr = curent_ptr;
    curent_ptr += header->vertex_stride * header->vertex_count;

    char* index_data_ptr = curent_ptr;
    curent_ptr += header->index_stride * header->index_count;

    int* submeshes = (int*)curent_ptr;

    uint32_t vertex_buffer_size = header->vertex_stride * header->vertex_count;
    uint32_t index_buffer_size = header->index_stride * header->index_count;

    gfx_buffer_t* vertex_buffer = nullptr;
    gfx_buffer_t* index_buffer = nullptr;

    bool ispooled = false;
    if(pool != nullptr)
    {
        int vertex_left = pool->vertex_buffer_size - pool->vertex_buffer_offset;
        int index_left = pool->index_buffer_size - pool->index_buffer_offset;

        if (vertex_left > vertex_buffer_size && index_left > index_buffer_size)
        {
            vertex_buffer = pool->vertex_buffer;
            index_buffer = pool->index_buffer;

            gfx_update_buffer_data(ctx, vertex_buffer, vertex_data_ptr, vertex_buffer_size, pool->vertex_buffer_offset);
            gfx_update_buffer_data(ctx, index_buffer, index_data_ptr, index_buffer_size, pool->index_buffer_offset);

            out_mesh->vertex_buffer_offset = pool->vertex_buffer_offset;
            out_mesh->index_buffer_offset = pool->index_buffer_offset;

            pool->vertex_buffer_offset += vertex_buffer_size;
            pool->index_buffer_offset += index_buffer_size;
        }
    }

    if(vertex_buffer == nullptr)
    {
        gfx_buffer_desc_t vb_desc = {};
            vb_desc.label = name;
            vb_desc.usage = gfx_buffer_usage_vertex;
            vb_desc.data = (uint8_t*)vertex_data_ptr;
            vb_desc.size = header->vertex_stride * header->vertex_count;
        vertex_buffer = gfx_create_buffer2(ctx, &vb_desc);
    }

    if(index_buffer == nullptr)
    {
        gfx_buffer_desc_t ib_desc = {};
            ib_desc.label = name;
            ib_desc.usage = gfx_buffer_usage_index;
            ib_desc.data = (uint8_t*)index_data_ptr;
            ib_desc.size = header->index_stride * header->index_count;
        index_buffer = gfx_create_buffer2(ctx, &ib_desc);
    }

    out_mesh->index_format = header->index_stride == 2 ? gfx_index_format_16 : gfx_index_format_32;
    out_mesh->submesh_count = header->submesh_count;
    out_mesh->index_buffer  = index_buffer;
    out_mesh->vertex_buffer = vertex_buffer;
    out_mesh->vertex_count  = header->vertex_count;
    out_mesh->index_count   = header->index_count;

    for(uint32_t i = 0; i < out_mesh->vertex_count; ++i)
    {
        out_mesh->bounds.extend(*(vec3*)(vertex_data_ptr + header->vertex_stride * i));
    }

    for (uint32_t i = 0; i < header->submesh_count; ++i)
        out_mesh->submeshes[i] = submeshes[i];
}



#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((uint32_t)(ch0) | ((uint32_t)(ch1) << 8) | ((uint32_t)(ch2) << 16) | ((uint32_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

static const uint32_t dds_magic  = 542327876;   // MAKEFOURCC('D', 'D', 'S', ' ');
static const uint32_t astc_magic = 0x5CA1AB13;
static const uint32_t ktx_magic  = 0x58544BAB;
static const uint32_t pvr_magic  = 0x03525650;  // v3, legacy(0x21525650)


#pragma pack(push, 1)
typedef struct dds_header_t
{
    uint32_t magic;
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitch_or_linear_size;
    uint32_t depth;
    uint32_t mipmap_count;
    uint32_t reserved1[11];
    struct {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t RGBBitCount;
        uint32_t r_mask;
        uint32_t g_mask;
        uint32_t b_mask;
        uint32_t a_mask;
    } dds_pixel_format; 
    uint32_t caps1;
    uint32_t caps2;
    uint32_t reserved2[3];
} dds_header_t;

typedef struct dds_header_dx10_t {
    uint32_t    dxgiFormat;
    uint32_t    resourceDimension;
    uint32_t    miscFlag;
    uint32_t    arraySize;
    uint32_t    miscFlags2;
} dds_header_dx10_t;

typedef struct pvr_header_t
{
    uint32_t    magick;             // Version of the file header, used to identify it.
    uint32_t    flags;              // Various format flags.
    uint64_t    pixel_format;       // The pixel format, 8cc value storing the 4 channel identifiers and their respective sizes.
    uint32_t    colour_space;       // The Colour Space of the texture, currently either linear RGB or sRGB.
    uint32_t    channel_type;       // Variable type that the channel is stored in. Supports signed/unsigned int/short/byte or float for now.
    uint32_t    height;             // Height of the texture.
    uint32_t    width;              // Width of the texture.
    uint32_t    depth;              // Depth of the texture. (Z-slices)
    uint32_t    num_surfaces;       // Number of members in a Texture Array.
    uint32_t    num_faces;          // Number of faces in a Cube Map. Maybe be a value other than 6.
    uint32_t    mipmap_count;       // Number of MIP Maps in the texture - NB: Includes top level.
    uint32_t    meta_data_size;    // Size of the accompanying meta data.
} pvr_header_t;

typedef struct astc_header_t
{
    uint32_t    magic;
    uint8_t     blockdimX;
    uint8_t     blockdimY;
    uint8_t     blockdimZ;
    uint8_t     xsize[3];
    uint8_t     ysize[3];
    uint8_t     zsize[3];
} astc_header_t;

typedef struct ktx_header_t
{
    uint8_t     identifier[12];
    uint32_t    endianness;
    uint32_t    glType;
    uint32_t    glTypeSize;
    uint32_t    glFormat;
    uint32_t    glInternalFormat;
    uint32_t    glBaseInternalFormat;
    uint32_t    width;
    uint32_t    height;
    uint32_t    depth;
    uint32_t    array_element_count;
    uint32_t    face_count;
    uint32_t    mipmap_count;
    uint32_t    keyValueDataLength;
} ktx_header_t;
#pragma pack(pop)

void create_texture_from_file_path(gfx_context_t* ctx, const char* path, gfx_texture_t** out_texture)
{
    char* data = nullptr;

    size_t size = read_file_data(path, &data);

    if (size != 0)
        create_texture_from_file_data(ctx, data, size, out_texture);

    free(data);

    if (*out_texture == nullptr)
        printf("\nfailed to create: %s texture", path);
}

void create_texture_from_file_data(gfx_context_t * ctx, char * data, size_t size, gfx_texture_t **out_texture)
{
    uint32_t magik = *(uint32_t*)data;

    bool stbi = false;
    gfx_texture_desc_t desc = {};
    switch (magik)
    {
        case astc_magic: {
            astc_header_t* header = (astc_header_t*)(data);
            desc.width  = (header->xsize[2] << 16) + (header->xsize[1] << 8) + header->xsize[0];
            desc.height = (header->ysize[2] << 16) + (header->ysize[1] << 8) + header->ysize[0];
            desc.depth  = (header->zsize[2] << 16) + (header->zsize[1] << 8) + header->zsize[0];
            if((header->blockdimX == header->blockdimY) && header->blockdimX == 4){
                desc.format = gfx_pixel_format_astc4x4;
            }else if((header->blockdimX == header->blockdimY) && header->blockdimX == 5){
                desc.format = gfx_pixel_format_astc5x5;
            }else if((header->blockdimX == header->blockdimY) && header->blockdimX == 6){
                desc.format = gfx_pixel_format_astc6x6;
            }else if((header->blockdimX == header->blockdimY) && header->blockdimX == 8){
                desc.format = gfx_pixel_format_astc8x8;
            }else if((header->blockdimX == header->blockdimY) && header->blockdimX == 10){
                desc.format = gfx_pixel_format_astc10x10;
            }else if((header->blockdimX == header->blockdimY) && header->blockdimX == 12){
                desc.format = gfx_pixel_format_astc12x12;
            }
            desc.mip_levels = 1;//log2(desc.width) + 1;
            desc.data = (data + sizeof(astc_header_t));
        } break;

        case dds_magic: {
            dds_header_t* header = (dds_header_t*)(data);

            //bool iscube = header->caps2 & 0x00000200;
            desc.data = (data + sizeof(dds_magic) + header->size);
            switch (header->dds_pixel_format.fourCC)
            {
                case MAKEFOURCC('D', 'X', 'T', '1'): desc.format = gfx_pixel_format_bc1; break;
                case MAKEFOURCC('D', 'X', 'T', '3'): desc.format = gfx_pixel_format_bc2; break;
                case MAKEFOURCC('D', 'X', 'T', '5'): desc.format = gfx_pixel_format_bc3; break;
                case MAKEFOURCC('D', 'X', '1', '0'): desc.format = gfx_pixel_format_bc7; break;
                default: debug::breakpoint(); break;
            };

            if(desc.format == gfx_pixel_format_bc7)
            {
                dds_header_dx10_t* header10 = (dds_header_dx10_t*)desc.data;
                desc.data = (data + sizeof(dds_magic) + header->size + sizeof(dds_header_dx10_t));
                if(header10->dxgiFormat == 95)
                    desc.format = gfx_pixel_format_bc6;
                if(header10->dxgiFormat == 98)
                    desc.format = gfx_pixel_format_bc7;
            }
            desc.width      = header->width;
            desc.height     = header->height;
            desc.depth      = header->depth == 0? 1: header->depth;
            desc.mip_levels = header->mipmap_count;
        } break;

        case pvr_magic: {
            pvr_header_t* header = (pvr_header_t*)(data);
            desc.data = (data + sizeof(pvr_header_t) + header->meta_data_size);
            switch (header->pixel_format)
            {
                case 0: desc.format = gfx_pixel_format_pvrtc_rgb_2bpp;   break;
                case 1: desc.format = gfx_pixel_format_pvrtc_rgba_2bpp;  break;
                case 2: desc.format = gfx_pixel_format_pvrtc_rgb_4bpp;   break;
                case 3: desc.format = gfx_pixel_format_pvrtc_rgba_4bpp;  break;
                case 23:desc.format = gfx_pixel_format_etc2_rgba8;       break;
            };
            desc.width       = header->width;
            desc.height      = header->height;
            desc.depth       = header->depth;
            desc.mip_levels  = header->mipmap_count;
        } break;

        case ktx_magic: {
            ktx_header_t * header = (ktx_header_t*) (data);
            desc.data = (data + sizeof(ktx_header_t) + header->keyValueDataLength);
            bool srgb = false;
            //https://registry.khronos.org/OpenGL/extensions/KHR/KHR_texture_compression_astc_hdr.txt
            switch (header->glInternalFormat)
            {
                case 0x93D0: srgb = true;
                case 0x93B0: desc.format = gfx_pixel_format_astc4x4;        break;  //COMPRESSED_RGBA_ASTC_4x4_KH
                    
                case 0x93D2: srgb = true;
                case 0x93B2: desc.format = gfx_pixel_format_astc5x5;        break;  //COMPRESSED_RGBA_ASTC_4x4_KH
                    
                case 0x93D4: srgb = true;
                case 0x93B4: desc.format = gfx_pixel_format_astc6x6;        break;  //COMPRESSED_RGBA_ASTC_6x6_KHR
                    
                case 0x93D7: srgb = true;
                case 0x93B7: desc.format = gfx_pixel_format_astc8x8;        break;  //COMPRESSED_RGBA_ASTC_8x8_KHR
                    
                case 0x93DB: srgb = true;
                case 0x93BB: desc.format = gfx_pixel_format_astc10x10;      break;  //COMPRESSED_RGBA_ASTC_10x10_KHR
                    
                case 0x93DD: srgb = true;
                case 0x93BD: desc.format = gfx_pixel_format_astc12x12;      break;  //COMPRESSED_RGBA_ASTC_12x12_KHR

                case 0x9274: desc.format = gfx_pixel_format_etc1;           break;
                case 0x9276: desc.format = gfx_pixel_format_etc2_rgb8a1;    break;
                case 0x9278: desc.format = gfx_pixel_format_etc2_rgba8;     break; // GL_COMPRESSED_RGBA8_ETC2_EAC
                default: return;
            }
            
            uint32_t offset = 4;
            uint8_t * layer_ptr = (uint8_t*)desc.data;
            
            for(uint32_t i = 0; i < header->mipmap_count; ++i)
            {
                uint32_t layersize = ((uint32_t*)(layer_ptr))[i];
                auto data_ptr = (uint8_t*)desc.data + offset;
                memmove(layer_ptr, data_ptr, layersize);
                layer_ptr += layersize;
                offset += layersize + 4;
            } /**/

            desc.width      = header->width;
            desc.height     = header->height;
            desc.depth      = header->depth == 0?1:header->depth;
            desc.mip_levels = header->mipmap_count;
        } break;

        default: {
            int channels_in_file = 0;
            desc.depth      = 1;
            desc.mip_levels = 1;
            desc.format     = gfx_pixel_format_rgba8;
            desc.data       = stbi_load_from_memory((stbi_uc*)data, (int)size, (int*)&desc.width, (int*)&desc.height, &channels_in_file, 4);
            stbi = desc.data != nullptr;
        };
    }
    if(desc.data != nullptr)
    {
        desc.type = gfx_texture2d;
        gfx_texture_t* texture = gfx_create_texture2(ctx, &desc);
        *out_texture = texture;
    }

    if(stbi)
        stbi_image_free(desc.data);
}


// Shader
#if __has_include("assets/asset_shader.h")
#include "assets/asset_shader.h"
#endif

static const char * shader_cash_path = "./cash/shaders/";


static void create_cash_path(const char * path)
{
    if(std::filesystem::exists(path))
        return;

    std::error_code error;
    bool result = std::filesystem::create_directories(path, error);
};


typedef struct shader_blob_t {
    const char* vsdata; size_t vssize;
    const char* psdata; size_t pssize;
    const char* csdata; size_t cssize;
} shader_blob_t;


typedef struct shader_blob2_t {
    shader_blob_t src;
    shader_blob_t spirv;    // vulkan dx12
    shader_blob_t msl;      // metal
    shader_blob_t wlsl;     // webgpu
} shader_blob2_t;

static void read_shader_cash(const char* filename, shader_blob_t * blob)
{
}

static void write_shader_cash(const char * filename, shader_blob_t * blob)
{
    //write 

}


void create_shader_from_file_path(gfx_context_t* ctx, const char* path, gfx_shader_t** out_shader)
{
    char* data = nullptr;

    size_t size = read_file_data(path, &data);

    std::hash<char*> _hash;

    size_t hash = _hash(data);

    const char* name = strrchr(path, '/');
    create_shader_from_file_data(ctx, name ? &name[1] : nullptr, data, size, out_shader);

    free(data);

    if (*out_shader == nullptr)
        printf("failed to create: %s shader", path);
}

const char* vs = R"( 
        struct VertexInput {
            @location(0) position : vec4<f32>,
            @location(1) uv : vec2<f32>,
        };

        @vertex fn main( vertex : VertexInput, @builtin(vertex_index) VertexIndex : u32 ) -> @builtin(position) vec4<f32> {
            return vec4<f32>(vertex.position.x, vertex.position.y, 0.0, 0.0);
        })";

const char* fs = R"( 
        @group(0) @binding(0) var mySampler: sampler;
        @group(0) @binding(1) var myTexture: texture_2d<f32>;

        @fragment fn main( @builtin(position) position: vec4<f32>, ) -> @location(0) vec4<f32> {
            return vec4<f32>(1.0, 1.0, 1.0, 1.0);
           // let color = textureSample(myTexture, mySampler, position.xy/1024);
           // return /*vec4<f32>(1.0, 1.0, 1.0, 1.0) */ color;
        })";


#include "gfx/gfx_reflection.h"
void create_shader_from_file_data(gfx_context_t* ctx, const char * name, char* data, size_t size, gfx_shader_t** out_shader)
{
    std::string vs_blob, vs_error,
                ps_blob, ps_error,
                cs_blob, cs_error;

    #ifdef __asset_shader_h__
    char entry_point[64] = "";
    if(AssetShader::find_pragma_entry(data, "compute", entry_point, sizeof(entry_point)))
    {
        if (!AssetShader::compile_shader_form_data(data, size, L"../data/shaders/", AssetShader::Compute, entry_point, &cs_blob, &cs_error))
            printf("%s", cs_error.c_str());
    }

    if (AssetShader::find_pragma_entry(data, "vertex", entry_point, sizeof(entry_point)))
    {
        if (!AssetShader::compile_shader_form_data(data, size, L"../data/shaders/", AssetShader::Vertex, entry_point, &vs_blob, &vs_error))
            printf("%s", vs_error.c_str());
    }

    if (AssetShader::find_pragma_entry(data, "fragment", entry_point, sizeof(entry_point)))
    {
        if (!AssetShader::compile_shader_form_data(data, size, L"../data/shaders/", AssetShader::Fragment, entry_point, &ps_blob, &ps_error))
            printf("%s", ps_error.c_str());
    }
    #endif


    shader_blob_t blob = {
         vs_blob.data(), vs_blob.size(),
         ps_blob.data(), ps_blob.size(),
         cs_blob.data(), cs_blob.size()
    };
    write_shader_cash(name, &blob);

    /**/
    uint32_t uniform_count = 0;
    gfx_uniform_t uniforms[128] = {};

    gfx_shader_reflection(vs_blob.c_str(), (uint32_t)vs_blob.size(), uniforms, &uniform_count);
    gfx_shader_reflection(ps_blob.c_str(), (uint32_t)ps_blob.size(), &uniforms[uniform_count], &uniform_count);
    gfx_shader_reflection(cs_blob.c_str(), (uint32_t)cs_blob.size(), &uniforms[uniform_count], &uniform_count);


    uniform_count = gfx_merge_uniforms(uniforms, uniform_count);
   // vs_blob = vs;
   // ps_blob = fs;

    gfx_shader_data sdata[] = {
        {gfx_shader_vertex,   (uint32_t*)vs_blob.c_str(), (uint32_t)vs_blob.size()},
        {gfx_shader_fragment, (uint32_t*)ps_blob.c_str(), (uint32_t)ps_blob.size()},
    };

    gfx_shader_desc_t shader_desc = {};
        shader_desc.label = "name";
        shader_desc.stages = sdata;
        shader_desc.stages_count = _countof(sdata);

        shader_desc.uniforms = uniforms;
        shader_desc.uniform_count = uniform_count;

    *out_shader = gfx_create_shader2(ctx, &shader_desc);
}

void create_material_from_file_path(gfx_context_t* ctx, const char* path, struct gfx_material_instance_t** material)
{
    char* data = nullptr;

    size_t size = read_file_data(path, &data);

    if (size != 0)
        create_material_from_file_data(ctx, data, size, material);

    free(data);
}

void create_material_from_file_data(gfx_context_t* ctx, char* data, size_t size, struct gfx_material_instance_t** insance)
{
  //  std::string jstr(data, size);
  //  json::from_json<gfx_material_instance_t>(jstr);
}