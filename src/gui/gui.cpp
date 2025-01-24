#include "gui.h"


void gui_test(int width, int height)
{
    auto test_gui = new gui(width, height);
    //auto wgui = test_gui->load_from_json();//load_html(...); load_xml(...);
   // auto psnrl = test_gui->

    auto btn0 = test_gui->create<button>("label 0", []() { printf("callback"); });
  /*  auto btn1 = wgui->create<button>("label 0", []() { printf("callback"); });
 
    gui->advance(dt);

    gui->on_mouse_move(x, y);
    gui->on_mouse_button(x, y, btn, state);
    gui->on_keyboard(btn, state);


    var ctx = gui->context();
    var verts = ctx->vertexes();
    var indexes = ctx->indexes();

    cmd_bind_buffers(verts, indexes);

    for(int i = 0; i < ctx->bathches(); ++i)
    {
        var batch = ctx->batch(i);
        cmd_bind_descriptor_set(batch.descriptor);
        cmd_draw_indexed(batch.start_idx, batch.count);
    }
    
    auto layout = gui::layout::create_box_layout();
    layout->add(btn0);
    layout->add(btn1);

    auto window = gui::create<window>();
    window->set_layout(layout);*/

 //   auto layout = gui::layout::create_box_layout(/*horisontal*/);

    auto root = new widget();
    root->set_fixed_size({1024, 1024});
//    root->set_layout(layout);
    for (size_t i = 0; i < 10; i++)
    {
        auto tmp = test_gui->create<widget>(root);
    }

    auto t = root->child(5);
    root->remove(t);

    root->draw(nullptr);
}



gui::gui(int width, int height)
:m_width(width), m_height(height) 
{
}

widget* gui::load_from_json()
{
    return new widget();
}

widget* gui::load_from_xml()
{
    return nullptr; 
}

void gui::on_mouse_move(int x, int y, int id)
{
}

void gui::on_mouse_button(int x, int y, int btn, int state)
{
}

void gui::on_keyboard(int btn, int state)
{
}


void gui::resize(int w, int h)
{
}


const sprite* gui::get_sprite(const char* name)
{
    if(name == nullptr)
        return nullptr;

    auto it = m_sprites.find(name);
    if(it != m_sprites.end())
        return it->second;

    return nullptr;
}

const gui_batch_t* gui::batch()
{
    return nullptr;
}


bool point_in_rect(const vec2 &p, const vec4 & rect)
{
    float width  = rect.x + rect.z;
    float height = rect.y + rect.w;
    return (p.x > rect.x && p.y > rect.y &&
            p.x < width  && height);
}



widget::widget(widget* parent)
:m_state(gui::State::normal)
{
    if(parent != nullptr)
        parent->add(this);
}

widget::~widget()
{
    if(m_action)
        m_action->stop();

    if (m_parent)
        m_parent->remove(this);

    for (size_t i = 0; i < m_childs.size(); ++i)
        m_childs[i]->release();

    m_childs.clear();
}

widget* widget::add(widget* w)
{
    if(w == nullptr)
        return nullptr;
    
    auto parent = w->parent();
    
    if(parent)
        parent->remove(w);

    w->retain();
    w->set_parent(this);
    m_childs.push_back(w);
    return w;
}

void widget::remove(widget* w)
{
    if(w == nullptr)
        return;

    auto it = std::remove(m_childs.begin(), m_childs.end(), w);
    m_childs.erase(it, m_childs.end());

    w->set_parent(nullptr);
    w->release();
}

void widget::move(const vec2& p)
{
    m_rect.x = p.x;
    m_rect.y = p.y;
}

void widget::set_fixed_size(const vec2& size)
{
   // m_rect.set
} 

void widget::set_layout(uilayout* layout)
{
    m_layout = layout;
}



void widget::set_visible(bool value)
{
    m_visible = value;
    for (size_t i = 0; i < m_childs.size(); ++i)
        m_childs[i]->set_visible(m_visible);
}

bool widget::contain(const vec2& p)
{
    return point_in_rect(p, m_rect);
}

void widget::update()
{
}


void widget::draw(gui* ctx)
{
 //
 // update_transform()
 // {
 //     var vertexes = ctx->lock(handle);
 //     vertexes[0].position = 
 // }
 //  auto handle = ctx->allocate(this);
 // 
 // 
 //   if(!m_visible)
 //       return; 
}

bool widget::on_mouse_move(const vec2& p)
{
    if (!contain(p))
        return false;

    for(size_t i = 0; i < m_childs.size(); ++i)
        m_childs[i]->on_mouse_move(p);

    return true;
}


bool widget::on_mouse_button(const vec2& p, int button, int modifiers)
{
    if (!contain(p))
        return false;

    for (size_t i = 0; i < m_childs.size(); ++i)
        m_childs[i]->on_mouse_button(p, button, modifiers);

    return true;
}



bool widget::on_scroll(const vec2& p)
{
    if (!contain(p))
        return false;

    for (size_t i = 0; i < m_childs.size(); ++i)
        m_childs[i]->on_scroll(p);

    return true;
}


bool widget::on_keyboard(int key, bool down)
{
    for (size_t i = 0; i < m_childs.size(); ++i)
        m_childs[i]->on_keyboard(key, down);

    return true;
}