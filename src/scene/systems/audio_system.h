#ifndef __audio_system_h__
#define __audio_system_h__

struct audio_object: icomponent
{
    void set_rtcp(int id, int value);
};

struct audio_listener: icomponent{};
struct audio_room: icomponent{};
struct audio_portal: icomponent{};
struct audio_ambient: icomponent{};
struct audio_bank;

class audio_system
{
    audio_listener * create_listener();
    audio_room *    create_room();
    audio_portal *  create_portal();
    audio_ambient * create_ambient();
    audio_bank *    load_bank();
};


#endif // __resources_h__