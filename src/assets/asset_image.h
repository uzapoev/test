#ifndef __Asset_Image_h__
#define __Asset_Image_h__

#include <string>

class AssetImage
{
    
public:

    AssetImage();

    void load(const char * path);

    struct Image 
    {
        int         width;
        int         height;
        int         bpp;
        int         components;
        int         mip_count;
        uint8_t *   data;
    };

private:
    void encode_astc();
};

#endif
