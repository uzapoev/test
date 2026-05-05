#pragma once

#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>
#include <condition_variable>

#define MAX_TASKS           128u //1024u //
#define MAX_DEPENDENCIES    16u
#define MAX_CONTINUATIONS   16u
#define MASK                MAX_TASKS - 1u
#define TASK_SIZE_BYTES     128


enum eTaskStatus : uint16_t
{
    pending,
    inprogress,
    done
};

struct Task
{
    std::function<void(void*)>  function;
    char                        data[TASK_SIZE_BYTES]   = "";
    char *                      userdata                = nullptr;

    uint64_t                    time_stamp              = 0;
    std::atomic<uint16_t>       num_pending;
    uint16_t                    num_continuations       = 0;
    uint16_t                    num_dependencies        = 0;
    Task*                       dependencies[MAX_DEPENDENCIES] = {};
    Task*                       continuations[MAX_CONTINUATIONS] = {};
};


struct WorkQueue
{
    WorkQueue()  {}
    ~WorkQueue() {}
    
    Task* allocate()
    {
        uint32_t task_index = m_num_tasks++;
        return &m_task_pool[task_index & (MAX_TASKS - 1u)];
    }

    void push(Task* task)
    {
        std::lock_guard<std::mutex> lock(m_critical_section);
        
        for(uint32_t i = 0; i < m_back; ++i)
        {
            if(m_task_queue[i] == task)
                return;
        }
        
        m_task_queue[m_back & MASK] = task;

        ++m_back;
        m_num_pending_tasks++;
    }
    
    
    Task* pop()
    {
       std::lock_guard<std::mutex> lock(m_critical_section);
        
        const uint32_t job_count = m_back;
        if (job_count == 0)
            return nullptr;
        
        --m_back;
        Task * task  = m_task_queue[m_back & MASK];
        m_task_queue[m_back & MASK] = nullptr;
        return task;
    }
    

    bool has_pending_tasks()
    {
        return (m_num_pending_tasks != 0);
    }
    
    
    bool empty()
    {
        return (m_back == 0);
    }

//private:
    std::mutex              m_critical_section;
    Task                    m_task_pool[MAX_TASKS]  = {};
    Task*                   m_task_queue[MAX_TASKS] = {};
    uint32_t                m_back                  = 0;
    //std::atomic_int         m_back = 0;
    uint32_t                m_num_tasks             = 0;
    std::atomic<uint32_t>   m_num_pending_tasks     = 0;
};
    
// -----------------------------------------------------------------------------------------------------------------------------------
    
struct WorkerThread
{
    WorkerThread()  = default;
    WorkerThread(const WorkerThread &) {};

    ~WorkerThread()
    {
        wakeup();
        m_thread.join();
    }

    inline void wakeup()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_signal = true;
        m_condition.notify_all();
    }

    inline void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&] { return m_signal; });
        m_signal = false;
    }

    std::thread                 m_thread;
    std::mutex                  m_mutex;
    std::condition_variable     m_condition;
    bool                        m_signal = false;
};


class ThreadPool
{
public:
    ThreadPool()
    {
        m_shutdown = false;
        m_num_logical_threads = std::thread::hardware_concurrency();
        m_num_worker_threads = m_num_logical_threads;
        initialize_workers();
    }

    ThreadPool(uint32_t workers)
    {
        m_shutdown = false;
        m_num_logical_threads = std::thread::hardware_concurrency();
        m_num_worker_threads = std::min(workers, m_num_logical_threads);
        
        initialize_workers();
    }

    ~ThreadPool()
    {
        m_shutdown = true;
        m_worker_threads.clear();
    }

    inline Task* allocate(std::function<void(void*)> fn, char * userdata)
    {
        Task* task_ptr = m_queue.allocate();
        task_ptr->num_pending = 1;
        task_ptr->num_continuations = 0;
        task_ptr->num_dependencies = 0;
        task_ptr->function = fn;
        return task_ptr;
    }
    

    inline void define_dependency(Task* child, Task* parent)
    {
        if (parent && child)
            child->dependencies[child->num_dependencies++] = parent;
    }

    inline void define_continuation(Task* parent, Task* continuation)
    {
        if (parent && continuation)
        {
            if (parent->num_continuations < MAX_CONTINUATIONS)
            {
                parent->continuations[parent->num_continuations] = continuation;
                parent->num_continuations++;
            }
        }
    }

    inline void enqueue(Task* task)
    {
        if (task)
        {
            m_queue.push(task);
    
            for(uint32_t i = 0; i < m_num_worker_threads; i++)
            {
                WorkerThread& thread = m_worker_threads[i];
                thread.wakeup();
            }
        }
    }

    inline bool is_done(Task* task)
    {
        return task->num_pending == 0;
    }
    

    inline void wait_for_all()
    {
        while (m_queue.has_pending_tasks())
        {
            Task* task = m_queue.pop();
            
            if (task)
                run_task(task);
            else
                std::this_thread::yield();
        }
    }
    

    inline void wait_for_one(Task* pending_task)
    {
        while (pending_task->num_pending > 0)
        {
            Task* task = m_queue.pop();
            
            if (task)
                run_task(task);
        }
    }
    

    inline uint32_t num_logical_threads()       { return m_num_logical_threads; }
    inline uint32_t num_worker_threads()        { return m_num_worker_threads; }

private:

    inline void initialize_workers()
    {
        m_worker_threads.resize(m_num_worker_threads);
        
        for (uint32_t i = 0; i < m_num_worker_threads; i++)
            m_worker_threads[i].m_thread = std::thread(&ThreadPool::worker, this, i);
    }

    inline void worker(uint32_t index)
    {
        WorkerThread& worker_thread = m_worker_threads[index];
    
        while (!m_shutdown)
        {
            Task* task = m_queue.pop();
    
            if (task)
            {
                run_task(task);
                continue;
            }

            worker_thread.wait();
        }
    }

    inline void run_task(Task* task)
    {
        // Wait untill all of dependent tasks are done
        wait_for_dependencies(task);
    
        // Execute the current task
        task->function(task->data);
    
        // Submit continuation tasks.
        for (uint32_t i = 0; i < task->num_continuations; i++)
            enqueue(task->continuations[i]);
    
        if (task->num_pending > 0)
            task->num_pending--;
    
        if (m_queue.m_num_pending_tasks > 0)
            m_queue.m_num_pending_tasks--;
    }
    
// -----------------------------------------------------------------------------------------------------------------------------------

    inline void wait_for_dependencies(Task* task)
    {
        if (task->num_dependencies > 0)
        {
            for (uint32_t i = 0; i < task->num_dependencies; i++)
            {
                while (task->dependencies[i]->num_pending > 0)
                {
                    Task* wait_task = m_queue.pop();
    
                    if (wait_task)
                        run_task(wait_task);
                }
            }
        }
    }

private:
    bool                       m_shutdown;
    uint32_t                   m_num_logical_threads;
    WorkQueue                  m_queue;
    std::vector<WorkerThread>  m_worker_threads;
    uint32_t                   m_num_worker_threads;
};


class thread_pool
{
public:
    typedef uint64_t task_handle_t;
    struct task_t
    {
        std::atomic<uint32_t>               status;     // 0 pending to start, 1 in progress, 2 done
        std::function<void(void* data)>     function;
        void* data;
        task_t* dependencies[32];
    };

    struct thread_handle
    {
        static uint64_t make_handle(uint32_t idx, uint32_t timestamp)
        {
            thread_handle thandle;
            thandle.idx = idx;
            thandle.timestamp = timestamp;
            return thandle.h;
        }
        union
        {
            uint64_t    h;
            struct {
                uint32_t idx, timestamp;
            };
        };
    };

    ~thread_pool()
    {
        stop();
    }

    void init()
    {
        int m_num_logical_threads = std::thread::hardware_concurrency();
        m_threads.reserve(m_num_logical_threads);
        for (int i = 0; i < m_num_logical_threads; ++i)
        {
            m_threads.emplace_back(std::thread([this](int index) {
                int jobs_done = 0;
                while (!m_stop)
                {
                    auto task = m_task_queue.pop();
                    if (task != nullptr)
                    {
           //             wait_for_dependencies(task);

                        task->function(task->data);
                        jobs_done++;

                        if (m_task_queue.m_num_pending_tasks > 0)
                            m_task_queue.m_num_pending_tasks--;
                    }
                    else
                    {
                        std::unique_lock<std::mutex> locker(m_mutex);
                        m_cond_var.wait(locker/*, [&]{return m_task_queue.m_num_pending_tasks != 0;}*/);
                    }
                }
                printf("\nthread %d is finished, done %d jobs", index, jobs_done);
                }, i));
        }
    }/**/

    void stop()
    {
        m_stop = true;
        m_cond_var.notify_all();

        for (auto& thread : m_threads)
            thread.join();
    }

    task_handle_t create_task(std::function<void(void* data)> fn, void* userdata)
    {
        auto task = m_task_queue.allocate();
        task->function = fn;
        task->userdata = (char*)userdata;
        task->time_stamp = time(0);
        m_task_queue.push(task);
       // task->status.store(0);
        return 0;
    }

    void chain(task_handle_t master, task_handle_t slave)
    {
    }

    void enqueue(task_handle_t handle)
    {
        std::scoped_lock lock(m_mutex);
        m_cond_var.notify_all();
    }

    void wait_for(task_handle_t* handles, uint32_t count)
    {
        /*   task* pending_task
           while (pending_task->num_pending > 0)
           {
               Task* task = m_queue.pop();

               if (task)
                   run_task(task);
           }*/
    }

    void wait_for_all()
    {
        m_cond_var.notify_all();
        while (!m_task_queue.empty() || m_task_queue.m_num_pending_tasks != 0)
        {
            std::this_thread::yield();
        }
    }

private:
    std::atomic_bool            m_stop;
    std::mutex                  m_mutex;
    std::condition_variable     m_cond_var;
    std::vector<std::thread>    m_threads;

    WorkQueue                   m_task_queue;
};

static float s_result = 0.0f;

void payload()
{
    float sum = 0.0;
    for(int i = 0; i < 10'000; ++i)
        sum += math::distance(math::make_vec3(s_result, sum, sum), math::make_vec3(sum, s_result, sum));
    s_result += sum * 0.01f;
    printf("%f", s_result);
}


size_t thread_id()
{
    std::hash<std::thread::id> thread_hasher;
    return thread_hasher(std::this_thread::get_id());
}


void AnimationPreTransformUpdateTask(void* data)
{
    printf("\n\tAnimationPreTransformUpdateTask, %zd ", thread_id());
}

void TransformUpdateTask(void* data)
{
    printf("\n\tTransformUpdateTask, %zd ", thread_id());
    payload();
}

void PhysicsSyncTask(void* data)
{
    printf("\n\tPhysicsSyncTask, %zd ", thread_id());
    payload();
}

void AnimationPostTransformUpdateTask(void* data)
{
     printf("\n\tAnimationPostTransformUpdateTask, %zd ", thread_id());
     payload();
}

void AudioListenerUpdateTask(void* data)
{
    printf("\n\tAudioListenerUpdateTask, %zd ", thread_id());
    payload();
}

void AudioSourceUpdateTask(void* data)
{
    printf("\n\tAudioSourceUpdateTask, %zd ", thread_id());
    payload();
}

void ParticleUpdateTask(void* data)
{
    printf("\n\tParticleUpdateTask, %zd ", thread_id());
    payload();
}

void ScriptUpdateTask(void* data)
{
    printf("\n\tScriptUpdateTask, %zd ", thread_id());
    payload();
}


void ecs_update(ThreadPool& tp)
{
    Task* animation_pre_transform_update_task   = tp.allocate(AnimationPreTransformUpdateTask, nullptr);
    Task* transform_update_task                 = tp.allocate(TransformUpdateTask, nullptr);
    Task* physics_sync_task                     = tp.allocate(PhysicsSyncTask, nullptr);
    Task* animation_post_transform_update_task  = tp.allocate(AnimationPostTransformUpdateTask, nullptr);
    Task* audio_listener_update_task            = tp.allocate(AudioListenerUpdateTask, nullptr);
    Task* audio_source_update_task              = tp.allocate(AudioSourceUpdateTask, nullptr);
    Task* particle_update_task                  = tp.allocate(ParticleUpdateTask, nullptr);
    Task* script_update_task                    = tp.allocate(ScriptUpdateTask, nullptr);


    // Continuations
    tp.define_continuation(animation_pre_transform_update_task, transform_update_task);
    tp.define_continuation(animation_pre_transform_update_task, physics_sync_task);

    tp.define_continuation(transform_update_task, animation_post_transform_update_task);
    tp.define_continuation(transform_update_task, audio_listener_update_task);
    tp.define_continuation(transform_update_task, audio_source_update_task);
    tp.define_continuation(transform_update_task, particle_update_task);

    tp.define_continuation(animation_post_transform_update_task, script_update_task);

    // Dependencies
    tp.define_dependency(script_update_task, audio_listener_update_task);
    tp.define_dependency(script_update_task, audio_source_update_task);
    tp.define_dependency(script_update_task, particle_update_task);

    tp.enqueue(animation_pre_transform_update_task);

    tp.wait_for_all();
}



void thread_test()
{
    thread_pool tp0;
    tp0.init();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
/*
    auto animation_pre_transform_update_task    = tp0.create_task(AnimationPreTransformUpdateTask, nullptr);
    auto transform_update_task                  = tp0.create_task(TransformUpdateTask, nullptr);
    auto physics_sync_task                      = tp0.create_task(PhysicsSyncTask, nullptr);
    auto animation_post_transform_update_task   = tp0.create_task(AnimationPostTransformUpdateTask, nullptr);
    auto audio_listener_update_task             = tp0.create_task(AudioListenerUpdateTask, nullptr);
    auto audio_source_update_task               = tp0.create_task(AudioSourceUpdateTask, nullptr);
    auto particle_update_task                   = tp0.create_task(ParticleUpdateTask, nullptr);
    auto script_update_task                     = tp0.create_task(ScriptUpdateTask, nullptr);



    {
        measure ms("\nsingle");
        AnimationPreTransformUpdateTask(nullptr);
    }

    {
        measure ms("\nraw");

        AnimationPreTransformUpdateTask(nullptr);
        TransformUpdateTask(nullptr);
        PhysicsSyncTask(nullptr);
        AnimationPostTransformUpdateTask(nullptr);
        AudioListenerUpdateTask(nullptr);
        AudioSourceUpdateTask(nullptr);
        ParticleUpdateTask(nullptr);
        ScriptUpdateTask(nullptr);
    }

    {
        measure ms("\ntp");
        tp0.wait_for_all();
    }*/
    // tppp.stop();
     //std::this_thread::sleep_for(std::chrono::seconds(2));

    ThreadPool tp;
    {
         measure ms("\necs_update");
         ecs_update(tp);
    } /**/
    printf("");
}

static void foo()
{
    thread_pool pool;

    auto handle_0 = pool.create_task([](void* data) {}, nullptr);
    auto handle_1 = pool.create_task([](void* data) {}, nullptr);

    pool.chain(handle_0, handle_1);

    pool.enqueue(handle_0);
    pool.wait_for_all();
}
