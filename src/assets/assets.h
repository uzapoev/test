#ifndef __assets_h__
#define __assets_h__

#include <string>
#include <vector>

#include "asset_image.h"
#include "asset_shader.h"
#include "asset_usd.h"
#include "asset_fbx.h"


/*
* typedef struct asset_header {
*   uint32_t magick    ;    //
*   uint32_t type;      //
*   uint32_t checksum;  //
*   uint32_t reserved;  //
* } asset_header;
*/

class AssetBundle
{
    //  
    // std::vector<guid>    m_guids;
    // std::vector<string>  m_names;
};

class Assets
{
protected:
    const char * mesh_folder = "mesh";
    const char * shader_folder = "shader";
public:
    bool check_in_cash(const char * filepath)
    {
        return false;
    }

    // dst_path - path
    // version - MAKE_FOUR(1,1,1,b);
    // compression - zlib/lzma/fastlz etc
    static void create_asset_pack(std::vector<std::string> files, const char * dst_path, uint32_t version, int compression)
    {  /* 
        * auto pack = fopen(dst_path, "rb");
        * 
        * fwrite(pack, header);
        * 
        * foreach(file in files)
        * {
        *   char * data = nullptr;
        *   size_t size = file_data(file.c_str(), &data);
        *   compress(data, size, &new_data, &new_size);
        *   fwrite(pack, new_data, new_size);
        *   
        * }
        */
    }

    static void cash_shader(const char * filepath, bool embed)
    {
        // auto src_crc = checksum(filepath);
        // auto raw_path = get_dst_file_name(filepath, raw_prefix());
        // auto raw_crc = file_read_checksum(raw_path);
        // 
        // if(raw_crc == src_crc)
        //     return;
        // 
        // for( paltform in platforms )
        // {
        //      auto dst_filename = get_dst_file_name(filepath, paltform.prefix());
        //      auto dst_crc = file_read_crc(dst_filename);
        // } 
    }


    static void cash_mesh()
    {
    }
};

#endif
