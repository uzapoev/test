#include "render_system.h"
#include "components.h"
#include "../resource_manager.h"


entity_query<transform, renderer> g_renderer_with_transforms;


render_system * render_system::s_shared = nullptr;


render_system::render_system(gfx_context_t* ctx) : m_ctx(ctx)
{
    m_allocator = new aligned_allocator("render_system");
    m_renderer_allocator = new paged_pool_allocator(m_allocator, sizeof(renderer), 4096);

    component_manager::instance().register_query(&g_renderer_with_transforms);
    component_manager::instance().register_query(this);
}


void render_system::on_node_changed(struct tinynode* node, class component_manager& manager)
{
    auto* rb_comp = manager.get_component<renderer>(node);
    if(rb_comp == nullptr) return;
    if(rb_comp->mesh_handle.handle == 0)
        manager.remove_component<renderer>(node);
}

void render_system::garbage_collect()
{
}

void render_system::draw(gfx_command_buffer_t* cmd)
{
    gfx_pipeline_t* pipeline = nullptr;
    gfx_buffer_t* vertex_buffer = nullptr;
    gfx_buffer_t* index_buffer = nullptr;

    auto rm = resource_manager::shared();

    for(auto [node, transf, render] : g_renderer_with_transforms)
    {
        //TODO: remove to on_node_changed processing
        if(render->mesh_handle.handle == 0){
            component_manager::instance().remove_component<renderer>(node);
            continue;
         }

        if (render->material && render->material->instance->pipeline != pipeline)
        {
            pipeline = render->material->instance->pipeline;
            gfx_cmd_bind_pipeline(cmd, pipeline);
        }

        const render_mesh_t * mesh = rm->get_mesh(render->mesh_handle); // todo: move to on_node_changed, sort/cell spatialize/ batch
                                                                        // map<pipeline, array<mesh_info>> 
                                                                        // mesh info - vb/ib offsets in megamesh + gloabl transform + descritorset
        if(mesh == nullptr)
            continue;

        if (vertex_buffer != mesh->vertex_buffer) {
            vertex_buffer = mesh->vertex_buffer;
            gfx_cmd_bind_vertex_buffer(cmd, 0, 0, vertex_buffer);
        }

        if (index_buffer != mesh->index_buffer)
        {
            index_buffer = mesh->index_buffer;
            gfx_cmd_bind_index_buffer(cmd, mesh->index_format, 0, mesh->index_buffer);
        }
    }
}