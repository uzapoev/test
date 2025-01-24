#ifndef __FBX_NEW_LOADER__
#define __FBX_NEW_LOADER__

#ifdef _MSC_VER 
    #pragma warning(push)
    #pragma warning(disable: 26495)
#endif

#include <assert.h>
#include <string>
#include <vector>
#include <map>


#if __has_include(<fbxsdk.h>)
    #define FBXSDK_INSTALLED
#endif

#ifdef FBXSDK_INSTALLED
    #define FBXSDK_SHARED
    #include <fbxsdk.h>
#endif

#ifdef _MSC_VER 
#pragma warning(pop)
#endif

#include "../mathlib.h"

struct vertexPNBTWI
{
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec4 texcoord;
    vec4 weights;
    vec4 indices;
};


struct vertexPNBTWIidx
{
    vec3        position;
    vec3        normal;
    vec3        tangent;
    vec2        texcoord0;
    vec2        texcoord1;
    vec2        texcoord2;
    vec2        texcoord3;
    vec4        weights;
    vec4        indices;
    uint32_t    color = 0;

    unsigned int idx = 0; // control point idx
    mutable unsigned int _hash = 0;

    vertexPNBTWIidx() {}

    inline bool operator == (const vertexPNBTWIidx & v) throw()
    {
        bool ci = idx == v.idx;
        bool cp = position == v.position;
        bool ct0 = texcoord0 == v.texcoord0;
        bool ct1 = texcoord1 == v.texcoord1;
        bool res = ci && cp && ct0 && ct1;
        return res;
    }
    int hash() const
    {
        if (_hash != 0) return _hash;

        _hash = (_hash * 397) ^ (int)idx;
        _hash = (_hash * 397) ^ (int)floor(position.x * 10000.0f);
        _hash = (_hash * 397) ^ (int)floor(position.y * 10000.0f);
        _hash = (_hash * 397) ^ (int)floor(position.z * 10000.0f);

        _hash = (_hash * 397) ^ (int)floor(normal.x * 10000.0f);
        _hash = (_hash * 397) ^ (int)floor(normal.y * 10000.0f);
        _hash = (_hash * 397) ^ (int)floor(normal.z * 10000.0f);

        _hash = (_hash * 397) ^ (int)floor(texcoord0.x * 10000.0f);
        _hash = (_hash * 397) ^ (int)floor(texcoord0.y * 10000.0f);

        _hash = (_hash * 397) ^ (int)floor(texcoord1.x * 10000.0f);
        _hash = (_hash * 397) ^ (int)floor(texcoord1.y * 10000.0f);

        return _hash;
    }
};

class dumper
{
public:
    dumper(const char *file)
    {
        m_file = fopen(file, "w");
    }
    ~dumper()
    {
        fclose(m_file);
    }

    void newline()
    {
        fwrite("\n", 1, 1, m_file);
        fflush(m_file);
    }

    template<class T> void dump(const T & value);

    template<> void dump(const std::string & value)
    {
        fwrite(value.c_str(), value.size(), 1, m_file);
        fflush(m_file);
    }

    template<> void dump(const int & value)
    {
        char buff[512];
        sprintf(buff, "%4d", value);
        fwrite(buff, strlen(buff), 1, m_file);
        fflush(m_file);
    }

    template<> void dump(const unsigned int & value)
    {
        char buff[512];
        sprintf(buff, "%4d", value);
        fwrite(buff, strlen(buff), 1, m_file);
        fflush(m_file);
    }

    template<> void dump(const vec3& value)
    {
        char buff[512];
        sprintf(buff, "(%.3f %.3f %.3f)", value.x, value.y, value.z);
        fwrite(buff, strlen(buff), 1, m_file);
        fflush(m_file);
    }

    template<> void dump(const FbxDouble4 & value)
    {
        dump(math::make_vec4((float)value[0], (float)value[1], (float)value[2], (float)value[4]));
    }

    template<> void dump(const vec4& value)
    {
        char buff[512];
        sprintf(buff, "(%.3f %.3f %.3f %.3f)", value.x, value.y, value.z, value.w);
        fwrite(buff, strlen(buff), 1, m_file);
        fflush(m_file);
    }

    template<> void dump(const quat& value)
    {
        char buff[512];
        sprintf(buff, "(%.3f %.3f %.3f %.3f)", value.x, value.y, value.z, value.w);
        fwrite(buff, strlen(buff), 1, m_file);
        fflush(m_file);
    }
private:
    FILE * m_file;
};



class AssetFbx
{
public:
    AssetFbx();

    bool load(const char * path);
    void unload();

    void scan(FbxNode * node);
   // void draw(iEngine * engine);
private:
    static void load_static_mesh(const std::string & path, std::vector<vec3> * verts, std::vector<uint16_t> *indx);
    void save(const char * path);
//    const aabbox &  aabb()const  { return  m_aabb; }
public:

    struct joint
    {
        int id = -1;
        int parentid = -1;
        void *userdata = nullptr; 
        char name[64];

        vec3 bind_pos;
        quat bind_rot;

        vec3 animated_pos; // animated
        quat animated_rot; //

        void evaluate_global()
        {
            // matGloal = matAnim * matInv
            globalrot = quat::mul(animated_rot, bind_rot.inverted());
            globalpos = quat::mul(globalrot, -bind_pos) + animated_pos;
        }

        vec3 globalpos;
        quat globalrot;
    };

    struct s_mesh
    {
        std::string                     name;
        std::vector<joint>              skeleton;
        std::vector<vertexPNBTWIidx>    vertices;
        std::vector<uint32_t>           indexes;
        bool                            skinned;
    };

    struct s_node
    {
        struct {
            vec3                        position;
            vec3                        rotation;
            vec3                        scale;
        } global, local;
        s_mesh *                        mesh = nullptr;
    };

    struct s_animation
    {
        std::vector<joint>              joints;
        unsigned int                    trackCount;
        struct AnimationTrack*          tracks;
    };

private:
    s_mesh *                            loadmesh(FbxMesh * node);
    s_animation *                       loadanim();
    void                                buildhierarchy();
    void                                load_bind_poses();

private:
    FbxAnimLayer *                      get_animlayer(int id, int * frames_count);

private:
    void                                load_skin(FbxMesh * fbxMesh, s_mesh * mesh);
    void                                load_blendshape(FbxMesh * fbxMesh, s_mesh* mesh);

public:
    int                                 m_frames;
    float                               m_time;
    std::vector<joint>                  m_joints;
    std::vector<s_mesh*>                m_meshes_new;
    std::vector<s_node*>                m_nodes;

    std::vector<FbxMesh*>               m_fbx_node_meshes;
    std::vector<FbxNode*>               m_fbx_node_bones;

private:
    FbxManager *                        m_pFbxSdkManager;
    FbxScene *                          m_pScene;
    FbxArray<FbxString*>                m_AnimStackNameArray;
};

#endif
