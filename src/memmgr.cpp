/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <yuriy> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return 
 * ---------------------------------------------------------------------------- */
//int size = (length+3)&~3;
#include <assert.h>
#include "memmgr.h"
//#include "debug.h"


#define THREAD_SAFE                 1
#define TRACE_MEMORY_ALLOCATION     0
#define USE_CUSTOM_NEW_ALLOCATION   1
#define USE_ALLOCATION_CALLBACK     1

bool                s_allocatorDeepLogEnable    = true;
allocationCallback  s_allocationCallback        = nullptr;
void*               s_allocationCallbackData    = nullptr;


#ifdef _WIN32
    #pragma warning(disable: 6387 28183 28196 28251 28252 28253 )

    #define sys_malloc(x)           ::malloc(x)
    #define sys_free(x)             ::free(x)
    #define sys_msize(x)            ::_msize(x)

    #define memlog(...)             printf(__VA_ARGS__)
#elif defined (__APPLE__)
    #define sys_malloc(x)           ::malloc(x)
    #define sys_free(x)             ::free(x)
    #define sys_msize(x)            ::malloc_size(x)

    #define memlog(...)             printf(__VA_ARGS__)
    #define mutex_lock( x )         pthread_mutex_lock( x )
    #define mutex_unlock( x )       pthread_mutex_unlock( x );
    #define mutex_destroy( x )      pthread_mutex_destroy( x );
#elif defined (__ANDROID__)
    #include <android/log.h>
    #define sys_malloc(x)           ::malloc(x)
    #define sys_free(x)             ::free(x)
    #define sys_msize(x)            ::malloc_usable_size(x)

    #define msize(x)                0 //malloc_usable_size(x)
    #define memlog(...)             __android_log_print(ANDROID_LOG_INFO, "memory", __VA_ARGS__)
    #define mutex_lock( x )         pthread_mutex_lock( x )
    #define mutex_unlock( x )       pthread_mutex_unlock( x );
    #define mutex_destroy( x )      pthread_mutex_destroy( x );

#elif defined (__NINTENDO__)
    #define memlog(...)             printf(__VA_ARGS__)

    #define sys_malloc(x)           ::malloc(x)
    #define sys_free(x)             ::free(x)
    #define sys_msize(x)            (0)

    #define mutex_lock( x )
    #define mutex_unlock( x )
    #define mutex_destroy( x )
#endif

static volatile size_t g_total_allocated_memory = 0;
static volatile bool g_trace_allocations = 0;

static memory_stats_t mem_total_allocs = { 0, 0x0fffffff, 0, 0 };
static memory_stats_t mem_frame_allocs;
static memory_stats_t mem_frame_frees;

void printMemoryStats();
void Mem_UpdateAllocStats( size_t size );
void Mem_UpdateFreeStats ( size_t size );


static void setAllocationCallback(allocationCallback cb, void* data)
{
    s_allocationCallback = cb;
    s_allocationCallbackData = data;
}

MemoryManager * g_pMemoryManager = nullptr;
static char _reserved[sizeof(MemoryManager)] = {};

void memory_enable_tracking()
{
    if(g_pMemoryManager == nullptr)
        g_pMemoryManager = new(_reserved) MemoryManager();
}

void memory_enable_allocation_traking(bool value)
{
    g_trace_allocations = value;
}

void memory_dump(memory_stats_t* stats)
{
    if(stats != nullptr)
        memcpy(stats, &mem_total_allocs, sizeof(memory_stats_t));
}

size_t memory_allocated()
{
    return g_total_allocated_memory;
}


MemoryManager::MemoryManager()
{
    printMemoryStats();
#if USE_ALLOCATION_CALLBACK
    setAllocationCallback(MemoryManager::callback, this);
#endif
}



MemoryManager::~MemoryManager()
{
    g_pMemoryManager = nullptr;
    setAllocationCallback(nullptr, nullptr);

    {
    #if THREAD_SAFE
        std::lock_guard lock(m_mutex);
    #endif
        mem_block_map(m_blocks).swap(m_blocks);//the STL swap trick to trim memory
        memlog("\n----------------------------------------------------------");
        memlog("\nMemoryTrackerReport");
        memlog("\nLeaked Memory"       );
        memlog("\n----------------------------------------------------------\n");
    
        size_t totalLeak = 0;    
    //    MemBlocks::const_iterator it, ite = m_blocks.end();
        FILE * file = fopen("memlog.log", "w");

        //char  framesinfo[stack_size][16] = { 0 };
        char * framesinfo[memblock_t::k_max_stack_size] = { };

        for(int i = 0; i < memblock_t::k_max_stack_size; ++i)
            framesinfo[i] = (char*)malloc(sizeof(char)*2048);
         
        for(auto & it = m_blocks.begin(); it != m_blocks.end(); ++it)
        {
            for(int i = 0; i< memblock_t::k_max_stack_size; ++i)
                if(framesinfo[i] != nullptr)
                    memset(framesinfo[i], 0, sizeof(char)* 2048);
    //        StackInfoBlocks::iterator infoIt = m_infoblocks.find(it->first);

            const memblock_t& block = (*it).second;
            memlog("\n %llx  size: %zd", (uintptr_t)block.ptr, block.size);

            if(file)
                fprintf(file, "\n %llx  size: %zd", (uintptr_t)block.ptr, block.size);

            if(block.frames[0] != 0)
            {
                int framescount = sizeof(block.frames)/sizeof(block.frames[0]);
             //   debug::stacktrace_names(block.frames, framescount, (char**)framesinfo);

                for (int i = 0; i < framescount; i++)
                {
                    if(block.frames[i] == 0)
                        break;

                    if(file)
                        fprintf(file, "\n\t%s", framesinfo[i]);
                }
            }
            totalLeak += block.size;
        }
        if (file)fclose(file);
    
        for(int i = 0; i< memblock_t::k_max_stack_size; ++i)
            ::free(framesinfo[i]);
    
        memlog("\n************************************************************");
        memlog("\n* Total leak is: %zd kB, , bytes in %zd blocks", totalLeak/1024, m_blocks.size() );
        memlog("\n************************************************************\n");
    }
#ifdef _DEBUG
//    system("pause");
#endif/**/


    m_blocks.clear();
}

void MemoryManager::initialize()
{    
//    mem_heap = new Heap();
//    Mem_ClearFrameStats();
}

void MemoryManager::destroy()
{
}

void * MemoryManager::alloc(size_t size )
{
#if THREAD_SAFE
    std::lock_guard lock(m_mutex);
#endif
    
    if ( !size )
        return nullptr;

    if(size > 1024*64)
        printf("");

    void *mem = nullptr;

#ifdef CRASH_ON_STATIC_ALLOCATION
        *((int*)0x0) = 1;
#endif
    //  size = ( size + 3 & ~3); 4 bit align
    size = ( size + 15 & ~15 );//16 bit align
    mem = sys_malloc(size);
    g_total_allocated_memory += size;

    if(s_allocationCallback)
    {
        Mem_UpdateAllocStats(size);
        callback(size, mem, this);
    }
    
    return mem;
}

void * MemoryManager::alloc16(size_t size)
{
    if ( !size )
        return nullptr;
    //  size = ( size + 3 & ~3); 4 bit align
    size = (size + 15 & ~15);//16 bit align

#ifdef CRASH_ON_STATIC_ALLOCATION
    *((int*)0x0) = 1;
#endif
    g_total_allocated_memory += size;
    return sys_malloc(size);
}


void MemoryManager::free(void *ptr)
{
#if THREAD_SAFE
    std::lock_guard lock(m_mutex);
#endif
    if ( !ptr )
        return;
    
    if(s_allocationCallback )
        callback(0, ptr, this);

    size_t originsize = sys_msize(ptr);
    g_total_allocated_memory -= originsize;
    Mem_UpdateFreeStats(originsize);
    sys_free( ptr );
}

void   MemoryManager::free16(void *ptr)
{
#if THREAD_SAFE
    std::lock_guard lock(m_mutex);
#endif
    if ( !ptr )
        return;

    sys_free( ptr );
}

void MemoryManager::dump()
{
#if THREAD_SAFE
    std::lock_guard lock(m_mutex);
#endif
    
    size_t totalMemUsage = 0;
    for(auto & it = m_blocks.begin(); it != m_blocks.end(); ++it)
    {
        totalMemUsage += it->second.size;
    }
    memlog("\nsize: %zd kB", totalMemUsage /1024);
}


void MemoryManager::callback(size_t sz, void* ptr, void* data)
{
    MemoryManager* t = static_cast<MemoryManager*>(data);
    if(sz > 0) 
    {
        memblock_t memBlock = memblock_t(sz,ptr);
        if(s_allocatorDeepLogEnable)
        {
            //skip next frames:
            //  [0] dbg_backtrace(...)
            //  [1] MemoryManager::callback(...)
            //  [2] MemoryManager::alloc(...)
            //  [3] new
         //   Debug::stacktrace_frames(memBlock.frames, sizeof(memBlock.frames)/sizeof(intptr_t), 4);
        }
        t->m_blocks.emplace(intptr_t(ptr), memBlock);
    }
    else 
    {
        if(t->m_blocks.find((intptr_t)(ptr)) != t->m_blocks.end())
            t->m_blocks.erase((intptr_t)(ptr));
        else
            memlog("\nmemoryTracker: freeing memory that wasn't previously registered");
    }
}


void MemoryManager::Mem_ClearFrameStats( void )
{
    mem_frame_allocs.num = mem_frame_frees.num = 0;
    mem_frame_allocs.minSize = mem_frame_frees.minSize = 0x0fffffff;
    mem_frame_allocs.maxSize = mem_frame_frees.maxSize = -1;
    mem_frame_allocs.totalSize = mem_frame_frees.totalSize = 0;
}

void MemoryManager::Mem_GetFrameStats(memory_stats_t&allocs, memory_stats_t&frees )
{
    allocs = mem_frame_allocs;
    frees = mem_frame_frees;
}

void MemoryManager::Mem_GetStats(memory_stats_t&stats )
{
    stats = mem_total_allocs;
}



void Mem_UpdateStats(memory_stats_t&stats, size_t size )
{
    stats.num++;
    if ( size < stats.minSize )
        stats.minSize = size;
    if ( size > stats.maxSize )
        stats.maxSize = size;

    stats.totalSize += size;
}

void Mem_UpdateAllocStats( size_t size )
{
    Mem_UpdateStats( mem_frame_allocs, size );
    Mem_UpdateStats( mem_total_allocs, size );
}

void Mem_UpdateFreeStats( size_t size )
{
    Mem_UpdateStats( mem_frame_frees, size );
    mem_total_allocs.num--;
    mem_total_allocs.totalSize -= size;
}

/*
void* Mem_ClearedAlloc( const int size )
{
    void *mem = Mem_Alloc( size );
#ifdef _SIMDMMX
    SIMDMMXMemset( mem, 0, size );
#else
    memset( mem, 0, size );
#endif
    return mem;
}*/
/*
void MemoryManager::Mem_AllocDefragBlock( void )
{
    mem_heap->AllocDefragBlock();
}

char* Mem_CopyString( const char *in )
{
    char *out;

    out = (char *)Mem_Alloc( strlen(in) + 1 );
    strcpy( out, in );
    return out;
}*/




#define SMALL_HEADER_SIZE    ( (int) ( sizeof( uint8_t ) + sizeof( byte ) ) )
#define MEDIUM_HEADER_SIZE    ( (int) ( sizeof( mediumHeapEntry_s ) + sizeof( byte ) ) )
#define LARGE_HEADER_SIZE    ( (int) ( sizeof( uint32_t * ) + sizeof( byte ) ) )

#define ALIGN_SIZE( bytes )    ( ( (bytes) + ALIGN - 1 ) & ~(ALIGN - 1) )
#define SMALL_ALIGN( bytes ) ( ALIGN_SIZE( (bytes) + SMALL_HEADER_SIZE ) - SMALL_HEADER_SIZE )
#define MEDIUM_SMALLEST_SIZE ( ALIGN_SIZE( 256 ) + ALIGN_SIZE( MEDIUM_HEADER_SIZE ) )



void printMemoryStats()
{
#ifdef _WIN32
#elif  defined(__APPLE__) || defined(PLATFORM_APPLE)
    vm_statistics_data_t info;
    vm_size_t pagesize = 0;
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    kern_return_t success;
    
    host_page_size(mach_host_self(), &pagesize);
    success = host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&info, &count);
    if (success != KERN_SUCCESS)
        return ;
    
    double total = info.wire_count + info.active_count + info.inactive_count + info.free_count;
    double wired = info.wire_count / total;
    double active = info.active_count / total;
    double inactive = info.inactive_count / total;
    double free = info.free_count / total;
    
    
    memlog("Total:     %8d pages\n", info.wire_count + info.active_count + info.inactive_count + info.free_count);
    memlog("\tWired:      %9lu bytes (%0.2f %%)\n", (unsigned long)(info.wire_count * pagesize),    wired * 100.0);
    memlog("\tActive:     %9lu bytes (%0.2f %%)\n", (unsigned long)(info.active_count * pagesize),  active * 100.0);
    memlog("\tInactive:   %9lu bytes (%0.2f %%)\n", (unsigned long)(info.inactive_count * pagesize),inactive * 100.0);
    memlog("\tFree:       %9lu bytes (%0.2f %%)\n", (unsigned long)(info.free_count * pagesize),    free * 100.0);
    
#endif
}

//#define TRACE_MEMORY_ALLOCATION 1
//volatile const unsigned int memoryUsed = 0;

////////////////////////////////////////////////////////////////
//
//MemoryTracker ttr;

#if USE_CUSTOM_NEW_ALLOCATION

void *operator new(size_t size)/* noexcept*/
{
    if (g_trace_allocations)
        memlog("+");

    void* ptr = nullptr;
    if(g_pMemoryManager)
    {
        ptr = g_pMemoryManager->alloc(size);
    }
    else
    {
        ptr = sys_malloc(size);
        if(ptr != nullptr)
        {
            size_t originsize = sys_msize(ptr);
            Mem_UpdateAllocStats(originsize);
            g_total_allocated_memory += originsize;
        }
    }
    return ptr;
}

void * operator new[](size_t size) /*noexcept*/
{
    if (g_trace_allocations)
        memlog("+");

    void* ptr = nullptr;
    if(g_pMemoryManager)
    {
        ptr = g_pMemoryManager->alloc(size);
    }
    else
    {
        ptr = sys_malloc(size);
        size_t originsize = sys_msize(ptr);
        Mem_UpdateAllocStats(originsize);
        g_total_allocated_memory += originsize;
    }
    
    return ptr;
}

void operator delete(void *ptr) noexcept
{
    if (g_trace_allocations)
        memlog("+");

    if(ptr)
    {
        if(g_pMemoryManager)
        {
            g_pMemoryManager->free(ptr);
        }
        else
        {
            size_t originsize = sys_msize(ptr);
            Mem_UpdateFreeStats(originsize);
            g_total_allocated_memory -= originsize;
            sys_free(ptr);
        }
    }
}

void operator delete[](void * ptr) noexcept
{
    if (g_trace_allocations)
        memlog("+");

    if(ptr)
    {
        if(g_pMemoryManager)
        {
            g_pMemoryManager->free(ptr);
        }
        else
        {
            size_t originsize = sys_msize(ptr);
            g_total_allocated_memory -= originsize;
            Mem_UpdateFreeStats(originsize);
            sys_free(ptr);
        }
    }
}
/**/
#endif
