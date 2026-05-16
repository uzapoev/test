#include "gfx_dx12.h"

#ifdef DX12_AVAILABLE


#include <d3d12.h>

#include <string.h>
#include "gfx_stub.h"

// Minimal stub backend: provides all required entrypoints so the project links.
// The implementation intentionally does not talk to D3D12 yet.

typedef struct dx12_context_t {
    gfx_context_t   handle;
    gfx_callback    dbglog;
    gfx_allocator_t* allocator;
} dx12_context_t;

typedef struct dx12_swapchain_t { gfx_swapchain_t handle; } dx12_swapchain_t;
typedef struct dx12_buffer_t { gfx_buffer_t handle; uint32_t size; } dx12_buffer_t;
typedef struct dx12_texture_t { gfx_texture_t handle; uint32_t width, height, depth; gfx_pixel_format fmt; } dx12_texture_t;
typedef struct dx12_sampler_t { gfx_sampler_t handle; } dx12_sampler_t;
typedef struct dx12_shader_t { gfx_shader_t handle; } dx12_shader_t;
typedef struct dx12_descriptor_set_t { gfx_descriptor_set_t handle; } dx12_descriptor_set_t;
typedef struct dx12_pipeline_t { gfx_pipeline_t handle; } dx12_pipeline_t;
typedef struct dx12_compute_pipeline_t { gfx_pipeline_compute_t handle; } dx12_compute_pipeline_t;
typedef struct dx12_render_target_t { gfx_render_target_t handle; } dx12_render_target_t;
typedef struct dx12_command_buffer_t { gfx_command_buffer_t handle; } dx12_command_buffer_t;

static dx12_context_t* from_ctx(gfx_context_t* ctx) { return (dx12_context_t*)ctx; }

static void dx12_stub_log(gfx_context_t* ctx, gfx_msg type, const char* msg)
{
    dx12_context_t* dctx = from_ctx(ctx);
    if (dctx && dctx->dbglog) dctx->dbglog(type, "%s", msg);
}

extern "C" void dx12_init(gfx_settings_t* settings, gfx_context_t** out_ctx)
{
    dx12_context_t* dctx = (dx12_context_t*)calloc(1, sizeof(dx12_context_t));
    if (!dctx) return;

    dctx->dbglog = settings ? settings->dbglog : nullptr;
    dctx->allocator = settings ? settings->allocator : nullptr;
    *out_ctx = &dctx->handle;

    dx12_stub_log(*out_ctx, gfx_msg_warning, "DX12 backend is stubbed (no device created).");
}

extern "C" void dx12_create_swapchain(gfx_context_t* ctx, intptr_t /*handle*/, gfx_swapchain_t** out_swapchain)
{
    (void)ctx;
    dx12_swapchain_t* sc = (dx12_swapchain_t*)calloc(1, sizeof(dx12_swapchain_t));
    *out_swapchain = sc ? &sc->handle : nullptr;
}

extern "C" void dx12_get_caps(gfx_context_t* ctx, gfx_caps_t* caps)
{
    if (!caps) return;
    memset(caps, 0, sizeof(*caps));
    // Conservative defaults: "unknown", no compression, no advanced features.
    (void)ctx;
}

extern "C" int32_t dx12_acquire_img(gfx_context_t* ctx, gfx_swapchain_t* /*swapchain*/, gfx_render_target_t** out_target)
{
    (void)ctx;
    static dx12_render_target_t s_target = {};
    if (out_target) *out_target = &s_target.handle;
    // Return 0 as "image index".
    return 0;
}

extern "C" void dx12_present_img(gfx_context_t* /*ctx*/, gfx_swapchain_t* /*swapchain*/, uint32_t /*idx*/) {}

extern "C" void dx12_create_buffer(gfx_context_t* /*ctx*/, gfx_buffer_desc_t* desc, gfx_buffer_t** out_buffer)
{
    dx12_buffer_t* b = (dx12_buffer_t*)calloc(1, sizeof(dx12_buffer_t));
    if (b && desc) b->size = desc->size;
    *out_buffer = b ? &b->handle : nullptr;
}

extern "C" void dx12_create_shader(gfx_context_t* /*ctx*/, gfx_shader_desc_t* /*desc*/, gfx_shader_t** out_shader)
{
    dx12_shader_t* s = (dx12_shader_t*)calloc(1, sizeof(dx12_shader_t));
    *out_shader = s ? &s->handle : nullptr;
}

extern "C" void dx12_create_sampler(gfx_context_t* /*ctx*/, gfx_sampler_desc_t* /*desc*/, gfx_sampler_t** out_sampler)
{
    dx12_sampler_t* s = (dx12_sampler_t*)calloc(1, sizeof(dx12_sampler_t));
    *out_sampler = s ? &s->handle : nullptr;
}

extern "C" void dx12_create_texture(gfx_context_t* /*ctx*/, gfx_texture_desc_t* desc, gfx_texture_t** out_texture)
{
    dx12_texture_t* t = (dx12_texture_t*)calloc(1, sizeof(dx12_texture_t));
    if (t && desc) {
        t->width = desc->width;
        t->height = desc->height;
        t->depth = desc->depth;
        t->fmt = desc->format;
    }
    *out_texture = t ? &t->handle : nullptr;
}

extern "C" void dx12_create_pipeline(gfx_context_t* /*ctx*/, gfx_pipeline_desc_t* /*desc*/, gfx_pipeline_t** out_pipeline)
{
    dx12_pipeline_t* p = (dx12_pipeline_t*)calloc(1, sizeof(dx12_pipeline_t));
    *out_pipeline = p ? &p->handle : nullptr;
}

extern "C" void dx12_create_compute_pipeline(gfx_context_t* /*ctx*/, gfx_compute_pipeline_desc_t* /*desc*/, gfx_pipeline_compute_t** out_pipeline)
{
    dx12_compute_pipeline_t* p = (dx12_compute_pipeline_t*)calloc(1, sizeof(dx12_compute_pipeline_t));
    *out_pipeline = p ? &p->handle : nullptr;
}

extern "C" void dx12_create_render_target(gfx_context_t* /*ctx*/, gfx_render_target_desc_t* /*desc*/, gfx_render_target_t** out_target)
{
    dx12_render_target_t* t = (dx12_render_target_t*)calloc(1, sizeof(dx12_render_target_t));
    *out_target = t ? &t->handle : nullptr;
}

extern "C" void dx12_create_descriptor_set(gfx_context_t* /*ctx*/, gfx_shader_t* /*shader*/, gfx_descriptor_set_t** out_descriptor)
{
    dx12_descriptor_set_t* s = (dx12_descriptor_set_t*)calloc(1, sizeof(dx12_descriptor_set_t));
    *out_descriptor = s ? &s->handle : nullptr;
}

extern "C" void dx12_create_cmd(gfx_context_t* /*ctx*/, gfx_command_buffer_t** out_cmd)
{
    dx12_command_buffer_t* c = (dx12_command_buffer_t*)calloc(1, sizeof(dx12_command_buffer_t));
    *out_cmd = c ? &c->handle : nullptr;
}

extern "C" void dx12_destroy_buffer(gfx_context_t* /*ctx*/, gfx_buffer_t* buffer) { free(buffer); }
extern "C" void dx12_destroy_shader(gfx_context_t* /*ctx*/, gfx_shader_t* shader) { free(shader); }
extern "C" void dx12_destroy_sampler(gfx_context_t* /*ctx*/, gfx_sampler_t* sampler) { free(sampler); }
extern "C" void dx12_destroy_texture(gfx_context_t* /*ctx*/, gfx_texture_t* texture) { free(texture); }
extern "C" void dx12_destroy_pipeline(gfx_context_t* /*ctx*/, gfx_pipeline_t* pipeline) { free(pipeline); }
extern "C" void dx12_destroy_render_target(gfx_context_t* /*ctx*/, gfx_render_target_t* target) { free(target); }
extern "C" void dx12_destroy_descriptor_set(gfx_context_t* /*ctx*/, gfx_descriptor_set_t* descriptor) { free(descriptor); }
extern "C" void dx12_destroy_cmd(gfx_context_t* /*ctx*/, gfx_command_buffer_t* cmd) { free(cmd); }

extern "C" void dx12_update_buffer_data(gfx_context_t* ctx, gfx_buffer_t* /*buffer*/, void* /*data*/, uint32_t /*size*/, uint32_t /*offset*/)
{
    dx12_context_t* dctx = from_ctx(ctx);
    gfx_stub_not_implemented(dctx ? dctx->dbglog : nullptr, "dx12_update_buffer_data");
}

extern "C" uint64_t dx12_uniform_location(gfx_shader_t* /*shader*/, const char* /*name*/) { return 0; }
extern "C" void dx12_uniform_set_buffer(gfx_descriptor_set_t* /*set*/, uint64_t /*handle*/, gfx_buffer_t* /*buffer*/, uint32_t /*offset*/) {}
extern "C" void dx12_uniform_set_buffer_data(gfx_descriptor_set_t* /*set*/, uint64_t /*handle*/, void* /*data*/, uint32_t /*size*/) {}
extern "C" void dx12_uniform_set_texture(gfx_descriptor_set_t* /*set*/, uint64_t /*handle*/, gfx_texture_t* /*texture*/) {}
extern "C" void dx12_uniform_set_sampler(gfx_descriptor_set_t* /*set*/, uint64_t /*handle*/, gfx_sampler_t* /*sampler*/) {}

extern "C" void dx12_update_texture_data(gfx_context_t* ctx, gfx_texture_t* /*texture*/, void* /*data*/, uint32_t /*size*/, uint32_t /*offset*/)
{
    dx12_context_t* dctx = from_ctx(ctx);
    gfx_stub_not_implemented(dctx ? dctx->dbglog : nullptr, "dx12_update_image_data");
}

extern "C" void dx12_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* /*cmd*/)
{
    dx12_context_t* dctx = from_ctx(ctx);
    gfx_stub_not_implemented(dctx ? dctx->dbglog : nullptr, "dx12_texture_get_data");
}

extern "C" void dx12_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* /*texture*/)
{
    dx12_context_t* dctx = from_ctx(ctx);
    gfx_stub_not_implemented(dctx ? dctx->dbglog : nullptr, "dx12_texture_generate_mipmap");
}

extern "C" void dx12_blit_image(gfx_context_t* ctx, gfx_texture_t* /*src*/, gfx_texture_t* /*dst*/)
{
    dx12_context_t* dctx = from_ctx(ctx);
    gfx_stub_not_implemented(dctx ? dctx->dbglog : nullptr, "dx12_blit_image");
}

extern "C" void dx12_update_bindless_texture(gfx_context_t* ctx, gfx_texture_t* /*texture*/, uint32_t /*idx*/)
{
    dx12_context_t* dctx = from_ctx(ctx);
    gfx_stub_not_implemented(dctx ? dctx->dbglog : nullptr, "dx12_update_bindless_texture");
}

extern "C" void dx12_cmd_begin(gfx_command_buffer_t* /*cmd*/) {}
extern "C" void dx12_cmd_begin_pass(gfx_command_buffer_t* /*cmd*/, gfx_render_target_t* /*target*/) {}
extern "C" void dx12_cmd_end_pass(gfx_command_buffer_t* /*cmd*/) {}
extern "C" void dx12_cmd_scissor(gfx_command_buffer_t* /*cmd*/, uint32_t /*x*/, uint32_t /*y*/, uint32_t /*w*/, uint32_t /*h*/) {}
extern "C" void dx12_cmd_viewport(gfx_command_buffer_t* /*cmd*/, uint32_t /*x*/, uint32_t /*y*/, uint32_t /*w*/, uint32_t /*h*/) {}
extern "C" void dx12_cmd_bind_pipeline(gfx_command_buffer_t* /*cmd*/, gfx_pipeline_t* /*pipeline*/) {}
extern "C" void dx12_cmd_bind_descriptor_set(gfx_command_buffer_t* /*cmd*/, uint32_t /*slot*/, gfx_descriptor_set_t* /*descriptor*/) {}
extern "C" void dx12_cmd_bind_buffer_ib(gfx_command_buffer_t* /*cmd*/, gfx_index_format /*format*/, uint32_t /*offset*/, gfx_buffer_t* /*buffer*/) {}
extern "C" void dx12_cmd_bind_buffer_vb(gfx_command_buffer_t* /*cmd*/, uint32_t /*slot*/, uint32_t /*offset*/, gfx_buffer_t* /*buffer*/) {}
extern "C" void dx12_cmd_draw(gfx_command_buffer_t* /*cmd*/, uint32_t /*vertex_count*/, uint32_t /*instance_count*/) {}
extern "C" void dx12_cmd_draw_indexed(gfx_command_buffer_t* /*cmd*/, uint32_t /*idx_count*/, uint32_t /*first_idx*/, uint32_t /*instance_count*/, uint32_t /*vertex_offset*/) {}
extern "C" void dx12_cmd_draw_indexed_indirect(gfx_command_buffer_t* /*cmd*/, gfx_buffer_t* /*buffer*/, uint32_t /*offset*/, uint32_t /*draw_count*/, uint32_t /*stride*/) {}
extern "C" void dx12_cmd_dispatch_compute(gfx_command_buffer_t* /*cmd*/, uint32_t /*x*/, uint32_t /*y*/, uint32_t /*z*/) {}
extern "C" void dx12_cmd_push_marker(gfx_command_buffer_t* /*cmd*/, const char* /*marker*/) {}
extern "C" void dx12_cmd_pop_marker(gfx_command_buffer_t* /*cmd*/) {}
extern "C" void dx12_cmd_buffer_barrier(gfx_command_buffer_t* /*cmd*/, gfx_buffer_t** /*buffers*/, uint32_t /*count*/, gfx_barrier /*src*/, gfx_barrier /*dst*/) {}
extern "C" void dx12_cmd_texture_barrier(gfx_command_buffer_t* /*cmd*/, gfx_texture_t** /*textures*/, uint32_t /*count*/, gfx_barrier /*src*/, gfx_barrier /*dst*/) {}
extern "C" void dx12_cmd_end(gfx_command_buffer_t* /*cmd*/) {}
extern "C" void dx12_submit_cmd(gfx_context_t* /*ctx*/, gfx_command_buffer_t* /*cmd*/, gfx_submit_options /*options*/) {}

// Filled from gfx.cpp via: extern void gfx_init_dx12(gfx_api_pfn*)
extern "C" void gfx_init_dx12(gfx_api_pfn* func_table)
{
    memset(func_table, 0, sizeof(*func_table));

    // CONTEXT
    func_table->pfn_init     = dx12_init;
    func_table->pfn_get_caps = dx12_get_caps;

    // SWAPCHAIN
    func_table->pfn_create_swapchain = dx12_create_swapchain;
    func_table->pfn_acquire_img      = dx12_acquire_img;
    func_table->pfn_present_img      = dx12_present_img;

    // BUFFER
    func_table->pfn_create_buffer      = dx12_create_buffer;
    func_table->pfn_update_buffer_data = dx12_update_buffer_data;
    func_table->pfn_destroy_buffer     = dx12_destroy_buffer;

    // SHADER
    func_table->pfn_create_shader    = dx12_create_shader;
    func_table->pfn_uniform_location = dx12_uniform_location;
    func_table->pfn_destroy_shader   = dx12_destroy_shader;

    // SAMPLER
    func_table->pfn_create_sampler  = dx12_create_sampler;
    func_table->pfn_destroy_sampler = dx12_destroy_sampler;

    // TEXTURE
    func_table->pfn_create_texture          = dx12_create_texture;
    func_table->pfn_update_texture_data     = dx12_update_texture_data;
    func_table->pfn_update_bindless_texture = dx12_update_bindless_texture;
    func_table->pfn_texture_generate_mipmap = dx12_texture_generate_mipmap;
    func_table->pfn_blit_image              = dx12_blit_image;
    func_table->pfn_texture_get_data        = dx12_texture_get_data;
    func_table->pfn_destroy_texture         = dx12_destroy_texture;

    // PIPELINE
    func_table->pfn_create_pipeline         = dx12_create_pipeline;
    func_table->pfn_create_compute_pipeline = dx12_create_compute_pipeline;
    func_table->pfn_destroy_pipeline        = dx12_destroy_pipeline;

    // RENDER TARGET
    func_table->pfn_create_render_target  = dx12_create_render_target;
    func_table->pfn_destroy_render_target = dx12_destroy_render_target;

    // DESCRIPTOR SET
    func_table->pfn_create_descriptor_set   = dx12_create_descriptor_set;
    func_table->pfn_uniform_set_buffer      = dx12_uniform_set_buffer;
    func_table->pfn_uniform_set_buffer_data = dx12_uniform_set_buffer_data;
    func_table->pfn_uniform_set_texture     = dx12_uniform_set_texture;
    func_table->pfn_uniform_set_sampler     = dx12_uniform_set_sampler;
    func_table->pfn_destroy_descriptor_set  = dx12_destroy_descriptor_set;

    // COMMAND BUFFER
    func_table->pfn_create_cmd = dx12_create_cmd;
    func_table->pfn_destroy_cmd = dx12_destroy_cmd;

    func_table->pfn_cmd_begin      = dx12_cmd_begin;
    func_table->pfn_cmd_begin_pass = dx12_cmd_begin_pass;
    func_table->pfn_cmd_end_pass   = dx12_cmd_end_pass;

    func_table->pfn_cmd_scissor               = dx12_cmd_scissor;
    func_table->pfn_cmd_viewport              = dx12_cmd_viewport;
    func_table->pfn_cmd_bind_pipeline         = dx12_cmd_bind_pipeline;
    func_table->pfn_cmd_bind_descriptor_set   = dx12_cmd_bind_descriptor_set;
    func_table->pfn_cmd_bind_buffer_ib        = dx12_cmd_bind_buffer_ib;
    func_table->pfn_cmd_bind_buffer_vb        = dx12_cmd_bind_buffer_vb;
    func_table->pfn_cmd_draw                  = dx12_cmd_draw;
    func_table->pfn_cmd_draw_indexed          = dx12_cmd_draw_indexed;
    func_table->pfn_cmd_draw_indexed_indirect = dx12_cmd_draw_indexed_indirect;
    func_table->pfn_cmd_dispatch_compute      = dx12_cmd_dispatch_compute;

    func_table->pfn_cmd_push_marker = dx12_cmd_push_marker;
    func_table->pfn_cmd_pop_marker  = dx12_cmd_pop_marker;

    func_table->pfn_cmd_buffer_barrier  = dx12_cmd_buffer_barrier;
    func_table->pfn_cmd_texture_barrier = dx12_cmd_texture_barrier;

    func_table->pfn_cmd_end    = dx12_cmd_end;
    func_table->pfn_submit_cmd = dx12_submit_cmd;
}

#endif // DX12_AVAILABLE

