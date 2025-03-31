#ifndef __world_h__
#define __world_h__


#include "scene.h"
#include "../gui/gui.h"


class world
{
public:
    scene * load_scene(std::string_view path)
    {
       // NOT_IMPLEMENTED("scene * load_scene");
       /*if(!resources.exist(path))
            return nullptr;

        return scene::create_from_file(path.data());*/

        return nullptr;
    }

    void update(float dt)
    {
        m_gui->advance(dt);

        for(auto & s : m_scenes)
            s->update();
    }
   
    void draw()
    {
        for (size_t i = 0; i < m_scenes.size(); ++i)
        {
            m_scenes[i]->draw(m_active_camera);
        }

        for(uint32_t i = 0; i < m_gui->batches(); ++i)
        {
            auto batch = m_gui->batch(/*i*/);
        }
    }

private:
    
    camera                  m_active_camera;
    gui *                   m_gui;
    std::vector<scene*>     m_scenes;
};


#endif