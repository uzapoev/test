#ifndef __components_h__
#define __components_h__

#include "../mathlib.h"
#include "../render_manager.h"
#include "../resource_manager.h"


enum component_type : uint16_t
{
    component_unknown,      //

    component_node,         // name, guid, tag, flags

    component_transform,    // 
    component_renderer,     // mesh, materials, lightmap, renderparams(cast shadow, etc), occluder
    component_lodgroup,     // level of details
    component_streaming,    // streaming data
    component_collider,     // box, sphere, capsule, mesh, trigger
    component_rigidbody,    // 
    component_light,        // point/dir/area, type(static/dynamic/mixed)
    component_animator,     // skinned mesh animator
    component_cinematic,    // kinda dotweens/ transform animations
    component_navagent,     // pathfinding
    component_script,       // scripts(backends: lua/c#/native)
    component_custom,       // user data component
};

namespace components
{
    struct icomponent { protected: component_type m_type; };

    struct transform : icomponent
    {
        vec3                    position;
        vec3                    scale;
        quat                    rotation;

        mat4                    local_transform;
        mat4                    global_transform;
    };

    struct renderer : icomponent
    {
        interned_string         mesh_guid;          // mesh guid
        interned_string         material_guid;      // material guid
        interned_string         lightmap_guid;      // lightmap guid
        vec4                    lightmap_scale_offset;

        bool                    is_in_lodgroup;    //
        interned_string         occulder_guid;

    public:
        void clear()
        {
            mesh_guid.clear();
            material_guid.clear();
            lightmap_guid.clear();
            occulder_guid.clear();

       //     if(m_material)
       //         m_material->release();
        }

        gfx_mesh_t *            m_mesh;
        material   *            m_material;
    };

    struct occluder : icomponent        { };

    struct lodgroup : icomponent
    {
        float                   distance;
        std::vector<renderer>   renderers;
    };
     
    struct iphysic  : icomponent        { };
    struct physic2d : iphysic           { };
    struct physic3d : iphysic           { };
    struct collider : icomponent        { };
    struct box_collider : collider      { };
    struct sphere_collider : collider   { };
    struct capsule_collider : collider  { };
    struct mesh_collider : collider     { };

    struct rigidbody: iphysic       { };

    struct navagent : icomponent    { };
    struct animator : icomponent    { };
    struct cinematic: icomponent    { };
    struct script:    icomponent    { };
    struct hierarchy: icomponent    { };
};



class component_storage
{
    class istorage{
        virtual void clean() = 0;
    };

    template<class T>
    class storage : istorage
    {
        
    };

public:
    template<class T>
    T * allocate(char * data)
    {
        auto storage = get_storage(type<T>::index())
        if(storage == nullptr)
            return nullptr;

        auto tmpstarage = (ComponentStorage<T>*)(storage);
        return 
    }


    istorage* get_storage(uint32_t type)
    {
        return m_storage.at(type);
    }


    std::unordered_map<uint32_t, class istorage*> m_storage;
};


#endif