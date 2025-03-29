/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <yuriy> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return 
 * ---------------------------------------------------------------------------- */

#ifndef __MEMORY_H__
#define __MEMORY_H__

#ifdef _WIN32
    #pragma warning (disable: 4530)// C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
 //   #include <windows.h>
#elif defined (__APPLE__)
    #include <stdio.h>
    #include <stdlib.h>
    #include <pthread.h>
    #include <mach/mach.h>
    #include <malloc/malloc.h>
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include <pthread.h>
    #include <malloc.h>
    #include <string.h>
#endif

#include <map>
#include <mutex>
#include <unordered_map>

////////////////////////
//
typedef void    (*allocationCallback)(size_t sz, void* ptr, void* data);
extern bool                 s_allocatorDeepLogEnable;
extern allocationCallback   s_allocationCallback;
extern void*                s_allocationCallbackData;

void    memory_enable_tracking();
void    memory_enable_allocation_traking(bool value);
void    memory_dump(struct memory_stats_t* stats);
size_t  memory_allocated();


typedef struct memory_stats_t
{
    int num;
    size_t minSize;
    size_t maxSize;
    size_t totalSize;
} memory_stats_t;



/***************************************************************
 *        MemoryManager
 ***************************************************************/

class MemoryManager
{
public:
     MemoryManager();
    ~MemoryManager();
public:

    void    initialize();
    void    destroy();

    void *  alloc( size_t size );
    void *  alloc16( size_t size );

    void    free(void *);
    void    free16(void *);

    void    dump();

//private:

    void Mem_ClearFrameStats( void );
        
    void Mem_GetFrameStats(memory_stats_t& allocs, memory_stats_t& frees );
    void Mem_GetStats(memory_stats_t& stats );

private:

    static void callback(size_t sz, void* ptr, void* data);// throw();

protected:
    std::mutex        m_mutex;

    struct memblock_t
    {
        memblock_t(size_t sz, void* p) :ptr(p), size(sz) { memset(frames, 0, sizeof(frames)); }

        memblock_t(const memblock_t& src)               { size = src.size; ptr = src.ptr; memcpy(frames, src.frames, sizeof(frames)); }
        memblock_t& operator = (const memblock_t& src)  { size = src.size; ptr = src.ptr; memcpy(frames, src.frames, sizeof(frames)); return *this; }

        bool    operator == (void* ptr) const { return ptr == ptr; }

      //  inline friend bool operator == (const memblock& b1, const memblock& b2) { return b1.ptr == b2.ptr; }
      //  inline friend bool operator <  (const memblock& b1, const memblock& b2) { return b1.ptr < b2.ptr; }

        static const int    k_max_stack_size = 16;

        void*               ptr;
        size_t              size;
        intptr_t            frames[k_max_stack_size] = {}; // replace to stack frames 
    };


    template<class T>
    struct internal_allocator
    {
        typedef T value_type;

        internal_allocator() = default;

        template<class U>
        internal_allocator(const internal_allocator <U>&) noexcept {};

        inline T*   allocate(std::size_t n)     noexcept { return (T*)std::malloc(n * sizeof(T)); }
        inline void deallocate(T* p, size_t n)  noexcept { std::free(p); }
    };

    using memblock_allocator = internal_allocator< std::pair<const intptr_t, memblock_t>>;
    using mem_block_map      = std::unordered_map< intptr_t, memblock_t, std::hash<intptr_t>, std::equal_to<intptr_t>, memblock_allocator >;

    mem_block_map   m_blocks;
};

template<class T, class U>
static bool operator==(const MemoryManager::internal_allocator <T>&, const MemoryManager::internal_allocator <U>&) { return true; }

template<class T, class U>
static bool operator!=(const MemoryManager::internal_allocator <T>&, const MemoryManager::internal_allocator <U>&) { return false; }


#endif