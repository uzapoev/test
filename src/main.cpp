#include <stdio.h>
#include <time.h> 
#include <sys/stat.h> // stat
#include <stdarg.h> // stat

#include <array>
#include <filesystem>
#include <unordered_set>

#include "platform/platform.h"
#include "mathlib.h"
#include "common.h"

#include "scene/scene.h"

#include "render_manager.h"
#include "resource_manager.h"

#include "assets/asset_unity.h"

//#include "json.h"
//#include "ecs.h"
//#include "gui/gui.h"
//#include "memmgr.h"


extern void memory_enable_tracking();
extern void memory_enable_allocation_traking(bool);


struct instance_data
{
    mat4    model;
};

struct logger
{
    static void log(const char * msg)         { printf("\x1b[37m %s \033[0m", msg? msg : ""); }
    static void log_error(const char * msg)   { printf("\x1B[31m %s \033[0m", msg? msg : ""); }
    static void log_warning(const char * msg) { printf("\x1b[33m %s \033[0m", msg? msg : ""); }
};


static scene                g_scene;


void scene_test(gfx_context_t* ctx, const char * data_path, const char* scene_name)
{
    g_scene = scene::load(scene_name);
}


void log_func(gfx_msg type, const char* msg, ...)
{
    va_list arglist;
    va_start(arglist, msg);

    // 30 - black, 31 - red, 32 - green, 33 - yellow, 34 - blue, 
    switch(type) {
        case gfx_msg_info:      printf("\x1b[32m"); break;
        case gfx_msg_warning:   printf("\x1B[33m"); break;
        case gfx_msg_error:     printf("\x1b[31m"); break; // 
    }
    vprintf(msg, arglist);
    va_end(arglist);
    printf("\033[0m\n");
}


camera g_camera;
static gfx_context_t* ctx = nullptr;
static gfx_swapchain_t* swapchain = nullptr;
static gfx_pipeline_t* pipeline = nullptr;
static gfx_command_buffer_t* cmds[2] = {};

static uint64_t mvp_location = 0;
uintptr_t g_handle;


void platform_main(uintptr_t handle, int argc, char** argv)
{
    g_handle = handle;
    gfx_settings_t settings = { "test.app" };
        settings.handle = handle;
        settings.backend = gfx_backend_vulkan;
     //   settings.options = gfx_options_debug | gfx_options_verbose | gfx_options_callstack;
        settings.dbglog = log_func;
        settings.allocator.allocate_pfn = [](size_t size) -> void* { return calloc(1, size); };
        settings.allocator.realloc_pfn = [](void* ptr, size_t size) -> void* { return realloc(ptr, size); };
        settings.allocator.free_pfn = [](void* ptr) -> void { return free(ptr); };
    gfx_init(&settings, &ctx);
    gfx_create_swapchain(ctx, handle, &swapchain);

    gfx_shader_t* compute = nullptr;
    create_shader_from_file_path(ctx, "../data/shaders/compute.hlsl", &compute);

    resource_manager::create_and_make_shader(ctx);
    resource_manager::shared()->mount("../data/");

    cmds[0] = gfx_create_cmd2(ctx);
    cmds[1] = gfx_create_cmd2(ctx);

    gfx_shader_t* shader = nullptr;
    create_shader_from_file_path(ctx, "../data/shaders/simple.hlsl", &shader);

    //gfx_descriptor_set_t* sets = gfx_create_descriptor_set2(ctx, shader);

    gfx_vertex_attribute attributes[] = {
        { 0, 0, gfx_vertex_format_float4,   offsetof(vertex, position)  },
        { 1, 0, gfx_vertex_format_float4,   offsetof(vertex, uv)        },
        { 2, 0, gfx_vertex_format_float4,   offsetof(vertex, normal)    },
    };

    gfx_vertex_slot_t slots[] = {
        {0, sizeof(vertex), gfx_vertex_rate_vertex},
   //     {1, sizeof(instance_data), gfx_vertex_rate_instance}
    };

    gfx_pipeline_desc_t piplene_desc = {};
        piplene_desc.shader = shader;
        piplene_desc.assembly.topology = gfx_topology_triangles;
        piplene_desc.assembly.attributes = attributes;
        piplene_desc.assembly.attributes_count = _countof(attributes);
        piplene_desc.assembly.slots = slots;
        piplene_desc.assembly.slot_count = _countof(slots);
    pipeline = gfx_create_pipeline2(ctx, &piplene_desc);

    mvp_location            = gfx_uniform_location(shader, "mvp");
    auto color_location     = gfx_uniform_location(shader, "_color");
    auto texture_location   = gfx_uniform_location(shader, "_texture0");
    auto sampler_location   = gfx_uniform_location(shader, "_textureSampler");
    auto lightmap_location  = gfx_uniform_location(shader, "_lightmap");
    auto lightmap_scale_offset_location = gfx_uniform_location(shader, "lightmap_scale_offset");

    gfx_sampler_desc_t linear_filtering = {};
        linear_filtering.minmag = gfx_filter_linear;
        linear_filtering.mipmap = gfx_filter_linear;
   //     linear_filtering.anisotropy = 8;
    gfx_sampler_t* sampler = gfx_create_sampler2(ctx, &linear_filtering);

    rect_t rc = platform_get_window_size(handle);
    uint16_t width = rc.w - rc.x;
    uint16_t height = rc.h - rc.y;
    float aspect = (float)(width) / (float)(height);

    g_camera.set_fov(60);
    g_camera.set_near_far(0.1f, 1500.0f);
    g_camera.set_apect(aspect);
    g_camera.set_pos(math::make_vec3(0, 0, -10));
    g_camera.set_target(math::make_vec3(0, 0, 0));

 //   scene_test(ctx, "../data/unity", "Southside.big.json");
    scene_test(ctx, "../data/unity", "../data/unity/Southside.big.json");
}


void update_camera(camera & cam)
{
    vec3 dir;
    dir += input_kb_state(Input::Keyboard::Up)    ?  math::Up      : math::Zero;
    dir += input_kb_state(Input::Keyboard::Down)  ?  math::Down    : math::Zero;
    dir += input_kb_state(Input::Keyboard::W)     ? -cam.forward() : math::Zero;
    dir += input_kb_state(Input::Keyboard::S)     ?  cam.forward() : math::Zero;
    dir += input_kb_state(Input::Keyboard::A)     ?  cam.left()    : math::Zero;
    dir += input_kb_state(Input::Keyboard::D)     ? -cam.left()    : math::Zero;
    dir *= input_kb_state(Input::Keyboard::Shift) ?  8.0f : 1.0f;
    auto p = input_point_pos();

    vec2 mp = { (float)-p.dx, (float)p.dy };
    cam.set_mouse_dt(mp);
    cam.move(dir * Time::dt() * 20.0f);
    cam.update();
}


void platform_tick(void* userdata)
{
    Time::tick();

    update_camera(g_camera);

    if(input_kb_state(Input::Keyboard::NumPad_Add) )
        g_camera.set_fov(g_camera.m_fov + 0.1f);

    if (input_kb_state(Input::Keyboard::NumPad_Subtract))
        g_camera.set_fov(g_camera.m_fov - 0.1f);

    rect_t rect = platform_get_window_size(g_handle);

    float width = (float)(rect.w - rect.x);
    float height = (float)(rect.h - rect.y);

    g_camera.setup(g_camera.m_fov, width / height, 0.001f, 1550.0f);

    mat4 vp = g_camera.vp();

    auto & render_queue = g_scene.visible();

    for (int i = 0; i < render_queue.size(); ++i)
    {
        mat4 mvp = math::mul(vp, render_queue[i].transform);
        gfx_uniform_set_buffer_data(render_queue[i].material->descriptor_set, mvp_location, &mvp, sizeof(mat4));
    }


    gfx_render_target_t* target = nullptr;
    int32_t idx = gfx_acquire_img(ctx, swapchain, &target);
    if (idx < 0) 
        return;

    auto cmd = cmds[idx];

    gfx_cmd_begin(cmd);
    gfx_cmd_begin_pass(cmd, target);
    gfx_cmd_bind_pipeline(cmd, pipeline);
    
    gfx_pipeline_t * curr_pipeline = nullptr;

    for (int i = 0; i < render_queue.size(); ++i)
    {
        const renderer_t& renderer = render_queue[i];
        if(curr_pipeline != renderer.material->instance->pipeline)
        {
            curr_pipeline = renderer.material->instance->pipeline;
            gfx_cmd_bind_pipeline(cmd, curr_pipeline);
        }
        draw_renderer(cmd, &renderer);
    }
/**/
    gfx_cmd_end_pass(cmd);
    gfx_cmd_end(cmd);

    gfx_submit_cmd(ctx, cmd, gfx_submit_wait_for_image_ready);

    gfx_present_img(ctx, swapchain, idx);
}