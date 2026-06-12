#include "common.h"
#include <stdarg.h>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
    #define snprintf _snprintf

    #include <direct.h>
    #define sys_mkdir(path) _mkdir(path)

    #pragma warning( disable: 26819)
#else
    #include <unistd.h>
    /* S_IRWXU: read, write, execute permissions for owner on POSIX systems */
    #define sys_mkdir(path) mkdir(path, S_IRWXU)
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


uint32_t hasher::murmur32(const void* key, uint32_t size, uint32_t seed)
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


uint64_t hasher::murmur64(const void* key, uint32_t len, uint32_t seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995ull;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t * data = (const uint64_t *)key;
    const uint64_t * end = data + (len / 8);

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

size_t hasher::bernstein_ci(const void* data_in, uint32_t size, uint32_t seed)
{
    const unsigned char * data = (const unsigned char*)data_in;
    unsigned int h = seed;
    while (size > 0) {
        size--;
        h = ((h << 5) + h) ^ (unsigned)tolower(data[size]);
    }
    return h;
}

static inline unsigned long long rotl64(unsigned long long x, int r) {
    return (x << r) | (x >> (64 - r));
}

uint64_t hasher::xxhash64(const void* input, size_t length, uint64_t seed)
{
    const uint64_t PRIME64_1 = 11400714785074694791ULL;
    const uint64_t PRIME64_2 = 14029467366897019727ULL;
    const uint64_t PRIME64_3 = 8545103131759506653ULL;
    const uint64_t PRIME64_4 = 5973393777823847427ULL;
    const uint64_t PRIME64_5 = 2870177450012600261ULL;

    const unsigned char* p = (const unsigned char*)input;
    const unsigned char* const end = p + length;
    uint64_t h64;

    if (length >= 32) {
        const unsigned char* const limit = end - 32;
        uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t v2 = seed + PRIME64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - PRIME64_1;

        do {
            uint64_t k1, k2, k3, k4;
            memcpy(&k1, p, 8);      v1 += k1 * PRIME64_2; v1 = rotl64(v1, 31); v1 *= PRIME64_1; p += 8;
            memcpy(&k2, p, 8);      v2 += k2 * PRIME64_2; v2 = rotl64(v2, 31); v2 *= PRIME64_1; p += 8;
            memcpy(&k3, p, 8);      v3 += k3 * PRIME64_2; v3 = rotl64(v3, 31); v3 *= PRIME64_1; p += 8;
            memcpy(&k4, p, 8);      v4 += k4 * PRIME64_2; v4 = rotl64(v4, 31); v4 *= PRIME64_1; p += 8;
        } while (p <= limit);

        h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);

        v1 *= PRIME64_2; v1 = rotl64(v1, 31); v1 *= PRIME64_1; h64 ^= v1; h64 = h64 * PRIME64_1 + PRIME64_4;
        v2 *= PRIME64_2; v2 = rotl64(v2, 31); v2 *= PRIME64_1; h64 ^= v2; h64 = h64 * PRIME64_1 + PRIME64_4;
        v3 *= PRIME64_2; v3 = rotl64(v3, 31); v3 *= PRIME64_1; h64 ^= v3; h64 = h64 * PRIME64_1 + PRIME64_4;
        v4 *= PRIME64_2; v4 = rotl64(v4, 31); v4 *= PRIME64_1; h64 ^= v4; h64 = h64 * PRIME64_1 + PRIME64_4;
    }
    else {

        h64 = seed + PRIME64_5;
    }

    h64 += (uint64_t)length;

    while (p + 8 <= end) {
        uint64_t k1;
        memcpy(&k1, p, 8);
        k1 *= PRIME64_2; k1 = rotl64(k1, 31); k1 *= PRIME64_1;
        h64 ^= k1;
        h64 = rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p += 8;
    }

    if (p + 4 <= end) {
        unsigned int k1;
        memcpy(&k1, p, 4);
        h64 ^= (uint64_t)k1 * PRIME64_1;
        h64 = rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p += 4;
    }

    while (p < end) {
        h64 ^= (uint64_t)(*p) * PRIME64_5;
        h64 = rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;

}

int path::ensure_directory_exists(char* canonical_dir_path) {

    std::filesystem::create_directories(canonical_dir_path);
    return 1;
}


int path::copy_file(const char* src_path, const char* dest_path)
{
    if (!src_path || !dest_path) return 0;

    char parent_buff[1024];
    const char* last_slash = strrchr(dest_path, '/');
    if (last_slash) {
        size_t len = last_slash - dest_path;
        if (len >= sizeof(parent_buff)) {
            fprintf(stderr, "Error: Destination path is too long\n");
            return 0;
        }

        if (len == 0) {
            strcpy(parent_buff, "/");
        }
        else {
            strncpy(parent_buff, dest_path, len);
            parent_buff[len] = '\0';
        }

        if (!ensure_directory_exists(parent_buff)) {
            fprintf(stderr, "Error: Could not prepare directory structure for: %s\n", dest_path);
            return 0;
        }
    }

    /* Open the source file in binary read mode */
    FILE* src = fopen(src_path, "rb");
    if (!src) {
        perror("Error opening source file");
        return 0;
    }

    /* Open the destination file in binary write mode */
    FILE* dest = fopen(dest_path, "wb");
    if (!dest) {
        perror("Error creating destination file");
        fclose(src);
        return 0;
    }

    char * buffer = (char*)calloc(1, 65536);
    size_t bytes_read;
    int success = 1;

    /* Read and write data block by block */
    while ((bytes_read = fread(buffer, 1, 65536, src)) > 0) {
        size_t bytes_written = fwrite(buffer, 1, bytes_read, dest);

        /* If bytes written mismatch bytes read, an I/O error occurred */
        if (bytes_written < bytes_read) {
            perror("Error writing to destination file");
            success = 0;
            break;
        }
    }

    free(buffer);
    
    fclose(src);
    fclose(dest);
    return success;
}

uint32_t path::canonicalize_resource_path(const char* src_path, char* out_canonical, uint32_t max_size)
{
    if (!src_path || src_path[0] == '\0') {
        out_canonical[0] = '\0';
        return 0;
    }
    
    uint32_t dst_idx = 0;   
    uint32_t src_idx = 0;
    while (src_path[src_idx] != '\0' && dst_idx < max_size - 1) {
        char c = src_path[src_idx];
    
        if (c == '\\') {
            c = '/';
        }
        else if (c >= 'A' && c <= 'Z') {
            c = c + ('a' - 'A');
        }
    
        out_canonical[dst_idx] = c;
        dst_idx++;
        src_idx++;
    }
    
    out_canonical[dst_idx] = '\0';
    return dst_idx;
}


bool utf8::is_ascii(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (data[i] > 0x7F)
            return false;
    }
    return true;
}

bool utf8::is_ascii(const wchar_t* data, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (data[i] > 0x7F)
            return false;
    }
    return true;
}

size_t utf8::wchar_to_utf8(const wchar_t* w, size_t size, uint8_t* s)
{
    uint32_t  c;
    short* p = (short*)w;
    uint8_t* q = (uint8_t*)s; uint8_t* q0 = q;
    while (1) {
        c = *p++;
        if (c == 0)
            break;
        if (c < 0x080)
            *q++ = c;
        else
            if (c < 0x800) 
                *q++ = 0xC0 + (c >> 6), *q++ = 0x80 + (c & 63); 
            else
                *q++ = 0xE0 + (c >> 12), *q++ = 0x80 + ((c >> 6) & 63), *q++ = 0x80 + (c & 63);
    }
    *q = 0;
    return q - q0;
}

size_t utf8::utf8_to_wchar(const uint8_t* s, size_t size, wchar_t* w)
{
    uint32_t  cache = 0, wait = 0, c = 0;
    uint8_t* p = (uint8_t*)s;
    short* q = (short*)w; short* q0 = q;
    while (1) {
        c = *p++;
        if (c == 0) 
            break;
        if (c < 0x80) 
            cache = c, wait = 0; 
        else
            if ((c >= 0xC0) && (c <= 0xE0)) 
                cache = c & 31, wait = 1; 
            else
                if ((c >= 0xE0)) 
                    cache = c & 15, wait = 2; 
                else if (wait) 
                    (cache <<= 6) += c & 63, wait--;
        if (wait == 0) 
            *q++ = cache;
    }
    *q = 0;
    return q - q0;
}


std::wstring utf8::from_utf8(const uint8_t* data, size_t size)
{
    std::wstring result;
    result.reserve(size);
    utf8::utf8_to_wchar(data, size, result.data());
    return result;
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
    printf("\n\033[0m");
}

void debug::log_error(const char* msg, ...)
{
    printf("\n\x1b[31m");
    arg_vprintf(msg);
    printf("\033[0m");
}

void debug::log_warning(const char* msg, ...)
{ 
    printf("\n\x1B[33m");
    arg_vprintf(msg);
    printf("\033[0m");
}

void debug::breakpoint()
{
#ifdef _WIN32 
    __debugbreak();
#else
    __builtin_trap();
#endif
}

int debug::callstack(uintptr_t* frames, uint32_t count)
{
#ifdef _WIN32
    static bool lazyinit = false;
    if (!lazyinit) {
        SymInitialize(GetCurrentProcess(), NULL, TRUE);
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        lazyinit = true;
    }

    int skipframes = 2;
    uint32_t frame_count = RtlCaptureStackBackTrace(skipframes, (DWORD)count, (PVOID*)frames, NULL) - 5;
    if(frame_count > count)
        frame_count = count;
    return frame_count;
#endif
    return -1;
}

void debug::callstack_names(uintptr_t* frames, uint32_t count, char** names)
{
#ifdef _WIN32
    char* name_buffer = nullptr;
    if (name_buffer == nullptr)
        name_buffer = (char*)malloc(1024);
    if(!name_buffer)
        return;

    memset(name_buffer, 0, 1024);
    HANDLE hprocess = GetCurrentProcess();
    char tmpbuffer[sizeof(SYMBOL_INFO) + 64] = "";
    for (uint64_t i = 0; i < count; ++i)
    {
        DWORD ldsp = 0;
        IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)tmpbuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 64;

        // SymGetLineFromAddr64(hprocess, adress, &ldsp, &line);
        SymFromAddr(hprocess, frames[i], 0, symbol);

        if (names != nullptr)
        {
            strcpy(name_buffer, symbol->Name);
            strcat(name_buffer, "\0");
            names[i] = name_buffer;
            name_buffer += strlen(name_buffer) + 1;
        }

        debug::log("%s", symbol->Name);
    }
#endif
}


static size_t filesize(FILE* file)
{
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}
//
// stream_impl
//
struct stream_impl
{
    stream_impl(FILE* file, uint32_t buffer_size = 512 * 1024) : m_file(file)
    {
        m_file_size = filesize(m_file);

        m_write_buffer_size = buffer_size;
        m_write_buffer = new char[m_write_buffer_size]();

      //  if(filesize(m_file) < buffer_size)
      //      m_read_buffer_size = filesize(m_file);
            
        m_read_buffer_size = buffer_size;
        m_read_buffer_capacity = buffer_size;
        m_read_buffer = new char[m_write_buffer_size]();

        refill_buffer();
    }

    stream_impl(FILE* file, char * buffer, uint32_t buffer_size) : m_file(file)
    {
        m_write_buffer_size = 0;
      //  m_write_buffer = new char[m_write_buffer_size]();
        m_read_buffer_capacity = buffer_size;
        m_read_buffer_size = buffer_size;
        m_read_buffer = buffer;
        m_external_read_buffer = true;

        refill_buffer();
    }


    ~stream_impl()
    {
        if (m_read_buffer && !m_external_read_buffer)
            delete [] m_read_buffer;

        if (m_write_buffer) delete [] m_write_buffer;
    }

    size_t read(uint32_t size, char* data)
    {
        size_t bytes_read = 0;
        while (bytes_read < size) {
            if (m_read_pos >= m_read_buffer_size) {
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

    size_t seek(int64_t offset, int whence)
    {
        int64_t target_pos = 0;
        int64_t current_virtual_pos = static_cast<int64_t>(tell());

        if (whence == SEEK_SET) {
            target_pos = offset;
        }
        else if (whence == SEEK_CUR) {
            target_pos = current_virtual_pos + offset;
        }
        else if (whence == SEEK_END) {
            flush();
            if (fseek(m_file, offset, SEEK_END) != 0) return static_cast<size_t>(-1);
            m_read_pos = 0;
            m_read_buffer_size = 0;
            return tell();
        }

        if (target_pos < 0) return static_cast<size_t>(-1);

        if (m_read_buffer_size > 0) {
            long sys_pos = ftell(m_file);

            int64_t buffer_start_pos = sys_pos - m_read_buffer_size;
            int64_t buffer_end_pos = sys_pos;

            if (target_pos >= buffer_start_pos && target_pos <= buffer_end_pos) {

                m_read_pos = static_cast<size_t>(target_pos - buffer_start_pos);
                return static_cast<size_t>(target_pos);
            }
        }

        flush();
        if (fseek(m_file, target_pos, SEEK_SET) != 0) {
            return static_cast<size_t>(-1);
        }
        m_read_pos = 0;
        m_read_buffer_size = 0;

        return static_cast<size_t>(target_pos);
    }

    uint32_t tell()
    {
        return m_read_virtual_pos + m_read_pos;
    }

    void flush()
    {
        write_buffer();
    }

private:
    void refill_buffer()
    {
        m_read_virtual_pos = ftell(m_file);
        m_read_buffer_size = fread(m_read_buffer, 1, m_read_buffer_capacity, m_file);
        m_read_pos = 0;
        m_file_pos = ftell(m_file);
    }

    void write_buffer()
    {
        size_t result = fwrite(m_write_buffer, 1, m_write_pos, m_file);
        fflush(m_file);
        m_write_pos = 0;
        m_file_pos += result;
    }

private:
    FILE*       m_file = nullptr;
    size_t      m_file_pos = 0;
    size_t      m_file_size = 0;


    char*       m_read_buffer = nullptr;
    size_t      m_read_buffer_capacity = 0;
    size_t      m_read_buffer_size = 0;
    size_t      m_read_pos = 0;
    size_t      m_read_virtual_pos = 0;

    bool        m_external_read_buffer = false;  // external buffer, don't dealocate on destroy

    char*       m_write_buffer = nullptr;
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
    if(m_impl)
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

uint32_t filestream::tell()
{
    return m_impl->tell();
}


static const char _guid_digits[] = "0123456789abcdef";

static inline uint8_t hex_char_to_val(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return 0;
}

guid_t uuid::generate_from_seed(size_t seed)
{
    char buf[33] = "";
    srand((unsigned int)seed);
    char* ptr = buf;
    for (size_t i = 0; i < 16; i++)
    {
        *ptr++ = _guid_digits[(rand() % 255 >> 4) & 0xf];
        *ptr++ = _guid_digits[(rand() % 255 >> 0) & 0xf];
    }
    return str_to_guid(buf);
}


bool uuid::is_guid_str(const char *buf)
{
    size_t len = buf ? strlen(buf) : 0;
    for (size_t i = 0; i < len; ++i) {
        if (!strchr(_guid_digits, buf[i]))
            return false;
    }
    return len ? true : false;
}

guid_t uuid::generate_uuid_v4()
{
    static std::atomic<uint32_t> g_counter;
    g_counter.fetch_add(1, std::memory_order_relaxed);
    return generate_from_seed(clock() + g_counter);
}

guid_t uuid::str_to_guid(const char* str)
{
    guid_t result = {};

    for (int i = 0; i < 16; ++i) {
        result.high = (result.high << 4) | hex_char_to_val(str[i]);
    }

    for (int i = 16; i < 32; ++i) {
        result.low = (result.low << 4) | hex_char_to_val(str[i]);
    }

    return result;
}

char* uuid::guid_to_str(guid_t g, char* buff)
{
    for (int i = 15; i >= 0; --i) {
        buff[i] = _guid_digits[g.high & 0x0F];
        g.high >>= 4;
    }

    for (int i = 31; i >= 16; --i) {
        buff[i] = _guid_digits[g.low & 0x0F];
        g.low >>= 4;
    }
    return buff;
}


uint64_t uuid::runtime_guid(guid_t _guid)
{
    if(_guid.high == 0 && _guid.low == 0) return 0;
    return hasher::xxhash64(&_guid, sizeof(guid_t), 0 );
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

