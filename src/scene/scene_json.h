#ifndef __scene_json_h__
#define __scene_json_h__

#include <functional>
#include <string_view>
#include <unordered_map>

#include "../mathlib.h"
#include "../json_serializer.h"
#include "scene.h"

ReflectObjectExternal2(vec2, x, y);
ReflectObjectExternal2(vec3, x, y, z);
ReflectObjectExternal2(vec4, x, y, z, w);
ReflectObjectExternal2(quat, x, y, z, w);

ReflectObjectExternal2(node, guid, parent, name, tag, flags);
ReflectObjectExternal2(transform, position, scale, rotation);
ReflectObjectExternal2(lodgroup, lod_count);
ReflectObjectExternal2(light, color, dir, radius, intensity, mode, shadowMode);
ReflectObjectExternal2(streaming_component, scene_guid, load_distance, unload_distance);

ReflectObjectExternal(renderer,
    ReflectObjectFieldWithKey("mesh", mesh_guid),
    ReflectObjectFieldWithKey("material", material_guid),
    ReflectObjectFieldWithKey("materials", material_guids),
    ReflectObjectFieldWithKey("lightmap", lightmap_color_guid),
    ReflectObjectFieldWithKey("lightmap_mask", lightmap_mask_guid),
    ReflectObjectFieldWithKey("lightmapScaleOffset", lightmap_scale_offset));


ReflectObjectExternal(scene,    
    ReflectObjectFieldWithKey("nodes",      m_nodes),
    ReflectObjectFieldWithKey("meshes",     m_mesh_guids),
    ReflectObjectFieldWithKey("materials",  m_materials_guids)
   );


// ======================================================================================================================
// Dynamic, component based json scene (editor format). Implementation: scene_json.cpp
//
//   scene json: { "meshes": [guid...], "textures": [...], "shaders": [...], "materials": [...], "nodes": [ node, node, ... ] }
//   The four guid lists are the assets the scene needs (the header of the binary scene: scene_header_t), the nodes are a flat list.
//   node json:  { "name", "guid", "parent": "<guid>", "flags", "transform", "prefab": {"guid", "overrides", "added"}, "<component>": {...}, ... }
//
//   - the hierarchy is the "parent" guid (a node of the same list, none = root), resolved after all the nodes are created,
//     so the order of the list is free; the order of siblings is the order in the list. A prefab file has the same format,
//     its first node is the root,
//   - a node is read by json::deserialize_sajson(tinynode, json, ctx): the fields of tinynode (name, parent, flags) by
//     reflection, every other key is handed to load_ctx::unused() = a component ("transform", "renderer", "scripts", ...).
//     The order of the keys doesn't matter. A component is added to the node only when its key is in the json,
//   - a key nobody has a component for is NOT lost: it is kept as raw json (unknown_fields component) and written back by save().
//
//   A new component: ReflectObject(...) in the component, a line in load_ctx::unused() and in write_components() (scene_json.cpp).
//
// Reading is a patch: a key that is missing in json leaves the field as it is. A prefab instance ("prefab": guid) is loaded
// first, then the instance's own keys are read on top of it, so { "prefab": g, "renderer": { "material": m } } overrides
// the material only. (A vector field is appended to, not replaced.)
// ======================================================================================================================
namespace scene_json
{
    struct load_options
    {
        // guid of a prefab -> path of its json file ("" = not found). Prefab file = scene file with one root node.
        std::function<std::string(const char* guid)> prefab_path;
    };

    // Creates the nodes (tinynode + components) of the json in `target`. Returns false when the json can't be parsed.
    bool            load(scene& target, size_t size, char* data, const load_options& options = {});
    bool            load_file(scene& target, const std::string_view& path, const load_options& options = {});

    // Writes the nodes of `source` (m_tiny_nodes) back to the json format above.
    std::string     save(scene& source);
}

struct scene_ctx
{
  //  scene * owner = nullptr;

    template<class T, class Ctx> 
    void unused(T& obj, const sajson::string& k, const sajson::value& v, Ctx& ctx)
    {
        bool processed = true;
        if constexpr(std::is_same_v<T, node>)
        {
            node & _node = obj;
            if (json::detail::key_equals(k, "transform")) {
                auto t = _node.add_component<transform>();
                json::deserialize_sajson(*t, v, ctx);
            } 
            else if (json::detail::key_equals(k, "renderer")) {
                auto r = _node.add_component<renderer>();
                json::deserialize_sajson(*r, v, ctx);
            } 
            else if (json::detail::key_equals(k, "collider")) {
                auto c = _node.add_component<collider>();
                json::deserialize_sajson(*c, v, ctx);
            } 
            else if (json::detail::key_equals(k, "light")) {
                auto l = _node.add_component<light>();
                json::deserialize_sajson(*l, v, ctx);
            }
            else if (json::detail::key_equals(k, "prefab")) {
                // overrides + added:
            }
            else {
                processed = false;
            }
        }
        if(!processed)
            debug::log_warning("[json_serializer]:unutilized json object with key: \'%s\'", k.as_string().c_str());
    };
};

struct scene_reader_json
{
    static scene create_from_file(const std::string_view& path)
    {
        scene_ctx ctx;// = { &target };

        auto data = resource_manager::file_data(path);
        scene target = json::from_json_string_ctx<scene>(data.size(), data.data(), ctx);
        target.link_hierarchy();
        return target;
    }
};

#endif
