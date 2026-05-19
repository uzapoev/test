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
#include "memmgr.h"

#include "scene/scene.h"
#include "scene/render_system.h"

#include "resource_manager.h"

#include "assets/asset_unity.h"



static scene                g_scene;

void scene_test(gfx_context_t* ctx, const char * data_path, const char* scene_name)
{
    g_scene = scene::create_from_json_file(scene_name);
    /*
    std::string bin_path = std::string(scene_name).append(".bin");

    if(std::filesystem::exists(bin_path))
        g_scene = scene::create_from_file(bin_path.c_str());
    else
        g_scene = scene::create_from_json_file(scene_name);*/
}


void log_func(gfx_msg type, const char* msg, ...)
{
    va_list arglist;
    va_start(arglist, msg);

    // 30 - black, 31 - red, 32 - green, 33 - yellow, 34 - blue, 
    switch(type) {
        case gfx_msg_info:      printf("\n\x1b[32m"); break;
        case gfx_msg_warning:   printf("\n\x1b[33m"); break;
        case gfx_msg_error:     printf("\n\x1b[31m"); break; // 
    }
    vprintf(msg, arglist);
    va_end(arglist);
    printf("\033[0m\n");
  //  flushall();
}

gfx_allocator_t gfx_allocator = {
    [] (size_t size, void * userdata) {
        static size_t total = 0;
        total += size;
        debug::log_warning("gfx allocation %10d   %d", size, total);
        return calloc(1, size);
    },

    [](void* ptr, void* userdata) {
        return free(ptr);
    }
 };
#include "threads.h"


camera g_camera;
static gfx_context_t* ctx = nullptr;
static gfx_surface_t surface = {};
static gfx_pipeline_t* pipeline = nullptr;

uintptr_t g_handle;



void platform_main(uintptr_t handle, int argc, char** argv)
{
    auto p0 = sizeof(vertex);
    auto p1 = sizeof(vertex_packed);
    memory::enable_tracking();

    thread_test();

    g_handle = handle;
    gfx_settings_t settings = {0};
        settings.handle     = handle;
        settings.backend    = gfx_backend_vulkan;
    //    settings.backend    = gfx_backend_webgpu;
     //   settings.options    = gfx_options_debug;
        settings.dbglog     = log_func;
        settings.allocator  = &gfx_allocator;
    gfx_init(&settings, &ctx);


    gfx_surface_desc_t surface_desc = {};
        surface_desc.label              = "main_view";
        surface_desc.window_handle      = handle;
        surface_desc.vsync              = true;
        surface_desc.preferred_format   = gfx_pixel_format_rgba8;
        surface_desc.sample_count       = gfx_sample_1x;
    surface = gfx_surface_create(ctx, &surface_desc);

    gfx_shader_t* compute = nullptr;
    load_shader_from_file_path(ctx, "../data/shaders/compute.hlsl", &compute);

    gfx_compute_pipeline_desc_t compute_desc = {};
    compute_desc.shader = compute;
    gfx_pipeline_compute_t * compute_pipeline = gfx_compute_pipeline_create(ctx, &compute_desc);


    resource_manager::create_and_make_shader(ctx);
    render_system::create_and_make_shader(ctx);

    resource_manager::shared()->mount("../data/");

    
    gfx_shader_t* shader = nullptr;
    load_shader_from_file_path(ctx, "../data/shaders/simple.hlsl", &shader);


    gfx_vertex_attribute attributes[] = {
        { 0, 0, gfx_vertex_format_float4,   offsetof(vertex, position)  },
        { 1, 0, gfx_vertex_format_float4,   offsetof(vertex, uv)        },
        { 2, 0, gfx_vertex_format_float4,   offsetof(vertex, normal)    },
        { 3, 0, gfx_vertex_format_float4,   offsetof(vertex, tangent)   },
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
    pipeline = gfx_pipeline_create(ctx, &piplene_desc);
    
   /* mvp_location            = gfx_uniform_location(shader, "mvp");
    auto color_location     = gfx_uniform_location(shader, "_color");
    auto texture_location   = gfx_uniform_location(shader, "_texture0");
    auto sampler_location   = gfx_uniform_location(shader, "_textureSampler");
    auto lightmap_location  = gfx_uniform_location(shader, "_lightmap");
    auto lightmap_scale_offset_location = gfx_uniform_location(shader, "lightmap_scale_offset");

    gfx_sampler_desc_t linear_filtering = {};
        linear_filtering.minmag = gfx_filter_linear;
        linear_filtering.mipmap = gfx_filter_linear;
   //     linear_filtering.anisotropy = 8;
    gfx_sampler_t* sampler = gfx_create_sampler2(ctx, &linear_filtering);*/

    rect_t rc = platform_get_window_size(handle);
    uint16_t width = rc.w - rc.x;
    uint16_t height = rc.h - rc.y;
    float aspect = (float)(width) / (float)(height);

    g_camera.set_fov(55);
    g_camera.set_near_far(0.01f, 1500.0f);
    g_camera.set_apect(aspect);
    g_camera.set_pos(math::make_vec3(4.366324f, 9.78074f, 185.8569f));
  //  g_camera.set_pos(math::make_vec3(0, 0, -5));
    g_camera.set_target(g_camera._pos + math::make_vec3(0, 0, 1));

  //  scene_test(ctx, "../data/unity", "Southside.big.json");
  //  scene_test(ctx, "../data/unity", "../data/unity/Southside.big.json");
   // scene_test(ctx, "../data/unity", "../data/unity/City.json");
    scene_test(ctx, "../data/gungsta", "../data/gungsta/Demo.json");
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
    dir *= input_kb_state(Input::Keyboard::Shift) ?  10.0f : 1.0f;
    auto p = input_point_pos();

    vec2 mp = { (float)-p.dx, (float)p.dy };
    cam.set_mouse_dt(mp * 0.5f);
    cam.move(dir * Time::dt() * 1.0f);
    cam.update();
}


// main    | input, network_in, scripts, network_out                |
// render  | animation, upload_resorces | gpu_culling | render      |
// job0    | evaluate_mip,                                          |
// job1    | cpu_culling + instancing   |                           |
// job2    | streaming                                              |
// job3    | streaming                                              |
// sound   | mix, render                                            |


void platform_tick(void* userdata)
{
   // measure ms("\nplatform_tick");
    Time::tick();

    update_camera(g_camera);

  //  if(input_kb_state(Input::Keyboard::NumPad_Add) )
  //      g_camera.set_fov(g_camera.m_fov + 0.1f);

  //  if (input_kb_state(Input::Keyboard::NumPad_Subtract))
  //      g_camera.set_fov(g_camera.m_fov - 0.1f);

    if ( input_kb_state(Input::Keyboard::NumPad_Subtract)){
        g_camera.m_near -= 0.001f;
    }
    if ( input_kb_state(Input::Keyboard::NumPad_Add) ) {
        g_camera.m_near += 0.001f;
    }

    if (input_kb_state(Input::Keyboard::M))
    {
        memory_stats_t  stats = {};
        memory::dump(&stats);
        debug::log("\nmemsize : %.3f Mb", (float)(stats.total_allocated_size) / 1024.0f / 1024.0f);
    }

    rect_t rect = platform_get_window_size(g_handle);

    float width = (float)(rect.w - rect.x);
    float height = (float)(rect.h - rect.y);

    g_camera.setup(g_camera.m_fov, width / height, g_camera.m_near, g_camera.m_far);
    g_camera.update();
    
    auto frame = gfx_begin_frame(ctx, &surface);
        gfx_cmd_begin_pass(frame->cmd, frame->target);
        g_scene.draw(frame->cmd, g_camera);
        gfx_cmd_end_pass(frame->cmd);
    gfx_end_frame(frame);
}
 

void platform_destroy(void* userdata)
{
}