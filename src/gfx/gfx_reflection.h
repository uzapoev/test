#ifndef __gfx_reflection_h__
#define __gfx_reflection_h__

#include "gfx.h"
#include "spirvflect.h"

/*
static const unsigned int SPVMagicNumber = 0x07230203;
static const unsigned int SPVVersion = 0x00010600;
static const unsigned int SPVRevision = 1;
static const unsigned int SPVOpCodeMask = 0xffff;
static const unsigned int SPVWordCountShift = 16;

enum  _spvops
{
    SpvOpName = 5,
    SpvOpMemberName = 6,
    SpvOpEntryPoint = 15,
    SpvOpTypeVoid = 19,
    SpvOpTypeForwardPointer = 39,
    SpvOpDecorate = 71,
    SpvOpMemberDecorate = 72,
    SpvOpConstant = 43,
    SpvOpVariable = 59,

    SpvStorageClassUniformConstant = 0,
    SpvStorageClassInput = 1,
    SpvStorageClassUniform = 2,
    SpvStorageClassOutput = 3,
};

typedef struct spirv_variable_t
{
    uint16_t    type_id;
    uint16_t    decor_id;
    uint16_t    storage; //SpvStorageClass
} spirv_variable_t;
*/

static gfx_access_type spv_2_gfx_access(SpvAccessQualifier  access) {
    switch (access) {
        case SpvAccessQualifierReadOnly:    return gfx_access_read;
        case SpvAccessQualifierWriteOnly:   return gfx_access_write;
        case SpvAccessQualifierReadWrite:   return gfx_access_rw;
        default:                            return gfx_access_read;
    }
}


static void reflect_spirv(const char* data, uint32_t size, gfx_uniform_t* out_uniforms, uint32_t* uniforms_count)
{
    spirvflect_t* spvflect = nullptr;
    spirvflect_create((uint32_t*)data, size, &spvflect);

    *uniforms_count += spvflect->uniform_count;
    for(uint32_t i = 0; i < spvflect->uniform_count; ++i)
    {
        strcpy(out_uniforms[i].name, spvflect->uniforms[i].name);
        out_uniforms[i].binding = spvflect->uniforms[i].binding;
        out_uniforms[i].stage_mask  = spvflect->uniforms[i].stage_mask;

        switch (spvflect->uniforms[i].type)
        {
            default: break;
            case SpvOpTypeStruct: {

                out_uniforms[i].type = spvflect->uniforms[i].is_storage ? gfx_uniform_storage :
                                                                          gfx_uniform_ubo;

                out_uniforms[i].storage.access = spv_2_gfx_access(spvflect->uniforms[i].storage.access);


                out_uniforms[i].buffer.size = spvflect->uniforms[i].size;
                out_uniforms[i].buffer.field_count = spvflect->uniforms[i].field_count;
                for (uint32_t j = 0; j < out_uniforms[i].buffer.field_count; ++j)
                {
                    strcpy(out_uniforms[i].buffer.fields[j].name, spvflect->uniforms[i].fields[j].name);
                    out_uniforms[i].buffer.fields[j].offset = spvflect->uniforms[i].fields[j].offset;
                    out_uniforms[i].buffer.fields[j].stride = spvflect->uniforms[i].fields[j].size;
                }
            } break;

            case SpvOpTypeSampler: {
                out_uniforms[i].type = gfx_uniform_sampler;
            } break;

            case SpvOpTypeImage: { 
                out_uniforms[i].texture.access = spv_2_gfx_access(spvflect->uniforms[i].image_info.access);
                switch (spvflect->uniforms[i].image_info.dimension)
                {
                 // case     SpvDim1D:  out_uniforms[i].type = gfx_uniform_texture1d; break;
                    case     SpvDim2D:  out_uniforms[i].type = gfx_uniform_texture2d; 
                                        out_uniforms[i].texture.dimension = gfx_texture2d; break;

                    case     SpvDim3D:  out_uniforms[i].type = gfx_uniform_texture3d; 
                                        out_uniforms[i].texture.dimension = gfx_texture3d; break;

                    case     SpvDimCube:out_uniforms[i].type = gfx_uniform_texture2d_cube;
                                        out_uniforms[i].texture.dimension = gfx_texture2d_cube; break;

                    default: assert(false); break;
                }

            } break;
        }
      }
}

static void reflect_msl(const char* data, size_t size, gfx_uniform_t* out_uniforms, uint32_t* uniforms_count)
{
}

static int match(const char* pattern, const char* str, const char** out) throw()
{
    if (!pattern || !str) return 0;
    while (str && *str != '\0' && *str == ' ') str++;
    if (out) *out = str;
    if (*pattern == '\0') return 1;//*str;
    if (*pattern == '*')  return match(pattern + 1, str, out) | (*str & match(pattern, str + 1, out));
    if (*pattern == '?')  return *str & (*str != '.') & match(pattern + 1, str + 1, out);
    return (*str == *pattern) & match(pattern + 1, str + 1, out);
}


static void reflect_wgsl(const char* data, size_t size, gfx_uniform_t* out_uniforms, uint32_t* uniforms_count)
{
    const char k_group_key[] = "@group(";
    const char k_binding_key[] = "@binding(";
    const size_t k_group_key_len = sizeof(k_group_key) - 1;
    const size_t k_binding_key_len = sizeof(k_binding_key) - 1;

    uint32_t count = 0;
    const char* ptr = data;
    while (ptr)
    {
        char buffer[512] = {};
        const char* start_group = strstr(ptr, k_group_key);
        const char* end_group = start_group ? strstr(start_group, ";") : nullptr;

        const char* start_bind = strstr(ptr, k_binding_key);
        const char* end_bind = start_group ? strstr(start_group, ";") : nullptr;

        if (start_group == nullptr || start_bind == nullptr)
            break;
        if (end_group != end_bind)
            break;

        const char* start = start_group < start_bind ? start_group : start_bind;
        const char* end = end_group;

        intptr_t len = end - start;
        strncpy(buffer, start, len);
        ptr = end;

        out_uniforms[count].group = atoi(start_group + k_group_key_len);
        out_uniforms[count].binding = atoi(start_bind + k_binding_key_len);

        const char* last = nullptr;
        const char* var = strstr(buffer, "var");
        if (var && match("var<uniform>", var, &last)) {

        }
        else if (var && match("var <storage, read>", buffer, &last)) {
            out_uniforms[count].type = gfx_uniform_storage;
        }
        else if (var && match("var <storage", buffer, &last)) {
            out_uniforms[count].type = gfx_uniform_storage;
        }
        else if (var && strstr(var, "var ")) {
            const char* dblpoint = strstr(buffer, ":");
            strncpy(out_uniforms[count].name, var + sizeof("var"), dblpoint - var - sizeof("var"));
            dblpoint++;
            while (dblpoint && *dblpoint == ' ') dblpoint++;

            // https://www.w3.org/TR/WGSL/#sampled-texture-type
            if (dblpoint && !strncmp(dblpoint, "sampler", 7)) {
                out_uniforms[count].type = gfx_uniform_sampler;
            }
            else if (dblpoint && !strncmp(dblpoint, "texture_2d", strlen("texture_2d"))) {
                out_uniforms[count].type = gfx_uniform_texture2d;
            }
            else if (dblpoint && !strncmp(dblpoint, "texture_3d", strlen("texture_3d"))) {
                out_uniforms[count].type = gfx_uniform_texture3d;
            }
            else if (dblpoint && !strncmp(dblpoint, "texture_cube", strlen("texture_cube"))) {
                out_uniforms[count].type = gfx_uniform_texture2d_cube;
            }
            else if (dblpoint && !strncmp(dblpoint, "texture_2d_array", strlen("texture_2d_array"))) {
                out_uniforms[count].type = gfx_uniform_texture2d_array;
            }
            else {
                assert(false);
            }

        }
        count++;
    }

    *uniforms_count = count;
}


uint32_t gfx_merge_uniforms(gfx_uniform_t* uniforms, uint32_t count)
{
    auto cmp_uniform = [](gfx_uniform_t* a, gfx_uniform_t* b) -> int
        {
            int res = !strcmp(a->name, b->name) &&
                a->binding == b->binding &&
                a->type == b->type &&
                a->binding == b->binding &&
                !memcmp(&a->buffer, &b->buffer, sizeof(gfx_uniform_t::buffer));
         //       a->field_count == b->field_count;
            return res;
        };

    for (uint32_t i = 0; i < count; ++i)
    {
        for (uint32_t j = i + 1; j < count; ++j)
        {
            if (cmp_uniform(&uniforms[i], &uniforms[j]))
            {
                uniforms[i].stage_mask = uniforms[i].stage_mask | uniforms[j].stage_mask;
                memcpy(&uniforms[j], &uniforms[count - 1], sizeof(gfx_uniform_t));
                memset(&uniforms[count - 1], 0, sizeof(gfx_uniform_t));
                count--;
            }
        }
    }
    return count;
}

static void gfx_shader_reflection(const char* data, uint32_t size, gfx_uniform_t* out_uniforms, uint32_t* uniforms_count)
{
    if((data != nullptr) && (size > 0) && ((uint32_t*)data)[0] == SpvMagicNumber)
        reflect_spirv(data, size, out_uniforms, uniforms_count);
}



#endif