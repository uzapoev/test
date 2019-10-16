#include "common.h"

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef __NINTENDO__
#include <nn/os.h>
#endif

#include <math.h>
#include <stdio.h>

//#include <string>

#ifdef _WIN32
#define snprintf _snprintf
#endif


/*
void bin2cstr(const char * filepath, const char * varname, const void * _data, size_t size)
{
    const uint8_t* data = (uint8_t*)_data;

    std::string d;

    char buffer[1024] = "";
    sprintf(buffer, "static const unsigned char %s[%u] =\n{\n", varname, size);
    d += buffer;

    if (NULL != data)
    {
#define BX_STRINGIZE(_x) BX_STRINGIZE_(_x)
#define BX_STRINGIZE_(_x) #_x

#define HEX_DUMP_WIDTH 16
#define HEX_DUMP_SPACE_WIDTH 96
#define HEX_DUMP_FORMAT "%-" BX_STRINGIZE(HEX_DUMP_SPACE_WIDTH) "." BX_STRINGIZE(HEX_DUMP_SPACE_WIDTH) "s"

        char hex[HEX_DUMP_SPACE_WIDTH + 1];
        char ascii[HEX_DUMP_WIDTH + 1];

        uint32_t hexPos = 0;
        uint32_t asciiPos = 0;
        for (uint32_t ii = 0; ii < size; ++ii)
        {
            memset(buffer, 0, sizeof(buffer));
            snprintf(&hex[hexPos], sizeof(hex)-hexPos, "0x%02x, ", data[asciiPos]);
            hexPos += 6;

            ascii[asciiPos] = isprint(data[asciiPos]) && data[asciiPos] != '\\' ? data[asciiPos] : '.';
            asciiPos++;

            if (HEX_DUMP_WIDTH == asciiPos)
            {
                ascii[asciiPos] = '\0';
                const char *format = "\t" HEX_DUMP_FORMAT"// %s\n";
                sprintf(buffer, format, hex, ascii);
                data += asciiPos;
                hexPos = 0;
                asciiPos = 0;

                d += buffer;
            }
        }

        if (0 != asciiPos)
        {
            ascii[asciiPos] = '\0';
            sprintf(buffer, "\t" HEX_DUMP_FORMAT "// %s\n", hex, ascii);
            d += buffer;
        }

        d += "};\n";
    }
    if (filepath != NULL && _data != NULL)
    {
        FILE * file = fopen(filepath, "w+");
        if (file != NULL)
        {
            fwrite(d.c_str(), d.size(), 1, file);
            fclose(file);
        }
    }
}
*/

float Time::s_dt = 0.0f;
float Time::s_elapsed_time = 0.0f;

float Time::dt()
{
    return s_dt;
}

float Time::elapsed()
{
    return s_elapsed_time;
}


void Time::tick()
{
    float _dt = 1.0f / 60.0f;
#ifdef _WIN32
    static const int MAX_SAMPLE_COUNT = 50;

    static float frameTimes[MAX_SAMPLE_COUNT];
    static float timeScale = 0.0f;
    static float actualElapsedTimeSec = 0.0f;
    static INT64 freq = 0;
    static INT64 lastTime = 0;
    static int sampleCount = 0;
    static bool initialized = false;

    INT64 time = 0;
    float elapsedTimeSec = 0.0f;

    if (!initialized)
    {
        initialized = true;
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&lastTime));
        timeScale = 1.0f / freq;
    }

    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time));
    elapsedTimeSec = (time - lastTime) * timeScale;
    lastTime = time;

    if (fabsf(elapsedTimeSec - actualElapsedTimeSec) < 1.0f)
    {
        memmove(&frameTimes[1], frameTimes, sizeof(frameTimes)-sizeof(frameTimes[0]));
        frameTimes[0] = elapsedTimeSec;

        if (sampleCount < MAX_SAMPLE_COUNT)
            ++sampleCount;
    }

    actualElapsedTimeSec = 0.0f;

    for (int i = 0; i < sampleCount; ++i)
        actualElapsedTimeSec += frameTimes[i];

    if (sampleCount > 0)
        actualElapsedTimeSec /= sampleCount;

    _dt = actualElapsedTimeSec /** 5*/;
#elif defined(__APPLE__)
    static double start_time = CFAbsoluteTimeGetCurrent();
    double dt = CFAbsoluteTimeGetCurrent() - start_time;
    start_time = CFAbsoluteTimeGetCurrent();
    _dt = dt;
#elif defined(__ANDROID__)
    static double lasttime = 0.0;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    double currtime = tv.tv_sec*1000. + tv.tv_usec / 1000.;
    double elapsed = (currtime - lasttime) / 1000.0;
    lasttime = currtime;

    _dt = elapsed;
#elif defined(__NINTENDO__)
    static double lasttime = 0.0;
    static nn::os::Tick prevTick = nn::os::GetSystemTick();

    auto startTime = nn::os::GetSystemTick();
    auto diff = startTime - prevTick;
    prevTick = startTime;
    _dt = (float)(diff.ToTimeSpan().GetMilliSeconds()) / 1000.0f;
#endif

    s_dt = _dt;
    s_elapsed_time += s_dt; 
}


size_t Hash::murmur32(const void* key, size_t size, unsigned int seed)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value
    unsigned int h = seed ^ size;

    // Mix 4 bytes at a time into the hash
    const unsigned char * data = (const unsigned char *)key;

    while (size >= 4)
    {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        size -= 4;
    }

    // Handle the last few bytes of the input array
    switch (size)
    {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
        h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few bytes are well-incorporated.
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}


int64_t Hash::murmur64(const void* key, size_t len, int64_t seed)
{
    const int64_t m = 0xc6a4a7935bd1e995ull;
    const int r = 47;

    int64_t h = seed ^ (len * m);

    const int64_t * data = (const int64_t *)key;
    const int64_t * end = data + (len / 8);

    while (data != end)
    {
        int64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch (len & 7)
    {
    case 7: h ^= int64_t(data2[6]) << 48;
    case 6: h ^= int64_t(data2[5]) << 40;
    case 5: h ^= int64_t(data2[4]) << 32;
    case 4: h ^= int64_t(data2[3]) << 24;
    case 3: h ^= int64_t(data2[2]) << 16;
    case 2: h ^= int64_t(data2[1]) << 8;
    case 1: h ^= int64_t(data2[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

size_t Hash::bernstein_ci(const void* data_in, size_t size, unsigned int seed)
{
    const unsigned char * data = (const unsigned char*)data_in;
    unsigned int    h = seed;
    while (size > 0) {
        size--;
        h = ((h << 5) + h) ^ (unsigned)tolower(data[size]);
    }
    return h;
}





void Guid::generate(char *buff, size_t size)
{
    char tmp[38] = "";

    // int seed =  time(NULL);// ^ (clock() << 16);
    // printf("\n%d", seed);
    // srand(seed);// ^ tv.tv_sec ^ tv.tv_usec);
    for (size_t i = 0; i < sizeof(tmp); i++)
    {
        tmp[i] = rand() % 255;
    }
    const char digits[] = "0123456789abcdef";

    char * ptr = buff;
    for (size_t i = 0; i < (size >> 1); i++)
    {
        *ptr++ = digits[(tmp[i] >> 4) & 0xf];
        *ptr++ = digits[(tmp[i] >> 0) & 0xf];
    }
    *ptr = '\0';
}

bool Guid::isguid(const char *buff)
{
    if (!buff) return false;
    const char keys[] = "0123456789abcdef";
    size_t len = strlen(buff);
    // pch = strpbrk(str, key);
    for (size_t i = 0; i < len; ++i){
        const char * ptr = strchr(keys, buff[i]);
        if (!ptr)
            return false;
    }
    return true;
}


Guid::Guid(void)
{
    generate(m_uuid, sizeof(m_uuid));
}

Guid::Guid(const char * uuid)
{
    if (!uuid)
        generate(m_uuid, sizeof(m_uuid));
    else
        memcpy(m_uuid, uuid, sizeof(m_uuid));
}

Guid::Guid(const Guid & uuid)
{
    memcpy(m_uuid, uuid.m_uuid, sizeof(m_uuid));
}

void Guid::set(const char * uuid)
{
    assert(uuid);
    assert(strlen(uuid) <= sizeof(m_uuid));
    memcpy(m_uuid, uuid, sizeof(m_uuid));
}



namespace quantinizer
{
#pragma pack (push, 1)
    typedef union float16
    {
        struct{
            unsigned short fraction : 11;
            unsigned short exponent : 4;
            unsigned short sign : 1;
        }_float;
    }_float16;
    typedef union float32
    {
        float _f;
        struct{
            unsigned int fraction : 23;
            unsigned int exponent : 8;
            unsigned int sign : 1;
        }_float;
    }_float32;
#pragma pack (pop)

    union Bits
    {
        float    f;
        float32  f32;
        int32_t  si;
        uint32_t ui;
    };

    static int const shift = 13;
    static int const shiftSign = 16;

    static int32_t const infN = 0x7F800000; // flt32 infinity    {1 1111111 00000000000000000000000}
    static int32_t const maxN = 0x477FE000; // max flt16 normal as a flt32
    static int32_t const minN = 0x38800000; // min flt16 normal as a flt32
    static int32_t const signN = 0x80000000; // flt32 sign bit

    static int32_t const infC = infN >> shift;
    static int32_t const nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
    static int32_t const maxC = maxN >> shift;
    static int32_t const minC = minN >> shift;
    static int32_t const signC = signN >> shiftSign; // flt16 sign bit

    static int32_t const mulN = 0x52000000; // (1 << 23) / minN
    static int32_t const mulC = 0x33800000; // minN / (1 << (23 - shift))

    static int32_t const subC = 0x003FF; // max flt32 subnormal down shifted
    static int32_t const norC = 0x00400; // min flt32 normal down shifted

    static int32_t const maxD = infC - maxC - 1;
    static int32_t const minD = minC - subC - 1;

    uint16_t encode16f(float value)
    {
        Bits v, s;
        v.f = value;
        uint32_t sign = v.si & signN;
        v.si ^= sign;
        sign >>= shiftSign; // logical shift
        s.si = mulN;
        s.si = (int32_t)(s.f * v.f); // correct subnormals
        v.si ^= (s.si ^ v.si) & -(minN > v.si);
        v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
        v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
        v.ui >>= shift; // logical shift
        v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
        v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
        return v.ui | sign;
    }

    float decode16f(uint16_t value)
    {
        Bits v;
        v.ui = value;
        int32_t sign = v.si & signC;
        v.si ^= sign;
        sign <<= shiftSign;
        v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
        v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
        Bits s;
        s.si = mulC;
        s.f *= v.si;
        int32_t mask = -(norC > v.si);
        v.si <<= shift;
        v.si ^= (s.si ^ v.si) & mask;
        v.si |= sign;
        return v.f;
    }
}

