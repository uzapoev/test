#include "asset_shader.h"

#include <algorithm>
#include <filesystem>

#ifdef _WIN32
    #include <wrl/client.h>
    #include <dxc/dxcapi.h>
    #pragma comment(lib, "lib/dxcompiler.lib")

    #ifdef min
    #undef min
    #endif

    #ifdef max
    #undef max
    #endif

    #include <spirv_cross/spirv.h>
    #include <spirv_cross/spirv_cross.hpp>
    #include <spirv_cross/spirv_msl.hpp>

    #ifdef _DEBUG
    #pragma comment(lib, "lib/spirv-cross-cored.lib")
    #pragma comment(lib, "lib/spirv-cross-glsld.lib")
    #pragma comment(lib, "lib/spirv-cross-msld.lib")
    #else
    #pragma comment(lib, "lib/spirv-cross-core.lib")
    #pragma comment(lib, "lib/spirv-cross-glsl.lib")
    #pragma comment(lib, "lib/spirv-cross-msl.lib")
    #endif
#endif

#include "../common.h"

#ifdef _WIN32
struct IncludeHandler : public IDxcIncludeHandler
{
    HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
    {
        Microsoft::WRL::ComPtr<IDxcUtils> pUtils;
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf()));

        Microsoft::WRL::ComPtr<IDxcBlobEncoding> pEncoding;

        if (std::filesystem::exists(pFilename))
            pUtils->LoadFile(pFilename, NULL, pEncoding.GetAddressOf());
        else
            return S_FALSE;

        *ppIncludeSource = pEncoding.Detach();
        return S_OK;
    }

    HRESULT QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override { return E_NOINTERFACE; }

    virtual ULONG AddRef(void) { return 1; }
    virtual ULONG Release(void) { return 1; }
};
#endif
/*
Compilation Options:
  -all-resources-bound    Enables agressive flattening
  -auto-binding-space <value>
                          Set auto binding space - enables auto resource binding in libraries
  -Cc                     Output color coded assembly listings
  -default-linkage <value>
                          Set default linkage for non-shader functions when compiling or linking to a library target (internal, external)
  -denorm <value>         select denormal value options (any, preserve, ftz). any is the default.
  -disable-payload-qualifiers
                          Disables support for payload access qualifiers for raytracing payloads in SM 6.7.
  -D <value>              Define macro
  -enable-16bit-types     Enable 16bit types and disable min precision types. Available in HLSL 2018 and shader model 6.2
  -enable-lifetime-markers
                          Enable generation of lifetime markers
  -enable-payload-qualifiers
                          Enables support for payload access qualifiers for raytracing payloads in SM 6.6.
  -encoding <value>       Set default encoding for source inputs and text outputs (utf8|utf16(win)|utf32(*nix)|wide) default=utf8
  -export-shaders-only    Only export shaders when compiling a library
  -exports <value>        Specify exports when compiling a library: export1[[,export1_clone,...]=internal_name][;...]
  -E <value>              Entry point name
  -Fc <file>              Output assembly code listing file
  -fdiagnostics-show-option
                          Print option name with mappable diagnostics
  -Fd <file>              Write debug information to the given file, or automatically named file in directory when ending in '\'
  -Fe <file>              Output warnings and errors to the given file
  -Fh <file>              Output header file containing object code
  -flegacy-macro-expansion
                          Expand the operands before performing token-pasting operation (fxc behavior)
  -flegacy-resource-reservation
                          Reserve unused explicit register assignments for compatibility with shader model 5.0 and below
  -fno-diagnostics-show-option
                          Do not print option name with mappable diagnostics
  -force-rootsig-ver <profile>
                          force root signature version (rootsig_1_1 if omitted)
  -Fo <file>              Output object file
  -Fre <file>             Output reflection to the given file
  -Frs <file>             Output root signature to the given file
  -Fsh <file>             Output shader hash to the given file
  -Gec                    Enable backward compatibility mode
  -Ges                    Enable strict mode
  -Gfa                    Avoid flow control constructs
  -Gfp                    Prefer flow control constructs
  -Gis                    Force IEEE strictness
  -HV <value>             HLSL version (2016, 2017, 2018, 2021). Default is 2018
  -H                      Show header includes and nesting depth
  -ignore-line-directives Ignore line directives
  -I <value>              Add directory to include search path
  -Lx                     Output hexadecimal literals
  -Ni                     Output instruction numbers in assembly listings
  -no-legacy-cbuf-layout  Do not use legacy cbuffer load
  -no-warnings            Suppress warnings
  -No                     Output instruction byte offsets in assembly listings
  -Odump                  Print the optimizer commands.
  -Od                     Disable optimizations
  -pack-optimized         Optimize signature packing assuming identical signature provided for each connecting stage
  -pack-prefix-stable     (default) Pack signatures preserving prefix-stable property - appended elements will not disturb placement of prior elements
  -recompile              recompile from DXIL container with Debug Info or Debug Info bitcode file
  -res-may-alias          Assume that UAVs/SRVs may alias
  -rootsig-define <value> Read root signature from a #define
  -T <profile>            Set target profile.
        <profile>: ps_6_0, ps_6_1, ps_6_2, ps_6_3, ps_6_4, ps_6_5, ps_6_6, ps_6_7,  
                   vs_6_0, vs_6_1, vs_6_2, vs_6_3, vs_6_4, vs_6_5, vs_6_6, vs_6_7,
                   gs_6_0, gs_6_1, gs_6_2, gs_6_3, gs_6_4, gs_6_5, gs_6_6, gs_6_7,
                   hs_6_0, hs_6_1, hs_6_2, hs_6_3, hs_6_4, hs_6_5, hs_6_6, hs_6_7,
                   ds_6_0, ds_6_1, ds_6_2, ds_6_3, ds_6_4, ds_6_5, ds_6_6, ds_6_7,
                   cs_6_0, cs_6_1, cs_6_2, cs_6_3, cs_6_4, cs_6_5, cs_6_6, cs_6_7,
                   lib_6_1, lib_6_2, lib_6_3, lib_6_4, lib_6_5, lib_6_6, lib_6_7,
                   ms_6_5, ms_6_6, ms_6_7,
                   as_6_5, as_6_6, as_6_7,

  -Vd                     Disable validation
  -Vi                     Display details about the include process.
  -Vn <name>              Use <name> as variable name in header file
  -WX                     Treat warnings as errors
  -Zi                     Enable debug information. Cannot be used together with -Zs
  -Zpc                    Pack matrices in column-major order
  -Zpr                    Pack matrices in row-major order
  -Zsb                    Compute Shader Hash considering only output binary
  -Zss                    Compute Shader Hash considering source information
  -Zs                     Generate small PDB with just sources and compile options. Cannot be used together with -Zi

OPTIONS:
  -MD        Write a file with .d extension that will contain the list of the compilation target dependencies.
  -MF <file> Write the specfied file that will contain the list of the compilation target dependencies.
  -M         Dumps the list of the compilation target dependencies.

Optimization Options:
  -O0 Optimization Level 0
  -O1 Optimization Level 1
  -O2 Optimization Level 2
  -O3 Optimization Level 3 (Default)

Rewriter Options:
  -decl-global-cb         Collect all global constants outside cbuffer declarations into cbuffer GlobalCB { ... }. Still experimental, not all dependency scenarios handled.
  -extract-entry-uniforms Move uniform parameters from entry point to global scope
  -global-extern-by-default
                          Set extern on non-static globals
  -keep-user-macro        Write out user defines after rewritten HLSL
  -line-directive         Add line directive
  -remove-unused-functions
                          Remove unused functions and types
  -remove-unused-globals  Remove unused static globals and functions
  -skip-fn-body           Translate function definitions to declarations
  -skip-static            Remove static functions and globals when used with -skip-fn-body
  -unchanged              Rewrite HLSL, without changes.

SPIR-V CodeGen Options:
  -fspv-debug=<value>     Specify whitelist of debug info category (file -> source -> line, tool)
  -fspv-entrypoint-name=<value>
                          Specify the SPIR-V entry point name. Defaults to the HLSL entry point name.
  -fspv-extension=<value> Specify SPIR-V extension permitted to use
  -fspv-flatten-resource-arrays
                          Flatten arrays of resources so each array element takes one binding number
  -fspv-print-all         Print the SPIR-V module before each pass and after the last one. Useful for debugging SPIR-V legalization and optimization passes.
  -fspv-reduce-load-size  Replaces loads of composite objects to reduce memory pressure for the loads
  -fspv-reflect           Emit additional SPIR-V instructions to aid reflection
  -fspv-target-env=<value>
                          Specify the target environment: vulkan1.0 (default), vulkan1.1, vulkan1.1spirv1.4, vulkan1.2, vulkan1.3, or universal1.5
  -fspv-use-legacy-buffer-matrix-order
                          Assume the legacy matrix order (row major) when accessing raw buffers (e.g., ByteAdddressBuffer)
  -fvk-auto-shift-bindings
                          Apply fvk-*-shift to resources without an explicit register assignment.
  -fvk-b-shift <shift> <space>
                          Specify Vulkan binding number shift for b-type register
  -fvk-bind-globals <binding> <set>
                          Specify Vulkan binding number and set number for the $Globals cbuffer
  -fvk-bind-register <type-number> <space> <binding> <set>
                          Specify Vulkan descriptor set and binding for a specific register
  -fvk-invert-y           Negate SV_Position.y before writing to stage output in VS/DS/GS to accommodate Vulkan's coordinate system
  -fvk-s-shift <shift> <space>
                          Specify Vulkan binding number shift for s-type register
  -fvk-support-nonzero-base-instance
                          Follow Vulkan spec to use gl_BaseInstance as the first vertex instance, which makes SV_InstanceID = gl_InstanceIndex - gl_BaseInstance (without this option, SV_InstanceID = gl_InstanceIndex)
  -fvk-t-shift <shift> <space>
                          Specify Vulkan binding number shift for t-type register
  -fvk-u-shift <shift> <space>
                          Specify Vulkan binding number shift for u-type register
  -fvk-use-dx-layout      Use DirectX memory layout for Vulkan resources
  -fvk-use-dx-position-w  Reciprocate SV_Position.w after reading from stage input in PS to accommodate the difference between Vulkan and DirectX
  -fvk-use-gl-layout      Use strict OpenGL std140/std430 memory layout for Vulkan resources
  -fvk-use-scalar-layout  Use scalar memory layout for Vulkan resources
  -Oconfig=<value>        Specify a comma-separated list of SPIRV-Tools passes to customize optimization configuration (see http://khr.io/hlsl2spirv#optimization)
  -spirv                  Generate SPIR-V code

Utility Options:
  -dumpbin              Load a binary file rather than compiling
  -extractrootsignature Extract root signature from shader bytecode (must be used with /Fo <file>)
  -getprivate <file>    Save private data from shader blob
  -link                 Link list of libraries provided in <inputs> argument separated by ';'
  -P <value>            Preprocess to file (must be used alone)
  -Qembed_debug         Embed PDB in shader container (must be used with /Zi)
  -Qstrip_debug         Strip debug information from 4_0+ shader bytecode  (must be used with /Fo <file>)
  -Qstrip_priv          Strip private data from shader bytecode  (must be used with /Fo <file>)
  -Qstrip_reflect       Strip reflection data from shader bytecode  (must be used with /Fo <file>)
  -Qstrip_rootsignature Strip root signature data from shader bytecode  (must be used with /Fo <file>)
  -setprivate <file>    Private data to add to compiled shader blob
  -setrootsignature <file>
                        Attach root signature to shader bytecode
  -verifyrootsignature <file>
                        Verify shader bytecode with root signature
*/

static const char* s_extensions[AssetShader::ShaderStage::Count][3] =
{
    "_vs.glsl", "_vs.spirv", "_vs.hlsl",
    "_hs.glsl", "_hs.spirv", "_hs.hlsl",
    "_ds.glsl", "_ds.spirv", "_ds.hlsl",
    "_gs.glsl", "_gs.spirv", "_gs.hlsl",
    "_ps.glsl", "_ps.spirv", "_ps.hlsl",
    "_cs.glsl", "_cs.spirv", "_cs.hlsl",
};

static const wchar_t* s_dxc_stages[AssetShader::ShaderStage::Count] =
{ 
   L"vs_5_0",   // Vertex,
   L"hs_5_0",   // Hull,
   L"ds_5_0",   // Domain,
   L"gs_5_0",   // Geometry,
   L"ps_4_0",   // Fragment,
   L"cs_5_0",   // Compute
};

AssetShader::ShaderStage s_stages[AssetShader::ShaderStage::Count]  = {
    AssetShader::Vertex,
    AssetShader::Hull,
    AssetShader::Domain,
    AssetShader::Geometry,
    AssetShader::Fragment,
    AssetShader::Compute 
};


const char* s_stage_pragmas[AssetShader::ShaderStage::Count] = { 
    "vertex", 
    "hull", 
    "domain", 
    "geometry", 
    "fragment", 
    "compute"
};


std::vector<uint8_t> read_file(const std::string& path)
{
    std::vector<uint8_t> data;

    FILE* file = fopen(path.c_str(), "rb");
    if (file != nullptr)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);

        data.resize(size);
        fread(data.data(), 1, size, file);
        fclose(file);
    }

    return data;
}


std::string spirv_to_metal(uint32_t* data, uint32_t len)
{
    std::string result;
#ifdef spirv_H
    std::vector<uint32_t> spirv_binary(data, data + len);

    spirv_cross::CompilerMSL metal(std::move(spirv_binary));

    spirv_cross::CompilerMSL::Options options;
    options.platform = spirv_cross::CompilerMSL::Options::Platform::iOS;
    metal.set_msl_options(options);
    result = metal.compile();
#endif
    return result;
}


// #pragma vertex vsmain()
// #pragma fragment psmain()
// #pragma compute csmain()
const char * AssetShader::find_pragma_entry(const char * data, const char* stage_name,  char * entry_buffer, size_t entry_buffer_size)
{
    if( (entry_buffer == nullptr) || (entry_buffer_size == 0))
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
    if((data != nullptr) && (entry_buffer != nullptr) && (entry_buffer_size > 0))
    {
        int i = 0;
        while(isprint(data[i]) && i < entry_buffer_size)
        {
            entry_buffer[i] = data[i];
            i++;
        }
        return entry_buffer;
    }

    return data;
}




bool AssetShader::compile_shader(const std::string& path, ShaderStage stage, const std::string & entry, std::string* blob, std::string* error)
{
    auto src_file = read_file(path);

    std::string include_path;
    size_t slash = path.find_last_of('/');
    if (slash != -1)
    {
        include_path.assign(path, 0, slash);
    }
    auto include_wpath = std::wstring(include_path.begin(), include_path.end());
    AssetShader::compile_shader_form_data((const char*)src_file.data(), src_file.size(), include_wpath, stage, entry, blob, error);

    return blob ? (!blob->empty()) : (false);
}


bool AssetShader::compile_shader_form_data(const char* data, size_t size, const std::wstring& include_path, ShaderStage stage, const std::string& entry, std::string* blob, std::string* error)
{
#ifdef _WIN32
    Microsoft::WRL::ComPtr<IDxcCompiler3>   compiler(nullptr);

    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

    std::wstring entry_func;
    if (entry.empty() && data)
    {
        auto pragma_entry = find_pragma_entry(data, s_stage_pragmas[stage]);
        if (pragma_entry)
        {
            const char* begin = pragma_entry;
            const char* end = pragma_entry;
            while (isalpha(*end) || *end == '_') ++end;
            entry_func = std::wstring(begin, end);
        }
    }
    else
    {
        entry_func = std::wstring(entry.begin(), entry.end());
    }

    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-E");    arguments.push_back(entry_func.c_str());
    arguments.push_back(L"-T");    arguments.push_back(s_dxc_stages[stage]);

    arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR);
    arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3); // -O3
    arguments.push_back(L"-Qstrip_debug");
    //  arguments.push_back(L"-Qstrip_reflect");
    arguments.push_back(L"-spirv");

    if (!include_path.empty())
    {
        arguments.push_back(L"-I");
        arguments.push_back(include_path.c_str());
    }

    if (stage == ShaderStage::Vertex || stage == ShaderStage::Domain || stage == ShaderStage::Geometry)
        arguments.push_back(L"-fvk-invert-y");  // vs/ds/gs

    DxcBuffer buffer = {};
    buffer.Encoding = DXC_CP_ACP;
    buffer.Ptr = data;
    buffer.Size = size;

    IncludeHandler handler;

    Microsoft::WRL::ComPtr<IDxcResult> result;
    compiler->Compile(&buffer, arguments.data(), (UINT32)arguments.size(), &handler, IID_PPV_ARGS(&result));

    Microsoft::WRL::ComPtr<IDxcBlob> bcode, pdb, signature, reflection;
    Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors, disassembly, hlsl, text, remarks;

    for (UINT32 i = 0; i < result->GetNumOutputs(); ++i)
    {
        DXC_OUT_KIND kind = result->GetOutputByIndex(i);

        switch (kind)
        {
            case DXC_OUT_OBJECT:        result->GetOutput(kind, IID_PPV_ARGS(&bcode), 0);       break;
            case DXC_OUT_ERRORS:        result->GetOutput(kind, IID_PPV_ARGS(&errors), 0);      break;
            case DXC_OUT_PDB:           result->GetOutput(kind, IID_PPV_ARGS(&pdb), 0);         break;
            case DXC_OUT_DISASSEMBLY:   result->GetOutput(kind, IID_PPV_ARGS(&disassembly), 0); break;
            case DXC_OUT_HLSL:          result->GetOutput(kind, IID_PPV_ARGS(&hlsl), 0);        break;
            case DXC_OUT_TEXT:          result->GetOutput(kind, IID_PPV_ARGS(&text), 0);        break;
        }
    }
    if (errors && error) {
        error->append(errors->GetStringPointer(), errors->GetStringLength());
    }
    if (bcode && blob) {
        blob->clear();
        blob->append((char*)bcode->GetBufferPointer(), bcode->GetBufferSize());

        //auto metal_data = spirv_to_metal((uint32_t*)blob->c_str(), blob->size()/sizeof(uint32_t));
        //printf("\n%s", metal_data.c_str());
    }
#endif
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