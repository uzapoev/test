#ifndef __asset_usd_h__
#define __asset_usd_h__

#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>

#include "../mathlib.h"

class pxrUsdPrim;

class AssetUsd
{
public:
    void                        load(const char * path);
    void                        set_export_path(const char*path);

private:
    void                        parse_prim_info(void* prim, bool load = false);
    void                        load_mesh(void* prim);
    void                        load_transform(void* prim);

    void                        scan_dir(const char* path);

    std::string                 resolve_asset_path(const std::string & path);

private:
    std::string                 m_export_path;
    std::string                 m_export_path_mesh;

    std::set<std::string>       m_reference_assets;
    std::set<std::string>       m_payload_assets;

    std::set<std::string>       m_meshes;
    std::vector<std::filesystem::path> m_pathes;
};

#endif
