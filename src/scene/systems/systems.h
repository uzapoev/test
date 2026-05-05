#ifndef __systems_h__
#define __systems_h__

struct base_system;

// renderer, material, occluder, light, lodgroup, render_pass
class render_system : base_system
{
    renderer    create_renderer();
    occluder    create_occluder();
    light       create_light();
    lodgroup    create_lodgroup();
};

// collider, rigid_body,
class physic_system : base_system
{
    collider    create_collider();
    rigidbody   create_rigidbody();
};

// animation
class animation_system : base_system
{
};

// audio_object, room, portal etc
class audio_system : base_system 
{
};

class script_system : base_system
{
};

class streaming_system : base_system
{
};

// navagent, obstacle
class navigation_system : base_system
{
};

class networking_system : base_system
{
};


#endif