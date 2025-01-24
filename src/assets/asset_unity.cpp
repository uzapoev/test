#include "asset_unity.h"
#include "../json.h"

static size_t filedata(const char* path, char** buff)
{
#ifdef _WIN32
    struct _stat32 st = { 0 };
    _stat32(path, &st);
#else
    struct stat st = { 0 };
    stat(path, &st);
#endif
    if (st.st_size == 0)
        return st.st_size;

    *buff = (char*)malloc(st.st_size + 1);
    memset(*buff, 0, st.st_size + 1);

    FILE* file = fopen(path, "rb");
    fread(*buff, 1, st.st_size, file);
    fclose(file);
    return st.st_size;
}


const std::string & UnityMaterial::mainTextureGuid() const
{
    for(auto it = textures.begin(); it != textures.end(); ++it)
    {
        if(it->name.find("_MainTex") != -1)
           return it->guid;
        if (it->name.find("Base") != -1)
            return it->guid;
        if (it->name.find("_Albedo") != -1)
            return it->guid;
    }

    static std::string empty;
    return empty;//textures.begin()->guid;
}


void UnityScene::compile()
{
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        traverse(*it);
    }
}

void UnityScene::clear()
{
    m_meshes.clear();

    m_drawable_nodes.clear();
    m_drawable_nodes.resize(0);

    m_nodes.clear();
    m_nodes.resize(0);
}


void UnityScene::traverse(UnityGameObject& gob)
{
    if(!gob.renderer.mesh.empty())
    {
        m_meshes.insert(gob.renderer.mesh);
        m_materials.insert(gob.renderer.material);
        m_drawable_nodes.emplace_back(&gob);
    }

    for( auto it = gob.childs.begin(); it != gob.childs.end(); ++it)
    {
        traverse(*it);
    }
}


void extrude_filename_guid_unsafe(const char* composed_name, char* name, char* guid)
{
    char buff[512] = "";
    strcpy(buff, composed_name);

    char* extension = strrchr(buff, '.');
    if (extension != nullptr)
    {
        memset(extension, 0, strlen(extension));
    }

    auto guid_ptr = strrchr(buff, '.');
    if (guid_ptr != nullptr)
    {
        strcpy(guid, guid_ptr + 1);
        memset(guid_ptr, 0, strlen(guid_ptr));
    }

    strcpy(name, buff);
}

void AssetUnity::set_data_path(const std::string& data_path)
{
    const std::filesystem::path mesh_extension(".mesh");
    const std::filesystem::path material_extension(".material");

    std::set<std::string> texture_extensions = {/*".ktx",*/ ".dds" , /*".pvr" , ".png", ".exr"*/ };

    if(std::filesystem::exists(data_path))
    {
       std::filesystem::recursive_directory_iterator  it(data_path);// = { dir };

       for(; it != std::filesystem::end(it); it++)
       {
            auto path = it->path();
            if(it->is_directory())
                continue;

            auto filename = path.filename();
            auto tmp = path.stem();// filename.replace_extension("");
            auto guid = tmp.extension();
            auto extension = path.extension().u8string();

            m_assets.emplace(path.filename().u8string(), path.u8string());

            std::string guidstr = guid.u8string();
            if(!guidstr.empty())
                guidstr.erase(guidstr.begin(), ++guidstr.begin());

            bool is_texture = texture_extensions.find(extension) != texture_extensions.end();
            if(is_texture)
                m_textures.emplace(guidstr, path.u8string());

            if (extension == mesh_extension)
            {
                m_meshes.emplace(guidstr, path.u8string());
                m_meshes_multimap.emplace(guidstr, path.u8string());
            }
            else if (extension == material_extension)
            {
                m_materials.emplace(guidstr, path.u8string());
            }
        }
    }
}


UnityScene AssetUnity::open_scene(const std::string& filename)
{
    measure ms("open scene");
    auto it = m_assets.find(filename);
    if(it == m_assets.end())
        return {};

    char *buffer = nullptr;
    filedata(it->second.c_str(), &buffer);

    std::string tmp(buffer);

    UnityScene scene;
    json::from_json(scene, tmp);
    free(buffer);

    scene.compile();
    return scene;
}



void AssetUnity::clear()
{
    m_assets.clear();
    m_meshes.clear();
    m_textures.clear();
    m_materials.clear();
    m_meshes_multimap.clear();
}

void AssetUnity::scan(const std::string& path)
{
}