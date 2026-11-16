#include "scene.h"
#include "world.h"
#include "../memmgr.h"
#include "scene_json.h"

#include <unordered_map>

struct scene_chunk_info_t {
    uint32_t type = 0;
    uint32_t size = 0;
};

struct scene_header_t {
    uint32_t magick;
    uint32_t version;

    uint32_t node_count;

    uint32_t meshes_count;
    uint32_t textures_count;
    uint32_t shaders_count;
    uint32_t materials_count;
};

void camera::update()
{
    _axis.y = math::clamp(_axis.y, -80.0f, 80.0f);
    _rot = quat::from_euler(_axis.y, _axis.x, 0.0f);

    vec3 dir = math::normalize(_pos - _target);
    vec3 target = math::mul(_rot, dir) + _pos;

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

    if(std::filesystem::exists(path)) {
        return scene_reader_json::create_from_file(path);
    } else {
        debug::log_error("no file at path %s", path.data());
        return {};
    }
}

uint64_t s_rt_counter = 0;
node::node()
{
    runtime_id = ++s_rt_counter;
}


node* scene::create_node(interned_string name, guid_t guid)
{
    node* result = new node();
    result->name = name;
    result->guid = guid;
    return result;
}

// The file has a flat list and "parent" = guid of another node of the list (any place of it), so the links are built
// when the whole list is read: node::parent -> node::hierarchy. An unknown parent or a cycle makes the node a root (reported).
void scene::link_hierarchy()
{
    const uint32_t none  = hierarchy::none;
    const size_t   count = m_nodes.size();

    std::unordered_map<guid_t, uint32_t> by_guid;
    by_guid.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        m_nodes[i].index     = i;
        m_nodes[i].hierarchy = hierarchy();
        if (!(m_nodes[i].guid == nullptr) && !by_guid.emplace(m_nodes[i].guid, i).second)
            debug::log_warning("[scene] node '%s': the guid is not unique, children can be linked to the first node with it", m_nodes[i].name.c_str());
    }

    // 1. the parent of every node
    std::vector<uint32_t> parent(count, none);
    for (uint32_t i = 0; i < count; ++i)
    {
        if (m_nodes[i].parent == nullptr)
            continue;

        auto it = by_guid.find(m_nodes[i].parent);
        if (it == by_guid.end() || it->second == i) {
            char text[64] = {};
            debug::log_warning("[scene] node '%s': the parent %s is not in the scene, the node becomes a root", m_nodes[i].name.c_str(), uuid::guid_to_str(m_nodes[i].parent, text));
            m_nodes[i].parent = {};
            continue;
        }
        parent[i] = it->second;
    }

    // 2. a cycle (a -> b -> a) is never reached from a root: cut it at the node where it was found
    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t steps = 0;
        for (uint32_t p = parent[i]; p != none && steps <= count; p = parent[p])
            ++steps;
        if (steps > count) {
            // `i` may only lead into the cycle: count steps up always land on a node of the cycle itself
            uint32_t on_cycle = i;
            for (size_t s = 0; s < count; ++s)
                on_cycle = parent[on_cycle];
            debug::log_warning("[scene] node '%s': the parents make a cycle, the node becomes a root", m_nodes[on_cycle].name.c_str());
            parent[on_cycle] = none;
            m_nodes[on_cycle].parent = {};
        }
    }

    // 3. the children lists, in the order of the nodes
    std::vector<uint32_t> last_child(count, none);
    for (uint32_t i = 0; i < count; ++i)
    {
        const uint32_t p = parent[i];
        if (p == none)
            continue;

        hierarchy& h  = m_nodes[i].hierarchy;
        hierarchy& ph = m_nodes[p].hierarchy;

        h.parent_index = p;
        ph.child_count++;
        if (last_child[p] == none) {
            ph.first_child = i;
        } else {
            m_nodes[last_child[p]].hierarchy.next_sibling = i;
            h.prev_sibling = last_child[p];
        }
        last_child[p] = i;
    }
}

template<class T> inline void deserialize_component(T& object, filestream* stream)
{
    auto properties = reflection::get_properties<T>();
    reflection::for_each(properties, [&](size_t idx, auto* arg) {
        using member_type = std::decay_t<decltype(object.*arg->member)>;

        if constexpr (std::is_trivially_copyable_v<member_type>) {
            object.*arg->member = stream->read<member_type>();
        }

        else if constexpr (!std::is_same_v<decltype(reflection::get_properties<member_type>()), void>) {
            deserialize_component(object.*arg->member, stream);
        }
    });
}


void scene::load(const std::string_view& path)
{
    component_manager::instance().register_query(&m_render_query);

    if(m_world == nullptr){
        m_world = new world();
    }

    PROFILE_SAMPLE("scene::load::bin")
    filestream * stream = filestream::open_rb(path.data());

    auto header = stream->read<scene_header_t>();
    m_nodes.resize(header.node_count);// = allocator->alloc<node>(nodes_count);

    // the asset lists of the header, in the order of the counts: meshes, textures, shaders, materials
    // (the same four lists as "meshes", "textures", "shaders", "materials" of the json scene)
    m_mesh_guids.resize(header.meshes_count);
    m_texture_guids.resize(header.textures_count);
    m_shaders_guids.resize(header.shaders_count);
    m_materials_guids.resize(header.materials_count);

    for (std::vector<guid_t>* list : { &m_mesh_guids, &m_texture_guids, &m_shaders_guids, &m_materials_guids })
    {
        if (!list->empty())
            stream->read((uint32_t)(sizeof(guid_t) * list->size()), list->data());
    }

    scene_chunk_info_t info = {};
    component_manager::instance().pause_notifications();
    while(stream->read(sizeof(scene_chunk_info_t), &info) != 0)
    {
        auto chunk_start = stream->tell();
        switch(info.type) {
            case component_node: {
                auto node = m_world->load_node(this, stream);
              //  auto tnode = world::create_node(uuid::str_to_guid(node->guid), this);
               // tnode->name = node->name;
               // node->guid = node->guid;

                m_tiny_nodes.push_back(node);
            } break;

            case component_transform: {
                uint32_t  index = stream->read<uint32_t>();
                tinynode* node  = m_tiny_nodes[index];

                auto component  = node->add_component<transform>();
                transform::deserialize(component, stream);
            } break;

            case component_light: {
                uint32_t  index = stream->read<uint32_t>();
                tinynode* node  = m_tiny_nodes[index];
                auto component  = node->add_component<light>();
                //light::deserialize(component, stream);
            }break;

            case component_renderer: {
                uint32_t  index = stream->read<uint32_t>();
                tinynode* node  = m_tiny_nodes[index];

                auto component  = node->add_component<renderer>();
                renderer::deserialize(component, stream);

                component->mesh_handle = resource_manager::shared()->load(component->mesh_guid, nullptr);
                component->material_handle = resource_manager::shared()->load(component->material_guid, nullptr);

                component->material = resource_manager::shared()->get_material_instance(component->material_handle);
                component->mesh = resource_manager::shared()->get_mesh(component->mesh_handle);
            } break;

            default: 
                //stream->seek(info.size, 1);
            break;
        };

        stream->seek(chunk_start + info.size, 0);
    }
    component_manager::instance().resume_notifications(m_tiny_nodes);
}


void scene::save(const std::string_view& path)
{
    filestream * stream = filestream::open_wb(path.data());

    scene_header_t header = {};
        header.magick       = gfx_fourcc('S', 'C', 'N', '0');
        header.version      = 1;
        header.node_count   = (uint32_t)m_nodes.size();
        header.meshes_count     = (uint32_t)m_mesh_guids.size();
        header.textures_count   = (uint32_t)m_texture_guids.size();
        header.shaders_count    = (uint32_t)m_shaders_guids.size();
        header.materials_count  = (uint32_t)m_materials_guids.size();
    stream->write(header);

    // asset lists, same order as the counts
    for (const std::vector<guid_t>* list : { &m_mesh_guids, &m_texture_guids, &m_shaders_guids, &m_materials_guids })
    {
        if (!list->empty())
            stream->write((uint32_t)(sizeof(guid_t) * list->size()), list->data());
    }
    // write meta

    for (uint32_t i = 0; i < m_nodes.size(); i++)
    {
        auto& node = m_nodes[i];
        node.index = i;
        node.runtime_id = i;
        m_world->save_node(&node, stream);
    }
    for (uint32_t i = 0; i < m_nodes.size(); i++)
    {
        auto& node = m_nodes[i];

        if (auto t = node.get_component<transform>())
        {
            binary_writer bw = {};
            bw.write(i);
            transform::serialize(t, &bw);
            write_chunk(stream, scene::component_transform, bw.size(), (char*)bw.data());
        }

        if (auto r = node.get_component<renderer>() )
        {
            binary_writer bw0 = {};
            bw0.write(i);
            renderer::serialize(r, &bw0);
            write_chunk(stream, scene::component_renderer, bw0.size(), (char*)bw0.data());
        }
     }

    stream->flush();
    delete stream;
}


void scene::clear()
{
    m_nodes.clear();
    m_mesh_guids.clear();
    m_materials_guids.clear();
}

void scene::update()
{
}

struct direct_batch_t
{
    gfx_pipeline_t* pipeline = nullptr;
    gfx_buffer_t* vertex_buffer = nullptr;
    gfx_buffer_t* index_buffer = nullptr;
    
    gfx_descriptor_set_t ** sets;
    submesh_t * mesh_ranges;
};

// Occlusion pass prototype:
//   1. render occluders (big/near static meshes) depth-only into a small depth target
//   2. build Hi-Z pyramid (max-depth mip chain) from that depth
//   3. return Hi-Z texture, it is consumed by the GPU culling pass
gfx_texture_t* scene_occlusion_pass(gfx_command_buffer_t* cmd, camera& camera)
{
    gfx_cmd_push_marker(cmd, "scene_occlusion_pass");

    // small depth target (e.g. 256x128), created once in real code
    gfx_texture_t * occluder_depth = nullptr;
    // Hi-Z pyramid, mip chain of the depth, one mip per compute dispatch
    gfx_texture_t * hiz_texture = nullptr;
    gfx_pipeline_t * occluder_pipeline = nullptr;
    gfx_descriptor_set_t * descriptor_set_camera = nullptr;
    gfx_descriptor_set_t * descriptor_set_hiz = nullptr;

    gfx_buffer_t * occluder_vertex_buffer = nullptr;
    gfx_buffer_t * occluder_index_buffer = nullptr;
    uint32_t occluder_index_count = 0;

    // 1. draw occluders into small depth-only target
    gfx_pass_info_t pass = {};
    pass.depth_attachment = occluder_depth;
    pass.depth_load_op = gfx_load_op_clear;
    pass.depth_clear_value = 1.0f;
    for (int i = 0; i < 4; i++) {
        pass.color_attachment[i] = nullptr;
        pass.color_load_op[i] = gfx_load_op_dont_care;
    }
    pass.target = nullptr;

    gfx_cmd_begin_pass(cmd, &pass);
    gfx_cmd_viewport(cmd, 0, 0, 256, 128);
    gfx_cmd_scissor(cmd, 0, 0, 256, 128);

    gfx_descriptor_set_write_buffer_data(descriptor_set_camera, 0, &camera, sizeof(camera));
    gfx_cmd_bind_pipeline(cmd, occluder_pipeline);
    gfx_cmd_bind_descriptor_set(cmd, 0, descriptor_set_camera);
    gfx_cmd_bind_index_buffer(cmd, gfx_index_format_16, 0, occluder_index_buffer);
    gfx_cmd_bind_vertex_buffer(cmd, 0, 0, occluder_vertex_buffer);
    gfx_cmd_draw_indexed(cmd, occluder_index_count, 0, 1, 0);   // real code: loop / indirect over occluders

    gfx_cmd_end_pass(cmd);

    // 2. depth -> Hi-Z. Depth must be readable by compute.
    gfx_cmd_texture_barrier(cmd, &occluder_depth, 1, gfx_barrier_depth_stencil, gfx_barrier_compute_read);

    // mip 0 = copy of depth, every next mip = max of 2x2 texels of previous mip.
    // (compute pipeline binding is not exposed in gfx.h yet, so only dispatches are shown)
    gfx_descriptor_set_write_texture(descriptor_set_hiz, 0, occluder_depth);
    gfx_descriptor_set_write_texture(descriptor_set_hiz, 1, hiz_texture);
    gfx_cmd_bind_descriptor_set(cmd, 0, descriptor_set_hiz);

    uint32_t mip_count = 8;
    uint32_t w = 256, h = 128;
    for (uint32_t mip = 0; mip < mip_count; mip++)
    {
        gfx_cmd_dispatch_compute(cmd, (w + 7) / 8, (h + 7) / 8, 1);
        gfx_cmd_texture_barrier(cmd, &hiz_texture, 1, gfx_barrier_compute_write, gfx_barrier_compute_read);
        w = w > 1 ? w / 2 : 1;
        h = h > 1 ? h / 2 : 1;
    }

    gfx_cmd_pop_marker(cmd);
    return hiz_texture;
}

void scene_draw(gfx_command_buffer_t* cmd, camera& camera)
{
}

void scene_draw_indirect(gfx_command_buffer_t* cmd, camera& camera)
{
    // One batch = one pipeline + one shared geometry pool (vertex/index buffers).
    // All objects of the batch are drawn with a single multi-draw indirect call;
    // each object is one gfx_indirect_data_t entry inside the batch's range of the indirect buffer.
    struct batch {
        gfx_pipeline_t * pipeline       = nullptr;
        gfx_buffer_t *   vertex_buffer  = nullptr;  // positions/normals/uv
        gfx_buffer_t *   instance_buffer = nullptr; // per-instance data (transforms, material ids)
        gfx_buffer_t *   index_buffer   = nullptr;
        uint32_t         first_draw     = 0;        // index of first gfx_indirect_data_t of this batch
        uint32_t         draw_count     = 0;        // number of draws in this batch
    };
    std::vector<batch> m_batches;

    gfx_cmd_push_marker(cmd, "scene_draw_indirect");

    gfx_buffer_t * indirect_buffer = nullptr;       // array of gfx_indirect_data_t, filled by GPU culling
    gfx_descriptor_set_t * descriptor_set_camera = nullptr;
    gfx_descriptor_set_t * descriptor_set_instance = nullptr;
    gfx_descriptor_set_t * descriptor_set_bindless = nullptr;

    //gfx_pipeline_compute_t
   
    // 0. Occlusion pass: draw occluders into small depth, build Hi-Z
    gfx_texture_t * hiz_texture = scene_occlusion_pass(cmd, camera);

    // 1. Upload camera data (view/proj) into the camera set. Handle 0 is a placeholder binding.
    gfx_descriptor_set_write_buffer_data(descriptor_set_camera, 0, &camera, sizeof(camera));
    // Hi-Z is read by the culling shader (handle 1 is a placeholder binding)
    gfx_descriptor_set_write_texture(descriptor_set_camera, 1, hiz_texture);

    // 2. GPU culling pass: compute shader reads instances + camera frustum and
    //    writes visible draws (gfx_indirect_data_t) into indirect_buffer.
    //    (compute pipeline + descriptor sets would be bound here; gfx.h has no compute bind call yet)
    gfx_cmd_dispatch_compute(cmd, 64, 1, 1);

    // 3. Make compute writes visible to the indirect draw argument fetch.
    gfx_cmd_buffer_barrier(cmd, &indirect_buffer, 1, gfx_barrier_compute_write, gfx_barrier_indirect);

    // 4. Draw: one indirect multi-draw per batch.
    for (size_t i = 0; i < m_batches.size(); i++)
    {
        const batch& b = m_batches[i];

        gfx_cmd_bind_pipeline(cmd, b.pipeline);
        gfx_cmd_bind_descriptor_set(cmd, 0, descriptor_set_camera);
        gfx_cmd_bind_descriptor_set(cmd, 1, descriptor_set_instance);
        gfx_cmd_bind_descriptor_set(cmd, 2, descriptor_set_bindless);

        gfx_cmd_bind_index_buffer(cmd, gfx_index_format_16, 0, b.index_buffer);
        gfx_cmd_bind_vertex_buffer(cmd, 0, 0, b.vertex_buffer);
        gfx_cmd_bind_vertex_buffer(cmd, 1, 0, b.instance_buffer);

        // offset is in bytes into indirect_buffer, stride - size of one command
        gfx_cmd_draw_indexed_indirect(cmd, indirect_buffer,
                                      b.first_draw * (uint32_t)sizeof(gfx_indirect_data_t),
                                      b.draw_count,
                                      (uint32_t)sizeof(gfx_indirect_data_t));
    }

    gfx_cmd_pop_marker(cmd);
}

void scene::draw(gfx_command_buffer_t* cmd, camera & camera)
{
    gfx_cmd_push_marker(cmd, "scene::draw");

    gfx_pipeline_t* pipeline = nullptr;
    gfx_buffer_t* vertex_buffer = nullptr;
    gfx_buffer_t* index_buffer = nullptr;

    for (auto [node, trans, renderer] : m_render_query) {
        if (!node) continue;

        auto mesh = renderer->mesh;
        auto material = renderer->material;

        if(material == nullptr) continue;
        if (material && material->instance->pipeline != pipeline)
        {
            pipeline = material->instance->pipeline;
            gfx_cmd_bind_pipeline(cmd, pipeline);
        }

        mat4 mvp = math::mul(camera.view_proj(), trans->global_transform);
        auto mvp_location = gfx_uniform_location(material->instance->shader, "mvp");
        gfx_descriptor_set_write_buffer_data(material->descriptor_set, mvp_location, &mvp, sizeof(mat4));

        gfx_cmd_bind_descriptor_set(cmd, 0, material->descriptor_set);

        if (vertex_buffer != mesh->vertex_buffer) 
            gfx_cmd_bind_vertex_buffer(cmd, 0, 0, mesh->vertex_buffer);

        if (index_buffer != mesh->index_buffer)
            gfx_cmd_bind_index_buffer(cmd, mesh->index_format, 0, mesh->index_buffer);

        index_buffer = mesh->index_buffer;
        vertex_buffer = mesh->vertex_buffer;

        assert(mesh->index_format != gfx_index_format_32);
        assert(mesh->submesh_count != 0);
  
        int32_t start_idx = 0;
        int32_t vertex_offset = mesh->vertex_buffer_offset / mesh->vertex_stride;
        int32_t index_offset = mesh->index_buffer_offset / sizeof(uint16_t);

        for (size_t sub_idx = 0; sub_idx < mesh->submesh_count; sub_idx++)
        {
            uint32_t start = mesh->submeshes[sub_idx].index_start;
            uint32_t count = mesh->submeshes[sub_idx].index_count;

            gfx_cmd_draw_indexed(cmd, count, index_offset + start, 1, vertex_offset);
        }
    }

    gfx_cmd_pop_marker(cmd);
}




struct renderer_sort
{
    bool operator()(renderer* a, renderer* b) const {
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
#if 0
const std::vector<renderer_t*>& scene::cull(const camera& camera)
{
   // measure ms("scene::cull");
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
}

#endif
