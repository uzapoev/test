#ifndef __spirvflect_h__
#define __spirvflect_h__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

//#include <vulkan/vulkan.h>
#include "spirv.h"


#define SPV_MAX_DATA_TYPES          (256)
#define SPV_MAX_UNIFORM_FIELDS      (32)

/*
*   typedef enum spirvflect_uniform_type {
*       spirvflect_buffer,
*       spirvflect_texture,
*       spirvflect_sampler,
*       spirvflect_storage,
* } spirvflect_uniform_type;
* */


typedef struct spirvflect_uniform_t
{
    const char *            name;
    uint32_t                stage_mask;      //  1 << SpvExecutionModel
    uint32_t                binding;
    uint32_t                descriptor_set;

    SpvOp                   type;
    int16_t                 size;
    int16_t                 field_count;

    int16_t                 is_storage;

    struct {
        SpvDim              dimension;
        SpvImageFormat      format;
        SpvAccessQualifier  access;
    } image_info;

    struct {
        SpvAccessQualifier  access;
    } storage;

    struct {
        const char *        name;
        SpvOp               type; 
        uint16_t            count;
        uint16_t            offset;
        uint16_t            size;

        union {
            struct { SpvOp type; uint16_t dim; } vec;
            struct { SpvOp type; uint16_t row, col; } mat;
        }; 
    } fields[SPV_MAX_UNIFORM_FIELDS];
} spirvflect_uniform_t;


typedef struct spirvflect_attribute_t {
    const char *            name;
    SpvOp                   type;
    uint32_t                count;
    uint32_t                binding;
} spirvflect_attribute_t;


typedef struct spirvflect_t
{
    SpvExecutionModel           stage;
    const char *                entry_point;

    uint32_t *                  data;
    uint32_t                    size;

    uint16_t                    attributes_count;
    spirvflect_attribute_t      attributes[16];

    uint16_t                    uniform_count;
    spirvflect_uniform_t        uniforms[16];
} spirvflect_t;


static int         spirvflect_create(const uint32_t* data, uint32_t size, spirvflect_t** spflect);

static void        spirvflect_destroy(spirvflect_t* info);

static uint32_t    spirvflect_merge_uniforms(spirvflect_uniform_t* src_a, uint32_t count_a,
                                      spirvflect_uniform_t* src_b, uint32_t count_b,
                                      spirvflect_uniform_t* dst, uint32_t capacity);

//  internal 

typedef struct spirvflect_variable_t
{
    SpvId                   type_id;
    SpvId                   decor_id;
    SpvStorageClass         storage;
} spirvflect_variable_t;

typedef struct spirvflect_name_t
{
    uint32_t               id;
    int8_t                 name[1];
}spirvflect_name_t;

typedef struct spirvflect_name_member_t
{
    SpvId                   parent_id;
    spirvflect_name_t       value;
}spirvflect_name_member_t;

typedef struct spirvflect_decor_t
{
    SpvId                   id;
    SpvDecoration           decor;
    uint32_t                value;
} spirvflect_decor_t;

typedef struct spirvflect_decor_member_t
{
    SpvId                   parent_id;
    spirvflect_decor_t      value;
} spirvflect_decor_member_t;

typedef struct spirvflect_constant_t
{
    SpvId                   type_id;
    SpvId                   id;
    union { uint32_t ui; float f;} value;
} spirvflect_constant_t;

typedef struct spirvflect_type_t
{
    SpvOp                   type  : 16;        // 
    int32_t                 count : 16;          // if type is struct, this field contain field count( fields = count - 2);
    SpvId                   id;             //
    union  
    {
        struct { uint32_t bit_count;}                   scalar;
        struct { uint32_t type_id, count;}              vector;
        struct { SpvStorageClass id; uint32_t type_id;} pointer;
        struct { uint32_t type_id, constant_id; }       spvarray;
        struct { uint32_t field_ids[32];}               spvstruct;
        struct { SpvId id; SpvDim dim; SpvAccessQualifier accsess; uint32_t unknown[3]; SpvImageFormat format; } texture;
    };
} spirvflect_type_t;



typedef struct sprivflect_info_t
{
   spirvflect_type_t            * types[128]            = { 0 };
   spirvflect_constant_t        * constants[128]        = { 0 };
   spirvflect_name_t            * names[128]            = { 0 };
   spirvflect_name_member_t     * name_members[128]     = { 0 };
   spirvflect_decor_t           * decors[128]           = { 0 };
   spirvflect_decor_member_t    * decors_members[128]   = { 0 };
}sprivflect_info_t;

static spirvflect_constant_t* _find_const(sprivflect_info_t* ctx, uint32_t id)
{
    for (int i = 0; i < _countof(ctx->constants); ++i)
        if (ctx->constants[i] && ctx->constants[i]->id == id)
            return ctx->constants[i];
    return 0;
}

static spirvflect_type_t * _find_type(sprivflect_info_t * ctx, uint32_t id)
{
    for(int i = 0; i < _countof(ctx->types); ++i)
        if(ctx->types[i] && ctx->types[i]->id == id)
            return ctx->types[i];
    return 0;
}

static spirvflect_name_t* _find_name(sprivflect_info_t* ctx, uint32_t id)
{
    for (int i = 0; i < _countof(ctx->names); ++i)
        if (ctx->names[i] && ctx->names[i]->id == id)
            return ctx->names[i];
    return 0;
}

static spirvflect_name_t* _find_child_name(sprivflect_info_t* ctx, uint32_t parent_id, uint32_t id)
{
    for (int i = 0; i < _countof(ctx->name_members); ++i)
        if (ctx->name_members[i] && (ctx->name_members[i]->parent_id == parent_id) && (ctx->name_members[i]->value.id == id))
            return &ctx->name_members[i]->value;
    return 0;
}

static spirvflect_decor_t* _find_decor(sprivflect_info_t* ctx, uint32_t id)
{
    for (int i = 0; i < _countof(ctx->decors); ++i)
        if (ctx->decors[i] && ctx->decors[i]->id == id)
            return ctx->decors[i];
    return 0;
}

static spirvflect_decor_t* _find_with_decor(sprivflect_info_t* ctx, uint32_t id, SpvDecoration decor)
{
    for (int i = 0; i < _countof(ctx->decors); ++i)
        if (ctx->decors[i] &&
            ctx->decors[i]->id == id &&
            ctx->decors[i]->decor == decor)
            return ctx->decors[i];
    return 0;
}

static spirvflect_decor_t* _find_with_decor_in_child(sprivflect_info_t* ctx, uint32_t parent_id, uint32_t id, SpvDecoration decor)
{
    for (int i = 0; i < _countof(ctx->decors_members); ++i)
        if (ctx->decors_members[i] && 
            ctx->decors_members[i]->parent_id == parent_id && 
            ctx->decors_members[i]->value.id == id &&
            ctx->decors_members[i]->value.decor == decor)
            return &ctx->decors_members[i]->value;
    return 0;
}


static void parse_struct(sprivflect_info_t * ctx, spirvflect_type_t* type)
{
    gfx_uniform_t uniform;
    uniform.type = gfx_uniform_storage_buffer;
    uniform.buffer.field_count = type->count - 2; // skip instruction(id(16bit) + count(16bit)) + id(32bit);

    for (uint32_t i = 0; i < uniform.buffer.field_count; ++i)
    {
        uint32_t field_id = type->spvstruct.field_ids[i];
        spirvflect_type_t* spv_type = _find_type(ctx, field_id);
        spirvflect_name_t* spv_name = _find_child_name(ctx, type->id, i);
        auto offset_decor = _find_with_decor_in_child(ctx, type->id, i, SpvDecorationOffset);

        strcpy(uniform.buffer.fields[i].name, (char*)spv_name->name);
    }
}

// return size of struct
// slang save float4x4 matrix as struct of 4 * float4
static int parse_substruct(sprivflect_info_t* ctx, spirvflect_type_t* type)
{
    uint32_t substruct_type_id = type->spvarray.type_id;
    uint32_t struct_size = 0;
    int field_count = type->count - 2;
    for(int i = 0; i < field_count; ++i)
    {
        uint32_t field_id = type->spvstruct.field_ids[i];
        spirvflect_type_t* field_type = _find_type(ctx, field_id);

        if(field_type->type == SpvOpTypeArray)
        {
            auto constv = _find_const(ctx, field_type->spvarray.constant_id);
            int array_length = constv->value.ui;
            auto array_field_type = _find_type(ctx, field_type->spvarray.type_id);

            if(array_field_type->type == SpvOpTypeVector)
            {
                auto array_element_type = _find_type(ctx, array_field_type->vector.type_id);
                
                switch(array_element_type->type)
                {
                    case SpvOpTypeInt:
                    case SpvOpTypeFloat:
                        struct_size += (sizeof(float) * array_field_type->count) * array_length;
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
            else
            {
                assert(false);
            }
        }
        else if (field_type->type == SpvOpTypeRuntimeArray) {
            auto array_field_type = _find_type(ctx, field_type->spvarray.type_id);
        }else{
            assert(false);
        }
    }
    return struct_size;
}

static int spirvflect_create(const uint32_t* data, uint32_t size, spirvflect_t** spvflect)
{
    if (data && *data != SpvMagicNumber)
        return -1;

    spirvflect_t* spvreflect = (spirvflect_t*)calloc(1, sizeof(spirvflect_t));
    if(spvreflect == NULL)
        return -1;

    spvreflect->stage = SpvExecutionModelMax;
    spvreflect->size  = size; 
    spvreflect->data  = (uint32_t*)malloc(size);
    spvreflect->data  = (uint32_t*)memcpy(spvreflect->data, data, size);

    int type_count  = 0, const_count = 0;
    int decor_count = 0, decor_member_count = 0;
    int name_count  = 0, name_member_count = 0;

    sprivflect_info_t ctx = {};

    auto & types           = ctx.types;
    auto & constants       = ctx.constants;
    auto & names           = ctx.names;
    auto & name_members    = ctx.name_members;
    auto & decors          = ctx.decors;
    auto & decors_members  = ctx.decors_members;

    uint32_t* ptr = spvreflect->data + 5; // skip header(SpvMagicNumber, SpvVersion, SpvRevision, SpvOpCodeMask, SpvWordCountShift)
    uint32_t* end = spvreflect->data + size / sizeof(uint32_t);

    for (; ptr < end; ptr += (*ptr >> SpvWordCountShift) & SpvOpCodeMask)
    {   
        SpvOp opcode = (SpvOp)(*ptr & SpvOpCodeMask);
        uint32_t * payload = ptr + 1; // skip 'id and 'word count

        switch(opcode) 
        {
            case SpvOpConstant:       constants[const_count++]             = (spirvflect_constant_t*)payload;     break;
            case SpvOpName:           names[name_count++]                  = (spirvflect_name_t*)payload;         break;
            case SpvOpMemberName:     name_members[name_member_count++]    = (spirvflect_name_member_t*)payload;  break;
            case SpvOpDecorate:       decors[decor_count++]                = (spirvflect_decor_t*)payload;        break;
            case SpvOpMemberDecorate: decors_members[decor_member_count++] = (spirvflect_decor_member_t*)payload; break;

            default: {
                if(opcode == SpvOpEntryPoint){
                    spvreflect->stage = *(SpvExecutionModel*)(payload);
                    spvreflect->entry_point = (const char*)(payload + 2);
                }
                if (opcode >= SpvOpTypeVoid && opcode <= SpvOpTypeForwardPointer)
                    types[type_count++] = (spirvflect_type_t*)ptr;
            } break;
        }

        if(opcode == SpvOpVariable)
        {
            spirvflect_variable_t* var      = (spirvflect_variable_t*)payload;
            spirvflect_type_t* var_type     = _find_type(&ctx, var->type_id);
            spirvflect_name_t* var_name     = _find_name(&ctx, var->decor_id);
            spirvflect_decor_t* var_decor   = _find_decor(&ctx, var->decor_id);
            switch (var->storage)
            {
                default: break;
                case SpvStorageClassStorageBuffer: {

                    auto binding  = _find_with_decor(&ctx, var->decor_id, SpvDecorationBinding);
                    auto set      = _find_with_decor(&ctx, var->decor_id, SpvDecorationDescriptorSet);
                    auto nowritable = _find_with_decor(&ctx, var->decor_id, SpvDecorationNonWritable);
                    auto noreadable = _find_with_decor(&ctx, var->decor_id, SpvDecorationNonReadable);

                    uint32_t base_type_id = var_type->pointer.type_id;
                    spirvflect_type_t* base_type = _find_type(&ctx, base_type_id);

                    spirvflect_uniform_t* uniform = &spvreflect->uniforms[spvreflect->uniform_count];
                    uniform->is_storage = 1;
                    uniform->name = (char*)var_name->name;
                    uniform->type = (SpvOp)base_type->type;
                    uniform->stage_mask = (SpvExecutionModel)(1 << spvreflect->stage);
                    uniform->binding = binding ? binding->value : 0xFFFFFFFF;
                    uniform->descriptor_set = set ? set->value : 0xFFFFFFFF;

                    if(nowritable)
                        uniform->storage.access = SpvAccessQualifierReadOnly;
                    else
                        uniform->storage.access = SpvAccessQualifierReadWrite;

                    parse_substruct(&ctx, base_type);

                    spvreflect->uniform_count++;
                } break;

                case SpvStorageClassInput:
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
                } break;

                case SpvStorageClassUniform:
                case SpvStorageClassUniformConstant:
                {
                    SpvOp _type = (SpvOp)var_type->type;
                    if(var_type && var_type->type == SpvOpTypePointer)
                    {
                        uint32_t id = var_type->pointer.type_id;

                        spirvflect_type_t* base_type = _find_type(&ctx, id);
                        spirvflect_name_t* base_name = _find_name(&ctx, id);

                        auto db = _find_with_decor(&ctx, var->decor_id, SpvDecorationBinding);
                        auto ds = _find_with_decor(&ctx, var->decor_id, SpvDecorationDescriptorSet);

                        spirvflect_uniform_t * uniform = &spvreflect->uniforms[spvreflect->uniform_count];
                        uniform->name           = (char*)var_name->name;
                        uniform->type           = (SpvOp)base_type->type;
                        uniform->stage_mask     = (SpvExecutionModel)(1 << spvreflect->stage);
                        uniform->binding        = db ? db->value : 0xFFFFFFFF;
                        uniform->descriptor_set = ds ? ds->value : 0xFFFFFFFF;

                        if(base_type->type == SpvOpTypeImage)
                        {
                            uniform->image_info.dimension = base_type->texture.dim;
                            uniform->image_info.format = base_type->texture.format;
                            uniform->image_info.access = base_type->texture.accsess;
                        }

                        if(base_type->type == SpvOpTypeStruct)
                        {
                            uint32_t prev_offset = 0;
                            uniform->field_count = base_type->count - 2; // skip instruction(id(16bit) + count(16bit)) + id(32bit);
                            for(int16_t i = 0; i < uniform->field_count; ++i)
                            {
                                uint32_t field_size = 0;

                                uint32_t field_id = base_type->spvstruct.field_ids[i];
                                spirvflect_type_t* orig_type = _find_type(&ctx, field_id);

                                // find member_name
                                spirvflect_name_t*  mn  = _find_child_name(&ctx, id, i);
                                auto offset_decor = _find_with_decor_in_child(&ctx, id, i, SpvDecorationOffset);
                                auto stride_decor = _find_with_decor_in_child(&ctx, id, i, SpvDecorationMatrixStride);

                                uint32_t stride = stride_decor ? stride_decor->value : 0;
                                uniform->fields[i].type   = (SpvOp)orig_type->type;
                                uniform->fields[i].name   = mn ? (char*)mn->name : "";
                                uniform->fields[i].offset = offset_decor ? offset_decor->value : 0;

                                if (orig_type->type == SpvOpTypeVector)
                                {
                                    spirvflect_type_t* element_type = _find_type(&ctx, orig_type->spvarray.type_id);

                                    field_size = 4 * orig_type->vector.count;
                                }
                                if(orig_type->type == SpvOpTypeMatrix)
                                {
                                    spirvflect_type_t* vec_type = _find_type(&ctx, orig_type->vector.type_id);
                                    spirvflect_type_t* element_type = _find_type(&ctx, vec_type->vector.type_id);

                                    uniform->fields[i].mat.type = (SpvOp)element_type->type;
                                    uniform->fields[i].mat.row  = 4;
                                    uniform->fields[i].mat.col  = vec_type->vector.count;

                                    field_size = uniform->fields[i].mat.row * uniform->fields[i].mat.col * 4;
                                }
                                //StructuredBuffer<BufType> Buffer1;
                                if(orig_type->type == SpvOpTypeRuntimeArray)
                                {
                                    uniform->is_storage = true;
                                    uint32_t array_element_type_id = orig_type->spvarray.type_id;
                                    spirvflect_type_t* array_element_type = _find_type(&ctx, array_element_type_id);
                                    spirvflect_name_t* array_element_type_name =  _find_name(&ctx, array_element_type_id);

                                    if(array_element_type->type == SpvOpTypeInt)
                                    {
                                        field_size = sizeof(int);
                                    }
                                    if (array_element_type->type == SpvOpTypeFloat)
                                    {
                                        field_size = sizeof(float);
                                    }

                                    if (array_element_type_name && array_element_type->type == SpvOpTypeStruct)
                                    {
                                        parse_struct(&ctx, array_element_type);
                                    }
                                }
                                if(orig_type->type == SpvOpTypeArray)
                                {
                                    uint32_t array_type_id = orig_type->spvarray.type_id;
                                    uint32_t array_size_id = orig_type->spvarray.constant_id;

                                    spirvflect_constant_t* constant = _find_const(&ctx, array_size_id);
                                    spirvflect_type_t* array_type   = _find_type(&ctx, array_type_id); 
                                    spirvflect_type_t* element_type = _find_type(&ctx, array_type->vector.type_id);

                                    uint32_t array_size = constant->value.ui;

                                    uniform->fields[i].type  = (SpvOp)array_type->type;
                                    uniform->fields[i].count = array_size;
                                    if(element_type == NULL)
                                    {
                                        field_size = 16 * array_size; //warning - do not use scalar in uniforms
                                    }
                                    if(element_type && uniform->fields[i].type == SpvOpTypeVector)
                                    {
                                        uniform->fields[i].vec.type = (SpvOp)element_type->type;
                                        uniform->fields[i].vec.dim = array_type->vector.count;
                                        field_size = uniform->fields[i].vec.dim * array_size * (element_type->scalar.bit_count / 8);
                                    }
                                    if (element_type && uniform->fields[i].type == SpvOpTypeMatrix)
                                    {
                                        uint32_t col = uniform->fields[i].mat.col = (stride > 0) ? stride / 4 : 4;
                                        uint32_t row = uniform->fields[i].mat.row = (stride > 0) ? stride / 4 : 4;
                                        field_size = col * row * 4;
                                    }
                                } 
                                if (orig_type->type == SpvOpTypeStruct)
                                {
                                    field_size = parse_substruct(&ctx, orig_type);
                                }
                                assert(field_size);
                                uniform->size += field_size;//uniform->fields[i].offset - prev_offset;
                                uniform->fields[i].size = field_size;
                                prev_offset = uniform->fields[i].offset;
                            } // for each field
                        }
                    } // if (type && type->type == SpvOpTypePointer)
                    spvreflect->uniform_count++;
                } break;
             } //switch (variable->storage)
        }
    }
    *spvflect = spvreflect;
    return 1;
}

static int spirvflect_cmp_uniform(spirvflect_uniform_t * a, spirvflect_uniform_t * b)
{
    int res = !strcmp(a->name, b->name) &&
               a->binding == b->binding &&
               a->type == b->type &&
               a->binding == b->binding &&
               a->descriptor_set == b->descriptor_set &&
               a->field_count == b->field_count;
    return res;
}

/* return merged uniform count*/
static uint32_t spirvflect_merge_uniforms(spirvflect_uniform_t * src_a, uint32_t count_a,
                                          spirvflect_uniform_t * src_b, uint32_t count_b,
                                          spirvflect_uniform_t * dst, uint32_t capacity)
{
    assert(capacity > (count_a + count_b));
    uint32_t uniform_count = count_a;
    memcpy(dst, src_a, sizeof(spirvflect_uniform_t) * count_a);
    for (uint32_t b = 0; b < count_b; ++b)
    {
        int found_idx = -1;
        for (uint32_t i = 0; i < uniform_count; ++i)
            if (spirvflect_cmp_uniform(&dst[i], &src_b[b]))
                found_idx = i;

        if (found_idx >= 0)
        {
           dst[found_idx].stage_mask = (dst[found_idx].stage_mask | src_b[b].stage_mask);
        }
        else
        {
            memcpy(&dst[uniform_count], &src_b[b], sizeof(spirvflect_uniform_t));
            uniform_count++;
        }
    }
    return uniform_count;
}

static void spirvflect_destroy(spirvflect_t* spvflect)
{
    free(spvflect->data);
    free(spvflect);
}

#endif