#ifndef __scene_h__
#define __scene_h__

#include <atomic>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

#include "../mathlib.h"
#include "../resource_manager.h"
#include "../common.h"

#include "components.h"

#include "ecs.h"

constexpr uint32_t const_hash(char const* input) {
    return *input ? static_cast<uint32_t>(*input) + 33 * const_hash(input + 1) : 5381;
}



struct camera : icomponent
{
    void set_fov(float v)                                       { m_fov = v; }
    void set_apect(float v)                                     { m_aspect = v; }
    void set_near_far(float n, float f)                         { m_near = n; m_far = f; }

    void setup(float fov, float aspect, float near, float far)  { m_fov = fov;  m_aspect = aspect;  m_near = near;  m_far = far; }

    void set_pos(const vec3& v)                                 { _pos = v; }
    void set_target(const vec3& v)                              { _target = v; }

    vec3 forward()                                              { return math::mul(_rot, math::Forward); }
    vec3 left()                                                 { return math::mul(_rot, math::Left); }
    const mat4& view_proj() const                               { return m_view_proj; }

    void move(const vec3& dir)                                  { _pos += dir; _target += dir; }
    void set_mouse_dt(const vec2& delta)                        { _axis += delta * 0.5; }

    void update();

    // Drive the camera from an external controller (a game's player) instead of mouse delta + target.
    // Angles are in radians, y up, right handed: at yaw = pitch = roll = 0 the camera looks along -z; positive yaw
    // turns left, positive pitch looks up (three.js 'YXZ' euler order, R = Ry(yaw) * Rx(pitch) * Rz(roll)).
    // Uses m_fov / m_aspect / m_near / m_far (see setup()) and updates _pos, _target and m_view_proj.
    // Returns the view matrix, because the struct is uploaded to the GPU as is and has no room for another member.
    mat4 set_pose(const vec3& eye, float yaw, float pitch, float roll)
    {
        const float sy = sinf(yaw), cy = cosf(yaw), sp = sinf(pitch), cp = cosf(pitch), sr = sinf(roll), cr = cosf(roll);
        const vec3 fwd = { -sy * cp, sp, -cy * cp };
        const vec3 up  = { -cy * sr + sy * sp * cr, cp * cr, sy * sr + cy * sp * cr };

        _pos = eye;
        _target = eye + fwd;
        mat4 proj = math::perspective_matrix(math::deg2rad(m_fov), m_aspect, m_near, m_far, true);
        mat4 view = math::look_at_matrix(eye, _target, up, true);
        m_view_proj = math::mul(proj, view);
        return view;
    }

    float m_fov         = 45.0f;
    float m_aspect      = 4.0f / 3.0f;
    float m_near        = 0.1f;
    float m_far         = 500.0f;

   // vec4 _fanf          = { 45.0f , 4.0f / 3.0f ,0.1f , 50.0f };// fow, aspect, near, far
    quat _rot           = quat::identity();
    vec3 _pos           = math::Zero; 
    vec3 _target        = math::Backward;
    vec2 _axis;
    mat4 m_view_proj    = mat4::identity();
};


struct node : tinynode
{
    node();
    uint32_t                index = 0;
    hierarchy               hierarchy;  // links by index, filled by scene::link_hierarchy(); TODO: editor only (#ifdef) if the memory gets tight
};


class scene
{
public:
    enum component_type : uint32_t {
        scene_meta_data             = const_hash("meta"),

        component_node              = const_hash("node"),               // name, guid, tag, flags
        component_hierarchy         = const_hash("hierarchy"),          // parent, childs;
        component_transform         = const_hash("transform"),          // 

        component_camera            = const_hash("camera"),
        component_renderer          = const_hash("renderer"),           // mesh, materials, lightmap, renderparams(cast shadow, etc), occluder
        component_decal             = const_hash("decal"),              // <-- NEW:
        component_lightprobes       = const_hash("lightprobes"),        // sh/sg
        component_reflection_probe  = const_hash("reflection_probe"),   // <-- NEW:
        component_particle_system   = const_hash("particles"),          // <-- NEW: (VFX)

        component_light             = const_hash("light"),              // point/dir/area, type(static/dynamic/mixed)
        component_lodgroup          = const_hash("lodgroup"),           // level of details
        component_occlusion         = const_hash("occlusion"),          // occlusion data
        component_streaming         = const_hash("streaming"),          // streaming data

        component_volume_profile    = const_hash("volume_profile"),

        component_collider          = const_hash("collider"),           // box, sphere, capsule, mesh, trigger
        component_collider2d        = const_hash("collider2d"),         // box, sphere, capsule, mesh, trigger
        component_rigidbody         = const_hash("rigidbody"),          // 

        component_animator          = const_hash("animator"),           // skinned mesh animator
        component_cinematic         = const_hash("cinematic"),          // kinda dotweens/ transform animations

        component_navagent          = const_hash("navagent"),           // pathfinding
        component_navmap            = const_hash("navmap"),             // pathfinding
        component_navobstacle       = const_hash("navobstacle"),        // pathfinding

        component_audio_ambient     = const_hash("audio_ambient"),      // 
        component_audio_geometry    = const_hash("audio_geometry"),     // room/reflector/reverbzone etc
        component_audio_portal      = const_hash("audio_portal"),       // 

        component_audio_room        = const_hash("audio_room"),         // 
        component_audio_reflector   = const_hash("audio_reflector"),    // 

        component_canvas            = const_hash("canvas"),
        component_script            = const_hash("script"),             // scripts(backends: lua/c#/native)
        component_userdata          = const_hash("userdata"),           // user data component
    };

    static scene    create_from_json_file(const std::string_view& path);
    static scene    create_from_xml_file(const std::string_view& path);
    static scene    create_from_file(const std::string_view& path);

public:
    void            load(const std::string_view& path);
    void            save(const std::string_view& path);

    // node::parent (guid) -> node::hierarchy (indexes). Call when all the nodes are in m_nodes (a parent may come after its child).
    void            link_hierarchy();

    void            clear();
    void            update();
    void            draw(gfx_command_buffer_t* cmd, camera & cam);

 public:
   node *           create_node(interned_string name = "", guid_t guid = {});

public:
    friend struct scene_reader_json;
    friend struct scene_reader_xml;

    class world*                            m_world = nullptr;

    std::vector<node>                       m_nodes;
     
    entity_query<transform, renderer>       m_render_query;
    std::vector<tinynode*>                      m_tiny_nodes;
    
    //scene resources
    std::vector<guid_t> m_mesh_guids;
    std::vector<guid_t> m_texture_guids;
    std::vector<guid_t> m_shaders_guids;
    std::vector<guid_t> m_materials_guids;

private:

  //  SerializeObject(scene, ReflectObjectFieldWithKey("nodes", m_nodes));
};

#endif