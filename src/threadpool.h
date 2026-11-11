#ifndef __thread_pool_h__
#define __thread_pool_h__

#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>

constexpr uint32_t kMaxTasks = 256;          // 
constexpr uint32_t kTaskMask = kMaxTasks - 1;
constexpr uint32_t kMaxContinuations = 16;


enum class task_status : uint8_t {
    Invalid,
    Pending,
    Ready,
    InProgress,
    Done
};

// opaque: low 32 bits - slot index, high 32 bits - generation (0 is never a valid generation)
typedef uint64_t task_handle_t;

constexpr task_handle_t kInvalidTask = 0;

class thread_pool 
{
public:

    thread_pool(uint32_t worker_count = std::thread::hardware_concurrency());
    ~thread_pool();

    bool            try_allocate(std::function<void()> fn, task_handle_t* out_handle);

    bool            try_submit(std::function<void()> fn, task_handle_t* out_handle = nullptr);

    void            add_dependency(task_handle_t child, task_handle_t parent); // todo: DAG check

    void            submit(task_handle_t handle);// todo: prevent submit with same handles

    void            yield();

    void            wait_for(task_handle_t handle);

    void            wait_for_all();

    task_status     status(task_handle_t handle) const;

private:
    struct Task {
        std::function<void()>       fn                  = []{}; // todo: replace to "void (*pfn)(void* user_data);"
        std::atomic<uint32_t>       generation          = 1;
        std::atomic<task_status>    status              = task_status::Invalid;
        std::atomic<uint32_t>       dependency_count    = 0;

        uint32_t                    num_continuations   = 0;
        task_handle_t               continuations[kMaxContinuations]{};
    };

private:
    Task*           get_task(task_handle_t h);

    task_handle_t   allocate_unlocked(std::function<void()> fn);

    void            free_unlocked(task_handle_t h);

    void            enqueue_ready(task_handle_t h);

    bool            try_pop(task_handle_t* handle);

    void            execute(task_handle_t handle);

private:
    Task                        m_tasks[kMaxTasks];
    task_handle_t               m_queue[kMaxTasks];
    uint32_t                    m_free_list[kMaxTasks];

    uint32_t                    m_free_count = 0;   
    uint32_t                    m_queue_head = 0;
    uint32_t                    m_queue_tail = 0;

    std::atomic<uint32_t>       m_live{ 0 };
    std::atomic<bool>           m_shutdown = false;

    mutable std::mutex          m_mutex;
    std::condition_variable     m_cv;

    std::vector<std::thread>    m_workers;
};

#endif //__thread_pool_h__