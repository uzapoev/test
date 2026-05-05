#ifndef __physic3d_system_h__
#define __physic3d_system_h__

class itrigger
{
    virtual void on_while_out() = 0;
    virtual void on_while_in() = 0;
    virtual void on_trigger_enter() = 0;
    virtual void on_trigger_exit() = 0;
};




#endif // __resources_h__