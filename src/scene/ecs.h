#ifndef __ecs_h__
#define __ecs_h__
#include <atomic>
#include <queue>
//#include "../memmgr.h"

// struct rigid_body : public ecs_component { float mass = 1.0f; };
// struct rigid_body_2d : public ecs_component { float mass = 1.0f; };
//
// class physics_system : public iquery_listener {
// private:
//     jph_physics_system* m_physics_world; // External physics world instance (Box2D world / Jolt PhysicsSystem)
// 
// public:
//     virtual void on_node_changed(tinynode* node, component_manager& manager) override {
//         auto* rb_comp = manager.get_component<rigid_body_component>(node);
//         
//         // Scenario A: Component added but physics body doesn't exist yet
//         if (rb_comp && rb_comp->physical_body_id == 0) {
//             
//             // 1. Create the external body using data from our component
//             uint32_t external_id = m_physics_world->create_body(
//                 node->guid, // Pass entity mapping if needed
//                 rb_comp->mass,
//                 rb_comp->is_static
//             );
// 
//             // 2. Link them together: Save external handle into our component
//             rb_comp->physical_body_id = external_id;
// 
//             // 3. Reverse Link: Store our runtime_id inside external user data
//             // This is critical for collision callbacks!
//             m_physics_world->set_body_user_data(external_id, (uint64_t)node->runtime_id);
//         }
//         
//         // Scenario B: Component was removed from node, but physical body still exists
//         if (!rb_comp) {
//             // We need to find out if this node had a physical body.
//             // This is why we usually track destruction via mask changes 
//             // or keep a local list of active physics nodes.
//         }
//     }
// };
// 
// void ecs_example() {
//     auto& manager = component_manager::instance();
//
//     // 1. Setup exclusion rules (Mask-based compatibility checks)
//     manager.register_mutually_exclusive<rigid_body, rigid_body_2d>();
//
//     // 2. Initialize dynamic reactive queries (Paged Sparse Cache under the hood)
//     entity_query<rigid_body> physical_bodies;
//     manager.register_query(&physical_bodies);
//
//     // 3. Runtime entity workflow (O(1) lookups via Paged Sparse Set)
//     tinynode entity { .runtime_id = 42 };
//     rigid_body* rb = entity.add_component<rigid_body>(); // Triggers query update
//
//     // 4. Thread-safe reverse reflection API
//     uint32_t type_idx = component_registry::type_index<rigid_body>();
//     const char* name = component_registry::type_name(type_idx); // returns "rigid_body"
//
//     // 5. Linear cache-friendly processing loop
//     for (auto [node, body] : physical_bodies) {
//         if (node) body->mass += 0.1f;
//     }
//
//     // 6. Post-frame pipeline processing
//     manager.post_frame_cleanup(); // Safe dense vector GC step
// }
// 


struct tinynode
{
    guid_t                  guid;
    interned_string         name;
    uint64_t                flags; // static, enabled
    uint64_t                runtime_id;
    uint64_t                component_mask;

    template <class T>    inline T * add_component();
    template <class T>    inline T * get_component();
};


class component_registry {
private:
    static inline std::atomic<uint32_t> g_component_counter { 0 };

    template<class CleanType>
    static inline uint32_t get_clean_type_index_() noexcept {
        static uint32_t index = g_component_counter.fetch_add(1, std::memory_order_relaxed);
        return index;
    }

public:
    template<class T>
    static inline uint32_t type_index() noexcept {
        //return get_clean_type_index_<std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>>(); //if remove_cv_t
        return get_clean_type_index_<std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>>();
    }

    template<typename T>
    static inline uint64_t component_bit() noexcept {
        static const uint64_t bit = 1ULL << type_index<T>();
        assert(bit != 0 && "Exceeded 64-component architecture limit!");
        return bit;
    }
};


class component_manager;

class iquery_listener {
public:
    virtual ~iquery_listener() = default;
    virtual void    on_node_changed(tinynode* node, component_manager& manager) = 0;
    virtual void    garbage_collect() = 0;
};

class icomponent_pool {
public:
    virtual ~icomponent_pool() = default;
    virtual void*   get_raw(uint32_t runtime_id) = 0;
    virtual void*   allocate_raw(uint32_t runtime_id) = 0;
    virtual void    add_extern_raw(uint32_t runtime_id, void* external_comp_ptr) = 0;
    virtual void    remove(uint32_t runtime_id) = 0;
};


template <typename V>
class paged_sparse_set {
public:
    static constexpr uint32_t PAGE_SIZE = 4096;
    static constexpr uint32_t INVALID_INDEX = static_cast<uint32_t>(-1);

    paged_sparse_set() = default;

    ~paged_sparse_set() {
        for (uint32_t* page : m_sparse_pages) {
            delete[] page;
        }
    }

    // Move-only or copyable depending on V, standard operations disabled for safety
    paged_sparse_set(const paged_sparse_set&) = delete;
    paged_sparse_set& operator=(const paged_sparse_set&) = delete;
    paged_sparse_set(paged_sparse_set&&) noexcept = default;
    paged_sparse_set& operator=(paged_sparse_set&&) noexcept = default;

    // O(1) Absolute lookup
    inline V* get(uint32_t runtime_id) noexcept {
        uint32_t dense_idx = get_sparse_cell(runtime_id, false);
        if (dense_idx == INVALID_INDEX) return nullptr;
        return &m_dense_table[dense_idx];
    }

    inline const V* get(uint32_t runtime_id) const noexcept {
        uint32_t dense_idx = get_sparse_cell(runtime_id, false);
        if (dense_idx == INVALID_INDEX) return nullptr;
        return &m_dense_table[dense_idx];
    }

    // O(1) Fast insertion or assignment tracking
    inline V& insert(uint32_t runtime_id, V&& value) {
        uint32_t& dense_idx_ref = get_sparse_cell(runtime_id, true);

        // If it already exists, overwrite the value tracking slot
        if (dense_idx_ref != INVALID_INDEX) {
            m_dense_table[dense_idx_ref] = std::move(value);
            return m_dense_table[dense_idx_ref];
        }

        uint32_t new_dense_idx = static_cast<uint32_t>(m_dense_table.size());
        m_dense_table.push_back(std::move(value));
        dense_idx_ref = new_dense_idx;

        return m_dense_table.back();
    }

    // O(1) Direct dense index extraction
    inline uint32_t get_dense_index(uint32_t runtime_id) const noexcept {
        return get_sparse_cell(runtime_id, false);
    }

    // O(1) Safe reference update for custom swap-and-pop implementations
    inline void set_dense_index(uint32_t runtime_id, uint32_t new_dense_idx) noexcept {
        uint32_t& dense_idx_ref = get_sparse_cell(runtime_id, false);
        if (dense_idx_ref != INVALID_INDEX) {
            dense_idx_ref = new_dense_idx;
        }
    }

    // O(1) Erasure marker reset
    inline void erase_sparse_link(uint32_t runtime_id) noexcept {
        uint32_t& dense_idx_ref = get_sparse_cell(runtime_id, false);
        if (dense_idx_ref != INVALID_INDEX) {
            dense_idx_ref = INVALID_INDEX;
        }
    }

    // Direct accessors for underlying dense iteration and low-level data overrides
    inline auto begin() noexcept { return m_dense_table.begin(); }
    inline auto end() noexcept { return m_dense_table.end(); }

    inline size_t size() const noexcept { return m_dense_table.size(); }
    inline V& operator[](uint32_t dense_idx) noexcept { return m_dense_table[dense_idx]; }
    inline const V& operator[](uint32_t dense_idx) const noexcept { return m_dense_table[dense_idx]; }

    inline void pop_back() noexcept { m_dense_table.pop_back(); }
    inline void resize_dense(size_t new_size) { m_dense_table.resize(new_size); }

private:
    inline uint32_t& get_sparse_cell(uint32_t runtime_id, bool allocate_on_miss) const noexcept {
        uint32_t page_idx = runtime_id / PAGE_SIZE;
        uint32_t cell_idx = runtime_id % PAGE_SIZE;

        if (page_idx >= m_sparse_pages.size()) {
            if (!allocate_on_miss) {
                static uint32_t dummy_invalid = INVALID_INDEX;
                return dummy_invalid;
            }
            const_cast<paged_sparse_set*>(this)->m_sparse_pages.resize(page_idx + 1, nullptr);
        }

        if (m_sparse_pages[page_idx] == nullptr) {
            if (!allocate_on_miss) {
                static uint32_t dummy_invalid = INVALID_INDEX;
                return dummy_invalid;
            }
            uint32_t* new_page = new uint32_t[PAGE_SIZE];
            std::fill_n(new_page, PAGE_SIZE, INVALID_INDEX);
            const_cast<paged_sparse_set*>(this)->m_sparse_pages[page_idx] = new_page;
        }
        return m_sparse_pages[page_idx][cell_idx];
    }

private:
    std::vector<V>          m_dense_table;
    std::vector<uint32_t*>  m_sparse_pages;
};


template <typename T>
class component_pool : public icomponent_pool {
private:
    struct dense_element {
        uint32_t runtime_id;
        T* component_ptr;
    };

    std::string                             m_type_name;
    aligned_allocator* m_mem_resources = nullptr;
    paged_pool_allocator* m_allocator = nullptr;

    // Using the utility container for storage and indexing mapping
    paged_sparse_set<dense_element>         m_set;

public:
    component_pool() {
        m_type_name = typeid(T).name();
        m_mem_resources = new aligned_allocator(m_type_name.c_str());
        m_allocator = new paged_pool_allocator(m_mem_resources, sizeof(T), 1024);
    }

    virtual ~component_pool() override {
        for (auto& element : m_set) {
            element.component_ptr->~T();
            m_allocator->deallocate(element.component_ptr);
        }
        delete m_allocator;
        delete m_mem_resources;
    }

    T* get(uint32_t runtime_id) {
        dense_element* element = m_set.get(runtime_id);
        return element ? element->component_ptr : nullptr;
    }

    virtual void* get_raw(uint32_t runtime_id) override { return get(runtime_id); }

    T* add(uint32_t runtime_id) {
        T* internal_storage = m_allocator->allocate<T>();
        new (internal_storage) T();
        m_set.insert(runtime_id, { runtime_id, internal_storage });
        return internal_storage;
    }

    virtual void* allocate_raw(uint32_t runtime_id) override { return add(runtime_id); }

    virtual void add_extern_raw(uint32_t runtime_id, void* external_comp_ptr) override {
        T* internal_storage = m_allocator->allocate<T>();
        new (internal_storage) T(std::move(*static_cast<T*>(external_comp_ptr)));
        m_set.insert(runtime_id, { runtime_id, internal_storage });
    }
    /*
    virtual void update_from_raw(uint32_t runtime_id, void* external_comp_ptr) override {
        T* existing = get(runtime_id);
        if (existing && external_comp_ptr) {
            *existing = std::move(*static_cast<T*>(external_comp_ptr));
        }
    }*/

    virtual void remove(uint32_t runtime_id) override {
        uint32_t dense_idx_to_remove = m_set.get_dense_index(runtime_id);
        if (dense_idx_to_remove == paged_sparse_set<dense_element>::INVALID_INDEX) return;

        uint32_t last_dense_idx = static_cast<uint32_t>(m_set.size() - 1);
        dense_element element_to_remove = m_set[dense_idx_to_remove];

        if (dense_idx_to_remove != last_dense_idx) {
            dense_element last_element = m_set[last_dense_idx];

            // Swap-and-Pop values inside the container wrapper
            m_set[dense_idx_to_remove] = last_element;
            m_set.set_dense_index(last_element.runtime_id, dense_idx_to_remove);
        }

        m_set.pop_back();
        m_set.erase_sparse_link(runtime_id);

        element_to_remove.component_ptr->~T();
        m_allocator->deallocate(element_to_remove.component_ptr);
    }
};


template <typename... Components>
class entity_query : public iquery_listener {
public:
    using tuple_type = std::tuple<tinynode*, Components*...>;

    template <typename... ExcludeComponents>
    entity_query& exclude() {
        m_exclude_mask |= (component_registry::component_bit<ExcludeComponents>() | ...);
        return *this;
    }

    bool matches(uint64_t node_mask) const {
        return (node_mask & m_all_of_mask) == m_all_of_mask && (node_mask & m_exclude_mask) == 0;
    }

    virtual void on_node_changed(tinynode* node, component_manager& manager) override {
        bool should_be_here = matches(node->component_mask);
        uint32_t dense_idx = m_set.get_dense_index(node->runtime_id);
        bool already_exists = (dense_idx != paged_sparse_set<tuple_type>::INVALID_INDEX);

        if (should_be_here && !already_exists) {
            m_set.insert(node->runtime_id, std::make_tuple(
                node, manager.get_pool<Components>()->get(node->runtime_id)...
            ));
        }
        else if (!should_be_here && already_exists) {
            std::get<0>(m_set[dense_idx]) = nullptr;
            m_set.erase_sparse_link(node->runtime_id);
            m_needs_gc = true;
        }
    }

    virtual void garbage_collect() override {
        if (!m_needs_gc) return;

        uint32_t write_idx = 0;
        size_t total_elements = m_set.size();

        for (uint32_t read_idx = 0; read_idx < total_elements; ++read_idx) {
            tinynode* node = std::get<0>(m_set[read_idx]);
            if (node != nullptr) {
                if (write_idx != read_idx) {
                    m_set[write_idx] = std::move(m_set[read_idx]);
                }
                m_set.set_dense_index(node->runtime_id, write_idx);
                write_idx++;
            }
        }
        m_set.resize_dense(write_idx);
        m_needs_gc = false;
    }

    inline auto begin() { return m_set.begin(); }
    inline auto end() { return m_set.end(); }

private:
    paged_sparse_set<tuple_type> m_set;
    uint64_t                     m_all_of_mask = (component_registry::component_bit<Components>() | ...);
    uint64_t                     m_exclude_mask = 0;
    bool                         m_needs_gc = false;
};


class component_manager {
public:
    static component_manager& instance() {
        static component_manager inst;
        return inst;
    }

    void pause_notifications() { m_notifications_paused = true; }

    void resume_notifications(const std::vector<tinynode*>& all_scene_nodes) {
        if (!m_notifications_paused) return;
        m_notifications_paused = false;
        for (auto* listener : m_listeners) {
            for (auto* node : all_scene_nodes) {
                listener->on_node_changed(node, *this);
            }
        }
    }

    void register_query(iquery_listener* listener) { m_listeners.push_back(listener); }

    void post_frame_cleanup() {
        for (auto* listener : m_listeners) { listener->garbage_collect(); }
    }

    template <typename T>
    component_pool<T>* get_pool() {
        uint32_t idx = component_registry::type_index<T>();
        if (idx >= m_pools.size()) m_pools.resize(idx + 1, nullptr);
        if (!m_pools[idx]) m_pools[idx] = new component_pool<T>();
        return static_cast<component_pool<T>*>(m_pools[idx]);
    }

    template <typename T>
    T* add_component(tinynode* node) {
        uint32_t idx = component_registry::type_index<T>();

        if (has_conflict(node->component_mask, idx)) {
            debug::log_error("Component configuration conflict detected! Cannot add component to node: %s", node->name.data());
            return nullptr; // Gracefully abort allocation
        }

        T* comp = get_pool<T>()->add(node->runtime_id);
        node->component_mask |= component_registry::component_bit<T>();
        if (!m_notifications_paused) {
            for (auto* listener : m_listeners) { listener->on_node_changed(node, *this); }
        }
        return comp;
    }

    template <typename T>
    T* get_component(tinynode* node) {
        return get_pool<T>()->get(node->runtime_id);
    }

    void add_component_by_type_index(tinynode* node, uint32_t type_idx, void* external_comp_ptr) {
        if (type_idx >= m_pools.size()) {
            m_pools.resize(type_idx + 1, nullptr);
        }

        icomponent_pool* pool = m_pools[type_idx];
        if (!pool) {
            debug::log_error("Type index out of bounds or pool not initialized! Did you forget to register the pool for type index: %d?", type_idx);
            return;
        }

        // Check conflicts before touching memory layouts
        if (has_conflict(node->component_mask, type_idx)) {
            debug::log_error("Serialization configuration conflict for type index %d on node: %s", type_idx, node->name.data());
            return;
        }

        // Component already exists, aborting reallocation (might be disabled via bitmask)
        if (pool->get_raw(node->runtime_id) != nullptr)
            return;

        if (external_comp_ptr == nullptr) {
            pool->allocate_raw(node->runtime_id);
        } else {
            // Correctly handles external object injection via polymorph proxy method
            pool->add_extern_raw(node->runtime_id, external_comp_ptr);
        }

        node->component_mask |= (1ULL << type_idx);

        if (!m_notifications_paused) {
            for (auto* listener : m_listeners) { 
                listener->on_node_changed(node, *this);
            }
        }
    }

    template <typename T>
    void remove_component(tinynode* node) {
        get_pool<T>()->remove(node->runtime_id);
        node->component_mask &= ~component_registry::component_bit<T>();
        if (!m_notifications_paused) {
            for (auto* listener : m_listeners) { listener->on_node_changed(node, *this); }
        }
    }

    template <typename T>
    void set_component_active(tinynode* node, bool active) {
        uint64_t bit = component_registry::component_bit<T>();

        assert(get_pool<T>()->get(node->runtime_id) != nullptr
            && "Cannot toggle mask: Component does not exist in the pool!");

        uint64_t old_mask = node->component_mask;

        if (active) {
            node->component_mask |= bit;
        } else {
            node->component_mask &= ~bit; 
        }

        if (old_mask != node->component_mask && !m_notifications_paused) {
            for (auto* listener : m_listeners) {
                listener->on_node_changed(node, *this);
            }
        }
    }

    // Configure a mutual exclusion rule between two component types
    template <typename T1, typename T2>
    void register_mutually_exclusive() {
        uint32_t idx1 = component_registry::type_index<T1>();
        uint32_t idx2 = component_registry::type_index<T2>();

        uint64_t bit1 = component_registry::component_bit<T1>();
        uint64_t bit2 = component_registry::component_bit<T2>();

        if (std::max(idx1, idx2) >= m_conflict_masks.size()) {
            m_conflict_masks.resize(std::max(idx1, idx2) + 1, 0ULL);
        }

        // T1 conflicts with T2, and T2 conflicts with T1
        m_conflict_masks[idx1] |= bit2;
        m_conflict_masks[idx2] |= bit1;
    }

    // Direct check against node's current component layout
    inline bool has_conflict(uint64_t current_mask, uint32_t type_idx) const noexcept {
        if (type_idx >= m_conflict_masks.size()) return false;
        return (current_mask & m_conflict_masks[type_idx]) != 0ULL;
    }

private:
    std::vector<uint64_t>           m_conflict_masks;    // Array where index = type_idx, and value = bitmask of conflicting components
    std::vector<icomponent_pool*>   m_pools;
    std::vector<iquery_listener*>   m_listeners;
    bool                            m_notifications_paused = false;
};


template <class T>    inline T* tinynode::add_component() {
    return component_manager::instance().add_component<T>(this);
}
template <class T>    inline T* tinynode::get_component() {
    return component_manager::instance().get_component<T>(this);
}
#endif