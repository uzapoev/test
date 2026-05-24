#ifndef __world_h__
#define __world_h__

#include "scene.h"

struct chunk_writer {

public:
    void write(char * data, size_t size){
        for(int i = 0; i < size; ++i)
            m_data.emplace_back(data[i]);
    }
    void    reset() { m_data.clear();  }
    size_t  size()  { return m_data.size(); }
    char *  data()  { return m_data.data(); }
private:
    size_t              m_pos;
    size_t              m_size;
    std::vector<char>   m_data;
};


inline void write_chunk_info(filestream* stream, uint32_t type, uint32_t size) {
    stream->write(type);
    stream->write(size);
}

inline void write_chunk(filestream* stream, uint32_t type, uint32_t size, const char* data) {
    stream->write(type);
    stream->write(size);
    stream->write(size, data);
}


class world
{
public:
    world()
    {
        m_renderer = render_system::shared();
        m_resources = resource_manager::shared();
    }

    void init()
    {
        //register_serializer<renderer>()
    }

    node* load_node(class scene* _scene, filestream* stream)
    {    
        interned_string guid    = stream->read<interned_string>();
        interned_string name    = stream->read<interned_string>();
        interned_string tag     = stream->read<interned_string>();
        uint64_t flags          = stream->read<uint64_t>();
        return _scene->create_node(name, guid);
    }

    void save_node(class node* _node, filestream* stream)
    {
        uint32_t size = _node->guid.length() + sizeof(uint16_t) +
                        _node->name.length() + sizeof(uint16_t) +
                        _node->tag.length()  + sizeof(uint16_t) +
                        sizeof(_node->flags);

        write_chunk_info(stream, scene::component_node, size);
        stream->write(_node->guid);
        stream->write(_node->name);
        stream->write(_node->tag);
        stream->write(_node->flags);
    }

    template<class T> T* deserialize(class scene* _scene, filestream* stream){}
    template<class T> void serialize(T * _component, filestream* stream){}


    /// <summary>
    ///  renderer
    /// </summary>
    /// <param name="_scene"></param>
    /// <param name="stream"></param>
    /// <returns></returns>
    template<> renderer* deserialize(class scene* _scene, filestream* stream)
    {
        auto component = m_renderer->allocate_renderer();

        component->mesh_guid                = stream->read<interned_string>();;
        component->material_guid            = stream->read<interned_string>();
        component->lightmap_guid            = stream->read<interned_string>();
        component->lightmap_scale_offset    = stream->read<vec4>();
       
        return component;
    }

    template<> void serialize(class renderer * _component, filestream* stream)
    {
        uint32_t chunk_size =   _component->mesh_guid.length()      + sizeof(uint16_t) +
                                _component->material_guid.length()  + sizeof(uint16_t) +
                                _component->lightmap_guid.length()  + sizeof(uint16_t) +
                                sizeof(_component->lightmap_scale_offset);

        write_chunk_info(stream, scene::component_renderer, chunk_size);
        stream->write(_component->mesh_guid);
        stream->write(_component->material_guid);
        stream->write(_component->lightmap_guid);
        stream->write(_component->lightmap_scale_offset);

        /*
        * archive->write(_component->mesh_guid);
        * archive->write(_component->material_count);
        * for(int i = 0; i <_component->material_count; ++i)
        * {
        *       archive->write(_component->materials[i].guid);
        *       archive->write_material_overrides();
        * }
        * 
        * stream->write_chunk(component_renderer, id, archive.size(), archive.data());
        */
    }

    template <> transform * deserialize(class scene* _scene, filestream* stream)
    {
        auto component = _scene->create_transform();

        component->position = stream->read<vec3>();
        component->rotation = stream->read<quat>();
        component->scale    = stream->read<vec3>();
        component->update_transform();/**/
        
        return component;
    }


    template <> void serialize(class transform* _transform, filestream* stream)
    {
        uint32_t size = sizeof(vec3) + sizeof(quat) + sizeof(vec3);
        write_chunk_info(stream, scene::component_transform, size);
        stream->write(_transform->position);
        stream->write(_transform->rotation);
        stream->write(_transform->scale); 
    }

    template <> collider* deserialize(class scene *_scene, filestream * stream)
    {
        auto collider_type = stream->read<uint16_t>();
        auto trigger_type = stream->read<uint16_t>();   // is trigger

        switch (collider_type)
        {
            case collider::sphere:  stream->read<vec4>(); break;                        // sphere:  x,y,z - pos, w - radius
            case collider::box:     stream->read<vec4>(); stream->read<vec4>(); break;  // box: center(float4) + extend (float4)
            case collider::mesh:    stream->read<interned_string>();  break;            // mesh: guid string
            default:
                debug::log_warning("(%s) has unknown colider type - %d", "m_nodes[i].name", collider_type);
                break;
        }
        return nullptr;
    }


    void register_serializer(uint32_t type, std::function<void(class scene*, class filestream*)>)
    {
    }

    bool resolve_component(uint32_t type, std::function<void(class scene*, class filestream*, struct icomponent**)>* cb)
    {
        return false;
    }

    class resource_manager *    m_resources     = nullptr;
    class render_system*        m_renderer      = nullptr;
    class physic2d_manager *    m_physics2d     = nullptr;
    class physic3d_manager *    m_physics3d     = nullptr;
    class navigation_manager *  m_navigation    = nullptr;   // recast navmesh
    class sound_system *        m_audio         = nullptr;   // recast navmesh
    char                        m_tmpbuffer[512] = {};
//  uicanvas *                  m_canvas;       // ui renderer
};

#endif