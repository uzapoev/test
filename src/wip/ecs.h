#ifndef __Ecs_h__
#define __Ecs_h__
/*
    feature movement;
    movement.register_system<jump>()
            .register_system<move>();

    feature animation;
    animation.register_system<environment_scanner>();


    struct Transform : Component { Transform transform;              };
    struct Renderable: Component { Mesh * mesh; Material * material; };
    struct Character : Component { AnimationController* controller;  };

    ecs _ecs;

    for(int i = 0; i < 100000;++i)
    {
        auto entity = _ecs.create_entity();

        if (i % 3 != 0) auto ptr0 = _ecs.add_component<Transform>(entity, pos, rot, scale);
        if (i % 3 != 1) auto ptr1 = _ecs.add_component(entity, Renderable{});
        if (i % 3 != 2) auto ptr2 = _ecs.add_component(entity, Character {});
    }

    auto query = _ecs.create_query();

    query.all_of<Transform, Renderable>()
         //.exclude<Character>()
         .for_each<Transform, Renderable>([=](entity_t e, Transform* transform, Renderable* renderable) {
             auto material = renderable->material;
             material->set_uniform(transform);
          });

    auto render_query = _ecs.create_query().allof<Renderable>();

    render_query.for_each<Renderable>([=](entity_t e, Renderable* renderable, transform) {
            auto material = renderable->material;
            for(int i = 0; i < renderable->submesh_count(); ++i)
            {
                auto submesh = renderable->submesh(i);

                gfx_cmd_bind_descriptor_set(cmd, material->desctiptor_set);
                gfx_cmd_bind_vb(cmd, submesh->vb);
                if(submesh->ib) 
                {
                    gfx_cmd_bind_ib(cmd, submesh->ib);
                    gfx_cmd_draw_indexed(cmd, submesh->ib->count, 0);
                }
                else 
                 {
                    gfx_cmd_draw(cmd, submesh->vb->count);
                }
            }
        });

    query.execute();
    transform_query.execute();
    render_query.execute();
*/

#include <assert.h>

#include <bitset>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>


#define MAX_COMPONENT (512)

using bitset512 = std::bitset<MAX_COMPONENT>;

typedef struct entity_t
{
    union 
    {
        struct {
            uint32_t id;
            uint32_t generation;
        };
        uint64_t     handle = 0;
    };
} entity_t;
//using entity_t  = uint64_t;

enum class eEcsEvent : uint8_t
{
    Added,      // component added
    Removed,    // component removed
    Destroed    // entity destroyed
};


struct Component            {};
struct SingletonComponent   {};


class Signal
{
public:
    class SignalHandle
    {
        friend class Signal;
        SignalHandle(Signal& sinal, std::weak_ptr<bool> alive);
    public:
        SignalHandle(const SignalHandle& copy) = delete;
        SignalHandle(const SignalHandle&& copy) noexcept;
        ~SignalHandle();
    private: 
        bool                m_connected;
        Signal &            m_signal;
        std::weak_ptr<bool> m_alive_w;
    };

public:
                    Signal();

    void            invoke(entity_t, int, eEcsEvent);
    SignalHandle    connect(std::function<entity_t> &&fn);
    void            disconnect(SignalHandle & handle);

private:
    std::shared_ptr<bool> m_alive;
};
/**/


class filter
{
    friend class ecs;
public:
    filter(class ecs & e):m_ecs(e)                      {};
    filter(filter &&)       noexcept                    = default;//{ printf("\nmove");    }
    filter(const filter &)  noexcept                    = delete;//{ printf("\ncopy");    }
    ~filter()               noexcept                    { printf("\ndelete");  }

public:
    template <class ...T> inline filter &   all_of()    { m_ecs.make_bitset<0, T...>(m_allof_mask);  return *this; }
 //   template <class ...T> inline filter &   any_of()    { m_ecs.make_bitset<0, T...>(m_allof_mask);  return *this; }
    template <class ...T> inline filter &   exclude()   { m_ecs.make_bitset<0, T...>(m_exclude_mask); return *this; }

    inline void                             execute()   { m_execute_func(); }
    inline size_t                           size()      { return m_entities.size(); }
    inline entity_t                         entity(size_t idx) { return m_entities_raw_ptr[idx]; }

    template <typename T> 
    struct Identity { typedef T type; };

    template <typename ...T>
    filter & for_each(typename Identity<std::function<void(entity_t, T*...)>>::type func)
    {
        bitset512 bitset;
        ecs::make_bitset<0, T...>(bitset);

        bitset512 valid_mask = m_allof_mask & bitset;
        bitset512 invalid_mask = m_exclude_mask & bitset;

        for (size_t i = 0; i < m_ecs.m_component_bitset.size(); i++)
        {
            auto all = (m_allof_mask & m_ecs.m_component_bitset[i]) == m_allof_mask;
            auto exclude = (m_exclude_mask & m_ecs.m_component_bitset[i]) == m_exclude_mask;

            if (all && !exclude)
            {
                entity_t entity = { i, 0 };
                m_entities.push_back(entity);
            }
        }
        m_entities_raw_ptr = m_entities.data();

        auto conflicts = (m_allof_mask & m_exclude_mask).any();
        if (conflicts) {
            printf("conflicted \"exlude\" and \"all_of\" arguments");
        }

        m_execute_func = [=] () {
            size_t count = m_entities.size();
            std::for_each(std::execution::par, m_entities.begin(), m_entities.end() [&](entity_t e){
                auto entity = m_entities[i];
                func(entity, m_ecs.get_component<T>(entity)...);
            });
            for (int i = 0; i < count; ++i)
            {
                auto entity = m_entities[i];
                func(entity, m_ecs.get_component<T>(entity)...);
            } /**/
        }; /**/
        return *this;
    }

    inline void for_all()
    {
     //   if(m_entities.empty())
    //        build();
    }

private:
  /*  void build()
    {
        for (size_t i = 0; i < m_ecs.m_component_bitset.size(); i++)
        {
            auto all = (m_allof_mask & m_ecs.m_component_bitset[i]) == m_allof_mask;
            auto exclude = (m_exclude_mask & m_ecs.m_component_bitset[i]) == m_exclude_mask;

            if (all && !exclude)
            {
                entity_t entity = { i, 0 };
                m_entities.push_back(entity);
            }
        }
        m_entities_raw_ptr = m_entities.data();
    }*/

private:
     std::vector<entity_t>      m_entities;
     bitset512                  m_allof_mask;
     bitset512                  m_exclude_mask;
     ecs &                      m_ecs;
     std::function<void()>      m_execute_func      = []{};
     entity_t*                  m_entities_raw_ptr  = nullptr;
};


class ecs
{
    friend class filter;
public:
                                    ecs();
    virtual                        ~ecs();
    entity_t                        create_entity();
    void                            destroy_entity(entity_t entity);
    bitset512                       entity_component_mask(entity_t entity)      { return m_component_bitset[entity.id]; }

    filter                          create_query()                              { return filter(*this); }
    
    template<class T> inline T *    add_component(entity_t entity, T& data);
    template<class T> inline T *    get_component(entity_t entity) noexcept     { return get_storage <T>()->get(entity); }
    template<class T> inline void   remove_component(entity_t entity)           { get_storage<T>()->remove(entity); }

private:
    template<std::size_t Idx = 0, typename... Tp> inline static
    typename std::enable_if < Idx == sizeof...(Tp), void>::type
        make_bitset(bitset512& bs)
    {
    }

    template<std::size_t Idx = 0, typename... Tp> inline static 
    typename std::enable_if < Idx < sizeof...(Tp), void>::type
        make_bitset(bitset512& bs)
    {
        using Type = std::tuple_element_t<Idx, std::tuple<Tp...>>;
        static_assert(std::is_base_of<Component, Type>(), "Type(%s) must be derrived from Component");
        make_bitset<Idx + 1, Tp...>(bs);
        int idx = ecs::type_index<std::tuple_element_t<Idx, std::tuple<Tp...>>>();
        bs.set(idx, true);
    }

    template<class T> static inline int type_index() noexcept
    {
        static int index = ++_type_index_counter;
        return index;
    }

    template<class T> static inline const char* type_name() noexcept
    {
        static const char* name = typeid(T).name();
        _typenames[type_index<T>()] = name;
        return name;
    }

public:
    struct IComponentStorage {virtual void clear() = 0;};

    template<class T>
    struct ComponentStorage final : public IComponentStorage
    {
        ComponentStorage(ecs & _ecas)           { 
            _ecas.register_storage(this);
        }
        void remove(entity_t e)                 { };

        inline T * get(entity_t e) noexcept     { 
            m_tptr = m_pool.data();
            m_iptr = m_id2idx.data();
            return &m_tptr[m_iptr[e.id]];
        }

        inline T * add(entity_t e, const T& data) {
            grow_size(e.id + 1 );

            m_pool.push_back(data);
            m_id2idx[e.id] = m_pool.size() - 1;
            return &m_pool.back();
        }

        inline void grow_size(size_t size) {
            if (m_id2idx.size() <= size)
                m_id2idx.resize(size + 1);

        }

        virtual void clear() {
           m_pool.clear();
           m_id2idx.clear();
        }

     private:
        std::vector<T>                          m_pool;
        std::vector<size_t>                     m_id2idx;
    }; 

    template<class T> __forceinline ComponentStorage<T>* get_storage() noexcept
    {
        static auto storage = ecs::ComponentStorage<T>(*this);
        return &storage;
      //  static auto *storage = new ecs::ComponentStorage<T>(*this);
      //  return storage;
    }
 
 private:
    inline void register_storage(IComponentStorage * storage)    { m_storages.push_back(storage);    }

private:
    static int                                  _type_index_counter;
    static const char *                         _typenames[MAX_COMPONENT];

    Signal                                      m_on_entity_changed;
    std::vector<entity_t>                       m_entities;
    std::vector<bitset512>                      m_component_bitset;
    static std::vector<IComponentStorage*>      m_storages;
}; 


template<class Type> 
inline Type * ecs::add_component(entity_t entity, Type & data = Type {})
{
    if(std::is_pointer<Type>::value)
    {
        printf("");
    };
    //static_assert(std::is_pointer<Type>::value, "Type(%s) must be not pointer");
    static_assert(std::is_base_of<Component, Type>(), "Type(%s) must be derrived from Component");

    auto typeidx = ecs::type_index<Type>();
    auto & mask = m_component_bitset[entity.id];
    if(mask.test(idx))
    {
        printf("entity already has component type(%s)", ecs::type_name<Type>());
        return nullptr;
    }
    mask.set(typeidx);

    m_on_entity_changed.invoke(entity, typeidx, eEcsEvent::Added);

    return get_storage<Type>()->add(entity, data);
}


struct ecs_system
{
    virtual void execute() = 0;
    virtual void dispose() = 0;
};


class feature
{
 public:
    
    template<class T>
    void register_system()
    {
        m_systems.push_back(std::make_shared<T>());
    }

    void execute()
    {
        for (auto &it : m_systems)
        {
            it->execute();
        }
    }
private:

    std::vector<std::shared_ptr<ecs_system>> m_systems;
};
#endif 