#ifndef __Unity__
#define __Unity__

#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>

#include <filesystem>

#include "../mathlib.h"

#include "../json_serializer.h"


class UnityTransform 
{
public:
    vec3    position;
    vec3    scale;
    quat    rotation;

    mat4 local_mat()
    {
        return mat4::trs(position, rotation, scale);
    }

    JsonSerialize(UnityTransform,
        SerializeField(position),
        SerializeField(scale),
        SerializeField(rotation)
    );
};

class UnityMaterial
{
public:
    const std::string & mainTextureGuid() const;

public:
    struct TextureSlot {
        std::string             name;           // shader uniform name
        std::string             guid;           // texture guid
        vec4                    scaleOffset;    // scaleOffset

        JsonSerialize(TextureSlot,
            SerializeField(name),
            SerializeField(guid),
            SerializeField(scaleOffset)
        );
    };

    struct Vector4Slot {
        std::string             key;
        vec4                    value;
        JsonSerialize(Vector4Slot,
            SerializeField(key),
            SerializeField(value)
        );
    };

    struct ScalarSlot {
        std::string             key;    //"key": "_ZWrite",
        float                   value;  //"value" : 1.0

        JsonSerialize(ScalarSlot,
            SerializeField(key),
            SerializeField(value)
        );
    };

    std::string                 shader;

    std::vector<TextureSlot>    textures;
    std::vector<Vector4Slot>    vectors;
    std::vector<ScalarSlot>     scalars;

    JsonSerialize(UnityMaterial,
        SerializeField(textures),
        SerializeField(vectors),
        SerializeField(scalars)
    );
};

class UnityRenderer
{
public:
    std::string mesh;       // mesh guid
    std::string meshName;   // mesh name 
    std::string material;   // material guid
    std::string lightmap;   // lightmap guid
    vec4        lightmapScaleOffset;

    JsonSerialize(UnityRenderer,
        SerializeField(mesh),
        SerializeField(meshName),
        SerializeField(material),
        SerializeField(lightmap),
        SerializeField(lightmapScaleOffset)
    );
};

class UnityCollider
{
public:
    JsonSerialize(UnityCollider,
    )
};

class UnityGameObject
{
public:
    UnityGameObject() {}
    UnityGameObject(const UnityGameObject& obj) = default;
    UnityGameObject(UnityGameObject && obj) = default;
    ~UnityGameObject() = default;

    std::string                     name;
    std::string                     tag;
    UnityTransform                  transform;
    UnityRenderer                   renderer;
    UnityCollider                   collider;
    std::vector<UnityGameObject>    childs;

    JsonSerialize(UnityGameObject,
        SerializeField(name),
        SerializeField(transform),
        SerializeField(renderer),
        SerializeField(childs)
    );
};


class UnityScene
{
public:
    UnityScene() = default;
    UnityScene(UnityScene&& obj) = default;

    void compile();
    void clear();

    const std::vector<UnityGameObject> &nodes() const { return m_nodes;};
    const std::vector<UnityGameObject*> &drawable() const { return m_drawable_nodes;};

private:
    void traverse(UnityGameObject & gob);

public:
    std::unordered_set<interned_string>   m_meshes;
    std::unordered_set<interned_string>   m_textures;
    std::unordered_set<interned_string>   m_materials;

    std::vector<UnityGameObject*>   m_drawable_nodes;
    std::vector<UnityGameObject>    m_nodes;


    JsonSerialize(UnityScene,
        SerializeFieldWithKey("childs", m_nodes)
    );
};


class AssetUnity
{
public:
    void            set_data_path(const std::string & path);

    UnityScene      open_scene(const std::string& path);

    void            clear();

    interned_string guid_2_path(std::string_view guid)
    {
        auto it = m_textures.find(std::string(guid));
        if(it != m_textures.end())
            return it->second;

        it = m_materials.find(std::string(guid));
        if (it != m_materials.end())
            return it->second;

        it = m_meshes.find(std::string(guid));
        if (it != m_meshes.end())
            return it->second;
        return "";
    }

private:
    void scan(const std::string& path);

public:

    std::unordered_map<interned_string, interned_string>        m_assets; // key - guid, value - path
    std::unordered_map<interned_string, interned_string>        m_meshes; // key - guid, value - path
    std::unordered_map<interned_string, interned_string>        m_textures; // key - guid, value - path
    std::unordered_map<interned_string, interned_string>        m_materials; // key - guid, value - path
    std::unordered_multimap<interned_string, interned_string>   m_meshes_multimap; // key - guid, value - path
};

#endif
