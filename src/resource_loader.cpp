#include "resource_loader.h"
#include "common.h"

#include "gfx/gfx_reflection.h"
//#include "render_system.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


size_t filesize(FILE* file)
{
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}


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


size_t read_file_data_text(const char* path, char** data_out)
{
    FILE* file = fopen(path, "r");
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


void create_mesh_pool(gfx_context_t* ctx, uint32_t vertex_buffer_size, uint32_t index_buffer_size, mesh_pool_t* pool)
{
    gfx_offset_allocator_create(&pool->vertex_buffer_allocator, vertex_buffer_size, 1024);
    gfx_offset_allocator_create(&pool->index_buffer_allocator, index_buffer_size, 128);

   /* gfx_offset_allocator_allocate(&pool->index_buffer_allocator, 8*1024*1024, 16);
    gfx_offset_allocator_allocate(&pool->index_buffer_allocator, 8*1024*1024, 16);
    gfx_offset_allocator_allocate(&pool->index_buffer_allocator, 8*1024*1024, 16);
    gfx_offset_allocator_allocate(&pool->index_buffer_allocator, 8*1024*1024, 16);
    gfx_offset_allocator_allocate(&pool->index_buffer_allocator, 8*1024*1024, 16);*/

    gfx_buffer_desc_t vb = {};
        vb.data     = nullptr;
        vb.mapped   = false;
        vb.size     = vertex_buffer_size;
        vb.usage    = gfx_buffer_usage_vertex;
    pool->vertex_buffer = gfx_create_buffer2(ctx, &vb);
    pool->vertex_buffer_size = vertex_buffer_size;

    gfx_buffer_desc_t ib = {};
        ib.data     = nullptr;
        ib.mapped   = false;
        ib.size     = index_buffer_size;
        ib.usage    = gfx_buffer_usage_index;
    pool->index_buffer = gfx_create_buffer2(ctx, &ib);
    pool->index_buffer_size = index_buffer_size;
}

bool load_mesh_from_file_path(gfx_context_t* ctx, mesh_pool_t * pool, const char * path, render_mesh_t* out_mesh)
{
    char* data = nullptr;
    size_t size = read_file_data2(path, &data);
    if(size != 0)
    {
        load_mesh_from_file_data(ctx, pool, strrchr(path, '/'), data, size, out_mesh);
    }
    else
    {
        printf("\nNo file at path or file is empty: %s", path);
        return false;
    }
    
    //free(data);
    return true;
}

void load_mesh_from_file_data(gfx_context_t * ctx, mesh_pool_t* pool, const char *name, char * data, size_t size, render_mesh_t* out_mesh)
{
    char* curent_ptr = data;

    mesh_header_t* header = (mesh_header_t*)curent_ptr;
    curent_ptr += sizeof(mesh_header_t);

    char* vertex_data_ptr = curent_ptr;
    curent_ptr += header->vertex_stride * header->vertex_count;

    char* index_data_ptr = curent_ptr;
    curent_ptr += header->index_stride * header->index_count;

    int* submeshes = (int*)curent_ptr;

    int32_t vertex_buffer_size = gfx_utils_align_up(header->vertex_stride * header->vertex_count, 16);
    int32_t index_buffer_size = gfx_utils_align_up(header->index_stride * header->index_count, 16);

    if(pool != nullptr)
    {
        auto offset_vb = gfx_offset_allocator_allocate(&pool->vertex_buffer_allocator, vertex_buffer_size, 16);
        auto offset_ib = gfx_offset_allocator_allocate(&pool->index_buffer_allocator, index_buffer_size, 16);

        if(offset_vb != -1)
        {
            out_mesh->vertex_buffer = pool->vertex_buffer;
            out_mesh->vertex_buffer_offset = offset_vb;

            gfx_update_buffer_data(ctx, pool->vertex_buffer, vertex_data_ptr, vertex_buffer_size, offset_vb);
        }
        else
        {
            debug::log_error("not enough memory in vb mesh pool");
        }

        if (offset_ib != -1)
        {
            out_mesh->index_buffer = pool->index_buffer;
            out_mesh->index_buffer_offset = offset_ib;

            gfx_update_buffer_data(ctx, pool->index_buffer, index_data_ptr, index_buffer_size, offset_ib);
        }
        else
        {
            debug::log_error("not enough memory in ib mesh pool");
        }
    }

    if(out_mesh->vertex_buffer == nullptr)
    {
        gfx_buffer_desc_t vb_desc = {};
            vb_desc.label = name;
            vb_desc.usage = gfx_buffer_usage_vertex;
            vb_desc.data = (uint8_t*)vertex_data_ptr;
            vb_desc.size = header->vertex_stride * header->vertex_count;
        out_mesh->vertex_buffer = gfx_create_buffer2(ctx, &vb_desc);
    }

    if(out_mesh->index_buffer == nullptr)
    {
        gfx_buffer_desc_t ib_desc = {};
            ib_desc.label = name;
            ib_desc.usage = gfx_buffer_usage_index;
            ib_desc.data = (uint8_t*)index_data_ptr;
            ib_desc.size = header->index_stride * header->index_count;
        out_mesh->index_buffer = gfx_create_buffer2(ctx, &ib_desc);
    }

    out_mesh->index_format = header->index_stride == 2 ? gfx_index_format_16 : gfx_index_format_32;
    out_mesh->submesh_count = header->submesh_count;
    out_mesh->vertex_stride = header->vertex_stride;
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

void load_texture_from_file_path(gfx_context_t* ctx, const char* path, gfx_texture_t** out_texture)
{
    char* data = nullptr;

    size_t size = read_file_data(path, &data);

    if (size != 0)
        load_texture_from_file_data(ctx, strrchr(path, '/'), data, size, out_texture);

    free(data);

    if (*out_texture == nullptr)
        printf("\nfailed to create: %s texture", path);
}


void load_texture_from_file_data(gfx_context_t * ctx, const char* name, char * data, size_t size, gfx_texture_t **out_texture)
{
    uint32_t magik = *(uint32_t*)data;

    bool free_stbi_buffer = false;
    gfx_texture_desc_t desc = {};
    desc.label = name;
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
                case MAKEFOURCC('D', 'X', 'T', '5'): desc.format = gfx_pixel_format_bc3; break;
                case MAKEFOURCC('D', 'X', '1', '0'): {
                    dds_header_dx10_t* header10 = (dds_header_dx10_t*)desc.data;
                    desc.data = (data + sizeof(dds_magic) + header->size + sizeof(dds_header_dx10_t));
                    switch(header10->dxgiFormat) {
                        case 95: desc.format = gfx_pixel_format_bc6h; break;
                        case 98: desc.format = gfx_pixel_format_bc7; break;
                        default:    
                            debug::log_error("texture %s has unsupported format: %d", name? name: "nullptr", header10->dxgiFormat);
                            debug::breakpoint();// wtf
                            break;
                     }
                }break;

                default: assert(false); break;
            };

            desc.width      = header->width;
            desc.height     = header->height;
            desc.depth      = header->depth == 0? 1: header->depth;
            desc.mip_levels = header->mipmap_count;
        } break;

        case ktx_magic: {
            ktx_header_t * header = (ktx_header_t*) (data);
            desc.data = (data + sizeof(ktx_header_t) + header->keyValueDataLength);
            bool srgb = false;
            //https://registry.khronos.org/OpenGL/extensions/KHR/KHR_texture_compression_astc_hdr.txt
            switch (header->glInternalFormat)
            {
                case 0x93D0: srgb = true; [[fallthrough]];
                case 0x93B0: desc.format = gfx_pixel_format_astc4x4;        break;  //COMPRESSED_RGBA_ASTC_4x4_KH
                    
                case 0x93D2: srgb = true; [[fallthrough]];
                case 0x93B2: desc.format = gfx_pixel_format_astc5x5;        break;  //COMPRESSED_RGBA_ASTC_4x4_KH
                    
                case 0x93D4: srgb = true; [[fallthrough]];
                case 0x93B4: desc.format = gfx_pixel_format_astc6x6;        break;  //COMPRESSED_RGBA_ASTC_6x6_KHR
                    
                case 0x93D7: srgb = true; [[fallthrough]];
                case 0x93B7: desc.format = gfx_pixel_format_astc8x8;        break;  //COMPRESSED_RGBA_ASTC_8x8_KHR
                    
                case 0x93DB: srgb = true; [[fallthrough]];
                case 0x93BB: desc.format = gfx_pixel_format_astc10x10;      break;  //COMPRESSED_RGBA_ASTC_10x10_KHR
                    
                case 0x93DD: srgb = true; [[fallthrough]];
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
            desc.depth       = 1;
            desc.mip_levels  = 1;
            desc.format      = gfx_pixel_format_rgba8;
            desc.data        = stbi_load_from_memory((stbi_uc*)data, (int)size, (int*)&desc.width, (int*)&desc.height, &channels_in_file, 4);
            free_stbi_buffer = desc.data != nullptr;
        };
    }

    if(desc.data != nullptr)
    {
        if(desc.mip_levels > 1)
        {
            uint32_t target_width = 512;
            uint32_t offset = 0;
            while (desc.width > target_width)
            {
                offset += gfx_utils_image_layer_size(desc.width, desc.height, desc.depth, desc.format);
                desc.width = desc.width >> 1;
                desc.height = desc.height >> 1;
                desc.mip_levels--;
            }
            desc.data = (char*)desc.data + offset;
        }
        desc.type = gfx_texture2d;
        gfx_texture_t* texture = gfx_create_texture2(ctx, &desc);
        *out_texture = texture;
    }

    if(free_stbi_buffer)
        stbi_image_free(desc.data);
}


// Shader

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



extern int asset_shader_compile(const char* name, const char* data, uint32_t size, const char* target, char** blobs, int* sizes, const char** stages);

static gfx_shader_stage str_2_stage(const char * str)
{
    if(str && !strcmp(str, "vertex"))
        return gfx_shader_vertex;

    if (str && !strcmp(str, "fragment"))
        return gfx_shader_fragment;

    if (str && !strcmp(str, "compute"))
        return gfx_shader_compute;
}

const uint64_t shader_magic = 0x20726564616873;

#pragma pack(push, 1)
typedef struct shader_header_t {
    uint64_t    magic_ui64;
    int32_t     stagecount;
} shader_header_t;
#pragma pack (pop)


void save_shader(const char * path, int count, char** blobs, int* sizes, const char** stages)
{
    FILE* file = fopen(path, "wb");
    fwrite(&shader_magic, sizeof(uint64_t), 1, file);
    fwrite(&count, sizeof(uint32_t), 1, file);
    for (int i = 0; i < count; ++i)
    {
        uint32_t stage = str_2_stage(stages[i]);
        fwrite(&stage, sizeof(uint32_t), 1, file);
        fwrite(&sizes[i], sizeof(uint32_t), 1, file);
        fwrite(blobs[i], sizes[i], 1, file);
        fflush(file);
    }
    fclose(file);
}

void load_shader_from_file_path(gfx_context_t* ctx, const char* path, gfx_shader_t** out_shader)
{
    const char* name = strrchr(path, '/');
    name = name ? &name[1] : path;

    char* data = nullptr;

    char compiled_name_buff[256] = "";
    sprintf(compiled_name_buff, "%s.spirv", path);

    // check for exported
    size_t size = read_file_data(compiled_name_buff, &data);
    if (size != 0)
    {
        load_shader_from_file_data(ctx, name, data, size, out_shader);
        free(data);
        return;
    }/**/

    size = read_file_data_text(path, &data);
    if (size == 0)
        return;

    char* blobs[16] = {};
    const char* stages[16] = {};
    int sizes[16] = {};
    
    int count = asset_shader_compile(name, data, size, "spirv", blobs, sizes, stages);
    save_shader(compiled_name_buff, count, blobs, sizes, stages);
    
    load_shader_from_file_path(ctx, path, out_shader);
    free(data);
}



void load_shader_from_file_data(gfx_context_t* ctx, const char * name, char* data, size_t size, gfx_shader_t** out_shader)
{
    uint32_t uniform_count = 0;
    gfx_uniform_t uniforms[32] = {};

    char* blobs[16] = {};
    const char* stages[16] = {};
    int sizes[16] = {};

    int count = 0;
    gfx_shader_stage_data stage_data[16] = {};

    shader_header_t * header = (shader_header_t*)data;
    if(header->magic_ui64 == shader_magic)
    {
        count = header->stagecount;
        char * ptr = data + sizeof(shader_header_t);
        for (int i = 0; i < header->stagecount; ++i)
        {
          gfx_shader_stage stg = *(gfx_shader_stage*)ptr;       ptr += sizeof(uint32_t);
          uint32_t stage_size = *(uint32_t*)ptr;                ptr += sizeof(uint32_t);

          uint32_t*dataptr = (uint32_t*)ptr;

          stage_data[i] = { stg,   (uint32_t*)ptr, (uint32_t)stage_size };
          ptr += stage_size;

          gfx_shader_reflection((char*)stage_data[i].data, stage_data[i].size, &uniforms[uniform_count], &uniform_count);
        }
    }
    else
    {
        count = asset_shader_compile(name, data, size, "spirv", blobs, sizes, stages);
        for (int i = 0; i < count; ++i)
        {
            gfx_shader_reflection(blobs[i], sizes[i], &uniforms[uniform_count], &uniform_count);

            if (stages[i] && !strcmp(stages[i], "vertex"))
                stage_data[i] = { gfx_shader_vertex,   (uint32_t*)blobs[i], (uint32_t)sizes[i] };
            if (stages[i] && !strcmp(stages[i], "fragment"))
                stage_data[i] = { gfx_shader_fragment,   (uint32_t*)blobs[i], (uint32_t)sizes[i] };
            if (stages[i] && !strcmp(stages[i], "compute"))
                stage_data[i] = { gfx_shader_compute,   (uint32_t*)blobs[i], (uint32_t)sizes[i] };
        }
    }

    uniform_count = gfx_merge_uniforms(uniforms, uniform_count);

    gfx_shader_desc_t shader_desc = {};
        shader_desc.label = name? name: "name";
        shader_desc.stages = stage_data;
        shader_desc.stages_count = count;

        shader_desc.uniforms = uniforms;
        shader_desc.uniform_count = uniform_count;

    *out_shader = gfx_create_shader2(ctx, &shader_desc);
}

void load_material_from_file_path(gfx_context_t* ctx, const char* path, struct gfx_material_instance_t** material)
{
    char* data = nullptr;

    size_t size = read_file_data(path, &data);

    if (size != 0)
        load_material_from_file_data(ctx, data, size, material);

    free(data);
}

void load_material_from_file_data(gfx_context_t* ctx, char* data, size_t size, struct gfx_material_instance_t** insance)
{
  //  std::string jstr(data, size);
  //  json::from_json<gfx_material_instance_t>(jstr);
}