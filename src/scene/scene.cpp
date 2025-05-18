#include "scene.h"
#include "scene_json.h"


void camera::update()
{
    _axis.y = math::clamp(_axis.y, -80.0f, 80.0f);
    _rot = quat::from_euler(_axis.y, _axis.x, 0.0f);

    vec3 dir = math::normalize(_pos - _target);
    vec3 target = quat::mul(_rot, dir) + _pos;

    bool right_hand = false;
    mat4 proj = math::perspective_matrix(math::deg2rad(m_fov), m_aspect, m_near, m_far, right_hand);
    mat4 view = math::look_at_matrix(_pos, target, math::Up, right_hand);

    m_view_proj = math::mul(proj, view);
}

scene scene::create_from_file(const std::string& path)
{
    scene result;
    if(path.empty())
        return result;
}


scene scene::create_from_json_file(const std::string& path)
{
    measure ms("\nscene loading");

    scene result;
    std::string bin_path = path;
    bin_path.append(".bin");
    if(std::filesystem::exists(bin_path))
    {
        result.load(bin_path);
        result.init();
    }
    else
    {
        result = scene_reader_json::create_form_file(path);
        result.init();
        result.save(bin_path);
    }

    int current_vb_size = 0;
    int current_ib_size = 0;
    int compressed_vb_size = 0;
    {
        measure ms("\nmesh loading");
        for(auto m : result.m_meshes)
        {
            auto mesh = resource_manager::shared()->load_mesh(m.c_str());
            if(mesh)
            {
                current_ib_size += mesh->mesh()->index_count * sizeof(uint16_t);
                current_vb_size += mesh->mesh()->vertex_count * sizeof(vertex);
                compressed_vb_size += mesh->mesh()->vertex_count * sizeof(vertex_compressed);
            }
            else
                debug::log_error("failed load mesh %s", m.c_str());
        }
    }
    float fi = (float)(current_ib_size) / (1024.0f * 1024.0f);
    float fv = (float)(current_vb_size) / (1024.0f * 1024.0f);
    float fcv = (float)(compressed_vb_size) / 1024.0f / 1024.0f;
    float profit = (fv + fi)/(fcv + fi);
     
    for (size_t i = 0; i < result.m_nodes_flat_list.size(); ++i)
    {
        renderer_t renderer = {};
        auto node = result.m_nodes_flat_list[i];
        renderer.transform = mat4::trs( node->transform.position,
                                        node->transform.rotation,
                                        node->transform.scale);

   //     if(strstr(node->name.c_str(), "LOD0"))          continue;
        if(strstr(node->name.c_str(), "LOD1"))   continue;
        if(strstr(node->name.c_str(), "LOD2"))   continue;
        if(strstr(node->name.c_str(), "LOD3"))   continue;
        if(strstr(node->name.c_str(), "Imposter"))   continue;
        if(strstr(node->name.c_str(), "Impostor"))   continue;

        // mesh
        auto mesh_guid = node->renderer.mesh_guid;
        auto lighmap_guid = node->renderer.lightmap_guid;
        auto material_guid = node->renderer.material_guid.c_str();

        if(mesh_guid.empty())
            continue;

        auto mesh = resource_manager::shared()->load_mesh(mesh_guid.c_str());
        if(mesh == nullptr)
            continue;

        renderer.mesh = mesh->mesh();
        renderer.material = resource_manager::shared()->load_material(material_guid).get();
        renderer.world_bounds = bbox::create(renderer.mesh->bounds, renderer.transform);

        if(renderer.material == nullptr)
            debug::log_error("failed to load material %s", material_guid);

        if(!lighmap_guid.empty())
        {
            renderer.lightmap.lightmap = resource_manager::shared()->load_texture(lighmap_guid.c_str())->texture_handle();
            renderer.lightmap.scale_offset = node->renderer.lightmap_scale_offset;
        }

        if(renderer.material != nullptr && renderer.mesh != nullptr)
            result.m_renderers.emplace_back(renderer);
    }

    auto ctx = resource_manager::shared()->ctx();
    gfx_sampler_desc_t desc = {};
        desc.mode = gfx_address_mode_repeat;
        desc.minmag = gfx_filter_linear;
        desc.mipmap = gfx_filter_linear;
        desc.anisotropy = 8;
    auto sampler = gfx_create_sampler2(ctx, &desc);

    // update lightmap data
    for (size_t i = 0; i < result.m_renderers.size(); ++i)
    {
        auto & renderer = result.m_renderers[i];

        auto shader = renderer.material->instance->shader;
        auto set = renderer.material->descriptor_set;

        auto texture_location = gfx_uniform_location(shader, "_texture0");
        auto sampler_location = gfx_uniform_location(shader, "_textureSampler");
        auto lightmap_location = gfx_uniform_location(shader, "_lightmap");
        auto lightmap_scale_offset_location = gfx_uniform_location(shader, "lightmap_scale_offset");

        if(texture_location && renderer.material->instance->textures[0].value)
            gfx_uniform_set_texture(set, texture_location, renderer.material->instance->textures[0].value);

       if(lightmap_scale_offset_location != 0)
            gfx_uniform_set_buffer_data(set, lightmap_scale_offset_location, &result.m_renderers[i].lightmap.scale_offset, sizeof(vec4));

        if(lightmap_location != 0)
            gfx_uniform_set_texture(set, lightmap_location, result.m_renderers[i].lightmap.lightmap);

        if (sampler_location != 0)
            gfx_uniform_set_sampler(set, sampler_location, sampler);
    }
    
    return result;
}


void scene::load(const std::string& path)
{
    PROFILE_SAMPLE("scene::load")
    filestream * stream = filestream::open_rb(path.c_str());

    //scene_header_t header = stream->read<scene_header_t>();
    uint32_t node_count = stream->read<uint32_t>();

    auto p0 = sizeof(node);
    auto p1 = sizeof(tinynode);

    m_nodes.resize(node_count);// = allocator->alloc<node>(nodes_count);
    for (uint32_t i = 0; i < node_count; i++)
    {
        char buffer[512] = "";

        while(1)
        {
            memset(buffer, 0, sizeof(buffer));
            uint16_t id = stream->read<uint16_t>();
            uint16_t size = stream->read<uint16_t>();

            if(id == component_node_end)
                break;

            switch (id)
            {
                case component_node_begin:
                    m_nodes[i].name  = stream->read_string(buffer);
                    m_nodes[i].guid  = stream->read_string(buffer);
                    m_nodes[i].tag   = stream->read_string(buffer);
                    m_nodes[i].flags = stream->read<uint64_t>();
                break;

                case component_transform:
                    stream->read(size, &m_nodes[i].transform); 
                break;

                case component_renderer: {
                    auto component = m_world.allocate_component<components::renderer>(this, buffer);

                    m_nodes[i].renderer.mesh_guid = stream->read_string(buffer);
                    m_nodes[i].renderer.material_guid = stream->read_string(buffer);
                    m_nodes[i].renderer.lightmap_guid = stream->read_string(buffer);
                    m_nodes[i].renderer.lightmap_scale_offset = stream->read<vec4>();
                } break;

                case component_collider: {
                   //     auto component = m_world.allocate_component<components::collider>(this, buffer);

                        auto collider_type = stream->read<uint16_t>();
                        auto trigger_type = stream->read<uint16_t>();   // is trigger

                        // read<float4> center + payload (radius for sphere and capsule), 
                        // read<float4> payload (extend for box)
                        // stream->read_string(buffer); // mesh guid
                        // stream->read_string(buffer); // material guid

                        switch(collider_type)
                        {
                            case 0: stream->read<vec4>(); break;                        // sphere:  x,y,z - pos, w - radius
                            case 1: stream->read<vec4>(); stream->read<vec4>(); break;  // box: center(float4) + extend (float4)
                            case 2: stream->read<vec4>(); stream->read<float>(); break; // capsule: center_radius(float4) + height(float) 
                            case 3: stream->read_string(buffer);  break;                // mesh: guid string
                            default: 
                                debug::log_warning("(%s) has unknown colider type - %d", m_nodes[i].name, collider_type);
                            break;
                        }
                        auto bbox_min = stream->read<vec3>();
                } break;

                default: 
                    stream->read(size, &buffer); 
                break;
            }
        }
        

  /*    auto chunk = (chunk*)stream->read(sizeof(chunk));
        auto payload = stream->read(chunk.size);

        auti node = nullptr;
        icompomemt component = nullptr;
        switch (chunk.type)
        {
            case component_node:        node      = m_world.allocate_node<m_nondes>(payload);     break;

            case component_transform: {
                int  id  = stream->read<int32_t>();
                auto component = m_components.allocate<transform>(payload);
                m_nodes[id].add_component(component);
            } break;

            case component_renderer:    component = m_world.allocate_component<renderer> (this, payload); break;
            case component_collider:    component = m_world.allocate_component<collider> (this, payload); break;
            case component_rigidbody:   component = m_world.allocate_component<rigidbody>(this, payload); break;
            case component_light:       component = m_world.allocate_component<light>    (this, payload); break;
            case component_animator:    component = m_world.allocate_component<animator> (this, payload); break;
            case component_cinematic:   component = m_world.allocate_component<light>    (this, payload); break;
            case component_navagent:    component = m_world.allocate_component<navagent> (this, payload); break;
            case component_lodgroup:    component = m_world.allocate_component<lodgroup> (this, payload); break;
            default:                    component = m_world.allocate_component<unknown>  (this, payload); break;
        }
        stream->seek(chunk.size, SEEK_CURR);    // anyway rewind to end of chunk

        m_world.allocate_component(scene, node, component);
        */
    }
}

void scene::save(const std::string& path)
{
    filestream * stream = filestream::open_wb(path.c_str());

    stream->write<uint32_t>((uint32_t)m_nodes_flat_list.size());

    for (uint32_t i = 0; i < m_nodes_flat_list.size(); i++)
    {
        auto & node = m_nodes_flat_list[i];

        auto chunk_size =   node->name.length() + sizeof(uint16_t) +
                            node->guid.length() + sizeof(uint16_t) +
                            node->tag.length()  + sizeof(uint16_t) +
                            sizeof(uint64_t);

        stream->write_chunk_info(component_node_begin, (uint16_t)chunk_size);
        stream->write_string((uint16_t)node->name.length(), node->name.data());
        stream->write_string((uint16_t)node->guid.length(), node->guid.data());
        stream->write_string((uint16_t)node->tag.length(),  node->tag.data());
        stream->write(node->flags);


        stream->write_chunk(component_transform, sizeof(components::transform), (char*)&node->transform);

        if(!node->renderer.mesh_guid.empty()) {
            auto chunk_size =   node->renderer.mesh_guid.length() + sizeof(uint16_t) +
                                node->renderer.material_guid.length() + sizeof(uint16_t) +
                                node->renderer.lightmap_guid.length() + sizeof(uint16_t) +
                                sizeof(node->renderer.lightmap_scale_offset);

            stream->write_chunk_info(component_renderer, (uint16_t)chunk_size);
            stream->write_string((uint16_t)node->renderer.mesh_guid.length(),     node->renderer.mesh_guid.data());
            stream->write_string((uint16_t)node->renderer.material_guid.length(), node->renderer.material_guid.data());
            stream->write_string((uint16_t)node->renderer.lightmap_guid.length(), node->renderer.lightmap_guid.data());
            stream->write(node->renderer.lightmap_scale_offset);
        }

        stream->write_chunk(component_node_end, 0, 0);
    }

    stream->flush();
   // stream->write(sizeof(),)
    delete stream;
}

void scene::init()
{
    std::function<void(node&)> cb = [this](node& n) {
        m_nodes_flat_list.push_back(&n);

        if (!n.renderer.mesh_guid.empty())
            m_meshes.insert(n.renderer.mesh_guid);

        if (!n.renderer.material_guid.empty())
            m_materials.insert(n.renderer.material_guid);
        };
   
    m_meshes.reserve(1024 * 8);
    m_nodes_flat_list.reserve(1024 * 8);

    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        cb(m_nodes[i]);
        traverse(m_nodes[i], cb);
    }

    m_visibles.reserve(m_meshes.size());
}

void scene::clear()
{
    m_renderers.clear();
    m_visibles.clear();

    m_nodes.clear();
    m_nodes_flat_list.clear();

    m_meshes.clear();
    m_materials.clear();
}

void scene::update()
{
}

void scene::draw(gfx_command_buffer_t* cmd, camera & camera)
{
    gfx_cmd_push_marker(cmd, "test");
    mat4 vp = camera.view_proj();
    auto visible_renderers = cull(vp); // culling

    uint64_t  mvp_location = 0;
    gfx_shader_t * shader = nullptr;
    for(int i = 0; i < visible_renderers.size(); ++i) 
    {
        auto & renderer = visible_renderers[i];
        if(renderer->material->instance->shader != shader) {
            shader = renderer->material->instance->shader;
            mvp_location = gfx_uniform_location(shader, "mvp");
        }

        mat4 mvp = math::mul(vp, renderer->transform);
        gfx_uniform_set_buffer_data(renderer->material->descriptor_set, mvp_location, &mvp, sizeof(mat4));
    }

    gfx_pipeline_t* pipeline = nullptr;
    for (int i = 0; i < visible_renderers.size(); ++i)
    {
        auto & renderer = visible_renderers[i];
        if(renderer->material->instance->pipeline != pipeline)
        {
            pipeline = renderer->material->instance->pipeline;
            gfx_cmd_bind_pipeline(cmd, pipeline);
        }
        draw_renderer(cmd, renderer);
    }
    gfx_cmd_pop_marker(cmd);
}

void scene::traverse(node & n, std::function<void(node&)> &cb)
{
    for (size_t i = 0; i < n.childs.size(); ++i)
    {
        cb(n.childs[i]);
        traverse(n.childs[i], cb);
    }
}


struct renderer_sort
{
    bool operator()(renderer_t* a, renderer_t* b) const {
        if (a->material->instance->pipeline == b->material->instance->pipeline) {
            if (a->material->instance == b->material->instance) {
                if (a->mesh == b->mesh) {
                    return a < b;
                }
                return a->mesh < b->mesh;
            }
            return a->material->instance < b->material->instance;
        }
        return a->material->instance->pipeline < b->material->instance->pipeline;
    }
};

const std::vector<renderer_t*>& scene::cull(const mat4& vp)
{
  //  PROFILE_SAMPLE("\nscene::cull");

    m_visibles.clear();
    frustum fr = frustum::from_view_proj(vp);

    for (int i = 0; i < m_renderers.size(); ++i)
    {
        auto & renderer = m_renderers[i];
        //if(renderer.is_in_lod)
        //  continue;

        if (!frustum::check_bbox(fr, renderer.world_bounds))
            continue;

        m_visibles.push_back(&renderer);
    }

    std::sort(m_visibles.begin(), m_visibles.end(), renderer_sort());

    return m_visibles;

    // frustum check acceleration structs(static objects)

 //   for(auto it = m_trees.begin(); it != m_trees.end(); ++it)
 //   {
 //       //auto visibles = it->traverse(mvp);
 //       //m_visibles.append(visibles);
 //   }
 //
 //   // frustum check without acceleration strucs(dynamic objects)
 //   for (auto it = m_dynamic_objects.begin(); it != m_dynamic_objects.end(); ++it)
 //   {
 //       //auto visibles = furustum_check(mvp, m_dynamic_objects);
 //       //m_visibles.append(visibles);
 //   }

   // hi-z occlusion culling
   // grab prev depth buffer, build hi-z, compute visibility
   // if(hiz_enabled)
   // {
   //   var depth_rt = renderer->get_depth_rt();
   //
   //   auto dept_target_h = gfx_uniform_get_location("depth_texture");
   //   auto visibility_h = gfx_uniform_get_location("visibility_buffer");
   // 
   //   gfx_uniform_set_texture(descriptor, dept_target_h, depth_rt);
   //   gfx_uniform_set_buffer(descriptor, visibility_h, depth_rt);
   // 
   //   // build hi-z buffer
   //   gfx_cmd_bind_pipeline(cmd, build_hi_z_buffer);
   //   gfx_cmd_bind_descriptor_set(cmd, descriptor);
   //   gfx_cmd_dispatch_compute(cmd, 16, 16, 16);
   // 
   //   // sort by distance?
   // 
   //   // compute visibilities
   //   gfx_cmd_bind_pipeline(cmd, build_visibility_buffer);
   //   gfx_cmd_bind_descriptor_set(cmd, descriptor);
   //   gfx_cmd_dispatch_compute(cmd, 16, 16, 16);
   // 
   //   gfx_submit(cmd);
   // } 

    return m_visibles;
}
