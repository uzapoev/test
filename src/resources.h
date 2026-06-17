#ifndef __resources_h__
#define __resources_h__

#include "gfx/gfx.h"
#include "mathlib.h"



typedef uint64_t asset_handle_t_;

struct asset_handle_t {
    union {
        uint64_t handle;
        struct {
            uint32_t slot_index;    // index
            uint16_t generation;    // generation 
            uint16_t flags;         // resource_type
        };
    };
};
const asset_handle_t INVALID_ASSET_HANDLE = { 0 };
inline bool operator==(asset_handle_t a, asset_handle_t b) {
    return a.handle == b.handle;
}




// all assets should start with this header
//
typedef struct asset_header_t {
    uint32_t magik;
    uint32_t type; // texture/mesh etc
} asset_header_t;



/*
struct material_instance_t {
    uint64_t                    uuid;
    char *                      keywords;   // keywords slpit by |, VERTEX|TEXTURE|PLANAR
    gfx_shader_t *              shader;
    gfx_render_states_desc_t    render_state;
};*/



#endif // __resources_h__
