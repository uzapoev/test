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
#endif

#include <map>
#include <mutex>
#include <unordered_map>

// https://github.com/suVrik/allocator_benchmark
struct iallocator
{
    virtual void *  allocate(size_t size, size_t alignment) = 0;

    virtual void    deallocate(void* memory) = 0;

    template <typename T>
    T* allocate(size_t count) {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }
};


class default_allocator : iallocator
{
public:
    virtual void *  allocate(size_t size, size_t alignment);

    virtual void    deallocate(void* memory);
};



class buddy_allocator : iallocator
{
public:
    buddy_allocator(void* buffer, size_t totalSize, size_t minBlockSize = 64);

    ~buddy_allocator();

    virtual void *      allocate(size_t size, size_t alignment);

    virtual void        deallocate(void* memory);

    int                 getLevel(size_t size) const                 {        return (int)(log2(size) - log2(m_minBlockSize));    }

    size_t              block_size(int level) const                 {        return m_minBlockSize << level;    }

    size_t              get_buddy(size_t offset, int level) const   {        return offset ^ block_size(level);    }

private:

    default_allocator * m_allocator = nullptr;
    uint8_t*            m_buffer;
    size_t              m_totalSize;
    size_t              m_minBlockSize;
    int                 m_maxLevel;
    int                 m_levelCount;

    struct FreeList *   m_freeLists;
    std::mutex          m_mutex;
};


struct paged_pool_allocator: iallocator
{
};


struct offset_allocator
{
    ptrdiff_t   allocate(size_t size, size_t alignment);
    void        deallocate(size_t offset);

protected:
    void        merge_free_blocks();

    size_t      buffer_size() const         {   return m_buffer_size;    }

private:
    struct Block {
        size_t offset;
        size_t size;

        Block(size_t off, size_t sz) : offset(off), size(sz) {}
    };

    size_t                  m_buffer_size;
    std::vector<Block>      m_free_blocks;
    std::vector<Block>      m_allocated_blocks;
    mutable std::mutex      m_mutex;
};


struct memory
{
    static void    enable_tracking();
    static void    enable_allocation_traking(bool value);
    static void    dump(struct memory_stats_t* stats);
    static size_t  allocated();
};


////////////////////////
//
typedef void    (*allocation_callback_pfn)(size_t sz, void* ptr, void* data);


 


typedef struct memory_stats_t
{
    int     active_allocations;
    size_t  total_allocated_size;

    size_t  min_size;
    size_t  max_size;
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
     //   memblock_t& operator = (const memblock_t& src)  { size = src.size; ptr = src.ptr; memcpy(frames, src.frames, sizeof(frames)); return *this; }

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