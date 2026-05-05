#include "ecs.h"

int ecs::_type_index_counter = 0;
const char* ecs::_typenames[MAX_COMPONENT] = {""};
std::vector<ecs::IComponentStorage*> ecs::m_storages;

Signal::SignalHandle::SignalHandle(Signal& sinal, std::weak_ptr<bool> alive) 
    : m_signal(sinal), m_alive_w(alive), m_connected(true)
{
}

Signal::SignalHandle::SignalHandle(const SignalHandle&& copy) noexcept
    : m_signal(copy.m_signal), m_alive_w(copy.m_alive_w), m_connected(copy.m_connected) 
{
};

Signal::SignalHandle::~SignalHandle()
{
    if (m_connected && !m_alive_w.expired()) 
        m_signal.disconnect(*this);
    m_connected = false;
}

Signal::Signal()
    :m_alive(std::make_shared<bool>(true))
{
}

void Signal::invoke(entity_t entity, int type_index, eEcsEvent type)
{
}

Signal::SignalHandle Signal::connect(std::function<entity_t>&& fn)
{
    return SignalHandle(*this, m_alive);
}

void Signal::disconnect(SignalHandle& handle)
{
}



ecs::ecs()
{
    m_entities.reserve(1024);
    m_component_bitset.reserve(1024);
}

ecs::~ecs()
{
    for each (auto var in m_storages)
    {
        var->clear();
    }
}
    
entity_t ecs::create_entity()
{
    entity_t entity = { m_entities.size(), 0 };

    m_entities.push_back(entity);
    m_component_bitset.push_back(bitset512());

    return entity;
}


void ecs::destroy_entity(entity_t entity)
{
}
