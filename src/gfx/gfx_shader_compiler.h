#ifndef __gfx_shader_compiler_h__
#define __gfx_shader_compiler_h__

#include <stdint.h> // uintXX_t 
#include <stdlib.h> // calloc/free
#include <stdio.h>  // printf
#include <string.h> // memset

#include "gfx.h"

typedef enum shader_compile_target {
    shader_target_spirv,    // vulkan/dx12
    shader_target_msl,      // metal
    shader_target_wgsl,     // webgpu
    shader_target_glsl,     // rip
    shader_target_dxil      // dx11/dx12
} shader_compile_target;

typedef enum shader_compile_option {
    shader_compile_option_default      = 0,     // find multicompile pragma, find entry point by pragmas
    shader_compile_option_invert_y     = 1 << 0,
} shader_compile_option;


typedef struct gfx_shader_compiler_stage_t {
    gfx_shader_stage        stage;
    char                    entry[64];      // entry point name
    uint32_t                size;
    char*                   data;
} gfx_shader_compiler_stage_t;


typedef struct gfx_shader_compiler_result_t {
    uint32_t                keyword_count;
    char*                   keywords[64];

    uint32_t                        blob_count;
    gfx_shader_compiler_stage_t*    blobs;

    // reflection of the target (Slang program layout); stage_mask = stages that really use the resource,
    // resources no stage uses are dropped.
    uint32_t                uniform_count;
    gfx_uniform_t*          uniforms;
} gfx_shader_compiler_result_t;


typedef struct gfx_shader_compiler_request_desc_t {
    const char*             name;
    const char*             data;
    uint32_t                size;
    shader_compile_target   target;

    uint32_t                options;

    uint32_t                define_count;
    const char**            defines;

    uint32_t                search_include_path_count;
    const char**            search_include_paths;
} gfx_shader_compiler_request_desc_t;

struct gfx_shader_compiler_context_t;

extern "C" void gfx_shader_compiler_context_create(uint64_t options, gfx_shader_compiler_context_t ** compiler_context);
extern "C" void gfx_shader_compiler_context_destroy(gfx_shader_compiler_context_t * compiler_context);

// Returns 1 on success: free the result with gfx_shader_compiler_result_free. Returns 0 on failure, nothing to free.
extern "C" int gfx_compile_shader(gfx_shader_compiler_context_t * context, gfx_shader_compiler_request_desc_t * desc, gfx_shader_compiler_result_t * result);
extern "C" void gfx_shader_compiler_result_free(gfx_shader_compiler_result_t * result);
//extern "C" int gfx_compile_shader(gfx_shader_compiler_context_t * context, const char* name, const char* data, uint32_t size, shader_compile_target target, uint64_t options, gfx_shader_compiler_result_t * result);


#if defined (GFX_SHADER_COMPILER_IMPL)

extern "C" void gfx_shader_compiler_result_free(gfx_shader_compiler_result_t * result)
{
    if (result == nullptr) return;
    for (uint32_t i = 0; i < result->blob_count; ++i)
        free(result->blobs[i].data);
    free(result->blobs);
    free(result->uniforms);
    memset(result, 0, sizeof(*result));
}

#if __has_include(<slang/slang.h>)
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

#if defined(_DEBUG) && defined(_WIN32)
    #pragma comment(lib, "lib/slang.lib")
#elif defined(_WIN32)
    #pragma comment(lib, "lib/slang.lib")
#endif



static const char* pragma_entry_point_vertex_name     = (char*)"vertex";
static const char* pragma_entry_point_fragment_name   = (char*)"fragment";
static const char* pragma_entry_point_compute_name    = (char*)"compute";

typedef struct gfx_shader_compiler_context_t {
    SlangSession*               global_session;
    //SlangSourceArtifac*   slang_cache;
} gfx_shader_compiler_context_t;


// #pragma vertex vsmain
// #pragma fragment psmain
// #pragma compute csmain
static const char* _find_pragma_entry_point(const char* data, const char* stage_name, char* entry_buffer, size_t entry_buffer_size)
{
    if ((entry_buffer == nullptr) || (entry_buffer_size == 0))
        return entry_buffer;

    // const char * pragma_stage = s_stage_pragmas[stage];
    static const char pragma_str[] = "#pragma";
    size_t pragma_stage_len = strlen(stage_name);
    while (data && data[0] != '\0')
    {
        data = strstr(data, pragma_str);
        if (data)
        {
            data += sizeof(pragma_str);
            while (isspace(*data)) data++;
            if (!strncmp(data, stage_name, pragma_stage_len))
            {
                data += strlen(stage_name);
                while (isspace(*data)) data++;
                break;
            }
        }
    }

    memset(entry_buffer, 0, entry_buffer_size);
    if ((data != nullptr) && (entry_buffer != nullptr) && (entry_buffer_size > 0))
    {
        int i = 0;
        while (isprint(data[i]) && i < entry_buffer_size)
        {
            entry_buffer[i] = data[i];
            i++;
        }
        return entry_buffer;
    }

    return nullptr;
}


//#pragma multicompile KEYWORD1 KEYWORD2 KEYWORD3
// -
// KEYWORD1
// KEYWORD2
// KEYWORD3
// KEYWORD1 KEYWORD2
// KEYWORD1 KEYWORD2 KEYWORD3
// KEYWORD1 KEYWORD3
// KEYWORD2 KEYWORD3

// #pragma multicompile USE_ALBEDO USE_NORMALMAP
static void _find_pragma_multicompile(const char* data, char keywords[32][64], int* keyword_count)
{
    *keyword_count = 0;
    const char* pragma_mc = "#pragma multicompile";
    const char* line = strstr(data, pragma_mc);

    if (!line) return;

    line += strlen(pragma_mc);
    while (isspace(*line) && *line != '\n') line++;

    while (*line != '\n' && *line != '\0' && *keyword_count < 32)
    {
        int char_idx = 0;
        while (!isspace(*line) && *line != '\0' && char_idx < 63)
        {
            keywords[*keyword_count][char_idx++] = *line++;
        }
        keywords[*keyword_count][char_idx] = '\0';
        if (char_idx > 0) (*keyword_count)++;
        while (isspace(*line) && *line != '\n') line++;
    }
}

// ---- reflection from the Slang program layout ----

static gfx_format _slang_scalar_format(slang::TypeReflection::ScalarType st, uint32_t n)
{
    switch (st) {
        case slang::TypeReflection::ScalarType::Int32:
        case slang::TypeReflection::ScalarType::Bool:    return n <= 2 ? gfx_format_int2  : gfx_format_int4;
        case slang::TypeReflection::ScalarType::UInt32:  return n <= 2 ? gfx_format_uint2 : gfx_format_uint4;
        default:                                         return n <= 1 ? gfx_format_float1 : (n == 2 ? gfx_format_float2 : gfx_format_float4);
    }
}

// Same conventions as the SPIR-V reflection the backends were written against:
// vector: count 1, stride 8/16; matrix: float4 x rows; array: count = elements, stride = array stride.
static void _slang_fill_field(slang::TypeLayoutReflection* tl, const char* name, uint32_t offset, gfx_uniform_t* u, uint32_t field_idx)
{
    auto* field = &u->buffer.fields[field_idx];
    memset(field, 0, sizeof(*field));
    strncpy(field->name, name ? name : "", sizeof(field->name) - 1);
    field->offset = (uint16_t)offset;

    const uint32_t size = (uint32_t)tl->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM);
    switch (tl->getKind())
    {
        case slang::TypeReflection::Kind::Scalar:
            field->type = _slang_scalar_format(tl->getScalarType(), 1);
            field->count = 1;
            field->stride = (uint16_t)size;
            break;

        case slang::TypeReflection::Kind::Vector: {
            uint32_t n = (uint32_t)tl->getElementCount();
            field->type = _slang_scalar_format(tl->getElementTypeLayout()->getScalarType(), n);
            field->count = 1;
            field->stride = (uint16_t)(n <= 2 ? 8 : 16);
        } break;

        case slang::TypeReflection::Kind::Matrix: {
            uint32_t rows = tl->getRowCount() ? tl->getRowCount() : 1;
            field->type = gfx_format_float4;
            field->count = (uint16_t)rows;
            field->stride = (uint16_t)(size / rows);
        } break;

        case slang::TypeReflection::Kind::Array: {
            slang::TypeLayoutReflection* el = tl->getElementTypeLayout();
            uint32_t n = (uint32_t)tl->getElementCount();
            switch (el->getKind()) {
                case slang::TypeReflection::Kind::Scalar: field->type = _slang_scalar_format(el->getScalarType(), 1); break;
                case slang::TypeReflection::Kind::Vector: field->type = _slang_scalar_format(el->getElementTypeLayout()->getScalarType(), (uint32_t)el->getElementCount()); break;
                default:                                  field->type = gfx_format_float4; break;
            }
            field->count = (uint16_t)n;
            field->stride = (uint16_t)tl->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM);
        } break;

        default: // structs and anything else: raw float4 rows, still writable through the field offset
            field->type = gfx_format_float4;
            field->stride = 16;
            field->count = (uint16_t)((size + 15) / 16);
            break;
    }
}

// Fields of a constant buffer element struct.
static void _slang_fill_ubo_fields(const char* shader_name, slang::TypeLayoutReflection* element, gfx_uniform_t* u)
{
    const uint32_t max_fields = (uint32_t)(sizeof(u->buffer.fields) / sizeof(u->buffer.fields[0]));
    uint32_t count = element ? element->getFieldCount() : 0;
    if (count > max_fields) {
        printf("\n[shader compiler] %s: '%s' has %u fields, only %u are reflected", shader_name, u->name, count, max_fields);
        count = max_fields;
    }
    for (uint32_t i = 0; i < count; ++i) {
        slang::VariableLayoutReflection* f = element->getFieldByIndex(i);
        _slang_fill_field(f->getTypeLayout(), f->getName(), (uint32_t)f->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM), u, i);
    }
    u->buffer.field_count = (uint16_t)count;
    u->buffer.size = (uint16_t)((element ? element->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM) + 15 : 15) & ~(size_t)15);
}

typedef struct _slang_location_t {
    SlangParameterCategory  category;
    uint32_t                space;
    uint32_t                index;
} _slang_location_t;

// Program layout -> gfx_uniform_t list (stage_mask left 0) + where each one lives, for the usage query.
static uint32_t _slang_reflect_layout(const char* shader_name, slang::ProgramLayout* layout, shader_compile_target target,
                                      gfx_uniform_t* uniforms, _slang_location_t* locations, uint32_t max_uniforms)
{
    uint32_t count = 0;
    // one shared binding space (Vulkan/WebGPU) or the D3D/Metal register classes (b/t/s/u, Metal buffer/texture/sampler)
    const bool descriptor_slots = target == shader_target_spirv || target == shader_target_wgsl || target == shader_target_glsl;
    const SlangParameterCategory cb_category = descriptor_slots ? SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT : SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER;
    const uint32_t max_fields = (uint32_t)(sizeof(uniforms->buffer.fields) / sizeof(uniforms->buffer.fields[0]));

    // global ordinary uniforms ("uniform float4 x;" at file scope) are packed by Slang into one implicit constant buffer
    const uint32_t param_count = layout->getParameterCount();
    if (layout->getGlobalConstantBufferSize() > 0 && count < max_uniforms)
    {
        gfx_uniform_t* u = &uniforms[count];
        memset(u, 0, sizeof(*u));
        strncpy(u->name, "globalParams", sizeof(u->name) - 1);
        u->type    = gfx_uniform_ubo;
        u->group   = 0;
        u->binding = (uint16_t)layout->getGlobalConstantBufferBinding();

        uint32_t fcount = 0;
        for (uint32_t i = 0; i < param_count; ++i) {
            slang::VariableLayoutReflection* p = layout->getParameterByIndex(i);
            if ((SlangParameterCategory)p->getCategory() != SLANG_PARAMETER_CATEGORY_UNIFORM) continue;
            if (fcount >= max_fields) {
                printf("\n[shader compiler] %s: more than %u global uniforms, '%s' is not reflected", shader_name, max_fields, p->getName());
                continue;
            }
            _slang_fill_field(p->getTypeLayout(), p->getName(), (uint32_t)p->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM), u, fcount++);
        }
        u->buffer.field_count = (uint16_t)fcount;
        u->buffer.size = (uint16_t)((layout->getGlobalConstantBufferSize() + 15) & ~(size_t)15);
        locations[count] = { cb_category, 0, u->binding };
        count++;
    }

    for (uint32_t i = 0; i < param_count && count < max_uniforms; ++i)
    {
        slang::VariableLayoutReflection* p = layout->getParameterByIndex(i);
        const SlangParameterCategory category = (SlangParameterCategory)p->getCategory();
        if (category == SLANG_PARAMETER_CATEGORY_UNIFORM || category == SLANG_PARAMETER_CATEGORY_NONE)
            continue;   // global ordinary data is handled above
        if (p->getCategoryCount() > 1) {
            printf("\n[shader compiler] %s: '%s' mixes resource kinds (struct with resources?), not reflected", shader_name, p->getName());
            continue;
        }

        slang::TypeLayoutReflection* tl = p->getTypeLayout();
        gfx_uniform_t* u = &uniforms[count];
        memset(u, 0, sizeof(*u));
        strncpy(u->name, p->getName() ? p->getName() : "", sizeof(u->name) - 1);
        u->binding = (uint16_t)p->getOffset(category);
        u->group   = (uint16_t)p->getBindingSpace(category);

        switch (tl->getKind())
        {
            case slang::TypeReflection::Kind::ConstantBuffer:
                u->type = gfx_uniform_ubo;
                _slang_fill_ubo_fields(shader_name, tl->getElementTypeLayout(), u);
                break;

            case slang::TypeReflection::Kind::SamplerState:
                u->type = gfx_uniform_sampler;
                break;

            case slang::TypeReflection::Kind::Resource: {
                const SlangResourceShape shape  = tl->getResourceShape();
                const SlangResourceShape base   = (SlangResourceShape)(shape & SLANG_RESOURCE_BASE_SHAPE_MASK);
                const SlangResourceAccess access = tl->getResourceAccess();
                const bool writable = access == SLANG_RESOURCE_ACCESS_READ_WRITE || access == SLANG_RESOURCE_ACCESS_WRITE;
                const bool arrayed  = (shape & SLANG_TEXTURE_ARRAY_FLAG) != 0;

                if (base == SLANG_STRUCTURED_BUFFER || base == SLANG_BYTE_ADDRESS_BUFFER) {
                    u->type = gfx_uniform_storage_buffer;
                    u->storage.access = writable ? gfx_access_rw : gfx_access_read;
                    if (base == SLANG_STRUCTURED_BUFFER)
                        _slang_fill_ubo_fields(shader_name, tl->getElementTypeLayout(), u);
                    break;
                }

                u->texture.access = access == SLANG_RESOURCE_ACCESS_WRITE ? gfx_access_write : (writable ? gfx_access_rw : gfx_access_read);
                switch (base) {
                    case SLANG_TEXTURE_2D:   u->texture.dimension = arrayed ? gfx_texture2d_array : gfx_texture2d;
                                             u->type = arrayed ? gfx_uniform_texture2d_array : gfx_uniform_texture2d; break;
                    case SLANG_TEXTURE_3D:   u->texture.dimension = gfx_texture3d;      u->type = gfx_uniform_texture3d;      break;
                    case SLANG_TEXTURE_CUBE: u->texture.dimension = gfx_texture2d_cube; u->type = gfx_uniform_texture2d_cube; break;
                    default:
                        printf("\n[shader compiler] %s: '%s' resource shape %d is not supported", shader_name, u->name, (int)base);
                        continue;
                }
                if (writable) u->type = gfx_uniform_storage_image;
            } break;

            default:
                printf("\n[shader compiler] %s: '%s' kind %d is not supported", shader_name, u->name, (int)tl->getKind());
                continue;
        }

        locations[count] = { category, u->group, u->binding };
        count++;
    }
    return count;
}

// Reflection of a compiled request: layout of the (single) target + which entry point really uses what
// (IMetadata::isParameterLocationUsed, after dead code elimination). Unused resources are dropped.
static void _slang_reflect_program(SlangCompileRequest* request, const char* shader_name, shader_compile_target target,
                                   const gfx_shader_stage* entry_point_stages, int entry_point_count, gfx_shader_compiler_result_t* program)
{
    slang::ProgramLayout* layout = (slang::ProgramLayout*)spGetReflection(request);
    if (layout == nullptr)
        return;

    gfx_uniform_t       uniforms[32];
    _slang_location_t   locations[32];
    const uint32_t count = _slang_reflect_layout(shader_name, layout, target, uniforms, locations, 32);

    slang::IComponentType* linked = nullptr;
    spCompileRequest_getProgramWithEntryPoints(request, &linked);
    for (int ep = 0; ep < entry_point_count; ++ep)
    {
        const uint16_t stage_bit = (uint16_t)(1u << entry_point_stages[ep]);
        slang::IMetadata* metadata = nullptr;
        if (linked == nullptr || SLANG_FAILED(linked->getEntryPointMetadata(ep, 0, &metadata)) || metadata == nullptr) {
            for (uint32_t i = 0; i < count; ++i) uniforms[i].stage_mask |= stage_bit;    // unknown: assume used
            continue;
        }
        for (uint32_t i = 0; i < count; ++i) {
            bool used = false;
            metadata->isParameterLocationUsed(locations[i].category, locations[i].space, locations[i].index, used);
            if (used) uniforms[i].stage_mask |= stage_bit;
        }
        metadata->release();
    }
    if (linked) linked->release();

    program->uniforms = (gfx_uniform_t*)calloc(count ? count : 1, sizeof(gfx_uniform_t));
    program->uniform_count = 0;
    for (uint32_t i = 0; i < count; ++i)
        if (uniforms[i].stage_mask != 0)
            program->uniforms[program->uniform_count++] = uniforms[i];
}
/*
static void _reflect_slang_program(SlangCompileRequest* compile_request, gfx_shader_meta_t* out_meta)
{
    if (!compile_request || !out_meta) return;

    memset(out_meta, 0, sizeof(gfx_shader_meta_t));

    SlangReflection* slang_reflection = spGetReflection(compile_request);
    if (!slang_reflection) return;

 //   slang::ProgramReflection* program_reflection = (slang::ProgramReflection*)spGetReflection(compile_request);

    unsigned parameter_count = spReflection_GetParameterCount(slang_reflection);

    for (uint32_t i = 0; i < parameter_count; ++i)
    {
        auto parameter = spReflection_GetParameterByIndex(slang_reflection, i);// program_reflection->getParameterByIndex(i);
        if (!parameter) continue;

        auto var = spReflectionVariableLayout_GetVariable(parameter);
        auto type = spReflectionVariableLayout_GetTypeLayout(parameter);

        const char* name = spReflectionVariable_GetName(var);

        unsigned binding = spReflectionParameter_GetBindingIndex(parameter);
        unsigned group = spReflectionParameter_GetBindingSpace(parameter);

        slang::TypeReflection * typer = (slang::TypeReflection*)spReflectionVariable_GetType(var);
    //    slang::TypeLayoutReflection* typeLayout = (slang::TypeLayoutReflection*)spReflectionVariableLayout_GetTypeLayout(type);
        auto kind = typer->getKind();
        spReflectionVariable_GetType(var);
       // slang::VariableReflection* var = var_layout->getVariable();
       printf("\n[%d] %s  bi[%d] space[%d]", i, name, binding, group);
    }
}

static void _reflect_slang_program(SlangCompileRequest* compile_request, int target_id, gfx_shader_meta_t* out_meta)
{
}
*/
extern "C" void gfx_shader_compiler_context_create(uint64_t options, gfx_shader_compiler_context_t **out_context)
{
    if(out_context == nullptr) return;
        
    gfx_shader_compiler_context_t* context = (gfx_shader_compiler_context_t*)calloc(1, sizeof(gfx_shader_compiler_context_t));

    if(context == nullptr) 
        return;

    context->global_session = spCreateSession(NULL);
    if (context->global_session) {
    } else {
        free(context);
        context = NULL;
    }

    *out_context = context;
}

extern "C" void gfx_shader_compiler_context_destroy(gfx_shader_compiler_context_t * context)
{
    if (context == nullptr) return;

    spDestroySession(context->global_session);
    free(context);
}


static int gfx_compile_shader(gfx_shader_compiler_context_t * context, const char* name, const char* data, uint32_t size, shader_compile_target target, uint64_t options, gfx_shader_compiler_result_t * program)
{
    if(context == nullptr) return 0;

    uint32_t option_count = 0;
    slang::CompilerOptionEntry compiler_options[32] = { };

    if((options & shader_compile_option_invert_y) == shader_compile_option_invert_y)
    {
        compiler_options[option_count].name = slang::CompilerOptionName::VulkanInvertY;
        compiler_options[option_count].value.intValue0 = 1;
        option_count++;
    }

    slang::SessionDesc session_desc         = { };
    session_desc.compilerOptionEntryCount   = option_count;
    session_desc.compilerOptionEntries      = compiler_options;
    session_desc.defaultMatrixLayoutMode    = SlangMatrixLayoutMode::SLANG_MATRIX_LAYOUT_ROW_MAJOR;

    slang::ISession* outSession = nullptr;
    if(SLANG_FAILED(context->global_session->createSession(session_desc, &outSession)))
    {
        printf("\n Failed to createSession ");
        return 0;
    }

    SlangCompileRequest* compile_request = nullptr;
    if (SLANG_FAILED(outSession->createCompileRequest(&compile_request)))
    {
        printf("\n Failed to create CompileRequest ");
        outSession->release();
        return 0;
    }

    // our own #pragma vertex/fragment/compute/multicompile are "unknown pragma" (15601) for slang
    spOverrideDiagnosticSeverity(compile_request, 15601, SLANG_SEVERITY_DISABLED);

    // add defines
    //spAddPreprocessorDefine(compile_request, "DEEP_THOUGHT", "" );    // #define DEEP_THOUGHT
    //spAddPreprocessorDefine(compile_request, "DEEP_THOUGHT", "42" );  // #define DEEP_THOUGHT (42)

    int translation_unit = spAddTranslationUnit(compile_request, SLANG_SOURCE_LANGUAGE_HLSL, nullptr);

    spAddTranslationUnitSourceString(compile_request, translation_unit, name, data);

    int target_id = -1;
    switch(target)
    {
       case shader_target_spirv: target_id = spAddCodeGenTarget(compile_request, SLANG_SPIRV);break;
       case shader_target_msl:   target_id = spAddCodeGenTarget(compile_request, SLANG_METAL);break;
       case shader_target_wgsl:  target_id = spAddCodeGenTarget(compile_request, SLANG_WGSL); break;
       case shader_target_glsl:  target_id = spAddCodeGenTarget(compile_request, SLANG_GLSL); break;
       case shader_target_dxil:  target_id = spAddCodeGenTarget(compile_request, SLANG_DXIL); break;
       default: assert(false); break;
    }

    // Slang's SPIR-V backend is stable when emitting SPIR-V 1.3 and later, however, support for SPIR-V 1.0, 1.1 and 1.2 is still experimental
    // spirv_1_3 spirv_1_4 spirv_1_5
    if (target == shader_target_spirv)
    {
        auto profile = context->global_session->findProfile("spirv_1_3");
        spSetTargetProfile(compile_request, target_id, profile);
    }

    const char* stage_names [] = {
        pragma_entry_point_vertex_name,
        pragma_entry_point_fragment_name,
        pragma_entry_point_compute_name
    };

    SlangStage slang_stages [] = {
        SLANG_STAGE_VERTEX, 
        SLANG_STAGE_FRAGMENT, 
        SLANG_STAGE_COMPUTE
    };

    const gfx_shader_stage gfx_stages [] = {
        gfx_shader_vertex,
        gfx_shader_fragment,
        gfx_shader_compute
    };

    int slang_entry_point_index_count = 0;
    int slang_entry_point_indexes[8] = { };
    gfx_shader_stage entry_point_stages[8] = { };
    char entry_point_names[8][64]    = { }; // creepy

    for(int i = 0; i < _countof(stage_names); ++i)
    {
        const char* ep_name = _find_pragma_entry_point(data, stage_names[i], entry_point_names[slang_entry_point_index_count], 64);
        if (ep_name == nullptr)
            continue;

        int entrypoint_id = spAddEntryPoint(compile_request, translation_unit, ep_name, slang_stages[i]);
        entry_point_stages[slang_entry_point_index_count] = gfx_stages[i];
        slang_entry_point_indexes[slang_entry_point_index_count++] = entrypoint_id;
    }

    const bool compiled = SLANG_SUCCEEDED(spCompile(compile_request));

    // errors, and warnings of a successful compilation
    const char * diagnostics = spGetDiagnosticOutput(compile_request);
    if (diagnostics && diagnostics[0])
        printf("\n[shader compiler] %s:\n%s", name ? name : "", diagnostics);

    int ok = compiled ? 1 : 0;
    if (compiled)
    {
        _slang_reflect_program(compile_request, name, target, entry_point_stages, slang_entry_point_index_count, program);

        program->blob_count = slang_entry_point_index_count;
        program->blobs = (gfx_shader_compiler_stage_t*)calloc(slang_entry_point_index_count, sizeof(gfx_shader_compiler_stage_t));

        for (int i = 0; i < slang_entry_point_index_count && ok; i++)
        {
            ISlangBlob* shlang_blob = nullptr;
            if (SLANG_FAILED(spGetEntryPointCodeBlob(compile_request, slang_entry_point_indexes[i], target_id, &shlang_blob)) || !shlang_blob)
            {
                printf("\n[shader compiler] %s: no code for entry point '%s'", name ? name : "", entry_point_names[i]);
                ok = 0;
                break;
            }

            gfx_shader_compiler_stage_t* stage = &program->blobs[i];
            stage->stage = entry_point_stages[i];
            stage->size  = (uint32_t)shlang_blob->getBufferSize();
            stage->data  = (char*)calloc(1, stage->size);
            strncpy(stage->entry, entry_point_names[i], sizeof(stage->entry) - 1);

            if (stage->data != nullptr)
                memcpy(stage->data, shlang_blob->getBufferPointer(), stage->size);
            else
                ok = 0;

            shlang_blob->Release();
        }

        if (!ok)
            gfx_shader_compiler_result_free(program);
    }

    spDestroyCompileRequest(compile_request);
    outSession->release();
    return ok;
}

extern "C" int gfx_compile_shader(gfx_shader_compiler_context_t * context, gfx_shader_compiler_request_desc_t * desc, gfx_shader_compiler_result_t * result)
{
    return gfx_compile_shader(context, desc->name, desc->data, desc->size, desc->target, desc->options, result);
}
/*
struct gfx_shader_meta_t{};
static void _reflect_slang_program(SlangCompileRequest* compile_request, gfx_shader_meta_t* out_meta)
{
    if (!compile_request || !out_meta) return;

    memset(out_meta, 0, sizeof(gfx_shader_meta_t));

    SlangReflection * sreflection =  spGetReflection(compile_request);
    
    // 1. Cast the compile request to the modern ComPtr/Interface to get the compiled program
    // In Slang, spCompile populates the internal program structure which we can query.
    slang::IComponentProgram* composed_program = nullptr;
    if (SLANG_FAILED(spGetLayout(compile_request, &composed_program)) || !composed_program)
    {
        // Fallback: If spGetLayout is not exposed in your specific C-wrapper config,
        // you can query it via the session's request interface:
        composed_program = (slang::IComponentProgram*)spGetCompileRequestProgram(compile_request);
        if (!composed_program) return;
    }

    // 2. Get the layout reflection from the composed program
    slang::IProgramLayout* program_reflection = composed_program->getLayout();
    if (!program_reflection) return;

    // 3. Iterate over shader parameters
    uint32_t parameter_count = program_reflection->getParameterCount();
    for (uint32_t i = 0; i < parameter_count; ++i)
    {
        slang::VariableLayoutReflection* var_layout = program_reflection->getParameterByIndex(i);
        if (!var_layout) continue;

        slang::VariableReflection* var = var_layout->getVariable();
        slang::TypeReflection* type = var_layout->getType();
        if (!var || !type) continue;

        // Filter for valid pipeline resources
        if (var_layout->getCategory() != slang::ParameterCategory::DescriptorTableSlot &&
            var_layout->getCategory() != slang::ParameterCategory::Uniform) {
            continue;
        }

        uint32_t idx = out_meta->uniform_count;
        if (idx >= 32) break; // Static bounds guard

        const char* var_name = var->getName();
        if (var_name) {
            strncpy(out_meta->uniforms[idx].name, var_name, sizeof(out_meta->uniforms[idx].name) - 1);
        }

        // Extract hardware locations (set/group and binding index)
        out_meta->uniforms[idx].binding = (uint16_t)var_layout->getBindingIndex();
        out_meta->uniforms[idx].group = (uint16_t)var_layout->getBindingSpace();

        // Classify types
        gfx_uniform_type detected_type = GFX_UNIFORM_TYPE_BUFFER;
        slang::TypeReflection::Kind kind = type->getKind();

        if (kind == slang::TypeReflection::Kind::Resource)
        {
            SlangResourceShape shape = type->getResourceShape();
            if ((shape & SLANG_RESOURCE_SHAPE_BASE_MASK) == SLANG_STRUCTURED_BUFFER) {
                detected_type = GFX_UNIFORM_TYPE_STORAGE_BUFFER;
            }
            else if ((shape & SLANG_RESOURCE_SHAPE_BASE_MASK) == SLANG_TEXTURE_2D) {
                if (shape & SLANG_RESOURCE_SHAPE_ARRAY_FLAG)
                    detected_type = GFX_UNIFORM_TYPE_TEXTURE2D_ARRAY;
                else
                    detected_type = GFX_UNIFORM_TYPE_TEXTURE2D;
            }
            else if ((shape & SLANG_RESOURCE_SHAPE_BASE_MASK) == SLANG_TEXTURE_3D) {
                detected_type = GFX_UNIFORM_TYPE_TEXTURE3D;
            }
            else if ((shape & SLANG_RESOURCE_SHAPE_BASE_MASK) == SLANG_TEXTURE_CUBE) {
                detected_type = GFX_UNIFORM_TYPE_TEXTURE2D_CUBE;
            }
        }
        else if (kind == slang::TypeReflection::Kind::SamplerState)
        {
            detected_type = GFX_UNIFORM_TYPE_SAMPLER;
        }
        else if (kind == slang::TypeReflection::Kind::ConstantBuffer ||
            kind == slang::TypeReflection::Kind::ParameterBlock)
        {
            detected_type = GFX_UNIFORM_TYPE_BUFFER;
        }

        // Setup stage masks using the program reflection entry points data
        uint32_t stage_mask = 0;
        uint32_t entry_point_count = program_reflection->getEntryPointCount();
        for (uint32_t ep = 0; ep < entry_point_count; ++ep)
        {
            slang::EntryPointReflection* ep_reflect = program_reflection->getEntryPointByIndex(ep);
            if (ep_reflect) {
                SlangStage stage = ep_reflect->getStage();
                if (stage == SLANG_STAGE_VERTEX)   stage_mask |= GFX_SHADER_STAGE_VERTEX;
                if (stage == SLANG_STAGE_FRAGMENT) stage_mask |= GFX_SHADER_STAGE_FRAGMENT;
                if (stage == SLANG_STAGE_COMPUTE)  stage_mask |= GFX_SHADER_STAGE_COMPUTE;
            }
        }

        if (stage_mask == 0) {
            stage_mask = GFX_SHADER_STAGE_VERTEX | GFX_SHADER_STAGE_FRAGMENT | GFX_SHADER_STAGE_COMPUTE;
        }

        out_meta->uniforms[idx].stage_mask = (uint16_t)stage_mask;
        out_meta->uniforms[idx].type = detected_type;
        out_meta->uniform_count++;
    }
}*/

#else
extern "C" void gfx_shader_compiler_context_create(uint64_t options, gfx_shader_compiler_context_t **compiler_context) {
    printf("\ngfx_shader_compiler_context_create: install slang shader compiler tool: ");
};

extern "C" void gfx_shader_compiler_context_destroy(gfx_shader_compiler_context_t * compiler_context) {
    printf("\ngfx_shader_compiler_context_destroy: install slang shader compiler tool: ");
}

extern "C" int gfx_compile_shader(gfx_shader_compiler_context_t * context, gfx_shader_compiler_request_desc_t * desc, gfx_shader_compiler_result_t * result) {
    printf("\ngfx_compile_shader: install slang shader compiler tool: ");
    return 0;
}
#endif

#endif

#endif 