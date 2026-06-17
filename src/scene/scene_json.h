#ifndef __scene_json_h__
#define __scene_json_h__

#include "../mathlib.h"
#include "../json_serializer.h"
#include "scene.h"

ReflectObjectExternal(vec2, 
    ReflectObjectFieldWithKey("x", x), 
    ReflectObjectFieldWithKey("y", y));

ReflectObjectExternal(vec3, 
    ReflectObjectFieldWithKey("x", x), 
    ReflectObjectFieldWithKey("y", y), 
    ReflectObjectFieldWithKey("z", z));

ReflectObjectExternal(vec4, 
    ReflectObjectFieldWithKey("x", x), 
    ReflectObjectFieldWithKey("y", y), 
    ReflectObjectFieldWithKey("z", z),
    ReflectObjectFieldWithKey("w", w));

ReflectObjectExternal(quat, 
    ReflectObjectFieldWithKey("x", x), 
    ReflectObjectFieldWithKey("y", y), 
    ReflectObjectFieldWithKey("z", z), 
    ReflectObjectFieldWithKey("w", w));

ReflectObjectExternal(lodgroup,
    ReflectObjectFieldWithKey("mesh", renderers));

ReflectObjectExternal(renderer,
    ReflectObjectFieldWithKey("mesh", mesh_guid),
    ReflectObjectFieldWithKey("material", material_guid),
    ReflectObjectFieldWithKey("lightmap", lightmap_guid),
    ReflectObjectFieldWithKey("lightmapScaleOffset", lightmap_scale_offset));

ReflectObjectExternal(transform,
    ReflectObjectFieldWithKey("position",   position),
    ReflectObjectFieldWithKey("scale",      scale),
    ReflectObjectFieldWithKey("rotation",   rotation));

ReflectObjectExternal(node,
    ReflectObjectFieldWithKey("name",       name),
    ReflectObjectFieldWithKey("tag",        tag),
    ReflectObjectFieldWithKey("transform",  transform),
    ReflectObjectFieldWithKey("renderer",   renderer),
    ReflectObjectFieldWithKey("childs",     childs));

ReflectObjectExternal(scene,    
    ReflectObjectFieldWithKey("childs",     m_nodes),
    ReflectObjectFieldWithKey("meshes",     m_meshes),
    ReflectObjectFieldWithKey("materials",  m_materials));

struct ed_node {
    interned_string         name;
    interned_string         guid;
    interned_string         tag;
    uint64_t                flags; // static, enabled

    uint32_t                index = 0;

    transform               transform;
    renderer                renderer;

    std::vector<node>       childs;

    ReflectObject(ed_node,
        ReflectObjectFieldWithKey("name", name),
        ReflectObjectFieldWithKey("guid", guid),
        ReflectObjectFieldWithKey("tag", tag),
        ReflectObjectFieldWithKey("transform", transform),
        ReflectObjectFieldWithKey("renderer", renderer),
        ReflectObjectFieldWithKey("childs", childs)
    );
};

struct ed_scene {
    // resource dependencie
    std::vector<interned_string> meshes;
    std::vector<interned_string> textures;
    std::vector<interned_string> materials;
    std::vector<interned_string> shaders;

    std::vector<node> nodes;

    ReflectObject(ed_scene, 
        ReflectObjectFieldWithKey("meshes", meshes),
        ReflectObjectFieldWithKey("textures", textures),
        ReflectObjectFieldWithKey("materials", materials),
        ReflectObjectFieldWithKey("shaders", shaders),
        ReflectObjectFieldWithKey("childs", nodes)
    );
};

struct scene_reader_json
{
    static scene create_from_file(const std::string_view& path)
    {
        auto data = resource_manager::file_data(path);

        //auto tmp = json::from_json_string<ed_scene>(data.size(), data.data());

        return json::from_json_string<scene>(data.size(), data.data());
    }


    static scene convert_ed_scene_to_scene(ed_scene & scene)
    {
    }
};

#endif