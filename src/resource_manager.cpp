#include "resource_manager.h"
#include "resource_loader.h"
#include "json_serializer.h"
#include "material_data.h"
#include "threadpool.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

static constexpr uint16_t INVALID_SLOT_INDEX = 0xFFFF;
static constexpr uint16_t INVALID_POOL_INDEX = 0xFFFF;
static constexpr uint32_t INVALID_INSTANCE_ID = 0xFFFFFFFF;

struct jmeta
{
    std::string guid;
    ReflectObject(jmeta, ReflectObjectField(guid));
};

static const char * platform_string(platform_type type)
{
    switch(type) {
        case platform_type_pc:      return "pc";
        case platform_type_mobile:  return "mobile";
        case platform_type_web:     return "web";
        default :                   return "common";
    }
    return "common";
}

static bool is_meta_file(const char* file)
{
    const char* ext = file ? strrchr(file, '.') : "";
    if (ext && !strcmp(ext, ".meta"))  return true;
    if (ext && !strcmp(ext, ".jmeta")) return true;

    return false;
}

static bool need_recompile(const char* src, const char* dst, const char* meta)
{
    if (!std::filesystem::exists(dst)) return true;
    if (!std::filesystem::exists(meta)) return true;

    auto src_time = std::filesystem::last_write_time(src);
    auto meta_time = std::filesystem::last_write_time(meta);
    auto cooked_time = std::filesystem::last_write_time(dst);
    return src_time > cooked_time || meta_time > cooked_time;
}

static asset_type deduce_asset_type(const char* file)
{
    const char* ext = file ? strrchr(file, '.') : "";
    if (ext != nullptr) {
        if (!strcmp(ext, ".scene") || !strcmp(ext, ".json"))
            return asset_type_scene;
        if (!strcmp(ext, ".mesh") || !strcmp(ext, ".fbx") || !strcmp(ext, ".obj") || !strcmp(ext, ".usd"))
            return asset_type_mesh;
        if (!strcmp(ext, ".dds") || !strcmp(ext, ".ktx") || !strcmp(ext, ".png") || !strcmp(ext, ".tga") || !strcmp(ext, ".exr"))
            return asset_type_texture;
        if (!strcmp(ext, ".mat") || !strcmp(ext, ".material") || !strcmp(ext, ".mtl"))
            return asset_type_material;
        if (!strcmp(ext, ".shader") || !strcmp(ext, ".hlsl"))
            return asset_type_shader;
    }
    return asset_type_raw_data;
}

static const char* compiled_asset_extension(asset_type type/*, platform_type type*/)
{
    switch (type) {
        case asset_type_scene:      return ".bscene";
        case asset_type_mesh:       return ".mesh";
        case asset_type_texture:    return ".dds";
        case asset_type_material:   return ".mat";
        case asset_type_shader:     return ".shader";
        default: return nullptr;
    }
    return nullptr;
}



static std::string to_lower_ascii(std::string s)
{
    for (char& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

static std::string trim_trailing_slashes(std::string s)
{
    while (!s.empty() && (s.back() == '/' || s.back() == '\\')) s.pop_back();
    return s;
}

// a name given by a user: lower case, '/' only, no leading "./" or '/'
static std::string normalize_name(const char* name)
{
    std::string s = to_lower_ascii(name);
    std::replace(s.begin(), s.end(), '\\', '/');
    while (true) {
        if (s.compare(0, 2, "./") == 0)       s.erase(0, 2);
        else if (!s.empty() && s[0] == '/')   s.erase(0, 1);
        else break;
    }
    return s;
}

// `whole` ends with `tail` and the match starts at a path element ("dir/name.png" ends with "name.png", not with "me.png")
static bool ends_with_path(const std::string& whole, const std::string& tail)
{
    if (tail.empty() || tail.size() > whole.size()) return false;
    if (whole.compare(whole.size() - tail.size(), std::string::npos, tail) != 0) return false;
    return whole.size() == tail.size() || whole[whole.size() - tail.size() - 1] == '/';
}

static size_t filesize(FILE* file)
{
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

/*
bool parse_guid(const char* guid_str, guid_t* out_guid) {
    if (!guid_str || std::strlen(guid_str) != 32) return false;

    char high_part[17] = { 0 };
    char low_part[17] = { 0 };

    std::memcpy(high_part, guid_str, 16);
    std::memcpy(low_part, guid_str + 16, 16);

    char* endptr;
    out_guid->high = std::strtoull(high_part, &endptr, 16);
    if (*endptr != '\0') return false;

    out_guid->low = std::strtoull(low_part, &endptr, 16);
    if (*endptr != '\0') return false;

    return true;
}*/


bool parse_name_with_guid_hand(const char* filename, char* out_name, guid_t* out_guid, uint16_t* out_flag){
    if (!filename || !out_name || !out_guid || !out_flag) return false;

    char guid_buf[64] = { 0 };
    char flag_or_ext[64] = { 0 };
    char ext_buf[64] = { 0 };
    int scanned = sscanf(filename, "%255[^.].%32[^.].%63[^.].%s", out_name, guid_buf, flag_or_ext, ext_buf);

    if(strlen(guid_buf) == 32 && uuid::is_guid_str(guid_buf)) {
        *out_guid = uuid::str_to_guid(guid_buf);
        return true;
    }else {
        return false;
    }
    return false;
}


asset_handle_t material_manager::create_instance(class resource_manager& m, asset_handle_t handle)
{
    asset_slot_t* base_slot = m.slot_validate_and_get(handle);
    assert(base_slot && base_slot->type == asset_type_material);

    auto base_material = m.get_material(handle);
    uint16_t inst_id = 0;
 /*   uint16_t inst_id = m_instance_pool.allocate();

    material_instance_t* inst = &m_instances[inst_id];
    inst->base_handle = base_handle;
    inst->descriptor_set = allocate_vk_descriptor_set(base_slot);
    */
    asset_handle_t instance_handle = handle;
    instance_handle.instance_id = inst_id;

    return instance_handle;
}

void material_manager::destroy_instance(asset_handle_t instance_handle)
{
    if (instance_handle.instance_id == 0) return; // base material - do't unload
/*
    uint16_t inst_id = instance_handle.instance_id;
    material_instance_t* inst = &m_instances[inst_id];

    gfx_descriptor_set_destroy(inst->descriptor_set);

    m_instance_pool.free(inst_id);*/
}




void material_manager::set_global_texture(uint64_t id, asset_handle_t texture_handle)
{
    (void)id;
    (void)texture_handle;
}
void material_manager::set_global_texture(const char* name, asset_handle_t texture_handle)
{
    (void)name;
    (void)texture_handle;
}

void material_manager::set_global_data(uint64_t id, void* data, uint32_t size)
{
    (void)id;
    (void)data;
    (void)size;
    
 /*
 * struct uniform_material_binding{
    uint32_t        uniform_id;
    const char *    uniform_name;
    uint64_t        gfx_handle;
    uint32_t        material_count;
    material_t *    material;
};
 int m_name_count = 0;
    uniform_material_binding * bindnigs = nullptr;
    for(int i = 0; i < m_name_count; ++i)
    {
        auto b = bindnigs[i];
        if(b.uniform_id != id) continue;
        for(uint32_t j = 0; j < bindnigs[i].material_count; ++i)
        {
            auto set = b.material[j].descriptor_set;
            gfx_descriptor_set_write_buffer_data(set, b.gfx_handle, data, size);
            
        }
    }*/
}
void material_manager::set_global_data(const char* name, void* data, uint32_t size)
{
    if(!name || !data || !size) return;
    uint32_t id = gfx_utils_hash(name, (uint32_t)strlen(name));
    set_global_data(id, data, size);
}

void material_manager::set_texture(asset_handle_t material_handle, uint64_t id, asset_handle_t texture_handle)
{
    auto mat_instance = m_resource_manager->get_material_instance(material_handle);
    if(!mat_instance) return;

    auto texture = m_resource_manager->get_texture(texture_handle);
    if (!texture) return;

    gfx_descriptor_set_write_texture(mat_instance->descriptor_set, id, texture->texture_handle);

    // get prev texture and release it
   /* for(int i = 0; i < material->texture_count; ++i)
    {
        if(material->texture[i].id == id)
        {
            auto old_texture = material->texture[i].handle;
            auto new_texture = texture_handle;

            m_resource_manager->retain(new_texture);
            m_resource_manager->release(old_texture);
            material->texture[i].handle = texture_handle;
        }
    };*/
}

void material_manager::set_texture(asset_handle_t material_handle, const char* name, asset_handle_t texture_handle)
{
  //  uint64_t unfirm = gfx_uniform_location(m_gfx, name);
  //  set_texture(material_handle, unfirm, texture_handle);
}

void material_manager::set_data(asset_handle_t material, uint64_t id, const void* data, uint32_t size)
{
}
void material_manager::set_data(asset_handle_t material, const char* name, const void* data, uint32_t size)
{
}



resource_manager * resource_manager::s_shared = nullptr;

resource_manager::resource_manager(gfx_context_t* ctx, iallocator* memory_resource)
:m_ctx(ctx)
,m_allocator(memory_resource)
,m_textures(memory_resource, 1024)
,m_render_meshes(memory_resource, 1024)
,m_materials(memory_resource, 1024)
,m_material_instances(memory_resource, 1024)
,m_shaders(memory_resource, 1024)
{ 
    s_shared = this; 
}

resource_manager* resource_manager::create_and_make_shared(gfx_context_t* ctx, iallocator* memory_resource)
{
    if (s_shared == nullptr) 
    {
        s_shared = new resource_manager(ctx, memory_resource);
        s_shared->init(platform_type_pc);
    }
    return s_shared;
}


void resource_manager::init(platform_type type, const char* cash_path)
{
    const char * platform_name = platform_string(type);

    char cahs_dir[256] = "";
    sprintf(cahs_dir, "%s/%s/", cash_path ? cash_path : "cash", platform_name);
    m_asset_db.set_cash(cash_path ? cash_path : "cash", cahs_dir);
    load_asset_db();

    m_staging_allocator = new staging_allocator(m_allocator, 4*1024*1024, true);
    
    
    gfx_shader_t* test = nullptr;
    load_shader_from_file_path(m_ctx, "../assets/shaders/test.hlsl", &test);
    load_shader_from_file_path(m_ctx, "../assets/shaders/simple.hlsl", &m_default_shader);

  //  create_mesh_pool(m_ctx, (64)*1024*1024, (8)*1024*1024, &m_mesh_pool);
    create_mesh_pool(m_ctx, 380*1024*1024, 32*1024*1024, &m_mesh_pool);

    gfx_vertex_attribute attributes[] = {
        { 0, 0, gfx_format_float4,   0                   },
        { 3, 0, gfx_format_float4,   sizeof(vec4) * 1    },  // uv
        { 2, 0, gfx_format_float4,   sizeof(vec4) * 2    },  // normal
        { 1, 0, gfx_format_float4,   sizeof(vec4) * 3    },  // color
    };

    gfx_vertex_slot_t slots[] = {
        {0, sizeof(vec4) * _countof(attributes), gfx_vertex_rate_vertex},
        //     {1, sizeof(instance_data), gfx_vertex_rate_instance}
    };

    gfx_pipeline_desc_t piplene_desc = { 0 };
        piplene_desc.shader                     = m_default_shader;
        piplene_desc.assembly.topology          = gfx_topology_triangles;
        piplene_desc.assembly.attribute_count   = _countof(attributes);
        piplene_desc.assembly.attributes        = attributes;

        piplene_desc.assembly.slots                 = slots;
        piplene_desc.assembly.slot_count            = _countof(slots);
        piplene_desc.render_states.blend.enable     = false;
        piplene_desc.render_states.blend.color_src  = gfx_blend_mode_src_alpha;// VK_BLEND_FACTOR_SRC_ALPHA;
        piplene_desc.render_states.blend.color_dst  = gfx_blend_mode_inv_src_alpha;// VK_BLEND_FACTOR_SRC_ALPHA;
    m_default_pipeline = gfx_pipeline_create(m_ctx, &piplene_desc);

    m_slots_capacity = 16384;
    m_slots = (asset_slot_t*)m_allocator->allocate(sizeof(asset_slot_t) * m_slots_capacity, alignof(asset_slot_t));
    for(uint32_t i = 0; i < m_slots_capacity; ++i)
    {
        uint16_t idx = static_cast<uint16_t>(i);
        
        m_slots[idx].handle.slot_index =  (i + 1 < m_slots_capacity) ? static_cast<uint16_t>(i + 1) : INVALID_SLOT_INDEX;
        m_slots[idx].handle.generation = 1;
        m_slots[idx].handle.instance_id = 0;
        
        m_slots[idx].state = asset_state::empty;
        m_slots[idx].type = asset_type_unknown;
    }
}

// the compiled file of a source, relative to the cash dir:
//   <type dir>/<guid><ext>                         a scene keeps its name and the folders of the source:
//   scenes/<dir of the source>/<name>.bscene
static std::string asset_compiled_dir(asset_type type, guid_t guid, const std::filesystem::path& source, const std::filesystem::path& assets_dir)
{
    const char* folder = "raw";
    switch (type)
    {
        case asset_type_mesh:       folder = "meshes";      break;
        case asset_type_texture:    folder = "textures";    break;
        case asset_type_material:   folder = "materials";   break;
        case asset_type_shader:     folder = "shaders";     break;
        case asset_type_scene:      folder = "scenes";      break;
        default: break;
    }

    std::string name;
    if (type == asset_type_scene) {
        const std::string rel_dir = std::filesystem::relative(source.parent_path(), assets_dir).generic_string();
        name = (rel_dir == "." ? "" : rel_dir + "/") + source.stem().string();
    } else {
        char guid_str[64] = "";
        name = uuid::guid_to_str(guid, guid_str);
    }

    const char* ext = compiled_asset_extension(type);
    return std::string(folder) + "/" + name + (ext ? ext : source.extension().string().c_str());
}

void resource_manager::set_assets_path(const std::string & dir)
{
    measure ms("resource_manager::set_assets_path");
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        // no sources (a build with the compiled resources only): resources.db is all we know about the resources
        validate_asset_db();
        return;
    }
    if (m_dirs.find(dir) != m_dirs.end())
        return;

    m_dirs.emplace(dir);

    for (const auto& it : std::filesystem::recursive_directory_iterator(dir)) 
    {
        auto path = it.path();
        if (std::filesystem::is_directory(it)) continue;
        
        auto u8path = path.generic_string(); // convert '\\' to '/'
        if(is_meta_file(u8path.c_str()) )
            continue;

        if (!utf8::is_ascii(u8path.data(), u8path.length())) {
            debug::log_error("none ascii symbols: %s", u8path.c_str());
            continue;
        }

        auto meta_path = u8path + ".jmeta";
       
        guid_t asset_guid = get_or_create_guid(u8path.c_str(), meta_path.c_str());
        asset_type type = deduce_asset_type(u8path.c_str());

        const std::string cooked_filename = asset_compiled_dir(type, asset_guid, path, dir);
        const std::string cooked_filepath = m_asset_db.cash_file(cooked_filename);

        if(need_recompile(u8path.c_str(), cooked_filepath.c_str(), meta_path.c_str()))
        {
            if(m_compilers[type] != nullptr){
                std::filesystem::create_directories(std::filesystem::path(cooked_filepath).parent_path());
                m_compilers[type](u8path.c_str(), cooked_filepath.c_str(), meta_path.c_str(), m_platform_type, this);
            } else {
                debug::log_error("no compiler for resource: %s", u8path.c_str());
            }
        }

        bool iscooked = true;
        if(!std::filesystem::exists(cooked_filepath)){
            iscooked = false;
            debug::log_error("filed cooking file: %s", u8path.c_str());
          //  return;
        }

        // the original path (relative to the assets dir) -> the compiled file: what resources.db keeps
        asset_db::entry entry;
        entry.source = std::filesystem::relative(path, dir).generic_string();
        entry.cooked = cooked_filename;
        entry.type   = type;

        m_path_2_guid[interned_string(u8path.c_str())] = asset_guid;
        register_cooked(asset_guid, entry);
        (void)iscooked;
    };

    thread_pool::shared()->wait_for_all();
    for(auto & it : m_file_infos)
    {
        if(std::filesystem::exists(it.second.path))
            it.second.size = std::filesystem::file_size(it.second.path);
    }

    // the compilation is over: check that everything the db knows is there, then write it down
    validate_asset_db();
    m_asset_db.save();
}

std::string resource_manager::find(const std::string & name)
{
    return get_path(name.c_str());
}

std::string resource_manager::get_path(guid_t guid)
{
    return m_asset_db.cooked_path(guid);
}

std::string resource_manager::get_path(const char* name_or_guid)
{
    const guid_t guid = find_guid(name_or_guid);
    return guid == nullptr ? std::string() : get_path(guid);
}

std::string resource_manager::get_name(guid_t guid)
{
    return m_asset_db.source(guid);
}

guid_t resource_manager::find_guid(const char* name_or_guid)
{
    return m_asset_db.find_guid(name_or_guid);
}


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


material_instance_t* resource_manager::load_material_instance(const std::string_view& guid)
{
    return nullptr;
/*
    auto it = m_material_instances.find(guid);
    if (it != m_material_instances.end())
        return it->second.get();

    auto path = find(guid.data());

    if (path.empty())
        return nullptr;

    char *data = nullptr;
    size_t size = read_file_data(path.c_str(), &data);
    auto descriptor = json::from_json_string<material_descriptor>(size, data);
    free(data);

    auto material = std::make_shared<material_instance_t>();

    if(descriptor.shader.empty())
    {
        material->shader = m_default_shader;
        material->pipeline = m_default_pipeline;
    }

    for(size_t i = 0; i < descriptor.textures.size(); ++i)
    {
       // auto tex = load_texture(descriptor.textures[i].value.c_str());
       // material->textures[i].key = descriptor.textures[i].key;
      //  material->textures[i].guid = descriptor.textures[i].value;
      //  material->textures[i].value = tex  ? tex->texture_handle() : nullptr;
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
    return material.get();/**/
}

void resource_manager::directory_changed(std::filesystem::path &path)
{
}


guid_t resource_manager::get_or_create_guid(const char* path, const char * meta_path)
{
    if (path == nullptr) return {};
    if (strlen(path) < 5) return {};

    if(meta_path && std::filesystem::exists(meta_path))
    {
        auto data = file_data(meta_path);
        auto meta = json::from_json_string<jmeta>(data.size(), data.data());
        if(uuid::is_guid_str(meta.guid.c_str()))
            return uuid::str_to_guid(meta.guid.c_str());
        else
            debug::log_error("failed to read guid form meta file(%s)", meta_path);
    }

    uint32_t path_len = (uint32_t)strlen(path);
    if (path_len > 512) {
        debug::log_error("abnormal file path: %d", path);
        return {};
    }

    if (path_len == 32 && uuid::is_guid_str(path)) {
        return uuid::str_to_guid(path);
    }

    uint32_t path_hash = hasher::murmur32(path, (uint32_t)strlen(path));
    return uuid::generate_from_seed(path_hash);
}


// ======================================================================================================================
// resources.db: guid -> original path, compiled file. Lives in the root of the cash dir (next to the platform dirs),
// written after the resources are compiled, read back when the cash path is set. That is what lets the compiled
// resources be found by the path of the original ones when the sources are not there (build, DLC).
//
//   #resdb 1
//   <guid> <TAB> <asset_type> <TAB> <original path, relative to the assets dir> <TAB> <compiled file, relative to the platform dir>
//
// Text, sorted by the original path: readable, no diff noise. The paths are ascii (set_assets_path skips the others).
// ======================================================================================================================
static const char* k_resource_db_name   = "resources.db";
static const char* k_resource_db_header = "#resdb 1";

void asset_db::set_cash(const std::string& root, const std::string& platform_dir)
{
    m_cash_root = trim_trailing_slashes(root);
    m_cash_path = trim_trailing_slashes(platform_dir);
}

std::string asset_db::db_file_path() const
{
    return m_cash_root + "/" + k_resource_db_name;
}

std::string asset_db::cash_file(const std::string& relative) const
{
    return m_cash_path + "/" + relative;
}

void asset_db::add(guid_t guid, const entry& e)
{
    m_entries[guid] = e;
    m_index_dirty = true;
}

const asset_db::entry* asset_db::find(guid_t guid) const
{
    auto it = m_entries.find(guid);
    return it != m_entries.end() ? &it->second : nullptr;
}

std::string asset_db::source(guid_t guid) const
{
    const entry* e = find(guid);
    return e ? e->source : std::string();
}

std::string asset_db::cooked_path(guid_t guid) const
{
    const entry* e = find(guid);
    return e ? cash_file(e->cooked) : std::string();
}

void asset_db::rebuild_name_index() const
{
    m_source_2_guid.clear();
    m_name_2_guids.clear();

    for (const auto& it : m_entries)
    {
        if (it.second.source.empty()) continue;

        const std::string lower = to_lower_ascii(it.second.source);
        m_source_2_guid[lower] = it.first;

        const size_t slash = lower.rfind('/');
        m_name_2_guids[slash == std::string::npos ? lower : lower.substr(slash + 1)].push_back(it.first);
    }
    m_index_dirty = false;
}

// A resource by name: guid string | path of the original | the end of it | a part of it.
//   "b5a0953624bca8644be523fa1c4cada7"      the guid
//   "textures/wall.png"                     the path relative to the assets dir (the exact one wins)
//   "wall.png", "textures/wall.png"         the end of the path: the shortest path that ends with it wins
//   "../data/textures/wall.png"             a path that is longer than the known one (the assets dir in front)
//   "wall"                                  a part of the path, the last resort
guid_t asset_db::find_guid(const char* name_or_guid) const
{
    if (name_or_guid == nullptr || *name_or_guid == 0) return {};

    if (strlen(name_or_guid) == 32 && uuid::is_guid_str(name_or_guid))
        return uuid::str_to_guid(name_or_guid);

    const std::string query = normalize_name(name_or_guid);
    if (query.empty()) return {};

    if (m_index_dirty) rebuild_name_index();

    auto exact = m_source_2_guid.find(query);
    if (exact != m_source_2_guid.end())
        return exact->second;

    struct match_t { guid_t guid; size_t score; const std::string* source; };
    std::vector<match_t> matches;

    // the file name is the key of the index: only the files with the same name are looked at
    const size_t slash = query.rfind('/');
    const std::string base = slash == std::string::npos ? query : query.substr(slash + 1);
    auto bucket = m_name_2_guids.find(base);
    if (bucket != m_name_2_guids.end())
    {
        for (const guid_t& guid : bucket->second)
        {
            const std::string& source = m_entries.at(guid).source;
            const std::string lower_source = to_lower_ascii(source);
            if (ends_with_path(lower_source, query))         matches.push_back({ guid, lower_source.size() - query.size(), &source });
            else if (ends_with_path(query, lower_source))    matches.push_back({ guid, query.size() - lower_source.size(), &source });
        }
    }

    bool partial = false;
    if (matches.empty())
    {
        partial = true;
        for (const auto& it : m_entries) {
            const std::string lower_source = to_lower_ascii(it.second.source);
            if (lower_source.find(query) != std::string::npos)
                matches.push_back({ it.first, lower_source.size() - query.size(), &it.second.source });
        }
    }

    if (matches.empty()) return {};

    std::sort(matches.begin(), matches.end(), [](const match_t& a, const match_t& b) {
        return a.score != b.score ? a.score < b.score : *a.source < *b.source;      // the closest, then a stable order
    });

    if (matches.size() > 1 && matches[1].score == matches[0].score)
        debug::log_warning("[resource_manager] '%s' is ambiguous (%d matches), using '%s'", name_or_guid, (int)matches.size(), matches[0].source->c_str());
    else if (partial)
        debug::log_warning("[resource_manager] '%s': no exact name, using '%s'", name_or_guid, matches[0].source->c_str());

    return matches[0].guid;
}

void asset_db::load()
{
    const std::string path = db_file_path();
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return;

    const std::vector<char> data = resource_manager::file_data(path);
    if (data.empty()) return;

    const char* it  = data.data();
    const char* end = data.data() + data.size();
    size_t line_number = 0, loaded = 0, broken = 0;

    while (it < end)
    {
        const char* eol = (const char*)memchr(it, '\n', (size_t)(end - it));
        const char* line_end = eol ? eol : end;
        std::string line(it, line_end);
        it = eol ? eol + 1 : end;

        if (!line.empty() && line.back() == '\r') line.pop_back();
        ++line_number;

        if (line_number == 1) {
            if (line != k_resource_db_header) {
                debug::log_error("[resource_db] %s: unknown format '%s', ignored (it will be written again)", path.c_str(), line.c_str());
                return;
            }
            continue;
        }
        if (line.empty() || line[0] == '#') continue;

        // guid TAB type TAB source TAB cooked
        const size_t t1 = line.find('\t');
        const size_t t2 = t1 == std::string::npos ? t1 : line.find('\t', t1 + 1);
        const size_t t3 = t2 == std::string::npos ? t2 : line.find('\t', t2 + 1);
        const std::string guid_text = line.substr(0, t1);
        if (t3 == std::string::npos || guid_text.size() != 32 || !uuid::is_guid_str(guid_text.c_str())) {
            ++broken;
            continue;
        }

        const int type = atoi(line.substr(t1 + 1, t2 - t1 - 1).c_str());
        entry e;
        e.type   = (type > 0 && type < (int)asset_type_count) ? (asset_type)type : asset_type_unknown;
        e.source = line.substr(t2 + 1, t3 - t2 - 1);
        e.cooked = line.substr(t3 + 1);
        if (e.cooked.empty()) { ++broken; continue; }

        add(uuid::str_to_guid(guid_text.c_str()), e);
        ++loaded;
    }

    if (broken > 0)
        debug::log_error("[resource_db] %s: %d broken lines skipped", path.c_str(), (int)broken);
}

// Everything the db lists must be there: a compiled file that is missing is reported and dropped
// (so a load of it fails at once instead of reading nothing).
std::vector<guid_t> asset_db::validate()
{
    std::vector<guid_t> dropped;
    for (auto it = m_entries.begin(); it != m_entries.end(); )
    {
        std::error_code ec;
        const std::string full = cash_file(it->second.cooked);
        if (std::filesystem::exists(full, ec)) { ++it; continue; }

        char guid_text[64] = "";
        debug::log_error("[resource_db] '%s' (%s): compiled file is missing: %s", it->second.source.c_str(), uuid::guid_to_str(it->first, guid_text), full.c_str());

        dropped.push_back(it->first);
        it = m_entries.erase(it);
    }

    if (!dropped.empty()) {
        m_index_dirty = true;
        debug::log_error("[resource_db] %d resources are not available (compile them again: remove the compiled files of the cash dir)", (int)dropped.size());
    }
    return dropped;
}

void asset_db::save() const
{
    struct line_t { std::string source; std::string text; };
    std::vector<line_t> lines;
    lines.reserve(m_entries.size());

    for (const auto& it : m_entries)
    {
        const entry& e = it.second;
        if (e.source.find_first_of("\t\r\n") != std::string::npos || e.cooked.find_first_of("\t\r\n") != std::string::npos)
            continue;       // can't be written in a line

        char guid_text[64] = "";
        uuid::guid_to_str(it.first, guid_text);
        lines.push_back({ e.source, std::string(guid_text) + "\t" + std::to_string((int)e.type) + "\t" + e.source + "\t" + e.cooked + "\n" });
    }
    std::sort(lines.begin(), lines.end(), [](const line_t& a, const line_t& b) { return a.source != b.source ? a.source < b.source : a.text < b.text; });

    std::string text = std::string(k_resource_db_header) + "\n";
    for (const auto& line : lines) text += line.text;

    // nothing changed: leave the file alone (the time stays, no useless writes)
    const std::string path = db_file_path();
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        const std::vector<char> old = resource_manager::file_data(path);
        if (old.size() == text.size() && memcmp(old.data(), text.data(), text.size()) == 0)
            return;
    }

    std::filesystem::create_directories(m_cash_root, ec);

    const std::string tmp_path = path + ".tmp";
    FILE* file = fopen(tmp_path.c_str(), "wb");
    if (file == nullptr) {
        debug::log_error("[resource_db] can't write %s", tmp_path.c_str());
        return;
    }
    const bool written = fwrite(text.data(), 1, text.size(), file) == text.size();
    fclose(file);

    if (!written) {
        debug::log_error("[resource_db] failed writing %s", tmp_path.c_str());
        std::filesystem::remove(tmp_path, ec);
        return;
    }

    std::filesystem::rename(tmp_path, path, ec);
    if (ec) {
        debug::log_error("[resource_db] can't replace %s: %s", path.c_str(), ec.message().c_str());
        std::filesystem::remove(tmp_path, ec);
    }
}

// ---- the manager side: the db entries become the file infos the loader works with ----------------------------------

void resource_manager::register_file_info(guid_t guid, const asset_db::entry& entry)
{
    const std::string full = m_asset_db.cash_file(entry.cooked);

    std::error_code ec;
    const bool exists = std::filesystem::exists(full, ec);

    file_info_t info        = {};
    info.type               = entry.type;
    info.guid_rt            = uuid::runtime_guid(guid);
    info.guid               = guid;
    info.size               = exists ? (uint32_t)std::filesystem::file_size(full, ec) : 0;
    info.compressed_size    = info.size;
    info.path               = interned_string(full.c_str()).c_str();
    info.name               = interned_string(strrchr(full.c_str(), '/')).c_str();
    m_file_infos.insert_or_assign(info.guid_rt, info);
}

// a compiled resource of the scan
void resource_manager::register_cooked(guid_t guid, const asset_db::entry& entry)
{
    m_asset_db.add(guid, entry);
    register_file_info(guid, entry);
}

void resource_manager::load_asset_db()
{
    m_asset_db.load();
    for (const auto& it : m_asset_db.entries())
        register_file_info(it.first, it.second);
}

void resource_manager::validate_asset_db()
{
    for (guid_t guid : m_asset_db.validate())
        m_file_infos.erase(uuid::runtime_guid(guid));
}


void resource_manager::set_compiler(asset_type type, asset_compile_func_t func)
{
    m_compilers[type] = func;
}

asset_handle_t resource_manager::load(const char* path, load_params_t* params)
{
    if (path == nullptr) return { 0 };

    // a guid, or the name of the original resource (see find_guid)
    guid_t guid = find_guid(path);
    if (guid == nullptr) {
        debug::log_error("[resource_manager] load: resource '%s' not found", path);
        return INVALID_ASSET_HANDLE;
    }

    return load(guid, params);
}


asset_handle_t resource_manager::load(guid_t guid, load_params_t* param)
{
    if(guid == 0) return INVALID_ASSET_HANDLE;

    uint64_t fileid = uuid::runtime_guid(guid);
    file_info_t* file = get_file_info(fileid);
    asset_slot_t* slot = get_slot(fileid);

    if(slot != nullptr) {
        if(slot->type == asset_type_material)
            return create_material_instance(slot->handle);

        slot->ref_count++;
        return slot->handle;
    }

    asset_handle_t result = INVALID_ASSET_HANDLE;
    if(file != nullptr) {

        auto asset_handle = slot_allocate(fileid, file->type);
        auto slot = slot_validate_and_get(asset_handle);

        auto loader = m_loaders[(uint32_t)file->type];

        const char * dbg_name = strrchr(file->path, '/');
        slot->state = asset_state::loading;
        auto blob = read_asset_blob(file);

        if(slot->type == asset_type_mesh) 
        {
            render_mesh_t *mesh = m_render_meshes.allocate();
            int32_t index = m_render_meshes.get_index(mesh);
            load_mesh_from_file_data(m_ctx, &m_mesh_pool, dbg_name, (char*)blob.cpu_data, blob.data_size, mesh);

            if(mesh->index_buffer == nullptr || mesh->vertex_buffer == nullptr)
            {
                m_render_meshes.deallocate(mesh);
                free_asset_blob(&blob);
                return {};
            }

            slot->state = asset_state::ready;
            slot->index_in_pool = index > 0 ? index : 0;
            result = slot->handle;
        }
        if(slot->type == asset_type_texture)
        {
            gfx_texture_t * gfx_texture = nullptr;
            load_texture_from_file_data(m_ctx, dbg_name, (char*)blob.cpu_data, blob.data_size, &gfx_texture);

            if(gfx_texture != nullptr)
            {
                auto texture = m_textures.allocate();
                texture->texture_handle = gfx_texture;

                slot->state = asset_state::ready;
                slot->index_in_pool = m_textures.get_index(texture);
            }
            result = slot->handle;
        }
        if (slot->type == asset_type_material)
        {
            material_t * material = m_materials.allocate();

            material_json desc;
            if (load_material_from_file_data(m_ctx, (char*)blob.cpu_data, blob.data_size, material, &desc))
            {
                for (uint16_t i = 0; i < material->texture_count; ++i)
                {
                    const asset_handle_t th = load(desc.textures[i].guid, nullptr);
                    material->textures[i] = const_cast<texture_t*>(get_texture(th));    // null if the texture is unknown
                }
            }

            material->manager = this;

            if(material->shader == nullptr)
                material->shader = m_default_shader;

            if(material->pipeline == nullptr)
                material->pipeline = m_default_pipeline;

            slot->state = asset_state::ready;
            slot->index_in_pool = m_materials.get_index(material);
            result = create_material_instance(slot->handle);
        }

        free_asset_blob(&blob);
        // job_system_push_io_request(slot->guid, handle.id, desired_mip);

        m_guid_2_slots.emplace(fileid, slot);

        return result;
    }

    return INVALID_ASSET_HANDLE;
}

void resource_manager::release(asset_handle_t handle)
{
    asset_slot_t* slot = slot_validate_and_get(handle);
    if(!slot) return;

    if (slot->ref_count > 0 && ((--slot->ref_count) == 0))
    {
        slot_free(handle);

        switch (slot->type)
        {
            case asset_type_texture:    break;
            case asset_type_material:   break;
            case asset_type_mesh:       break;
        }
    }
}

void resource_manager::set_cash_path(const char* path)
{
    char tmp[512] = "";
    strcat(tmp, path);

    uint32_t len = (uint32_t)strlen(tmp);
    if (tmp[len - 1] != '/') {
        tmp[len++] = '/'; 
        tmp[len++] = '\0';
    }

    strcat(tmp, platform_string(m_platform_type));

    std::error_code ec;     // no exceptions in this project: a throw here terminates the app (e.g. read-only cwd on iOS)
    if(!std::filesystem::exists(tmp, ec))
        std::filesystem::create_directories(tmp, ec);
    if (ec)
        debug::log_error("resource_manager::set_cash_path('%s'): %s", tmp, ec.message().c_str());

    m_asset_db.set_cash(path, std::filesystem::path(tmp).generic_string());
    load_asset_db();        // what the previous runs compiled, the compiled resources can be used without the sources
}

void resource_manager::set_dlc_path(const char* path)
{
    debug::log_error("resource_manager::set_dlc_path() - not implemented");
}

asset_slot_t* resource_manager::get_slot(uint64_t runtime_guid)
{
    auto it = m_guid_2_slots.find(runtime_guid);
    return it != m_guid_2_slots.end() ? it->second : nullptr;
}

file_info_t* resource_manager::get_file_info(uint64_t runtime_guid)
{
    auto it = m_file_infos.find(runtime_guid);
    return it != m_file_infos.end() ? &it->second : nullptr;
}

void resource_manager::update()
{
    gc_process();
    perform_resource_uploading();
}

void resource_manager::wait(asset_handle_t handle)
{
    debug::log_error("resource_manager::wait no implemented");

    const uint32_t idx = handle.slot_index;
    if (idx >= m_slots_capacity) return;

    asset_slot_t& slot = m_slots[idx];
    if (slot.handle.generation != handle.generation) return;

    if (slot.state == asset_state::ready) return;


    if(slot.state == asset_state::loading)
    {
       //auto request = job_steal_request(slot.guid);
       // load_internal(request.guid);

       // slot.state = asset_state::ready;
    }
}



asset_handle_t  resource_manager::slot_allocate(uint64_t guid_rt, asset_type type)
{
    if (m_first_free_slot == INVALID_SLOT_INDEX) {
        return INVALID_ASSET_HANDLE;
    }

    const uint16_t slot_idx = m_first_free_slot;
    asset_slot_t* slot = &m_slots[slot_idx];

    m_first_free_slot = slot->handle.slot_index;

    slot->handle.slot_index = slot_idx;
    slot->handle.instance_id = 0;

    slot->guid_rt = guid_rt;
    slot->index_in_pool = INVALID_POOL_INDEX;
    slot->ref_count = 1;
    slot->type = type;
    slot->state = asset_state::queued;

    m_allocated_count++;

    return slot->handle;
}

void resource_manager::slot_free(asset_handle_t handle)
{
    asset_slot_t* slot = slot_validate_and_get(handle);
    if (!slot) return;

    uint16_t next_gen = slot->handle.generation + 1;
    if (next_gen == 0) next_gen = 1;
    slot->handle.generation = next_gen;

    slot->guid_rt = 0;
    slot->ref_count = 0;
    slot->type = asset_type_unknown;
    slot->state = asset_state::empty;
    slot->index_in_pool = INVALID_POOL_INDEX;

    slot->handle.slot_index = m_first_free_slot;
    m_first_free_slot = handle.slot_index; 

    m_allocated_count--;
}


asset_handle_t resource_manager::create_material_instance(asset_handle_t handle)
{
    static std::unordered_map<interned_string, interned_string> remaps = {
        {"_MainTex", "_texture0"},
        {"_Albedo", "_texture0"}
    };
    asset_slot_t* slot = slot_validate_and_get(handle);
    if(slot->type != asset_type_material)
        return INVALID_ASSET_HANDLE;

    slot->ref_count++;
    material_t * material = m_materials.get_by_index(slot->index_in_pool);

    auto material_instance = m_material_instances.allocate();
    auto material_instance_id = m_material_instances.get_index(material_instance);

    material_instance->instance = material;
    material_instance->descriptor_set = gfx_descriptor_set_create(m_ctx, material->shader, 0);

    for(int i = 0; i < material->texture_count; ++i)
    {
        auto location = gfx_uniform_location(material->shader, material->texture_names[i]);
        if(location == 0)
        {
            auto remapped = remaps.find(material->texture_names[i]);
            if(remapped == remaps.end())
                continue;

            location = gfx_uniform_location(material->shader, remapped->second.c_str());
            if (location == 0)
                continue;
        }
        
        auto texture = material->textures[i];
        gfx_descriptor_set_write_texture(material_instance->descriptor_set, location, texture->texture_handle);
    }

    asset_handle_t result = slot->handle;
    result.instance_id = material_instance_id;

    return result;
}

void resource_manager::gc_process()
{
    if(m_gc_handles.empty()) return;

}

asset_io_blob_t resource_manager::read_asset_blob(file_info_t* info)
{
    asset_io_blob_t blob = {};
    blob.cpu_data = m_staging_allocator->allocate(info->size, 16);
    blob.data_size = info->size;

    // TODO: replace to virtual fs(android)
    FILE* file = fopen(info->path, "rb");
    if (file) {
        fread(blob.cpu_data, blob.data_size, 1, file);
        fclose(file);
    }
    return blob;
}

void resource_manager::free_asset_blob(asset_io_blob_t* blob)
{
    if (blob && blob->cpu_data) {
        memset(blob->cpu_data, 0, blob->data_size);
        m_staging_allocator->deallocate(blob->cpu_data);
        memset(blob, 0, sizeof(asset_io_blob_t));
    }
}


asset_handle_t resource_manager::load_material(asset_io_blob_t & blob)
{
    return INVALID_ASSET_HANDLE;
}


void resource_manager::perform_resource_uploading()
{
    size_t bytes_uploaded_this_frame = 0;

    asset_io_blob_t blob = {};

    uint32_t m_max_upload_budget_per_frame = 50;
    // We fetch ready data from I/O workers while it's still in the queue.
    // And while we haven't hit the frame limit (f.e. 50 MB)
    while (bytes_uploaded_this_frame < m_max_upload_budget_per_frame) {
         break;
     /*   // We try to retrieve the result of the background thread's work
        // If the queue is empty, we exit the loop; there's nothing else to do.
        if (!lock_free_queue_pop(&m_workers_results_queue, &blob)) {
            break;
        }

        // Protection: check if the slot index is valid
        if (blob.slot_index >= m_max_slots) {
            // Someone returned a broken index - free up the worker's staging memory and move on
            m_staging_allocator->deallocate(blob.cpu_data);
            continue;
        }

        asset_slot_t* slot = &m_slots[blob.slot_index];

        if (slot->type == asset_type::texture) {
            texture_t* tex = &slot->as.texture;

            gfx_texture_t* new_texture = gfx_create_texture_from_ram(m_gfx_ctx, &tex->desc, blob.cpu_data, blob.requested_mip);

            if (new_texture) {
             
                gfx_texture_t* old_texture = tex->gfx_handle;

                tex->gfx_handle = new_texture;
                tex->current_mip = blob.requested_mip;

                slot->state = asset_state_t::ready;

                // If there was an old texture, we safely destroy it on the GPU.
                if (old_texture && old_texture != m_fallback_texture) {
                    gfx_texture_destroy(m_gfx_ctx, old_texture);
                }

                bytes_uploaded_this_frame += blob.data_size;
            }
            else {
                // If gfx couldn't create a texture, mark the asset as broken
                slot->state = asset_state_t::failed;
            }
        }

        else if (slot->type == asset_type::asset_type_mesh) {
            // slot->as.mesh = gfx_create_mesh_buffers(m_gfx_ctx, blob.cpu_data);
            // slot->state = asset_state_t::ready;
            // bytes_uploaded_this_frame += blob.data_size;
        }
        m_staging_allocator->deallocate(blob.cpu_data);*/
    }
}
