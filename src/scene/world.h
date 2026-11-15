#ifndef __world_h__
#define __world_h__

#include "scene.h"

inline void write_chunk_info(filestream* stream, uint32_t type, uint32_t size) {
    stream->write(type);
    stream->write(size);
}

inline void write_chunk(filestream* stream, uint32_t type, uint32_t size, const void* data) {
    stream->write(type);
    stream->write(size);
    stream->write(size, data);
}


class entity_factory {
public:
    static entity_factory& instance() {
        static entity_factory inst;
        return inst;
    }

    entity_factory() {
        auto memory_resource = new aligned_allocator("entity_factory");
        m_node_allocator = new paged_pool_allocator(memory_resource, sizeof(tinynode), 1024 * 16);
    }

    tinynode* create_entity()
    {
        uint32_t assigned_id = 0;

        // Check if we can recycle an old ID to keep the pools dense
        if (!m_free_runtime_ids.empty()) {
            assigned_id = m_free_runtime_ids.front();
            m_free_runtime_ids.pop();
        }
        else {
            assigned_id = m_next_runtime_id++;
        }
        tinynode* tnode = m_node_allocator->allocate<tinynode>();
        tnode->guid = uuid::generate_uuid_v4();
        tnode->runtime_id = assigned_id;
        tnode->component_mask = 0;

        m_guid_to_node_map[tnode->guid] = tnode;

        return tnode;
    }

    tinynode* create_entity(guid_t persistent_guid) {
        uint32_t assigned_id = 0;
        if (!m_free_runtime_ids.empty()) {
            assigned_id = m_free_runtime_ids.front();
            m_free_runtime_ids.pop();
        }
        else {
            assigned_id = m_next_runtime_id++;
        }

        tinynode* tnode = create_entity();
        m_guid_to_node_map.erase(tnode->guid);
        tnode->guid = persistent_guid;
        m_guid_to_node_map[persistent_guid] = tnode;

        return tnode;
    }

    void destroy_entity(tinynode* node) {
        if (!node) return;
        m_free_runtime_ids.push(node->runtime_id);
        m_guid_to_node_map.erase(node->guid);
        node->~tinynode();
        m_node_allocator->deallocate(node);
    }

    // Quick runtime translation (useful for scripts working with asset references)
    tinynode* get_runtime_id(const guid_t& guid) const {
        auto it = m_guid_to_node_map.find(guid);
        return (it != m_guid_to_node_map.end()) ? it->second : nullptr;
    }

private:
    paged_pool_allocator*                               m_node_allocator = nullptr;
    uint32_t                                            m_next_runtime_id = 0;
    std::queue<uint32_t>                                m_free_runtime_ids;
    std::unordered_map<guid_t, tinynode*>  m_guid_to_node_map;
};

class world
{
public:

    static tinynode* create_node(class scene* owner = nullptr)
    {
        auto& factory = entity_factory::instance();
        return factory.create_entity();
    }

    static tinynode* create_node(guid_t guid, class scene* owner = nullptr)
    {
        auto& factory = entity_factory::instance();
        return factory.create_entity(guid);
    }

    static void destroy_node(tinynode * node)
    {
        entity_factory::instance().destroy_entity(node);
    }


    world()
    {
    }

    void init()
    {
    }



    tinynode * create_tinynode()
    {
        return entity_factory::instance().create_entity();
    }

    /*
    std::vector<icomponent*> get_all_components(tinynode* node) {
        std::vector<icomponent*> result;

        uint64_t mask = node->component_mask;
        if (mask == 0) return result;

        auto& manager = component_manager::instance();

        for (uint32_t i = 0; i < 64; ++i) {
            if ((mask & (1ULL << i)) != 0) {
                if (icomponent_pool* pool = manager.get_pool_by_type_index(i)) {
                    void* data = pool->get_raw(node->runtime_id);
                    const char* name = component_registry::type_name(i);

                   result.push_back((icomponent*)data);
                }
            }
        }
        return result;
    }*/
    
    node* load_node(class scene* _scene, filestream* stream)
    {    
        guid_t guid     = stream->read<guid_t>();
        guid_t parent   = stream->read<guid_t>();
        interned_string name    = stream->read<interned_string>();
        interned_string tag     = stream->read<interned_string>();
        uint64_t flags          = stream->read<uint64_t>();
        return _scene->create_node(name, guid);
    }

    void save_node(struct node* _node, filestream* stream)
    {
     /*   binary_writer writer;
        writer.write((uint32_t)_node->guid.length());

        uint32_t size = (uint32_t)_node->guid.length() + sizeof(uint16_t) +
                        (uint32_t)_node->name.length() + sizeof(uint16_t) +
                        (uint32_t)_node->tag.length()  + sizeof(uint16_t) +
                        sizeof(_node->flags);

        write_chunk_info(stream, scene::component_node, size);
        stream->write(_node->guid);
        stream->write(_node->name);
        stream->write(_node->tag);
        stream->write(_node->flags);*/
    }
    /**/
};

#endif