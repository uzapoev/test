#include "scene.h"
#include "world.h"
#include "../memmgr.h"
#include "scene_json.h"


struct scene_chunk_info_t {
    uint32_t type = 0;
    uint32_t size = 0;
  //  uint32_t id   = 0;
};

struct scene_header_t {
    uint32_t magick = MAKEFOURCC('S', 'C', 'N', '0');
    uint32_t version;
    uint32_t node_count;
};

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

scene scene::create_from_file(const std::string_view& path)
{
    scene result;
    result.load(path);
  //  result.init();
    return result;
}


scene scene::create_from_json_file(const std::string_view& path)
{
    measure ms("\nscene loading");

    scene result;
    std::string bin_path = path.data();
    bin_path.append(".bin");
    if(std::filesystem::exists(bin_path)) {
        result.load(bin_path);
        result.init();
    } else if(std::filesystem::exists(path)) {
        result = scene_reader_json::create_form_file(path);
        result.init();
        result.save(bin_path);
    } else {
        debug::log_error("no file at path %s", path.data());
        return result;
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
                compressed_vb_size += mesh->mesh()->vertex_count * sizeof(vertex_packed);
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

        for(int i = 0; i < 8; i++)
            renderer.bounds.extend({renderer.world_bounds.corners[i].x,
                                    renderer.world_bounds.corners[i].y,
                                    renderer.world_bounds.corners[i].z });

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



void scene::load(const std::string_view& path)
{
    if(m_node_allocator == nullptr){
        auto allocator = new aligned_allocator("scene node");

        m_world = new world();
        m_node_allocator = new paged_pool_allocator(allocator, sizeof(node), 1024*16);
        m_transform_allocator = new paged_pool_allocator(allocator, sizeof(transform), 1024*16);
        m_hierarchy_allocator = new paged_pool_allocator(allocator, sizeof(hierarchy), 1024*16);
    }

    PROFILE_SAMPLE("scene::load")
    filestream * stream = filestream::open_rb(path.data());

    //scene_header_t header = stream->read<scene_header_t>();
    auto header = stream->read<scene_header_t>();

    m_nodes.resize(header.node_count);// = allocator->alloc<node>(nodes_count);

    scene_chunk_info_t info = {};
    while(stream->read(sizeof(scene_chunk_info_t), &info) != 0)
    {
        if(info.type == component_node)        
        {
            auto node = m_world->load_node(this, stream);
            m_nodes_flat_list.emplace_back(node);
        } 
        else if(info.type == component_transform) 
        {
            auto component = m_world->deserialize<transform>(this, stream);
            m_nodes_flat_list.back()->transform = *component;
        } 
        else if(info.type == component_renderer) 
        {
            auto component = m_world->deserialize<renderer>(this, stream);
            m_nodes_flat_list.back()->renderer = *component;

            m_meshes.insert(component->mesh_guid.c_str());
        }/**/
        else 
        {
            stream->seek(info.size, 1);
        }
    }
}


node* scene::create_node(interned_string name, interned_string guid)
{
    node * result = m_node_allocator->allocate<node>();
    result->name = name;
    result->guid = guid;
    m_allocated_nodes.push_back(result);
    return result;
}

transform * scene::create_transform()
{
    return m_transform_allocator->allocate<transform>();
}



void scene::save(const std::string_view& path)
{
    filestream * stream = filestream::open_wb(path.data());

    scene_header_t header = {};
        header.magick       = MAKEFOURCC('S', 'C', 'N', '0');
        header.version      = 1;
        header.node_count   = (uint32_t)m_nodes_flat_list.size();
    stream->write(header);
    // write meta

    for (uint32_t i = 0; i < m_nodes_flat_list.size(); i++)
    {
        auto & node = m_nodes_flat_list[i];
        node->index = i;

        m_world->save_node(node, stream);
    }
    for (uint32_t i = 0; i < m_nodes_flat_list.size(); i++)
    {
        auto& node = m_nodes_flat_list[i];
        m_world->serialize<transform>(&node->transform, stream);
        if (!node->renderer.mesh_guid.empty())
            m_world->serialize<renderer>(&node->renderer, stream);
     }

    stream->flush();
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

void scene_draw_indirect(gfx_command_buffer_t* cmd, camera& camera)
{
    struct batch {
        gfx_pipeline_t * pipeline;
    };
    std::vector<batch> m_batches;

    gfx_cmd_push_marker(cmd, "scene_draw_indirect");

    gfx_buffer_t * indirect_buffer = nullptr;
    gfx_descriptor_set_t * descriptor_set_camera = nullptr;
    gfx_descriptor_set_t * descriptor_set_instance = nullptr;
    gfx_descriptor_set_t * descriptor_set_bindless = nullptr;

    for (size_t i = 0; i < m_batches.size(); i++)
    {
        gfx_cmd_bind_pipeline(cmd, m_batches[i].pipeline);
        gfx_cmd_bind_descriptor_set(cmd, 0, descriptor_set_camera);
        gfx_cmd_bind_descriptor_set(cmd, 1, descriptor_set_instance);
        gfx_cmd_bind_descriptor_set(cmd, 2, descriptor_set_bindless);

        gfx_cmd_draw_indexed_indirect(cmd, indirect_buffer, 0, m_batches.size(), sizeof(indirect_data_t));
    }

    gfx_cmd_pop_marker(cmd);
}

void scene::draw(gfx_command_buffer_t* cmd, camera & camera)
{
    gfx_cmd_push_marker(cmd, "scene::draw");
    mat4 vp = camera.view_proj();
    auto visible_renderers = cull(camera); // culling

    uint64_t  mvp_location = 0;
    gfx_shader_t * shader = nullptr;
    {
        measure ms("uniform:update");
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
    }

 /*   gfx_pipeline_t* pipeline = nullptr;
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
*/


    gfx_pipeline_t* pipeline = nullptr;
    gfx_buffer_t * vertex_buffer = nullptr;
    gfx_buffer_t * index_buffer = nullptr;

    for (int i = 0; i < visible_renderers.size(); ++i)
    {
        auto& renderer = visible_renderers[i];
        if (renderer->material->instance->pipeline != pipeline)
        {
            pipeline = renderer->material->instance->pipeline;
            gfx_cmd_bind_pipeline(cmd, pipeline);
        }

        auto mesh = renderer->mesh;

        gfx_cmd_bind_descriptor_set(cmd, 0, renderer->material->descriptor_set);

        if(vertex_buffer != mesh->vertex_buffer){
            vertex_buffer = mesh->vertex_buffer;
            gfx_cmd_bind_vertex_buffer(cmd, 0, 0, vertex_buffer);
        }

        if(index_buffer != mesh->index_buffer)
        {
            index_buffer = mesh->index_buffer;
            gfx_cmd_bind_index_buffer(cmd, mesh->index_format, 0, mesh->index_buffer);
        }

        if(mesh->index_format == gfx_index_format_32)
        {
            debug::log_error("fuck");
        }

        int32_t start_idx = 0;
        int32_t vertex_offset = mesh->vertex_buffer_offset/mesh->vertex_stride;
        int32_t index_offset = mesh->index_buffer_offset/sizeof(uint16_t);

        for (size_t sub_idx = 0; sub_idx < mesh->submesh_count; sub_idx++)
        {
            uint32_t count = mesh->submeshes[sub_idx];
            gfx_cmd_draw_indexed(cmd, count, index_offset + start_idx, 1, vertex_offset);
            start_idx += mesh->submeshes[sub_idx];
        }

        if (mesh->submesh_count == 0)
        {
            gfx_cmd_draw_indexed(cmd, mesh->index_count, index_offset, 1, vertex_offset);
        }
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
#include <execution>

bool DistanceCullSphere(const vec3& camPos, const vec3& sphereCenter, float radius, float maxDistance) {
    float distSq = math::distance_sq(camPos, sphereCenter);
    float maxDistSq = (maxDistance + radius);// * (maxDistance + radius);
    return distSq > maxDistSq;
}

const std::vector<renderer_t*>& scene::cull(const camera& camera)
{
    measure ms("scene::cull");
    m_visibles.clear();
    frustum fr = frustum::from_view_proj(camera.view_proj());

    for (int i = 0; i < m_renderers.size(); ++i)
    {
        auto & renderer = m_renderers[i];
        auto c = renderer.bounds.center();
        auto r = math::length(renderer.bounds.extends());
        auto d = math::distance(c, camera._pos);

        float lodMetric = r / d;

        if(lodMetric < 0.005)
            continue;

        if (!frustum::check_bbox(fr, renderer.world_bounds))
            continue;

        m_visibles.push_back(&renderer);
    }

     std::sort(std::execution::par, m_visibles.begin(), m_visibles.end(), renderer_sort());
    //std::sort(m_visibles.begin(), m_visibles.end(), renderer_sort());
    /*
    int batches_by_mesh = 0;
    int batches_by_material = 0;
    if (m_visibles.size()  > 10)
    {
        auto mesh = m_visibles.front()->mesh;
        auto material = m_visibles.front()->material->instance;
        for(int i = 0; i < m_visibles.size(); ++i)
        {
            if(mesh != m_visibles[i]->mesh) {
                batches_by_mesh++;
                mesh = m_visibles[i]->mesh;
            }
            if (material != m_visibles[i]->material->instance) {
                material = m_visibles[i]->material->instance;
                batches_by_material++;
            }
        }
    }*/

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
