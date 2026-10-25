#include "resource_manager.h"
#include "resource_loader.h"
#include "json_serializer.h"


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

static void make_cooked_filename_buf(const char* path, guid_t guid, asset_type type, const char* ext, char* cooked_filename, size_t buffer_size)
{
    assert(path && ext);
    char guid_str[33] = "";
    char canonical_path[256] = "";
    path::canonicalize_resource_path(path, canonical_path, sizeof(canonical_path));
    snprintf(cooked_filename, buffer_size, "%s.%s.%d%s", canonical_path, uuid::guid_to_str(guid, guid_str), type, ext);
}


static void for_each_file_recursive(const std::filesystem::path & path, std::function<void(const std::filesystem::path&)> callback)
{
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path) ) {
        return;
    }

    for (const auto& it : std::filesystem::recursive_directory_iterator(path)) {
        if(!std::filesystem::is_directory(it))
            callback(it.path());
    }
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
    char extra[8] = { 0 };
    int scanned = sscanf(filename, "%255[^.].%32[^.].%63[^.].%s", out_name, guid_buf, flag_or_ext, ext_buf);

    if(strlen(guid_buf) == 32 && uuid::is_guid_str(guid_buf)) {
        *out_guid = uuid::str_to_guid(guid_buf);
        return true;
    }else {
        return false;
    }
    return false;
}

bool asset_load_mesh(struct resource_manager& mgr, asset_io_blob_t& blob, asset_slot_t& slot)
{
 /*   render_mesh_t* mesh = mgr.m_render_meshes.allocate();
    int32_t index = m_render_meshes.get_index(mesh);

    load_mesh_from_file_data(mgr.ctx(), &m_mesh_pool, blob.asset_name, (char*)blob.cpu_data, blob.data_size, mesh);

    if (mesh->index_buffer == nullptr || mesh->vertex_buffer == nullptr)
    {
        m_render_meshes.deallocate(mesh);
        return false;
    }

    slot.state = asset_state::ready;
    slot.index_in_pool = index > 0 ? index : 0;
    */
    return false;
}

bool asset_load_texture(struct resource_manager& mgr, asset_io_blob_t& blob, asset_slot_t& slot)
{
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
{/*
    if (instance_handle.instance_id == 0) return; // base material - do't unload

    uint16_t inst_id = instance_handle.instance_id;
    material_instance_t* inst = &m_instances[inst_id];

    gfx_descriptor_set_destroy(inst->descriptor_set);

    m_instance_pool.free(inst_id);*/
}




void material_manager::set_global_texture(uint64_t id, asset_handle_t texture_handle)
{
}
void material_manager::set_global_texture(const char* name, asset_handle_t texture_handle)
{
}

void material_manager::set_global_data(uint64_t id, void* data, uint32_t size)
{
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
    m_cash_path.assign(cahs_dir);

    m_staging_allocator = new staging_allocator(m_allocator, 4*1024*1024, true);
    
    
    gfx_shader_t* test = nullptr;
    load_shader_from_file_path(m_ctx, "../data/shaders/test.hlsl", &test);
    load_shader_from_file_path(m_ctx, "../data/shaders/simple.hlsl", &m_default_shader);

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
        
        m_slots[idx].handle.slot_index =  (i + 1 < m_slots_capacity) ? static_cast<uint16_t>(i + 1) : INVALID_SLOT_INDEX;;
        m_slots[idx].handle.generation = 1;
        m_slots[idx].handle.instance_id = 0;
        
        m_slots[idx].state = asset_state::empty;
        m_slots[idx].type = asset_type_unknown;
    }
}

void resource_manager::set_assets_path(const std::string & dir)
{
    measure ms("resource_manager::set_assets_path");
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir) || (m_dirs.find(dir) != m_dirs.end()) ) {
        return;
    }

    m_dirs.emplace(dir);

    for_each_file_recursive(dir, [&](const std::filesystem::path & path){
        auto u8path = path.string();
        if(is_meta_file(u8path.c_str()) )
            return;

        if (!utf8::is_ascii(u8path.data(), u8path.length())) {
            debug::log_error("none ascii symbols: %s", u8path.c_str());
            return;
        }

        auto meta_path = u8path + ".jmeta";
       
        guid_t asset_guid = get_or_create_guid(u8path.c_str(), meta_path.c_str());
        asset_type type = deduce_asset_type(u8path.c_str());

        auto stem = path.stem().string();
        auto ext = path.extension().string();
        auto parent_path = path.parent_path().string();
        auto relative_parent = std::filesystem::relative(path.parent_path(), dir).string();

        if (relative_parent == ".")
            relative_parent = "";

        char guid_str_buf[64] = "";
        char cooked_filename[256] = "";
        const char * extension = compiled_asset_extension(type);
        extension = extension ? extension : ext.c_str();
        //make_cooked_filename_buf(stem.c_str(), asset_guid, type, extension, cooked_filename, sizeof(cooked_filename));
        if(type == asset_type_scene) {
            sprintf(cooked_filename, "%s%s", stem.c_str(), extension);
        } else {
            sprintf(cooked_filename, "%s%s", uuid::guid_to_str(asset_guid, guid_str_buf), extension);
        }

        char cooked_filepath[256] = "";
      //  !m_cash_path.empty()?strcat(cooked_filepath, m_cash_path.c_str()):"";
      //  !relative_parent.empty()?strcat(cooked_filepath, relative_parent.c_str()):"";
      //  strcat(cooked_filepath, cooked_filename);

        snprintf(cooked_filepath, sizeof(cooked_filepath), "%s/%s/%s",
            m_cash_path.c_str(), relative_parent.c_str(), cooked_filename);

        path::canonicalize_resource_path(cooked_filepath, cooked_filepath, sizeof(cooked_filename)); // replace '\\' to '/' and make strlwr
        
        if(need_recompile(u8path.c_str(), cooked_filepath, meta_path.c_str()))
        {
            if(m_compilers[type] != nullptr){
                m_compilers[type](u8path.c_str(), cooked_filepath, meta_path.c_str(), m_platform_type, this);
            } else {
                debug::log_error("no compiler for resource: %s", u8path.c_str());
            }
        }

        if(!std::filesystem::exists(cooked_filepath)){
            debug::log_error("filed cooking file: %s", u8path.c_str());
            return;
        }

       // char guid_str[33] = "";
        m_path_2_guid.emplace(u8path.c_str(), asset_guid);
        m_guid_2_path.emplace(asset_guid, std::filesystem::path(cooked_filepath));

        file_info_t info        = {};
        info.type               = type;
        info.guid_rt            = uuid::runtime_guid(asset_guid);
        info.guid               = asset_guid;
        info.size               = (uint32_t)std::filesystem::file_size(cooked_filepath);
        info.compressed_size    = info.size;
        info.path               = interned_string(cooked_filepath).c_str();
        m_file_infos.emplace(info.guid_rt, info);
    });
}

std::string resource_manager::find(const std::string & name)
{
    auto lowername(name);
    std::transform(lowername.begin(), lowername.end(), lowername.begin(), [](unsigned char c){ return std::tolower(c); });
    
    static std::string empty;

    if(uuid::is_guid_str(name.c_str()))
    {
        guid_t guid = uuid::str_to_guid(name.c_str());
        auto it2 = m_guid_2_path.find(guid);
        if (it2 != m_guid_2_path.end())
        {
            return it2->second.u8string();
        }
    }
    
    return empty;
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


void resource_manager::set_compiler(asset_type type, asset_compile_func_t func)
{
    m_compilers[type] = func;
}

asset_handle_t resource_manager::load(const char* path, load_params_t* params)
{
    if (path == nullptr) return { 0 };

    guid_t guid = get_or_create_guid(path, nullptr);

    return load(guid, params);
}


asset_handle_t resource_manager::load(guid_t guid, load_params_t* param)
{
    if(guid == 0) return INVALID_ASSET_HANDLE;

    uint64_t fileid = uuid::runtime_guid(guid);

    if(asset_slot_t* slot = get_slot(fileid)) {
        if(slot->type == asset_type_material)
        {
            auto instance = create_material_instance(slot->handle);
            debug::log_warning("");
        }
        slot->ref_count++;
        return slot->handle;
    }

    if(file_info_t* file = get_file_info(fileid)) {

        
        auto handle2 = slot_allocate(fileid, file->type);
        auto slot = slot_validate_and_get(handle2);

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
        }
        if(slot->type == asset_type_texture)
        {
            gfx_texture_t * texture_handle = nullptr;
            load_texture_from_file_data(m_ctx, dbg_name, (char*)blob.cpu_data, blob.data_size, &texture_handle);

            if(texture_handle != nullptr)
            {
                slot->state = asset_state::ready;
                auto texture = m_textures.allocate();
                auto index = m_textures.get_index(texture);
                texture->texture_handle = texture_handle;
                slot->index_in_pool = index;
            }
            
        }
        if (slot->type == asset_type_material)
        {
            material_t material = {};
            load_material_from_file_data(m_ctx, (char*)blob.cpu_data, blob.data_size, &material);
            material.manager = this;
            material.pipeline = m_default_pipeline;
            material.shader = m_default_shader;
            /*m_material_instances_new.emplace_back(instance);

            material_instance_t material = {};
            material.instance = &base_material;
            auto set = gfx_descriptor_set_create(m_ctx, m_default_shader, 0);
            m_materials_new.emplace_back(material);

            slot->index_in_pool = (uint32_t)m_materials_new.size();*/
        }

        free_asset_blob(&blob);
        // job_system_push_io_request(slot->guid, handle.id, desired_mip);

        m_guid_2_slots.emplace(fileid, slot);

        return slot->handle;
    }

    return {0};
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
    path::canonicalize_resource_path(path, tmp, sizeof(tmp));

    uint32_t len = (uint32_t)strlen(tmp);
    if (tmp[len - 1] != '/') {
        tmp[len++] = '/'; 
        tmp[len++] = '\0';
    }

    strcat(tmp, platform_string(m_platform_type));

    if(!std::filesystem::exists(tmp))
        std::filesystem::create_directories(tmp);

    m_cash_path = tmp;
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
    asset_slot_t* slot = slot_validate_and_get(handle);
    if(slot->type != asset_type_material)
        return INVALID_ASSET_HANDLE;

    //debug::breakpoint();
    return INVALID_ASSET_HANDLE;
    //m_material_mananget
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
    FILE* file = fopen(info->path, "r");
    if (file) {
        fread(blob.cpu_data, blob.data_size, 1, file);
        fclose(file);
    }
    return blob;
}

void resource_manager::free_asset_blob(asset_io_blob_t* blob)
{
    if (blob && blob->cpu_data) {
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