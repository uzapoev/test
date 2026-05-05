#ifndef __scene_json_h__
#define __scene_json_h__

#include "../mathlib.h"
#include "../json_serializer.h"
#include "scene.h"

JsonSerializeExternal(vec2, 
    SerializeFieldWithKey("x", x), 
    SerializeFieldWithKey("y", y));

JsonSerializeExternal(vec3, 
    SerializeFieldWithKey("x", x), 
    SerializeFieldWithKey("y", y), 
    SerializeFieldWithKey("z", z));

JsonSerializeExternal(vec4, 
    SerializeFieldWithKey("x", x), 
    SerializeFieldWithKey("y", y), 
    SerializeFieldWithKey("z", z),
    SerializeFieldWithKey("w", w));

JsonSerializeExternal(quat, 
    SerializeFieldWithKey("x", x), 
    SerializeFieldWithKey("y", y), 
    SerializeFieldWithKey("z", z), 
    SerializeFieldWithKey("w", w));

JsonSerializeExternal(lodgroup,
    SerializeFieldWithKey("mesh", renderers));

JsonSerializeExternal(renderer,
    SerializeFieldWithKey("mesh", mesh_guid),
    SerializeFieldWithKey("material", material_guid),
    SerializeFieldWithKey("lightmap", lightmap_guid),
    SerializeFieldWithKey("lightmapScaleOffset", lightmap_scale_offset));

JsonSerializeExternal(transform,
    SerializeFieldWithKey("position",   position),
    SerializeFieldWithKey("scale",      scale),
    SerializeFieldWithKey("rotation",   rotation));

JsonSerializeExternal(node,
    SerializeFieldWithKey("name",       name),
    SerializeFieldWithKey("tag",        tag),
    SerializeFieldWithKey("transform",  transform),
    SerializeFieldWithKey("renderer",   renderer),
    SerializeFieldWithKey("childs",     childs));

JsonSerializeExternal(scene,    
    SerializeFieldWithKey("childs",     m_nodes),
    SerializeFieldWithKey("meshes",     m_meshes),
    SerializeFieldWithKey("materials",  m_materials));


struct scene_reader_json
{
    static scene create_form_file(const std::string_view& path)
    {
        auto data = resource_manager::file_data(path);
        std::string tmp(data.begin(), data.end());

        return json::from_json_string<scene>(tmp);
    }
};

#endif