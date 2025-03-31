#include "common.h"
#include <stdarg.h>


#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
    #define snprintf _snprintf

    #pragma warning( disable: 26819)
#endif

std::unordered_set <std::string>interned_string::s_interned;

int measure::intend = 0;
auto s_prev = std::chrono::high_resolution_clock::now();


float s_dt = 0.0f;
float s_elapsed_time = 0.0f;

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
    auto curr = std::chrono::high_resolution_clock::now();
    
    auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(curr - s_prev);
    auto diff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(curr - s_prev);
    s_prev = curr;

    s_dt = diff_ms.count()/100.0f;
    s_elapsed_time += s_dt;
}


std::string bin2hex::dump(const char* data, size_t size, const char * name)
{
    const uint8_t* u8data = (uint8_t*)data;
    const int hex_symbol_count = 16;
    const int hex_symbol_width = hex_symbol_count * 6; // 6 is len of "0x%02x, "; 
    const int text_symbol_pos = hex_symbol_width + 3; // comment with space

    std::string result;
    result.reserve(size);

    char buffer[text_symbol_pos + hex_symbol_count + 2] = "";
    char tmp[256] = "";
    sprintf(buffer, "static const unsigned char %s[%llu] =\n{\n", name, size);

    result.append(buffer);

    uint32_t hexPos = 0;
    uint32_t asciiPos = 0;
    for (uint32_t ii = 0; ii < size; ++ii)
    {
        snprintf(&buffer[hexPos], hex_symbol_width - hexPos, "0x%02x, ", u8data[asciiPos]);
        snprintf(&buffer[hex_symbol_width], 3, "// " );

        sprintf(tmp, "%*0x %02x,", asciiPos*6, u8data[asciiPos]);
        buffer[text_symbol_pos + asciiPos] = isprint(u8data[asciiPos]) && u8data[asciiPos] != '\\' ? u8data[asciiPos] : '.';
    
        asciiPos++;  
        hexPos += 6;
    
        if (hex_symbol_count == asciiPos)
        {
            result.append("    ").append(buffer).append("\n");
            u8data += asciiPos;
            hexPos = 0;
            asciiPos = 0;
    
            memset(buffer, 0, sizeof(buffer));
        }
    }
    
    if (0 != asciiPos)
    {
        result.append("    ").append(buffer).append("\n");
    }
    
    result += "};\n";

    return std::move(result);
}


uint32_t Hash::murmur32(const void* key, uint32_t size, uint32_t seed)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value
    uint32_t h = seed ^ size;

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


uint64_t Hash::murmur64(const void* key, uint32_t len, uint32_t seed)
{
    const int64_t m = 0xc6a4a7935bd1e995ull;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const int64_t * data = (const int64_t *)key;
    const int64_t * end = data + (len / 8);

    while (data != end)
    {
        uint64_t k = *data++;

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

size_t Hash::bernstein_ci(const void* data_in, uint32_t size, uint32_t seed)
{
    const unsigned char * data = (const unsigned char*)data_in;
    unsigned int    h = seed;
    while (size > 0) {
        size--;
        h = ((h << 5) + h) ^ (unsigned)tolower(data[size]);
    }
    return h;
}


size_t Utf8::wchar_to_utf8(const wchar_t* w, size_t size, uint8_t* s)
{
    uint32_t  c;
    short* p = (short*)w;
    uint8_t* q = (uint8_t*)s; uint8_t* q0 = q;
    while (1) {
        c = *p++;
        if (c == 0) break;
        if (c < 0x080) *q++ = c; else
            if (c < 0x800) *q++ = 0xC0 + (c >> 6), *q++ = 0x80 + (c & 63); else
                *q++ = 0xE0 + (c >> 12), *q++ = 0x80 + ((c >> 6) & 63), *q++ = 0x80 + (c & 63);
    }
    *q = 0;
    return q - q0;
}

size_t Utf8::utf8_to_wchar(const uint8_t* s, size_t size, wchar_t* w)
{
    uint32_t  cache = 0, wait = 0, c = 0;
    uint8_t* p = (uint8_t*)s;
    short* q = (short*)w; short* q0 = q;
    while (1) {
        c = *p++;
        if (c == 0) break;
        if (c < 0x80) cache = c, wait = 0; else
            if ((c >= 0xC0) && (c <= 0xE0)) cache = c & 31, wait = 1; else
                if ((c >= 0xE0)) cache = c & 15, wait = 2; else
                    if (wait) (cache <<= 6) += c & 63, wait--;
        if (wait == 0) *q++ = cache;
    }
    *q = 0;
    return q - q0;
}


//
// debug
//

#define arg_vprintf(msg)    va_list arglist;        \
                            va_start(arglist, msg); \
                            vprintf(msg, arglist);  \
                            va_end(arglist);

void debug::log(const char* msg, ...) 
{
    arg_vprintf(msg);
    printf("\033[0m\n");
}

void debug::log_error(const char* msg, ...)
{
    printf("\x1b[31m");
    arg_vprintf(msg);
    printf("\033[0m\n");
}

void debug::log_warning(const char* msg, ...)
{ 
    printf("\x1B[33m");
    arg_vprintf(msg);
    printf("\033[0m\n");
}

void debug::breakpoint()
{
#ifdef _WIN32 
    __debugbreak();
#else
    __builtin_trap();
#endif
}

void debug::callstack(uintptr_t* frames, uint32_t count)
{
#ifdef _WIN32
    static bool lazyinit = false;
    if (!lazyinit) {
        SymInitialize(GetCurrentProcess(), NULL, TRUE);
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        lazyinit = true;
    }

    int skipframes = 2;
    int frame_count = RtlCaptureStackBackTrace(skipframes, (DWORD)count, (PVOID*)frames, NULL);

    HANDLE hprocess = GetCurrentProcess();
    char tmpbuffer[sizeof(SYMBOL_INFO) + 64] = "";
    for (uint64_t i = 0; i < frame_count; ++i)
    {
        DWORD ldsp = 0;
        IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)tmpbuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 64;

        // SymGetLineFromAddr64(hprocess, adress, &ldsp, &line);
        SymFromAddr(hprocess, frames[i], 0, symbol);

        debug::log("%s", symbol->Name);
    //    printf("\n\t%s", symbol->Name);
    }
#endif
}



//
// stream_impl
//
struct stream_impl
{
    stream_impl(FILE* file, uint32_t buffer_size = 4 * 1024) : m_file(file)
    {
        m_write_buffer_size = buffer_size;
        m_write_buffer = new char[m_write_buffer_size]();

        m_read_buffer_size = buffer_size;
        m_read_buffer = new char[m_write_buffer_size]();

        refill_buffer();
    }

    ~stream_impl()
    {
        if (m_read_buffer) delete m_read_buffer;
        if (m_write_buffer) delete m_write_buffer;
    }

    size_t read(uint32_t size, char* data)
    {
        size_t bytes_read = 0;
        while (bytes_read < size) {
            if (m_read_pos == m_read_buffer_size) {
                refill_buffer();
                if (m_read_pos == m_read_buffer_size) {
                    break; // End of file
                }
            }
            data[bytes_read++] = m_read_buffer[m_read_pos++];
        }
        return bytes_read;
    }

    size_t write(uint32_t size, const char* data)
    {
        size_t bytes_written = 0;
        while (bytes_written < size) {
            if (m_write_pos == m_write_buffer_size) {
                write_buffer();
            }
            m_write_buffer[m_write_pos++] = data[bytes_written++];
        }
        return bytes_written;
    }

    size_t seek(size_t offset, int whence)
    {
        flush(); // Flush the write buffer before seeking

        int result = fseek(m_file, (long)offset, whence);

        m_file_pos = result;
        m_read_pos = m_read_buffer_size; // Invalidate the read buffer

        return result;
    }

    void flush()
    {
        write_buffer();
    }

private:
    void refill_buffer()
    {
        size_t result = fread(m_read_buffer, 1, m_read_buffer_size, m_file);
        m_read_pos = 0;
        m_file_pos += result;
    }

    void write_buffer()
    {
        size_t result = fwrite(m_write_buffer, 1, m_write_pos, m_file);
        fflush(m_file);
        m_write_pos = 0;
        m_file_pos += result;
    }

private:
    FILE* m_file = nullptr;
    size_t      m_file_pos = 0;

    char* m_read_buffer = nullptr;
    size_t      m_read_buffer_size = 0;
    size_t      m_read_pos = 0;

    char* m_write_buffer = nullptr;
    size_t      m_write_buffer_size = 0;
    size_t      m_write_pos = 0;
};



filestream* filestream::open(const char* path, const char* mode)
{
    FILE* file = fopen(path, mode);
    if (file == nullptr)
        return nullptr;

    stream_impl* impl = new stream_impl(file);
    filestream* fstream = new filestream(impl);

    return fstream;
}

filestream* filestream::open_rb(const char* path)
{
    return filestream::open(path, "rb");
}

filestream* filestream::open_wb(const char* path)
{
    return filestream::open(path, "wb+");
}


filestream::filestream(stream_impl* imp)
    :m_impl(imp)
{
}

filestream::~filestream()
{
    delete m_impl;
}



uint32_t  filestream::read(uint32_t size, void* out_data)
{
    return (uint32_t)m_impl->read(size, (char*)out_data);
}

uint32_t  filestream::write(uint32_t size, const void* in_data)
{
    return (uint32_t)m_impl->write(size, (char*)in_data);
}

void filestream::flush()
{
    m_impl->flush();
}

void filestream::seek(uint32_t offset, int whence)
{
    m_impl->seek(offset, whence);
}


static const char _guid_digits[] = "0123456789abcdef";

Guid Guid::generate_from_seed(size_t seed)
{
    Guid result;
    srand((unsigned int)seed);
    char* ptr = result.m_uuid;
    for (size_t i = 0; i < (sizeof(result.m_uuid) >> 1); i++)
    {
        *ptr++ = _guid_digits[(rand() % 255 >> 4) & 0xf];
        *ptr++ = _guid_digits[(rand() % 255 >> 0) & 0xf];
    }
    return result;
}

void Guid::generate(char *buff, size_t size)
{
    char tmp[38] = "";

    time_t seed = time(NULL);// ^ (clock() << 16);
    srand((unsigned int)seed);// ^ tv.tv_sec ^ tv.tv_usec);
    for (size_t i = 0; i < sizeof(tmp); i++)
    {
        tmp[i] = rand() % 255;
    }

    char * ptr = buff;
    for (size_t i = 0; i < (size >> 1); i++)
    {
        *ptr++ = _guid_digits[(tmp[i] >> 4) & 0xf];
        *ptr++ = _guid_digits[(tmp[i] >> 0) & 0xf];
    }
    *ptr = '\0';
}

bool Guid::validate(const char *buff)
{
    size_t len = buff ? strlen(buff) : 0;
    for (size_t i = 0; i < len; ++i) {
        if (!strchr(_guid_digits, buff[i]))
            return false;
    }
    return len ? true : false;
}


Guid::Guid(void)
{
    memset(m_uuid, 0, sizeof(m_uuid));
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
        sign >>= shiftSign;
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

    void encode101010_quat(uint32_t& out, float x, float y, float z, float w)
    {
    }

    void decode101010_quat(float& x, float& y, float& z, float& w, uint32_t in)
    {
    }

    void encode555_vec(uint16_t& out, float x, float y, float z)
    {
    }

    void decode555_vec(float& x, float& y, float& z, uint16_t in)
    {
    }
}

