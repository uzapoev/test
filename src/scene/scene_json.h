#ifndef __scene_json_h__
#define __scene_json_h__

#include "../mathlib.h"
#include "../json_serializer.h"
#include "scene.h"

ReflectObjectExternal2(vec2, x, y);
ReflectObjectExternal2(vec3, x, y, z);
ReflectObjectExternal2(vec4, x, y, z, w);
ReflectObjectExternal2(quat, x, y, z, w);

ReflectObjectExternal(lodgroup,
    ReflectObjectFieldWithKey("lod_count", lod_count));

ReflectObjectExternal(renderer,
    ReflectObjectFieldWithKey("mesh", mesh_guid),
    ReflectObjectFieldWithKey("material", material_guid),
    ReflectObjectFieldWithKey("materials", material_guids),
    ReflectObjectFieldWithKey("lightmap", lightmap_color_guid),
    ReflectObjectFieldWithKey("lightmap_mask", lightmap_mask_guid),
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

ReflectObjectExternal(tinynode, 
    ReflectObjectField(guid),
    ReflectObjectField(name),
    ReflectObjectField(flags));

struct ed_node : tinynode {
    transform               transform;
    renderer                renderer;

    std::vector<node>       childs;

    ReflectObjectInherited(ed_node, tinynode,
        ReflectObjectFieldWithKey("childs", childs),
        ReflectObjectFieldWithKey("transform", transform),
        ReflectObjectFieldWithKey("renderer", renderer)
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
};

#endif