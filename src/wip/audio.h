
typedef object_t;
typedef event_t;
typedef soundbank_t;

class audiosystem
{
public:
    void register_object(int id);
    void unregister_object(int id);
    void update_transform(int id, const mat4 & transform);
    
    void upload_geometry(float * verts, uint16_t * indicines);
    
    void create_room();
    void create_portal();
    void create_reflector();
    void delete_room();
    void delete_portal();
    void delete_reflector();
    
    void post_event(int eventId, int objectId);
    void stop_all(int id);
private:
};