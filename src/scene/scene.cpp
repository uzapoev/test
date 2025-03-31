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
        auto result = scene_reader_json::create_form_file(path);
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

      //  if(strstr(node.name.c_str(), "LOD0"))          continue;
        if(strstr(node->name.c_str(), "LOD1"))   continue;
        if(strstr(node->name.c_str(), "LOD2"))   continue;
        if(strstr(node->name.c_str(), "LOD3"))   continue;
        if(strstr(node->name.c_str(), "Imposter"))   continue;
        if(strstr(node->name.c_str(), "Impostor"))   continue;

        // mesh
        auto mesh_guid = node->renderer.mesh_guid;
        auto lighmap_guid = node->renderer.lightmap_guid;
        auto material_guid = node->renderer.material_guid.c_str();

        auto mesh = resource_manager::shared()->load_mesh(mesh_guid.c_str());
        if(mesh == nullptr)
            continue;

        renderer.mesh = mesh->mesh();
        renderer.material = resource_manager::shared()->load_material(material_guid).get();
        renderer.world_bounds = bbox::create(renderer.mesh->bounds, renderer.transform);

        if(!lighmap_guid.empty())
        {
            renderer.lightmap.lightmap = resource_manager::shared()->load_texture(lighmap_guid.c_str())->texture_();
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
    filestream * stream = filestream::open_rb(path.c_str());

    //scene_header_t header = stream->read<scene_header_t>();
    uint32_t node_count = stream->read<uint32_t>();

    m_nodes.resize(node_count);// = allocator->alloc<node>(nodes_count);
    for (uint32_t i = 0; i < node_count; i++)
    {
        char buffer[512] = "";

        while(1)
        {
            uint16_t id = stream->read<uint16_t>();
            uint16_t size = stream->read<uint16_t>();

            if(id == component_node_end)
                break;

            switch (id)
            {
                case component_node_begin:
                    m_nodes[i].name = stream->read_string(buffer);
                    m_nodes[i].guid = stream->read_string(buffer);
                    m_nodes[i].tag = stream->read_string(buffer);
                    m_nodes[i].flags = stream->read<uint64_t>();
                break;

                case component_transform:
                    stream->read(size, &m_nodes[i].transform); 
                break;

                case component_renderer:
                    m_nodes[i].renderer.mesh_guid = stream->read_string(buffer);
                    m_nodes[i].renderer.material_guid = stream->read_string(buffer);
                    m_nodes[i].renderer.lightmap_guid = stream->read_string(buffer);
                    m_nodes[i].renderer.lightmap_scale_offset = stream->read<vec4>();
                break;

                default: 
                stream->read(size, &buffer); 
                break;
            }
        }
        

  /*      auto chunk = (chunk*)stream->read(sizeof(chunk));
        auto payload = stream->read(chunk.size);
        switch (chunk.id)
        {
            case component_node: m_nondes[i] = alocate_node<m_nondes>(payload);  break;

            case chunk_component:
                switch (payload.type)
                {
                    case component_transform:   auto component = components.allocate<transform>(payload);   break;
                    case component_renderer:    auto component = components.allocate<renderer>(payload);    break;
                    case component_collider:    auto component = components.allocate<collider>(payload);    break;
                    case component_rigidbody:   auto component = components.allocate<rigidbody>(payload);   break;
                    case component_light:       auto component = components.allocate<light>(payload);       break;
                    case component_animator:    auto component = components.allocate<animator>(payload);    break;
                    case component_cinematic:   auto component = components.allocate<light>(payload);       break;
                    case component_navagent:    auto component = components.allocate<navagent>(payload);    break;
                    case component_lodgroup:    auto component = components.allocate<lodgroup>(payload);    break;
                    default:                    auto component = components.allocate<unknown>(payload);     break;

                m_nondes[i].add_omponent(component);
        }*/
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
    m_instances.clear();

    m_nodes.clear();
    m_nodes_flat_list.clear();

    m_meshes.clear();
    m_materials.clear();
}

void scene::update()
{
}

void scene::draw(camera & camera)
{
    auto visible_renderers = cull(camera.m_view_proj); // culling

 /*   auto passes = sort_by_passes(visible_renderers); // sort by transparent, by passes

    for(size_t i = 0; passes.size(); ++i)
    {
        var batches = make_batches(pass, pass.renderers);
        foreach(var batch in batches)
        {
            draw(batch);
        }
    }*/
    /*
    gfx_cmd_bind_pipeline(cmd, pipeline);

    for (int i = 0; i < m_renderers.size(); ++i)
    {
        renderer_t& renderer = m_renderers[i];
        draw_renderer(cmd, &renderer);
    }

    gfx_cmd_end_pass(cmd);*/

}

void scene::traverse(node & n, std::function<void(node&)> &cb)
{
    for (size_t i = 0; i < n.childs.size(); ++i)
    {
        cb(n.childs[i]);
        traverse(n.childs[i], cb);
    }
}

const std::vector<renderer_t*>& scene::cull(const mat4& vp)
{
    PROFILE_SAMPLE("\nscene::cull");

    m_visibles.clear();
    frustum fr = frustum::from_view_proj(vp);

    for (int i = 0; i < m_renderers.size(); ++i)
    {
        auto & renderer = m_renderers[i];

        if (!frustum::check_bbox(fr, renderer.world_bounds))
            continue;

        m_visibles.push_back(&renderer);
    }

    make_instances(m_visibles);

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

void scene::make_instances(const std::vector<renderer_t*> & renderers)
{
    PROFILE_SAMPLE("\n  scene::instances");
    m_instances.clear();

    for (int i = 0; i < renderers.size(); ++i)
    {
        // renderers[i]->mesh
        m_instances[renderers[i]->mesh]++;
    }
    
}

std::vector<pass> scene::sort_by_passes(const std::vector<renderer_t> &)
{
    return {};
}