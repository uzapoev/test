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


typedef struct compiled_stage_blob_t {
    gfx_shader_stage    stage;
    char                stage_entry_point_name[64];
    uint32_t            stage_data_size;
    char*               stage_data;
} compiled_stage_blob_t;


typedef struct compiled_shader_program_t {
    uint32_t                    keyword_count;
    char*                       keywords[64];

    uint32_t                    blob_count;
    compiled_stage_blob_t*      blobs;
} compiled_shader_program_t;


typedef struct gfx_shader_compiler_request_desc_t {
    const char*             name;
    const char*             data;
    uint32_t                size;
    shader_compile_target   target;

    uint32_t                options;

    uint32_t                define_count;
    const char**            defines;

    uint32_t                search_path_count;
    const char**            search_paths;
} gfx_shader_compiler_request_desc_t;

struct gfx_shader_compiler_context_t;

extern "C" void gfx_shader_compiler_context_create(uint64_t options, gfx_shader_compiler_context_t ** compiler_context);
extern "C" void gfx_shader_compiler_context_destroy(gfx_shader_compiler_context_t * compiler_context);

extern "C" int gfx_compile_shader(gfx_shader_compiler_context_t * context, gfx_shader_compiler_request_desc_t * desc, compiled_shader_program_t * programs);
//extern "C" int gfx_compile_shader(gfx_shader_compiler_context_t * context, const char* name, const char* data, uint32_t size, shader_compile_target target, uint64_t options, compiled_shader_program_t * programs);



#if __has_include(<slang/slang.h>)
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

#if defined(_DEBUG) && defined(_WIN32)
    #pragma comment(lib, "lib/slangd.lib")
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
void _find_pragma_multicompile(const char* data, char keywords[32][64], int* keyword_count)
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



static int  gfx_compile_shader(gfx_shader_compiler_context_t * context, const char* name, const char* data, uint32_t size, shader_compile_target target, uint64_t options, compiled_shader_program_t * program)
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

    SlangCompileRequest* compile_request = nullptr;;
    if (SLANG_FAILED(outSession->createCompileRequest(&compile_request)))
    {
        printf("\n Failed to create CompileRequest ");
        return 0;
    }

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
    auto profile = context->global_session->findProfile("spirv_1_3");
    spSetTargetProfile(compile_request, target_id, profile);

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

    int slang_entry_point_index_count = 0;
    int slang_entry_point_indexes[8] = { };
    char entry_point_names[8][64]    = { }; // creepy

    for(int i = 0; i < _countof(stage_names); ++i)
    {
        const char* ep_name = _find_pragma_entry_point(data, stage_names[i], entry_point_names[slang_entry_point_index_count], 64);
        if (ep_name == nullptr)
            continue;

        int entrypoint_id = spAddEntryPoint(compile_request, translation_unit, ep_name, slang_stages[i]);
        slang_entry_point_indexes[slang_entry_point_index_count++] = entrypoint_id;
    }

    if(SLANG_FAILED(spCompile(compile_request)))
    {
        const char * diagnostics = spGetDiagnosticOutput(compile_request);
        printf(diagnostics);
        spDestroyCompileRequest(compile_request);
        return 0;
    }

    program->blob_count = slang_entry_point_index_count;
    program->blobs = (compiled_stage_blob_t*)calloc(slang_entry_point_index_count, sizeof(compiled_stage_blob_t));

    for (size_t i = 0; i < slang_entry_point_index_count; i++)
    {
        ISlangBlob* shlang_blob = nullptr;
        if (SLANG_SUCCEEDED(spGetEntryPointCodeBlob(compile_request, slang_entry_point_indexes[i], target_id, &shlang_blob)))
        {
            program->blobs[i].stage_data_size = (uint32_t)shlang_blob->getBufferSize();
            program->blobs[i].stage_data = (char*)calloc(1, shlang_blob->getBufferSize());
            strcpy(program->blobs[i].stage_entry_point_name, entry_point_names[i]);

            if (program->blobs[i].stage_data != nullptr)
                memcpy(program->blobs[i].stage_data, shlang_blob->getBufferPointer(), shlang_blob->getBufferSize());

            shlang_blob->Release();
        }
    }

    spDestroyCompileRequest(compile_request);
    return 1;
}

extern "C" int gfx_compile_shader(gfx_shader_compiler_context_t * context, gfx_shader_compiler_request_desc_t * desc, compiled_shader_program_t * programs)
{
    return gfx_compile_shader(context, desc->name, desc->data, desc->size, desc->target, desc->options, programs);
}

#else
extern "C" void gfx_shader_compiler_context_create(uint64_t options, gfx_shader_compiler_context_t **compiler_context) {
    printf("\ngfx_shader_compiler_context_create: install slang shader compiler tool: ");
};

extern "C" void gfx_shader_compiler_context_destroy(gfx_shader_compiler_context_t * compiler_context) {
    printf("\ngfx_shader_compiler_context_destroy: install slang shader compiler tool: ");
}

extern "C" int gfx_compile_shader(gfx_shader_compiler_context_t * context, gfx_shader_compiler_request_desc_t * desc, compiled_shader_program_t * programs) {
    printf("\ngfx_compile_shader: install slang shader compiler tool: ");
    return 0;
}
#endif

#endif 