 #ifndef __gui_h__
#define __gui_h__

#include <atomic>
#include <vector>
#include <string>
#include <functional>

#include "../mathlib.h"
#include "../common.h"

/*
* auto gui = gui::create(1024, 768);
* auto panel = gui->add<panel>();
* auto layout = gui->create_layout_box();

* auto button0 = gui->create<button>("label", [](button*) { printf("");} );
* auto button1 = gui->create<button>("label", [](button*) { printf("");} );
* auto button2 = gui->create<button>("label", [](button*) { printf("");} );
* auto button3 = gui->create<button>("label", [](button*) { printf("");} );
* 
* layout->add(button0);
* layout->add(button1);
* layout->add(button2);
* layout->add(button3);
* 
* panel->set_layout(layout);
* 
*/


// class font
// class sprite
// class layout
// class action 
// class canvas;
 
// class widget: sprite
// class panel:  widget
// class button: widget // toggle, 
// class lable:  widget
// class image:  widget
// class scroll: widget
// class slider: widget
// class progressbar : widget
// class combobox: widget
// class tabbar: widget

extern void gui_test(int w, int h);


class widget;
class canvas;


class font
{
    void                    init_ttf(const char * data, size_t size, int fontsize);
    void                    print(int x, int y, const char * str);
};

class sprite
{
    atomic_string           m_name;
    vec4                    m_uvrect;
    class texture *         m_texture;
};

struct gui_batch_t
{
    uint32_t                texture_id;
    uint32_t                material_id;
    uint16_t*               index;
};


class gui
{
public:
    enum Orientation        { vertical, horisontal };
    enum Aligment           { left, center, right, };
    enum State              { normal, hover, pressed };

    gui(int width, int height);

    static widget *         load_from_json();
    static widget *         load_from_xml();

    template<class T, typename ...Args>
    widget*                 create(Args... args) { return  new T(args...); }

    void                    on_mouse_move(int x, int y, int id);
    void                    on_mouse_button(int x, int y, int btn, int state);
    void                    on_keyboard(int btn, int state);

    void                    resize(int w, int h);

    const sprite *          get_sprite(const char * name);
    const gui_batch_t *     batch();

private:
    struct uivertex
    {
        uint64_t            position;    // 16 bit per component( x, y, z ,w )
        uint64_t            uv;          // 16 bit per component( uv0 + uv1 )
        uint32_t            color;       // 
    };

private:
    typedef std::unordered_map<std::string, sprite*>  sprtite_map;

    int                     m_width     = 0;
    int                     m_height    = 0; 
    widget *                m_root      = nullptr;
    canvas *                m_canvas    = nullptr;
    sprtite_map             m_sprites;
    std::vector<uint16_t>   m_indexes;
    std::vector<uivertex>   m_vertexes;
};


class uilayout
{
public:
    static uilayout*        create_box_layout();    // horisontal/vertical
    static uilayout*        create_grid_layout();   // 
};


class action
{
public:
    virtual void            start() {};
    virtual void            stop()  {};
private:
    std::function<void(float dt)>   m_function;
    float                   m_time;
    widget*                 m_widget = nullptr;
};


struct rc_ptr
{
    virtual                 ~rc_ptr() {};
    int                     retain()   { return m_counter.fetch_add(1); }
    int                     release()  { return m_counter.fetch_add(-1);}
    std::atomic<int>        m_counter; 
};


class widget : rc_ptr
{
public:
                            widget(widget * parent = nullptr);
    virtual                ~widget();

public:
    vec2                    postition()             { return {m_rect.x, m_rect.y}; }
    void                    move(const vec2& p);
    bool                    contain(const vec2& p);

    void                    set_fixed_size(const vec2 & size);
    void                    set_layout(uilayout* l);
    void                    set_visible(bool value);

    vec2                    fixed_size()            { return {m_rect.z, m_rect.w}; };
    uilayout*               layout()                { return m_layout; }
    bool                    visible()               { return m_visible; }

public:

    widget*                 add(widget*);
    void                    remove(widget*); 

    widget *                parent()                { return m_parent; }
    void                    set_parent(widget * w)  { m_parent = w; }

    widget*                 child(size_t i)         { return m_childs[i]; }
    size_t                  child_count()           { return m_childs.size(); };
    
    virtual void            update();
    virtual void            draw(gui * );

public:
    virtual bool            on_mouse_move(const vec2 &p);
    virtual bool            on_mouse_button(const vec2 &p, int button, int modifiers);
    virtual bool            on_keyboard(int key, bool down);
    virtual bool            on_scroll(const vec2 &p);

private:
    vec4                    m_rect;
    vec2                    m_anchor;
    gui::State              m_state;
    bool                    m_visible = true;       // mask{ visible, focused, }
    bool                    m_focused = false;      
    action *                m_action = nullptr;
    widget *                m_parent = nullptr;
    uilayout*               m_layout = nullptr;
    std::vector<widget*>    m_childs;


    enum gui_state_e
    {
     //   visible,
     //   focused,
     //   active,
    };

    /*
    * float,int, string, vector,  
    SerializebleObject(widget,
        SerializebleFieldWithKey("rect", m_rect),
        SerializebleFieldWithKey("anchor", m_anchor),
    );
    */

public:
    friend gui;
};

class panel : public widget
{
    void                    maximize();
};

class lable : public widget
{
public:
    void set_lable(const std::string_view & lable);
};


class button : public widget
{
public:
// enum style { default, checkbox, toggle }

    button( const char * text, std::function<void()> callback){
        m_callback = callback;
    }

public:
    void set_caption();
    void set_callback();
    void set_icon();
    
    void set_group();

private:
    lable*                      m_lable = nullptr;
    std::vector<button*>        m_group;
    std::function<void()>       m_callback;
};
/*

class slider : widget
{
public:
private:
    button *                    m_carrier;
    vec2                        m_minmax;
    float                       m_value;
    Orientation                 m_orienatation;
};


class progressbar : widget
{
}

class scroll : widget
{
public:
    class scroll_item : widget { };
    
    class scroll_handler
    {
        virtual void request_next(scroll_item * itemptr, int id);
    };
    
private:
    std::vector<item*>          m_items;
    slider *                    m_scroll_bar;
};

class tabwidget
{
    void add_tab(button*, widget*);

    widget * add_tab(button*);
    
    std::unordered_map<button*, widget*>
}
*/

#endif