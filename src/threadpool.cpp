#include "threadpool.h"
#include <cassert>

static inline task_handle_t make_handle(uint32_t id, uint32_t generation) { return (uint64_t(generation) << 32) | id; }
static inline uint32_t      handle_id(task_handle_t h)                    { return uint32_t(h & 0xFFFFFFFFull); }
static inline uint32_t      handle_gen(task_handle_t h)                   { return uint32_t(h >> 32); }

ThreadPool::ThreadPool(uint32_t worker_count) 
{
    worker_count = std::max(1u, std::min(worker_count, std::thread::hardware_concurrency()));

    for (uint32_t i = 0; i < kMaxTasks; ++i) {
        m_free_list[i] = i;
        m_tasks[i].generation.store(1, std::memory_order_relaxed);
    }
    m_free_count = kMaxTasks;

    m_workers.reserve(worker_count);
    for (uint32_t i = 0; i < worker_count; ++i) {
        m_workers.emplace_back([this] { 
            while (!m_shutdown.load()) {
                if (task_handle_t h = kInvalidTask; try_pop(&h)) {
                    execute(h);
                    continue;
                }

                std::unique_lock lock(m_mutex);
                m_cv.wait(lock, [this] { return m_shutdown.load() || (m_queue_head != m_queue_tail); });
            }
        });
    }
}

ThreadPool::~ThreadPool() 
{
    m_shutdown.store(true, std::memory_order_release);
    m_cv.notify_all();
    
    for (auto& t : m_workers) {
        if (t.joinable()) {
            t.join();
        }
    }
}

bool ThreadPool::try_allocate(std::function<void()> fn, task_handle_t* out_handle) 
{
    if (!out_handle) return false;

    std::lock_guard lock(m_mutex);

    if (m_free_count == 0) {
        return false;
    }

    *out_handle = allocate_unlocked(std::move(fn));
    return true;
}


bool ThreadPool::try_submit(std::function<void()> fn, task_handle_t* out_handle) 
{
    std::lock_guard lock(m_mutex);

    if (m_free_count == 0) {
        return false;
    }

    task_handle_t h = allocate_unlocked(std::move(fn));
    enqueue_ready(h);

    if (out_handle) {
        *out_handle = h;
    }
    return true;
}


void ThreadPool::yield()
{
    if (task_handle_t h = kInvalidTask; try_pop(&h))
        execute(h);
    else            
        std::this_thread::yield();
}

// todo: DAG check
void ThreadPool::add_dependency(task_handle_t child, task_handle_t parent) 
{
    if (handle_gen(child) == 0 || handle_gen(parent) == 0) return;
    if (child == parent) return;

    std::lock_guard lock(m_mutex);

    Task* c = get_task(child);
    Task* p = get_task(parent);
    if (!c || !p) return;

    assert(c->status.load(std::memory_order_relaxed) == task_status::Pending);

    if (p->num_continuations < kMaxContinuations) {
        p->continuations[p->num_continuations++] = child;
        c->dependency_count.fetch_add(1, std::memory_order_relaxed);
    }
}

// todo: prevent submit with same handles
void ThreadPool::submit(task_handle_t handle) 
{
    if (handle_gen(handle) == 0) return;

    std::lock_guard lock(m_mutex);
    Task* task = get_task(handle);
    if (!task) return;

    if (task->dependency_count.load(std::memory_order_relaxed) == 0) {
        enqueue_ready(handle);
    }
}


task_status ThreadPool::status(task_handle_t handle) const 
{
    const uint32_t id  = handle_id(handle);
    const uint32_t gen = handle_gen(handle);

    if (gen == 0 || id >= kMaxTasks) {
        return task_status::Invalid;
    }

    const Task& task = m_tasks[id];
    const uint32_t current_gen = task.generation.load(std::memory_order_acquire);

    if (current_gen == gen) {
        return task.status.load(std::memory_order_relaxed);
    }

    return task_status::Done;
}

void ThreadPool::wait_for(task_handle_t handle) 
{
    while (status(handle) != task_status::Done) {
        yield();
    }
}

void ThreadPool::wait_for_all() {
    while (m_live.load(std::memory_order_acquire) > 0) {
        yield();
    }
}


// ------------------------------------------------------------------
// Internal
// ------------------------------------------------------------------
ThreadPool::Task* ThreadPool::get_task(task_handle_t h) {
    const uint32_t id = handle_id(h);
    if (id >= kMaxTasks) return nullptr;
    Task& t = m_tasks[id];
    if (t.generation.load(std::memory_order_acquire) != handle_gen(h)) {
        return nullptr;
    }
    return &t;
}

task_handle_t ThreadPool::allocate_unlocked(std::function<void()> fn) {
    assert(m_free_count > 0);

    const uint32_t id = m_free_list[--m_free_count];
    Task& task = m_tasks[id];

    const uint32_t gen = task.generation.load(std::memory_order_relaxed);

    task.fn = std::move(fn);
    task.dependency_count.store(0, std::memory_order_relaxed);
    task.num_continuations = 0;
    task.status.store(task_status::Pending, std::memory_order_relaxed);

    m_live.fetch_add(1, std::memory_order_relaxed);

    return make_handle(id, gen);
}

void ThreadPool::free_unlocked(task_handle_t h) {
    const uint32_t id = handle_id(h);
    Task& task = m_tasks[id];

    uint32_t next = handle_gen(h) + 1;
    if (next == 0) next = 1;
    task.generation.store(next, std::memory_order_release);

    task.fn = nullptr;
    task.num_continuations = 0;
    task.dependency_count.store(0, std::memory_order_relaxed);
    task.status.store(task_status::Invalid, std::memory_order_relaxed);

    m_free_list[m_free_count++] = id;

    m_live.fetch_sub(1, std::memory_order_relaxed);
}

void ThreadPool::enqueue_ready(task_handle_t h) {
    Task* task = get_task(h);
    if (!task) return;

    if (task->status.load(std::memory_order_relaxed) != task_status::Pending) {
        return;
    }

    task->status.store(task_status::Ready, std::memory_order_relaxed);

    m_queue[m_queue_tail & kTaskMask] = h;
    ++m_queue_tail;

    m_cv.notify_one();
}

bool ThreadPool::try_pop(task_handle_t * handle) {
    std::lock_guard lock(m_mutex);

    if (m_queue_head == m_queue_tail) {
        return false;
    }

    *handle = m_queue[m_queue_head & kTaskMask];
    ++m_queue_head;
    return true;
}

void ThreadPool::execute(task_handle_t handle) 
{ 
    std::function<void()> fn;
    task_handle_t conts[kMaxContinuations] = {};
    uint32_t cont_count = 0;
    uint32_t gen = 0;

    {
        std::lock_guard lock(m_mutex);
        Task* task = get_task(handle);
        if (!task) return;

        task->status.store(task_status::InProgress, std::memory_order_relaxed);
        fn = std::move(task->fn);
        cont_count = task->num_continuations;
        for (uint32_t i = 0; i < cont_count; ++i) {
            conts[i] = task->continuations[i];
        }
        gen = handle_gen(handle);
    }

    if (fn) {
        fn();
    }

    {
        std::lock_guard lock(m_mutex);

        Task* task = get_task(handle);
        if (!task || task->generation.load(std::memory_order_relaxed) != gen) {
            return;
        }

        task->status.store(task_status::Done, std::memory_order_release);

        for (uint32_t i = 0; i < cont_count; ++i) {
            Task* child = get_task(conts[i]);
            if (!child) continue;

            const uint32_t left = child->dependency_count.fetch_sub(1, std::memory_order_acq_rel) - 1;
            if (left == 0) {
                enqueue_ready(conts[i]);
            }
        }

        free_unlocked(handle);
    }
}