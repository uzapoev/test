#include "scene.h"
//#include "../assets/asset_unity.h"


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


scene scene::load(const std::string& path)
{
    measure ms("\nscene loading");
    auto data = resource_manager::file_data(path);
    std::string tmp(data.begin(), data.end());

    auto result = json::from_json_string<scene>(tmp);
    //auto result = scene();

    std::unordered_set<atomic_string> meshes;
    std::unordered_set<atomic_string> materials;
    std::unordered_set<atomic_string> textures;

    std::vector<node*> allnodes;
    for(size_t i = 0; i < result.m_nodes.size(); ++i)
    {
        allnodes.push_back(&result.m_nodes[i]);
        result.traverse(result.m_nodes[i], allnodes);
    }

    for (size_t i = 0; i < allnodes.size(); ++i)
    {
        auto & node = allnodes[i];
        if (!node->renderer.mesh_guid.empty())
            meshes.insert(node->renderer.mesh_guid);

        if (!node->renderer.material_guid.empty())
            materials.insert(node->renderer.material_guid);

        if (!node->renderer.lightmap_guid.empty())
            textures.insert(node->renderer.lightmap_guid);
    }

    std::unordered_map<atomic_string, std::shared_ptr<gfx_mesh_t>> mesh_cash;
    int current_vb_size = 0;
    int current_ib_size = 0;
    int compressed_vb_size = 0;
    {
        measure ms("\nmesh loading");
        for(auto m : meshes)
        {
            auto _mesh = resource_manager::shared()->load_mesh(m.c_str());
            if(_mesh)
            {
                current_ib_size += _mesh->index_count * sizeof(uint16_t);
                current_vb_size += _mesh->vertex_count * sizeof(vertex);
                compressed_vb_size += _mesh->vertex_count * sizeof(vertex_compressed);
                mesh_cash.emplace(m, _mesh);
            }
        }
    }
    float fi = (float)(current_ib_size) / 1024.0f / 1024.0f;
    float fv = (float)(current_vb_size) / 1024.0f / 1024.0f;
    float fcv = (float)(compressed_vb_size) / 1024.0f / 1024.0f;
    float profit = (fv + fi)/(fcv + fi);

    for (size_t i = 0; i < allnodes.size(); ++i)
    {
        renderer_t renderer = {};
        auto & node = *allnodes[i];
      //  auto one = vec3 {1, 1, 1};
      //  node.transform.rotation = quat::identity();
        renderer.transform = mat4::trs( node.transform.position,
                                        node.transform.rotation,
                                        node.transform.scale);

      //  if(strstr(node.name.c_str(), "LOD0"))          continue;
        if(strstr(node.name.c_str(), "LOD1"))   continue;
        if(strstr(node.name.c_str(), "LOD2"))   continue;
        if(strstr(node.name.c_str(), "LOD3"))   continue;
        if(strstr(node.name.c_str(), "Imposter"))   continue;
        if(strstr(node.name.c_str(), "Impostor"))   continue;

        // mesh
        auto mesh_guid = node.renderer.mesh_guid.c_str();
        if(mesh_guid == nullptr)
            continue;

        auto mesh_it = mesh_cash.find(mesh_guid);
        if(mesh_it == mesh_cash.end())
            continue;

        renderer.mesh = mesh_it->second.get();
      //      renderer.mesh = resource_manager::shared()->load_mesh(mesh_guid).get();

        // material
        auto material_guid = node.renderer.material_guid.c_str();
        renderer.material = resource_manager::shared()->load_material(material_guid).get();

        // lighmap
        if(!node.renderer.lightmap_guid.empty())
        {
            renderer.lightmap.lightmap = resource_manager::shared()->load_texture(node.renderer.lightmap_guid.c_str()).get();
            renderer.lightmap.scale_offset = node.renderer.lightmap_scale_offset;
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


void scene::update()
{
}

void scene::draw(camera & camera)
{
    mat4 mv;
    auto visible_renderers = cull(mv); // culling

    auto passes = sort_by_passes(visible_renderers); // sort by transparent, by passes

    for(size_t i = 0; passes.size(); ++i)
    {
       /* var batches = make_batches(pass, pass.renderers);
        foreach(var batch in batches)
        {
            draw(batch);
        }*/
    }
    /*
    gfx_cmd_bind_pipeline(cmd, pipeline);

    for (int i = 0; i < m_renderers.size(); ++i)
    {
        renderer_t& renderer = m_renderers[i];
        draw_renderer(cmd, &renderer);
    }

    gfx_cmd_end_pass(cmd);*/

}

void scene::traverse(node &n, std::vector<node*>& allnodes)
{
    for(size_t i = 0; i < n.childs.size(); ++i)
    {
        allnodes.push_back(&n.childs[i]);
        traverse(n.childs[i], allnodes);
    }
}

std::vector<renderer_t> scene::cull(const mat4& mvp)
{
    // frustum check acceleration strucs(static objects)
    m_visibles.clear();
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

std::vector<pass> scene::sort_by_passes(const std::vector<renderer_t>&)
{
    return {};
}