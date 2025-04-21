#include "asset_shader.h"

#include <slang/slang.h>
#include <slang/slang-cpp-types.h>
#include <slang/slang-com-helper.h>
#include <slang/slang-com-ptr.h>

#ifdef _DEBUG
#pragma comment(lib, "lib/slangd.lib")
#else
#pragma comment(lib, "lib/slang.lib")
#endif


struct shader_info_t {
    const char *    pragma_name;
    SlangStage      stage;
    int             entrypoint_id;
} g_shader_infos [] = { 
    { pragma_vertex_name,           SLANG_STAGE_VERTEX,         -1 },
    { pragma_fragment_name,         SLANG_STAGE_FRAGMENT,       -1 },
                                                    
    { pragma_hull_name,             SLANG_STAGE_HULL,           -1 },
    { pragma_domain_name,           SLANG_STAGE_DOMAIN,         -1 },
    { pragma_geometry_name,         SLANG_STAGE_GEOMETRY,       -1 },
                           
    { pragma_compute_name,          SLANG_STAGE_COMPUTE,        -1 },
      
    { pragma_ray_gen_name,          SLANG_STAGE_RAY_GENERATION, -1 },
    { pragma_ray_intersect_name,    SLANG_STAGE_INTERSECTION,   -1 },
    { pragma_ray_anyhit_name,       SLANG_STAGE_ANY_HIT,        -1 },
    { pragma_ray_closesthit_name,   SLANG_STAGE_CLOSEST_HIT,    -1 },
    { pragma_ray_miss_name,         SLANG_STAGE_MISS,           -1 },
    { pragma_ray_callable_name,     SLANG_STAGE_CALLABLE,       -1 },
      
    { pragma_mesh_name,             SLANG_STAGE_MESH,           -1 },
    { pragma_amplification_name,    SLANG_STAGE_AMPLIFICATION,  -1 },
};


// #pragma vertex vsmain
// #pragma fragment psmain
// #pragma compute csmain
static const char* find_pragma_entry(const char* data, const char* stage_name, char* entry_buffer, size_t entry_buffer_size)
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
                while (isspace(*data)) data++;;
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

void find_pragma_multicompile(const char* data,  char ** keywords, int * keyword_count)
{
}



int shlang_get_pragma_entrypoints(const char* data, SlangCompileRequest* compile_request, int translation_idx, shader_info_t * infos, int size)
{
    int count = 0;
    for (int i = 0; i < _countof(g_shader_infos); ++i)
    {
        char buffer[256] = "";
        SlangStage stage = g_shader_infos[i].stage;
        const char* pragma_name = g_shader_infos[i].pragma_name;

        const char* ep_name = find_pragma_entry(data, pragma_name, buffer, sizeof(buffer));
        if(ep_name == nullptr)
            continue;

        assert(size > count);
        infos[count].stage = stage;
        infos[count].pragma_name = pragma_name;
        infos[count].entrypoint_id = spAddEntryPoint(compile_request, translation_idx, ep_name, stage);
        count++;
    }
    return count;
}



int asset_shader_compile(const char * name, const char* data, uint32_t size, const char * target, char ** out_blobs, int * out_sizes, const char** out_stages)
{
    assert(name);
    assert(data);
    assert(target);

    Slang::ComPtr<slang::IGlobalSession> slangSession(spCreateSession(NULL));

    slang::CompilerOptionEntry option = { slang::CompilerOptionName::VulkanInvertY };
    option.value.intValue0 = 1;

    slang::CompilerOptionEntry options [] = { option };

    slang::SessionDesc desc = {};
    desc.compilerOptionEntries = options;
    desc.compilerOptionEntryCount = 1;

    desc.defaultMatrixLayoutMode = SlangMatrixLayoutMode::SLANG_MATRIX_LAYOUT_ROW_MAJOR;

    slang::ISession* outSession = nullptr;
    auto newSession = slangSession->createSession(desc, &outSession);

    SlangCompileRequest* compile_request = nullptr;
    outSession->createCompileRequest(&compile_request);

   // SlangCompileRequest* compile_request = spCreateCompileRequest(slangSession);
    int translation_unit_index = spAddTranslationUnit(compile_request, SLANG_SOURCE_LANGUAGE_HLSL, nullptr);
    spAddTranslationUnitSourceString(compile_request, translation_unit_index, name, data);

    int target_id = -1;
    if(!strcmp(target, "spirv")){
        target_id = spAddCodeGenTarget(compile_request, SLANG_SPIRV);
    }else if(!strcmp(target, "metal")) {
        target_id = spAddCodeGenTarget(compile_request, SLANG_METAL);
    }else if (!strcmp(target, "wgsl")) {
        target_id = spAddCodeGenTarget(compile_request, SLANG_WGSL);
    }else if (!strcmp(target, "glsl")) {
        target_id = spAddCodeGenTarget(compile_request, SLANG_GLSL);
    }else if (!strcmp(target, "dxil")) {
        target_id = spAddCodeGenTarget(compile_request, SLANG_DXIL);
    }else {
        assert(false);
    }

    shader_info_t stage_infos[_countof(g_shader_infos)] = {};
    int count = shlang_get_pragma_entrypoints(data, compile_request, translation_unit_index, stage_infos, _countof(stage_infos));

    SlangResult compileRes = spCompile(compile_request);
    auto diagnostics = spGetDiagnosticOutput(compile_request);

    printf(diagnostics);

    for(int i = 0; i < count; ++i)
    {
        ISlangBlob* shlang_blob = nullptr;
        if (SLANG_SUCCEEDED(spGetEntryPointCodeBlob(compile_request, stage_infos[i].entrypoint_id, target_id, &shlang_blob)))
        {
            out_sizes[i] = shlang_blob->getBufferSize();
            out_blobs[i] = (char*)calloc(1, shlang_blob->getBufferSize());
            out_stages[i] = stage_infos[i].pragma_name;

            if (out_blobs[i] != nullptr)
                memcpy(out_blobs[i], shlang_blob->getBufferPointer(), shlang_blob->getBufferSize());

            shlang_blob->Release();
        }
    }

    outSession->release();
    return count;
}




bool AssetShader::compile_shader(const std::string& path, ShaderStage stage, const std::string & entry, std::string* blob, std::string* error)
{
    return false;
}


bool AssetShader::compile_shader_form_data(const char* data, size_t size, const std::wstring& include_path, ShaderStage stage, const std::string& entry, std::string* blob, std::string* error)
{
    return blob ? (!blob->empty()) : (false);
}


AssetShader::AssetShader()
{

}



void AssetShader::export_shader(const std::string& path, const std::string& dst_path)
{
 /*   std::vector<AssetShader::ShaderEntry> shaders;
    AssetShader::compile_shader2(path, shaders);

    auto src_file = read_file(path);
    std::replace(src_file.begin(), src_file.end(), '\r', ' ');

    size_t pos = 0;
    std::string filename = path;
    if((pos = filename.rfind('/')) != std::string::npos)
    {
        filename = filename.erase(0, pos + 1);
        std::replace(filename.begin(), filename.end(), '.', '_');
    }

    std::string dump_data = std::string("const char* ").append(filename).append(" =  R\"(\n");

    dump_data.append(src_file.begin(), src_file.end());
    dump_data.append("})\"; \n\n");

    for(auto it = shaders.begin(); it != shaders.end(); ++it)
    {
        auto var_name = filename + std::string("_sprv_") + s_stage_pragmas[it->stage];
        dump_data += bin2hex::dump(it->blob.data(), it->blob.size(), var_name.c_str());
        dump_data += "\n";
    }

    FILE * file = fopen(dst_path.c_str(), "w+");
    if(file != nullptr) 
    {
        fwrite(dump_data.data(), 1, dump_data.size(), file);
        fclose(file);
    }*/
}