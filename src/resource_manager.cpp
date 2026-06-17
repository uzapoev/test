#include "resource_manager.h"
#include "resource_loader.h"
#include "json_serializer.h"

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


resource_manager * resource_manager::s_shared = nullptr;

resource_manager* resource_manager::create_and_make_shared(gfx_context_t* ctx)
{
    if (s_shared == nullptr) 
    {
        s_shared = new resource_manager(ctx);
        s_shared->init(platform_type_pc);
    }
    return s_shared;
}


void resource_manager::init(platform_type type, const char* cash_path)
{
    const char * platform_name = platform_string(type);

    cash_path = cash_path? cash_path : "cash";
    char cahs_dir[256] = "";
    sprintf(cahs_dir, "%s/%s/", cash_path, platform_name);
    m_cash_path.assign(cahs_dir);

    m_allocator = new aligned_allocator("resource_manager");
    m_staging_allocator = new staging_allocator(m_allocator, 4*1024*1024, true);

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

    //m_meshes.reserve(1024);
    //m_textures.reserve(1024);

   // m_texture_allocator = new paged_pool_allocator(m_allocator, sizeof())
}

void resource_manager::set_assets_path(const std::string & dir)
{
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

        char cooked_filename[256] = "";
        const char * extension = compiled_asset_extension(type);
        extension = extension ? extension : ext.c_str();
        make_cooked_filename_buf(stem.c_str(), asset_guid, type, extension, cooked_filename, sizeof(cooked_filename));

        char cooked_filepath[256] = "";
        snprintf(cooked_filepath, sizeof(cooked_filepath), "%s/%s/%s",
            m_cash_path.c_str(), relative_parent.c_str(), cooked_filename);

        path::canonicalize_resource_path(cooked_filepath, cooked_filepath, sizeof(cooked_filename)); // replace '\\' to '/' and make strlwr
        
        if(need_recompile(u8path.c_str(), cooked_filepath, meta_path.c_str()))
        {
            auto asset_compiler = m_compilers[type];
            if(asset_compiler != nullptr){
                asset_compiler(u8path.c_str(), cooked_filepath, meta_path.c_str(), m_platform_type, this);
            } else {
                debug::log_error("no compiler for resource: %s", u8path.c_str());
            }
        }

        if(!std::filesystem::exists(cooked_filepath)){
            debug::log_error("filed cooking file: %s", u8path.c_str());
            return;
        }

        char guid_str[33] = "";
        m_path_to_guid.emplace(u8path.c_str(), asset_guid);
        m_guid_2_path.emplace(asset_guid, std::filesystem::path(cooked_filepath));
        m_guid_to_guid.emplace(uuid::guid_to_str(asset_guid, guid_str), asset_guid);

        file_info_t info        = {};
        info.type               = type;
        info.guid               = uuid::runtime_guid(asset_guid);
        info.size               = std::filesystem::file_size(cooked_filepath);
        info.compressed_size    = info.size;
        info.path               = interned_string(cooked_filepath).c_str();
        m_file_infos.emplace(info.guid, info);
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


std::shared_ptr<material_t> resource_manager::load_material(const char * name, gfx_shader_t* shader)
{
    auto instance = load_material_instance(name);
    if(instance == nullptr)
        return nullptr;

    auto material = std::make_shared<material_t>();
    material->instance = instance;
    material->descriptor_set = gfx_descriptor_set_create(m_ctx, instance->shader, 0);
    
    for(size_t i = 0; i < _countof(instance->textures); ++i)
    {
        uint64_t handle = gfx_uniform_location(instance->shader, instance->textures[i].key.c_str());
        if(handle && instance->textures[i].value)
            gfx_descriptor_set_write_texture(material->descriptor_set, handle, instance->textures[i].value);
    }

    for (size_t i = 0; i < _countof(instance->vectorsf); ++i)
    {
        uint64_t handle = gfx_uniform_location(instance->shader, instance->vectorsf[i].key.c_str());
        if (handle)
            gfx_descriptor_set_write_buffer_data(material->descriptor_set, handle, &instance->vectorsf[i].value, sizeof(vec4));
    }

    m_materials.push_back(material);
    
    return material;
}


/*
texture* resource_manager::load_texture(const char* name)
{
    return m_texture_manager.load(name, false, nullptr);
}*/



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
        char* data = nullptr;
        size_t size = read_file_data(meta_path, &data);
        auto meta = json::from_json_string<jmeta>(size, data);
        if(data) free(data);
        if(uuid::is_guid_str(meta.guid.c_str()))
            return uuid::str_to_guid(meta.guid.c_str());
        else
            debug::log_error("failed to read guid form meta file(%s)", meta_path);
    }

    uint32_t path_len = strlen(path);
    if (path_len > 512) {
        debug::log_error("abnormal file path: %d", path);
        return {};
    }

    if (path_len == 32 && uuid::is_guid_str(path)) {
        return uuid::str_to_guid(path);
    }

    uint32_t path_hash = hasher::murmur32(path, strlen(path));
    return uuid::generate_from_seed(path_hash);
}

bool resource_manager::need_recompile(const char* src, const char* compiled, const char* meta)
{
    if(!std::filesystem::exists(compiled)) return true;
    if(!std::filesystem::exists(meta)) return true;

    auto src_time = std::filesystem::last_write_time(src);
    auto meta_time = std::filesystem::last_write_time(meta);
    auto cooked_time = std::filesystem::last_write_time(compiled);
    return src_time > cooked_time || meta_time > cooked_time;
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
    uint64_t fileid = uuid::runtime_guid(guid);

    if (fileid == 0) {
        //  debug::log_error("no file at path: %d", path);
        return INVALID_ASSET_HANDLE;
    }

    if(asset_slot_t* slot = get_slot(fileid)) {
        slot->ref_count++;
        return slot->handle;
    }

    if(file_info_t* file = get_file_info(fileid)) {
        asset_slot_t slot = {};

        const char * dbg_name = strrchr(file->path, '/');
        slot.guid = fileid;
        slot.state = asset_state::loading;
        slot.type = file->type;

        slot.handle.generation++;
        slot.handle.slot_index = (uint32_t)m_slots.size();

        auto blob = read_asset_blob(file);

        if(slot.type == asset_type_mesh) 
        {
            render_mesh_t mesh = {};
            load_mesh_from_file_data(m_ctx, &m_mesh_pool, dbg_name, (char*)blob.cpu_data, blob.data_size, &mesh);
            //load_mesh_from_file_path(m_ctx, &m_mesh_pool, file->debug_path, &mesh);
            slot.state = asset_state::ready;

            m_render_meshes.emplace_back(mesh);
            slot.index_in_pool = (uint32_t)m_render_meshes.size() - 1;
        }
        if(slot.type == asset_type_texture)
        {
            gfx_texture_t * texture_handlde = nullptr;
            load_texture_from_file_data(m_ctx, dbg_name, (char*)blob.cpu_data, blob.data_size, &texture_handlde);

            slot.state = asset_state::ready;

            texture_t texture = { texture_handlde };
            m_textures_new.emplace_back(texture);
            slot.index_in_pool = (uint32_t)m_textures_new.size();
        }

        free_asset_blob(&blob);
        // job_system_push_io_request(slot->guid, handle.id, desired_mip);

        m_guid_to_loaded_asset_slots.emplace(fileid, slot);
        m_slots.emplace_back(slot);

        return slot.handle;
    }

    return {0};
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
}

asset_slot_t* resource_manager::get_slot(uint64_t runtime_guid)
{
    auto it = m_guid_to_loaded_asset_slots.find(runtime_guid);
    return it != m_guid_to_loaded_asset_slots.end() ? &it->second : nullptr;
}

file_info_t* resource_manager::get_file_info(uint64_t runtime_guid)
{
    auto it = m_file_infos.find(runtime_guid);
    return it != m_file_infos.end() ? &it->second : nullptr;
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