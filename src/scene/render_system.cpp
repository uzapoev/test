#include "render_system.h"
#include "components.h"

render_system * render_system::s_shared = nullptr;


render_system::render_system(gfx_context_t* ctx) : m_ctx(ctx)
{
    m_allocator = new aligned_allocator("render_system");
    m_renderer_allocator = new paged_pool_allocator(m_allocator, sizeof(renderer), 4096);
}



renderer* render_system::allocate_renderer() {
    return m_renderer_allocator->allocate<renderer>();
}


void render_system::deallocate_renderer(renderer* _renderer) {
    //_renderer->material->release();
    //_renderer->mesh->release();
    //m_allocator.deallocate(_renderer);
}


lodgroup* render_system::allocate_lodgroup() 
{
    return m_lodgroup_allocator->allocate<lodgroup>();
};


void render_system::deallocate_lodgroup(lodgroup* _lodgroup) 
{
}