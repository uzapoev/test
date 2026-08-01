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

//#include <malloc.h>
#include <mutex>
#include <unordered_map>


typedef struct memory_stats_t {
    int     active_allocations;

    size_t  total_allocated_size;

    size_t  min_size;

    size_t  max_size;
} memory_stats_t;


// https://github.com/suVrik/allocator_benchmark
struct iallocator
{
    virtual void *      allocate(size_t size, size_t alignment) = 0;

    virtual void        deallocate(void* memory) = 0;

    virtual const char * tag() {return "";}
};


//
// auto ptr = allocator.allocate(sizeof(transform), alignof(transform));
//
struct aligned_allocator : iallocator
{
    aligned_allocator(const char * tag = "aligned_allocator");

    virtual ~aligned_allocator();

    virtual void *      allocate(size_t size, size_t alignment);

    virtual void        deallocate(void* memory);

    virtual const char* tag() { return m_name; }

private:
    char                m_name[64] = "";
    size_t              m_allocated_size = 0;

    struct mem_traker * m_traker   = nullptr;
};


//
// Paged pool allocator
//  min alocation per page - 64(aligned up)
struct paged_pool_allocator : iallocator
{
                         paged_pool_allocator(iallocator* memory_resource, size_t allocation_size, size_t allocations_per_page);
    virtual             ~paged_pool_allocator();

    template <class T> T* allocate() {
        assert(m_allocation_size == sizeof(T));
        return new(allocate(sizeof(T), alignof(T))) T();
    }

    virtual void *      allocate(size_t size, size_t alignment);

    virtual void        deallocate(void* ptr);

    bool                contains_address(const void* ptr);

    void *              data_by_index(uint32_t index);

    int32_t             index_by_data(const void* ptr);

private:
    struct page {
        page* next = nullptr;
        uint64_t* bitmask = nullptr;
        char* data = nullptr;
    };


    page *       allocate_page();

    page *       find_page_with_free_block();

    page *       find_page_for_ptr(const void * ptr);

private:


private:
    iallocator*         m_allocator = nullptr;
    struct page*        m_page_head = nullptr;
    struct page*        m_page_current = nullptr;

    uint32_t            m_allocation_size;
    uint32_t            m_allocations_per_page;
    uint32_t            m_page_size;
    uint32_t            m_bitset_word_count;
};


template<class T>
struct paged_pool_allocator_t
{
    paged_pool_allocator_t(iallocator* memory_resource, uint32_t allocation_per_page)
    :m_ppallocator(memory_resource, sizeof(T), allocation_per_page) { }

    inline T*       allocate()              { return static_cast<T*>(m_ppallocator.allocate(sizeof(T), alignof(T))); }
    inline void     deallocate(T* ptr)      { m_ppallocator.deallocate(ptr);}

    inline T*       get_by_index(uint32_t idx) noexcept { return (T*)m_ppallocator.data_by_index(idx); }
    inline int32_t  get_index(T* t) noexcept            { return m_ppallocator.index_by_data(t); }

private:
    paged_pool_allocator                                    m_ppallocator;
};
 
//
//
class staging_allocator : public iallocator
{
public:
    staging_allocator(iallocator* memory_resource, size_t allocation_size, bool stretch);
    virtual ~staging_allocator();

    virtual void*   allocate(size_t size, size_t alignment) override;
    virtual void    deallocate(void* memory) override;
    
    void            trim_to_initial_size();     // Forces buffer shrinking back to initial size if it is empty
    bool            empty();                    // Checks if all allocated blocks have been freed

private:
    bool            reallocate_buffer(size_t new_size);

    struct BlockHeader {
        uint32_t size;
        uint32_t padding;
    };

    iallocator* m_allocator;
    size_t      m_initial_size; // Store initial size to shrink back to it later
    size_t      m_size;
    uint8_t* m_data = nullptr;

    uint32_t    m_head = 0;
    uint32_t    m_tail = 0;
    uint32_t    m_end = 0;
    bool        m_stretch = false;
    std::mutex  m_mutex;
};

//
//
//
class buddy_allocator : public iallocator
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
    iallocator *        m_allocator = nullptr;
    uint8_t*            m_buffer;
    size_t              m_totalSize;
    size_t              m_minBlockSize;
    int                 m_maxLevel;
    int                 m_levelCount;

    struct FreeList *   m_freeLists;
    std::mutex          m_mutex;
};


struct memory
{
    static void    enable_tracking();
    static void    enable_allocation_traking(bool value);
    static void    dump(struct memory_stats_t* stats);
    static size_t  allocated();
};

typedef void    (*allocation_callback_pfn)(size_t sz, void* ptr, void* data);

#endif