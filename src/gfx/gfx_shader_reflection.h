#ifndef __gfx_shader_reflection_h__
#define __gfx_shader_reflection_h__

#include "gfx.h"
#include <stdlib.h>
#include <string.h>

typedef struct gfx_shader_meta_t {
    char                entry_point[64];
    gfx_shader_stage    stage;

    uint32_t            uniform_count;
    gfx_uniform_t *     uniforms;

    // attribute_t       attributes[16]; vertex input atrributes
} gfx_shader_meta_t;




static void gfx_shader_reflection(const char* data, uint32_t size, gfx_shader_meta_t* meta);
static void gfx_shader_get_spirv_reflection(const void* data, uint32_t size, gfx_shader_meta_t* meta);
static void gfx_shader_get_wgsl_reflection(const void* data, uint32_t size, gfx_shader_meta_t* meta);
static void gfx_shader_get_msl_reflection(const void* data, uint32_t size, gfx_shader_meta_t* meta);


static void gfx_shader_reflection2(const char* data, uint32_t size, gfx_uniform_t* out_uniforms, uint32_t* uniforms_count)
{
    static const unsigned int spv_magic = 0x07230203;

    gfx_shader_meta_t meta = {};
    if ((data != nullptr) && (size > 0) && ((uint32_t*)data)[0] == spv_magic)
    {
        gfx_shader_get_spirv_reflection(data, size, &meta);
        memcpy(out_uniforms, meta.uniforms, meta.uniform_count * sizeof(gfx_uniform_t));
        *uniforms_count += meta.uniform_count;
    }
}

static uint32_t gfx_merge_uniforms(gfx_uniform_t* uniforms, uint32_t count)
{
    auto cmp_uniform = [](gfx_uniform_t* a, gfx_uniform_t* b) -> int
        {
            int res = !strcmp(a->name, b->name) &&
                a->binding == b->binding &&
                a->type == b->type &&
                a->group == b->group &&
                !memcmp(&a->buffer, &b->buffer, sizeof(gfx_uniform_t::buffer));
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

static const char* gfx_utils_internal_memstr(const char* buffer, uint32_t buffer_size, const char* substring)
{
    if (!buffer || !substring || buffer_size == 0) return NULL;

    size_t sub_len = strlen(substring);
    if (sub_len > buffer_size) return NULL;

    uint32_t search_limit = buffer_size - (uint32_t)sub_len + 1;
    for (uint32_t i = 0; i < search_limit; ++i) {
        if (buffer[i] == substring[0]) {
            if (memcmp(&buffer[i], substring, sub_len) == 0) {
                return &buffer[i];
            }
        }
    }
    return NULL;
}

static gfx_shader_format_flags gfx_utils_detect_shader_format(const void* shader_data, uint32_t size)
{
    if (!shader_data || size < 4) {
        return gfx_shader_format_unknown;
    }

    const uint32_t* magic = (const uint32_t*)shader_data;
    const char* text = (const char*)shader_data;

    // Check for Khronos SPIR-V standard or byte-swapped magic numbers (0x07230203)
    if (*magic == 0x07230203 || *magic == 0x03022307) {
        return gfx_shader_format_spirv;
    }

    // Check for DXIL containers: 'DXBC' (0x43425844) or raw LLVM bitcode 'BC\xC0\xDE' (0xDEC04342)
    if (*magic == 0x43425844 || *magic == 0xDEC04342) {
        return gfx_shader_format_dxil;
    }

    // Check for Apple Metal precompiled library container: 'MTLB' (0x424C544D)
    if (*magic == 0x424C544D) {
        return gfx_shader_format_msl;
    }

    // Scan for WebGPU Shading Language (WGSL) exclusive decorators and structural keywords
    if (gfx_utils_internal_memstr(text, size, "@vertex") ||
        gfx_utils_internal_memstr(text, size, "@fragment") ||
        gfx_utils_internal_memstr(text, size, "@group") ||
        gfx_utils_internal_memstr(text, size, "fn ")) {
        return gfx_shader_format_wgsl;
    }

    // Scan for your cross-compilation pipeline markers (WSL / Slang / HLSL Source text)
    if (gfx_utils_internal_memstr(text, size, "#pragma vertex") ||
        gfx_utils_internal_memstr(text, size, "#pragma fragment") ||
        gfx_utils_internal_memstr(text, size, "float4")) {
        return gfx_shader_format_hlsl;
    }

    // Scan for your cross-compilation pipeline markers (WSL / Slang / HLSL Source text)
    if (gfx_utils_internal_memstr(text, size, "vec2") ||
        gfx_utils_internal_memstr(text, size, "vec3") ||
        gfx_utils_internal_memstr(text, size, "mat4")) {
        return gfx_shader_format_glsl;
    }

    return gfx_shader_format_unknown;
}

//============================================================================
// SPIRV 
// 
//============================================================================
#include <spirv/unified1/spirv.h>
/*
static const unsigned int SpvOpCodeMask = 0xffff;
static const unsigned int SpvWordCountShift = 16;

typedef unsigned int SpvId;

typedef enum SpvOp_ {
    SpvOpNop = 0,
    SpvOpEntryPoint = 15,
    SpvOpName = 5,
    SpvOpMemberName = 6,
    SpvOpDecorate = 71,
    SpvOpMemberDecorate = 72,
    SpvOpVariable = 59,
    SpvOpConstant = 43,
    SpvOpTypeVoid = 19,
    SpvOpTypeBool = 20,
    SpvOpTypeInt = 21,
    SpvOpTypeFloat = 22,
    SpvOpTypeVector = 23,
    SpvOpTypeMatrix = 24,
    SpvOpTypeImage = 25,
    SpvOpTypePointer = 32,
    SpvOpTypeArray = 28,
    SpvOpTypeRuntimeArray = 29,
    SpvOpTypeStruct = 30,
    SpvOpTypeForwardPointer = 39
} SpvOp;

typedef enum SpvExecutionModel_ {
    SpvExecutionModelVertex = 0,
    SpvExecutionModelFragment = 4,
    SpvExecutionModelCompute = 5,
    SpvExecutionModelRayGeneration = 5313,
    SpvExecutionModelIntersection = 5314,
    SpvExecutionModelAnyHit = 5315,
    SpvExecutionModelClosestHit = 5316,
    SpvExecutionModelMiss = 5317,
    SpvExecutionModelCallable = 5318,
    SpvExecutionModelTask = 5364,
    SpvExecutionModelMesh = 5365,
    SpvExecutionModelMax = 0x7fffffff
} SpvExecutionModel;

typedef enum SpvDecoration_ {
    SpvDecorationNonWritable = 24,
    SpvDecorationNonReadable = 25,
    SpvDecorationBinding = 33,
    SpvDecorationDescriptorSet = 34,
    SpvDecorationOffset = 35,
    SpvDecorationArrayStride = 6,
    SpvDecorationMatrixStride = 7
} SpvDecoration;

typedef enum SpvStorageClass_ {
    SpvStorageClassUniformConstant = 0,
    SpvStorageClassInput = 1,
    SpvStorageClassUniform = 2,
    SpvStorageClassStorageBuffer = 12
} SpvStorageClass;

typedef enum SpvDim_ { SpvDim1D = 0, SpvDim2D = 1, SpvDim3D = 2, SpvDimCube = 3 } SpvDim;
typedef enum SpvImageFormat_ { SpvImageFormatUnknown = 0 } SpvImageFormat;
typedef enum SpvAccessQualifier_ { SpvAccessQualifierReadOnly = 0, SpvAccessQualifierWriteOnly = 1, SpvAccessQualifierReadWrite = 2 } SpvAccessQualifier;
*/

typedef enum SpvExecutionModel_222 {
    //SpvExecutionModelVertex = 0,
    //SpvExecutionModelFragment = 4,
    SpvExecutionModelCompute = 5,
    SpvExecutionModelRayGeneration = 5313,
    SpvExecutionModelIntersection = 5314,
    SpvExecutionModelAnyHit = 5315,
    SpvExecutionModelClosestHit = 5316,
    SpvExecutionModelMiss = 5317,
    SpvExecutionModelCallable = 5318,
    SpvExecutionModelTask = 5364,
    SpvExecutionModelMesh = 5365,
} SpvExecutionModel_22;

typedef struct spirv_variable_t {
    SpvId                   type_id;
    SpvId                   decor_id;
    SpvStorageClass         storage;
} spirv_variable_t;

typedef struct spirv_name_t {
    uint32_t                id;
    int8_t                  name[1];
} spirv_name_t;

typedef struct spirv_name_member_t {
    SpvId                   parent_id;
    spirv_name_t            value;
} spirv_name_member_t;

typedef struct spirv_decor_t {
    SpvId                   id;
    SpvDecoration           decor;
    uint32_t                value;
} spirv_decor_t;

typedef struct spirv_decor_member_t {
    SpvId                   parent_id;
    spirv_decor_t           value;
} spirv_decor_member_t;

typedef struct spirv_constant_t {
    SpvId                   type_id;
    SpvId                   id;
    union { uint32_t ui; float f; } value;
} spirv_constant_t;

typedef struct spirv_type_t {
    SpvOp                   type  : 16;     // 
    int32_t                 count : 16;     // word count, if type is struct, this field contain field count( fields = count - 2);
    SpvId                   id;             //
    union {
        struct { uint32_t bit_count; }                      scalar;
        struct { uint32_t type_id, count; }                 vector;
        struct { SpvStorageClass id; uint32_t type_id; }    pointer;
        struct { uint32_t type_id, constant_id; }           spv_array;
        struct { uint32_t field_ids[64]; }                  spv_struct;
        struct {
            SpvId              id;          // Sampled Type ()
            SpvDim             dim;         // Dimension (2D, 3D, Cube...)
            uint32_t           depth;       // 0: not a depth, 1: depth, 2: don't care
            uint32_t           arrayed;     // 0: normal, 1: array (Texture2DArray)
            uint32_t           ms;          // 0: normal, 1: multisampled (MSAA)
            uint32_t           sampled;     // 1: Texture (Sampled), 2: RWTexture (Storage)
            SpvImageFormat     format;      // 
            SpvAccessQualifier access;      // present if word "count" == 10 and sampled == 2
        } texture;
    };
} spirv_type_t;

typedef struct spriv_info_t {
    uint32_t                type_count;
    spirv_type_t*           types[1024] = { 0 };

    uint32_t                constant_count;
    spirv_constant_t*       constants[1024] = { 0 };

    uint32_t                name_count;
    spirv_name_t*           names[1024] = { 0 };

    uint32_t                name_member_count;
    spirv_name_member_t*    name_members[1024] = { 0 };

    uint32_t                decor_count;
    spirv_decor_t*          decors[1024] = { 0 };

    uint32_t                decor_member_count;
    spirv_decor_member_t*   decor_members[1024] = { 0 };
} spriv_info_t;

static gfx_shader_stage spv_stage_2_gfx(SpvExecutionModel model)
{
    switch (model) {
        case SpvExecutionModelVertex:      return gfx_shader_vertex;
        case SpvExecutionModelFragment:    return gfx_shader_fragment;
        case SpvExecutionModelCompute:   return gfx_shader_compute;

        case SpvExecutionModelTask:      return gfx_shader_amplify;
        case SpvExecutionModelMesh:      return gfx_shader_mesh;

        case SpvExecutionModelRayGeneration: return gfx_shader_rt_raygen;
        case SpvExecutionModelIntersection:  return gfx_shader_rt_intersect;
        case SpvExecutionModelAnyHit:        return gfx_shader_rt_any_hit;
        case SpvExecutionModelClosestHit:    return gfx_shader_rt_closest_hit;
        case SpvExecutionModelMiss:          return gfx_shader_rt_miss;
        case SpvExecutionModelCallable:      return gfx_shader_rt_callable;
        default: assert(false); break;
    }
    return gfx_shader_count;
}

static spirv_constant_t* spirv_find_const(spriv_info_t* ctx, uint32_t id) {
    for (uint32_t i = 0; i < ctx->constant_count; ++i)
        if (ctx->constants[i] && ctx->constants[i]->id == id)
            return ctx->constants[i];
    return nullptr;
}

static spirv_type_t* spirv_find_type(spriv_info_t* ctx, uint32_t id) {
    for (uint32_t i = 0; i < ctx->type_count; ++i)
        if (ctx->types[i] && ctx->types[i]->id == id)
            return ctx->types[i];
    return 0;
}

static spirv_name_t* spirv_find_name(spriv_info_t* ctx, uint32_t id) {
    for (uint32_t i = 0; i < ctx->name_count; ++i)
        if (ctx->names[i] && ctx->names[i]->id == id)
            return ctx->names[i];
    return 0;
}

static spirv_name_t* spirv_find_child_name(spriv_info_t* ctx, uint32_t parent_id, uint32_t id) {
    for (uint32_t i = 0; i < ctx->name_member_count; ++i)
        if (ctx->name_members[i] && (ctx->name_members[i]->parent_id == parent_id) && (ctx->name_members[i]->value.id == id))
            return &ctx->name_members[i]->value;
    return 0;
}

static spirv_decor_t* spirv_find_decor(spriv_info_t* ctx, uint32_t id) {
    for (uint32_t i = 0; i < ctx->decor_count; ++i)
        if (ctx->decors[i] && ctx->decors[i]->id == id)
            return ctx->decors[i];
    return 0;
}

static spirv_decor_t* spirv_find_with_decor(spriv_info_t* ctx, uint32_t id, SpvDecoration decor) {
    for (uint32_t i = 0; i < ctx->decor_count; ++i)
        if (ctx->decors[i] && ctx->decors[i]->id == id && ctx->decors[i]->decor == decor)
            return ctx->decors[i];
    return nullptr;
}

static spirv_decor_t* spirv_find_with_decor_in_child(spriv_info_t* ctx, uint32_t parent_id, uint32_t id, SpvDecoration decor) {
    for (uint32_t i = 0; i < ctx->decor_member_count; ++i)
        if (ctx->decor_members[i] &&
            ctx->decor_members[i]->parent_id == parent_id &&
            ctx->decor_members[i]->value.id == id &&
            ctx->decor_members[i]->value.decor == decor)
            return &ctx->decor_members[i]->value;
    return nullptr;
}

// return sub_struct size
static uint32_t spv_parse_sub_struct(spriv_info_t* spriv_info, spirv_type_t* type, uint32_t owner_id, gfx_uniform_t * uniform, int field_index)
{
    auto* field = &uniform->buffer.fields[field_index];

    uint32_t struct_size = 0;
    for (int16_t i = 0; i < type->count - 2; ++i)
    {
        uint32_t field_id = type->spv_struct.field_ids[i];
        spirv_type_t* field_type = spirv_find_type(spriv_info, field_id);
        spirv_name_t* field_name = spirv_find_child_name(spriv_info, owner_id, i);

        switch(field_type->type)
        {
            case SpvOpTypeInt:
            case SpvOpTypeBool:
            case SpvOpTypeFloat: {
                struct_size += sizeof(float);
            } break;

            case SpvOpTypeVector: {
                spirv_type_t* element_type = spirv_find_type(spriv_info, field_type->vector.type_id);
                uint32_t element_count = field_type->vector.count;
                if(element_type->type == SpvOpTypeFloat || element_type->type == SpvOpTypeInt)
                    struct_size += element_count * sizeof(float);
                else __debugbreak();
            } break;

            case SpvOpTypeArray: {
                spirv_type_t* array_type = spirv_find_type(spriv_info, field_type->spv_array.type_id);
                spirv_constant_t* array_count = spirv_find_const(spriv_info, field_type->spv_array.constant_id);

                uint16_t count = (uint16_t)array_count->value.ui;

                field->count = count;

                if(array_type->type == SpvOpTypeVector) {
                    spirv_type_t* element_type = spirv_find_type(spriv_info, array_type->vector.type_id);
                    uint32_t element_count = array_type->vector.count;

                    if(element_type->type == SpvOpTypeInt) field->type = (element_count <= 2) ? gfx_format_int2 : gfx_format_int4;
                    if(element_type->type == SpvOpTypeFloat) field->type = (element_count <= 2) ? gfx_format_float2 : gfx_format_float4;

                    field->stride = sizeof(float) * ((element_count <= 2) ? 2 : 4);
                }
                else if (array_type->type == SpvOpTypeStruct) {
                    auto* nn = spirv_find_child_name(spriv_info, owner_id, 0);
                    struct_size += spv_parse_sub_struct(spriv_info, array_type, array_type->id, uniform, 10);
                } else {
                    assert(false);
                }
            } break;
        }
    }
    return struct_size;
}

static void gfx_shader_get_spirv_reflection(const void * data, uint32_t size, gfx_shader_meta_t * meta)
{
    if(data == nullptr || size == 0 || meta == nullptr) return;

    spriv_info_t * spriv_info = (spriv_info_t*)calloc(1, sizeof(spriv_info_t));
    if(!spriv_info) return;

    auto& types         = spriv_info->types;
    auto& constants     = spriv_info->constants;
    auto& names         = spriv_info->names;
    auto& name_members  = spriv_info->name_members;
    auto& decors        = spriv_info->decors;
    auto& decors_members= spriv_info->decor_members;

    uint32_t* ptr = (uint32_t*)data + 5; // skip header(SpvMagicNumber, SpvVersion, SpvRevision, SpvOpCodeMask, SpvWordCountShift)
    uint32_t* end = (uint32_t*)data + size / sizeof(uint32_t);

    meta->uniforms = (gfx_uniform_t*)calloc(16, sizeof(gfx_uniform_t));

    for (; ptr < end; ptr += (*ptr >> SpvWordCountShift) & SpvOpCodeMask)
    {
        SpvOp opcode = (SpvOp)(*ptr & SpvOpCodeMask);
        uint32_t* payload = ptr + 1; // skip 'id and 'word count

        if (opcode == 0)
            break;

        switch (opcode)
        {
            case SpvOpConstant:       constants[spriv_info->constant_count++]           = (spirv_constant_t*)payload;     break;
            case SpvOpName:           names[spriv_info->name_count++]                   = (spirv_name_t*)payload;         break;
            case SpvOpMemberName:     name_members[spriv_info->name_member_count++]     = (spirv_name_member_t*)payload;  break;
            case SpvOpDecorate:       decors[spriv_info->decor_count++]                 = (spirv_decor_t*)payload;        break;
            case SpvOpMemberDecorate: decors_members[spriv_info->decor_member_count++]  = (spirv_decor_member_t*)payload; break;

            default: {
                if (opcode == SpvOpEntryPoint) {
                    meta->stage = spv_stage_2_gfx(*(SpvExecutionModel*)(payload));
                    strncpy(meta->entry_point, (const char*)(payload + 2), sizeof(meta->entry_point));
                }
                if (opcode >= SpvOpTypeVoid && opcode <= SpvOpTypeForwardPointer)
                    types[spriv_info->type_count++] = (spirv_type_t*)ptr;
            } break;
        }

        if (opcode == SpvOpVariable)
        {
            spirv_variable_t* var = (spirv_variable_t*)payload;
            spirv_type_t* var_type = spirv_find_type(spriv_info, var->type_id);
            spirv_name_t* var_name = spirv_find_name(spriv_info, var->decor_id);
            spirv_decor_t* var_decor = spirv_find_decor(spriv_info, var->decor_id);
            switch (var->storage)
            {
                default: break;
              /*  case SpvStorageClassInput:
                {
                    uint32_t base_type_id = var_type->pointer.type_id;
                    spirvflect_type_t* base_type = _find_type(&ctx, base_type_id);
                    if (base_type && base_type->type == SpvOpTypeVector)
                    {
                        spirvflect_type_t* element_type = _find_type(&ctx, base_type->vector.type_id);

                        spvreflect->attributes[spvreflect->attributes_count].name = (char*)var_name->name;
                        spvreflect->attributes[spvreflect->attributes_count].type = (SpvOp)element_type->type;
                        spvreflect->attributes[spvreflect->attributes_count].count = (SpvOp)base_type->count;
                        spvreflect->attributes[spvreflect->attributes_count].binding = var_decor->value;
                    }
                    spvreflect->attributes_count++;
                } break;*/

                case SpvStorageClassUniform:
                case SpvStorageClassUniformConstant:
                {
                    SpvOp _type = (SpvOp)var_type->type;
                    if (var_type && var_type->type == SpvOpTypePointer)
                    {
                        uint32_t pointer_id = var_type->pointer.type_id;

                        auto base_type  = spirv_find_type(spriv_info, pointer_id);
                        auto decor_bind = spirv_find_with_decor(spriv_info, var->decor_id, SpvDecorationBinding);
                        auto decor_set  = spirv_find_with_decor(spriv_info, var->decor_id, SpvDecorationDescriptorSet);

                        auto* uniform = &meta->uniforms[meta->uniform_count];
                        strcpy(uniform->name, (char*)var_name->name);
                        uniform->stage_mask = (1 << meta->stage);
                        uniform->binding = decor_bind ? decor_bind->value : 0xFFFFFFFF;
                        uniform->group   = decor_set  ? decor_set->value  : 0xFFFFFFFF;

                        if (base_type->type == SpvOpTypeImage)
                        {
                            auto access = (base_type->count >= 10 && base_type->texture.sampled == 2) ? base_type->texture.access: gfx_access_read;
                            bool arrayed = base_type->texture.arrayed;

                            switch (base_type->texture.dim) {
                                case SpvDim2D:   uniform->texture.dimension = arrayed ? gfx_texture2d : gfx_texture2d_array; break;
                                case SpvDim3D:   assert(!arrayed); uniform->texture.dimension = gfx_texture3d;         break;
                                case SpvDimCube: assert(!arrayed); uniform->texture.dimension = gfx_texture2d_cube;    break;
                                default: __debugbreak(); break;
                            }
                            switch (access) {
                                case SpvAccessQualifierReadOnly:    uniform->texture.access = gfx_access_read;  break;
                                case SpvAccessQualifierWriteOnly:   uniform->texture.access = gfx_access_write; break;
                                case SpvAccessQualifierReadWrite:   uniform->texture.access = gfx_access_rw;    break;
                                default: __debugbreak(); break;
                            }
                            switch (uniform->texture.dimension) {
                                 case gfx_texture2d:        uniform->type = gfx_uniform_texture2d; break;
                                 case gfx_texture2d_array:  uniform->type = gfx_uniform_texture2d_array; break;
                                 case gfx_texture2d_cube:   uniform->type = gfx_uniform_texture2d_cube; break;
                                 case gfx_texture3d:        uniform->type = gfx_uniform_texture3d; break;
                            }
                        }

                        if (base_type->type == SpvOpTypeStruct)
                        {
                            uniform->type = gfx_uniform_ubo;
                            uniform->buffer.field_count = base_type->count - 2; // skip instruction(id(16bit) + count(16bit)) + id(32bit);
                            for (int16_t i = 0; i < uniform->buffer.field_count; ++i)
                            {
                                uint32_t field_id = base_type->spv_struct.field_ids[i];
                                spirv_type_t* field_type = spirv_find_type(spriv_info, field_id);
                                spirv_name_t* filed_name = spirv_find_child_name(spriv_info, pointer_id, i);

                                auto offset_decor  = spirv_find_with_decor_in_child(spriv_info, pointer_id, i, SpvDecorationOffset);
                                auto astride_decor = spirv_find_with_decor_in_child(spriv_info, pointer_id, i, SpvDecorationArrayStride);
                                auto mstride_decor = spirv_find_with_decor_in_child(spriv_info, pointer_id, i, SpvDecorationMatrixStride);

                                strcpy(uniform->buffer.fields[i].name, filed_name ? (char*)filed_name->name : "");

                                uint32_t stride = mstride_decor ? mstride_decor->value : 0;

                                uniform->buffer.fields[i].offset = offset_decor ? offset_decor->value : 0;

                                switch(field_type->type) {
                                    case SpvOpTypeInt: {
                                        uniform->buffer.fields[i].count = 1;
                                        uniform->buffer.fields[i].stride = sizeof(int);
                                    } break;
                                    case SpvOpTypeVector: {
                                        spirv_type_t* element_type = spirv_find_type(spriv_info, field_type->vector.type_id);
                                        uint32_t count = field_type->vector.count;
                                        switch(element_type->type) {
                                            case SpvOpTypeBool:
                                            case SpvOpTypeInt:   uniform->buffer.fields[i].type = (count <= 2) ? gfx_format_int2 : gfx_format_int4; break;
                                            case SpvOpTypeFloat: uniform->buffer.fields[i].type = (count <= 2) ? gfx_format_float2: gfx_format_float4; break;
                                        } 
                                        uniform->buffer.fields[i].count = 1; 
                                        uniform->buffer.fields[i].stride = sizeof(float) * ((count <= 2) ? 2 : 4);
                                    } break;

                                    case SpvOpTypeMatrix: {
                                        spirv_type_t* element_type = spirv_find_type(spriv_info, field_type->vector.type_id);
                                        uniform->buffer.fields[i].type = gfx_format_float4;
                                        uniform->buffer.fields[i].count = 4;
                                        uniform->buffer.fields[i].stride = mstride_decor ? mstride_decor->value: 16;
                                    } break;
                                
                                    case SpvOpTypeStruct: { 
                                        spv_parse_sub_struct(spriv_info, field_type, pointer_id, uniform, i);
                                    } break;

                                    //StructuredBuffer<MaterialProps> b_materials : register(t0);
                                    case SpvOpTypeRuntimeArray: {
                                        uniform->type = gfx_uniform_storage_buffer;
                                        auto array_field_type = spirv_find_type(spriv_info, field_type->spv_array.type_id);
                                        if(array_field_type->type == SpvOpTypeStruct) {
                                            spv_parse_sub_struct(spriv_info, array_field_type, array_field_type->id, uniform, i);
                                        }
                                  //      __debugbreak();
                                    } break;

                                    default: { 
                                        assert(false); 
                                    } break; // todo
                                }
                                uniform->buffer.size += uniform->buffer.fields[i].stride * uniform->buffer.fields[i].count;
                            } // for each field
                            uniform->buffer.size = (uniform->buffer.size + 15) & ~15; // alignup
                        } // if (type && type->type == SpvOpTypeStruct)
                    } // if (var_type && var_type->type == SpvOpTypePointer)
                    meta->uniform_count++;
                } break;

                case SpvStorageClassStorageBuffer: {
                    auto base_type = spirv_find_type(spriv_info, var_type->pointer.type_id);
                    auto binding = spirv_find_with_decor(spriv_info, var->decor_id, SpvDecorationBinding);
                    auto set = spirv_find_with_decor(spriv_info, var->decor_id, SpvDecorationDescriptorSet);
                    auto nowritable = spirv_find_with_decor(spriv_info, var->decor_id, SpvDecorationNonWritable);
                    auto noreadable = spirv_find_with_decor(spriv_info, var->decor_id, SpvDecorationNonReadable);

                    gfx_uniform_t* uniform = &meta->uniforms[meta->uniform_count];
                    strcpy(uniform->name, (char*)var_name->name);

                    //uniform->type = (SpvOp)base_type->type;
                    uniform->stage_mask = (SpvExecutionModel)(1 << meta->stage);
                    uniform->binding = binding ? binding->value : 0xFFFFFFFF;
                    uniform->group = set ? set->value : 0xFFFFFFFF;
                    uniform->storage.access = nowritable ? gfx_access_read : gfx_access_rw;
                    __debugbreak();
                    meta->uniform_count++;
                } break;
            } //switch (variable->storage)
        }
    }
    free(spriv_info);
}


//---------------------------------------------
//          msl( metal shader language )
// 
//---------------------------------------------
static void gfx_shader_get_msl_reflection(const void* data, uint32_t size, gfx_shader_meta_t* meta)
{
}


//============================================================================
//  WGSL( webgpu shader language )
// 
//============================================================================

static uint16_t wgsl_parse_attribute_value(const char* start_key)
{
    while (*start_key && *start_key != '(') start_key++;
    if (*start_key == '(') start_key++;
    while (*start_key && isspace((unsigned char)*start_key)) start_key++;
    return (uint16_t)strtoul(start_key, NULL, 10);
}

// ============================================================================
// Custom Structure Data Definitions (Used inside uniform/storage blocks)
// ============================================================================
// struct ModelData {
//      matrix: mat4x4<f32>,
//      color : vec4<f32>,
// };
// 
// struct Particle {
//      position: vec3<f32>,
//      velocity: vec3<f32>,
// };
// 
// ============================================================================
// Hardware Resource Declarations (Uniforms, Images, Samplers)
// ============================================================================
// 
// @group(0) @binding(0) var<uniform> global_constants : ModelData;                             // Standard Uniform Buffer Object (UBO)
// @group(0) @binding(1) var<storage, read> dynamic_mesh_vertices : array<vec4<f32>>;           // Storage Buffer Object (SSBO) - Read-Only Access
// @group(0) @binding(2) var<storage, read_write> particle_simulation_pool : array<Particle>;   // Storage Buffer Object (SSBO) - Read-Write Access
// @group(1) @binding(0) var linear_clamp_sampler : sampler;                                    // Standard 2D Texture Sampler State Register
// @group(1) @binding(1) var albedo_texture_2d : texture_2d<f32>;                               // Classic 2D Material Albedo Diffuse Texture Map
// @group(1) @binding(2) var volumetric_density_texture_3d : texture_3d<f32>;                   // 3D Volumetric Density Sampling Map (Voxel fields)
// @group(1) @binding(3) var environment_skybox_texture_cube : texture_cube<f32>;               // Cube Map Environment Lookup Skybox Map
// @group(1) @binding(4) var shadow_cascades_texture_2d_array : texture_2d_array<f32>;          // Texture 2D Array Container (e.g., Terrain texture sheets or cascade shadow maps)
static void gfx_shader_get_wgsl_reflection(const void* data, uint32_t size, gfx_shader_meta_t* meta)
{
    if (!data || size == 0 || meta == nullptr) return;

    uint32_t count = 0;
    const char* ptr = (const char*)data;
    const char* end_of_data = ((const char*)data) + size;

    auto out_uniforms = meta->uniforms;

    while (ptr < end_of_data)
    {
        // Find variable declaration by "var" token. 
        // This is highly reliable since @group and @binding are always attached to a variable.
        ptr = strstr(ptr, "var");
        if (!ptr) break;

        // Ensure "var" is a whole token and not part of an identifier (e.g., my_var)
        if (ptr > data && (isalnum((unsigned char)ptr[-1]) || ptr[-1] == '_')) {
            ptr += 3; // Skip "var" and advance
            continue;
        }

        // Locate the statement terminator semicolon
        const char* stmt_end = strchr(ptr, ';');
        if (!stmt_end || stmt_end >= end_of_data) break;

        // Rollback from "var" to locate @group and @binding attributes associated with it.
        // Limit search bounds to 128 bytes backwards to prevent scanning the entire file.
        const char* search_start = (ptr - 128 < (const char*)data) ? (const char*)data : ptr - 128;

        const char* group_ptr = strstr(search_start, "@group");
        const char* bind_ptr = strstr(search_start, "@binding");

        // If the variable has no hardware layout bindings (e.g., local variable), skip it
        if (!group_ptr || group_ptr > stmt_end || !bind_ptr || bind_ptr > stmt_end) {
            ptr = stmt_end + 1;
            continue;
        }

        // Parse and assign hardware layout register slots
        out_uniforms[count].group = wgsl_parse_attribute_value(group_ptr);
        out_uniforms[count].binding = wgsl_parse_attribute_value(bind_ptr);

        // Advance past "var" token to parse the variable identifier name and type classification
        const char* var_content = ptr + 3;
        while (var_content < stmt_end && isspace((unsigned char)*var_content)) var_content++;

        // Step 1: Detect address space container block type (<uniform> or <storage>)
        gfx_uniform_type detected_type = gfx_uniform_ubo; // Default classification

        if (*var_content == '<') {
            if (strncmp(var_content, "<uniform>", 9) == 0) {
                detected_type = gfx_uniform_ubo;
                var_content += 9;
            }
            else if (strncmp(var_content, "<storage", 8) == 0) {
                detected_type = gfx_uniform_storage_buffer;
                // Skip inner access qualifiers (e.g., <storage, read> or <storage, read_write>)
                while (var_content < stmt_end && *var_content != '>') var_content++;
                if (*var_content == '>') var_content++;
            }
        }

        while (var_content < stmt_end && isspace((unsigned char)*var_content)) var_content++;

        // Step 2: Extract variable name using the colon separator position
        const char* colon = strchr(var_content, ':');
        if (colon && colon < stmt_end) {
            size_t name_len = colon - var_content;
            // Trim trailing spaces before the colon symbol
            while (name_len > 0 && isspace((unsigned char)var_content[name_len - 1])) name_len--;

            if (name_len >= sizeof(out_uniforms[count].name)) {
                name_len = sizeof(out_uniforms[count].name) - 1;
            }
            memcpy(out_uniforms[count].name, var_content, name_len);
            out_uniforms[count].name[name_len] = '\0';

            // Step 3: Classify the specific hardware resource type after the colon
            const char* type_ptr = colon + 1;
            while (type_ptr < stmt_end && isspace((unsigned char)*type_ptr)) type_ptr++;

            // Match against known WGSL resource type tokens
            if (strncmp(type_ptr, "sampler", 7) == 0) {
                detected_type = gfx_uniform_sampler;
            }
            else if (strncmp(type_ptr, "texture_2d", 10) == 0) {
                detected_type = gfx_uniform_texture2d;
            }
            else if (strncmp(type_ptr, "texture_3d", 10) == 0) {
                detected_type = gfx_uniform_texture3d;
            }
            else if (strncmp(type_ptr, "texture_cube", 12) == 0) {
                detected_type = gfx_uniform_texture2d_cube;
            }
            else if (strncmp(type_ptr, "texture_2d_array", 16) == 0) {
                detected_type = gfx_uniform_texture2d_array;
            }
            else {
                // If it's a custom user struct token (UBO), keep GFX_UNIFORM_TYPE_BUFFER
            }
        }

        out_uniforms[count].type = detected_type;
        count++;

        // Shift tracking pointer past the current semicolon statement block
        ptr = stmt_end + 1;
    }

    meta->uniform_count = count;
}
#endif