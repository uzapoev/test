#include "asset_image.h"

#include <vector>
/*
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"
#include "../external/stb/stb_image_resize.h"

AssetImage::AssetImage()
{
}


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

void AssetImage::load(const char* path)
{
    int x = 0;
    int y = 0;
    int channels;

    char * buffer = nullptr;
    int len = filedata(path, &buffer);

    auto pixels = stbi_load_from_memory((stbi_uc*)buffer, len , &x, &y, &channels, 4);
    stbi_image_free(pixels);
}*/