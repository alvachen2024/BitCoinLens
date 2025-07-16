# C++并发和线程编程教程 - 基于Bitcoin Core实践

## 1. 线程基础

### 1.1 线程创建和管理

#### 基本概念
C++11引入了标准线程库，提供了跨平台的线程支持。

#### Bitcoin Core中的应用
```cpp
// 来自 context.h
struct NodeContext {
    std::thread background_init_thread;          // 后台初始化线程
    std::atomic<int> exit_status{EXIT_SUCCESS};  // 原子退出状态
};
```

#### 实践练习
```cpp
#include <thread>
#include <iostream>
#include <chrono>

// 基本线程函数
void worker_function(int id) {
    std::cout << "Worker " << id << " started" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Worker " << id << " finished" << std::endl;
}

// 练习1: 基本线程创建
void basic_thread_demo() {
    std::cout << "Main thread started" << std::endl;

    // 创建线程
    std::thread worker(worker_function, 1);

    // 等待线程完成
    worker.join();

    std::cout << "Main thread finished" << std::endl;
}

// 练习2: 多个线程
void multiple_threads_demo() {
    std::vector<std::thread> threads;

    // 创建多个线程
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker_function, i);
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
}

// 练习3: 线程分离
void detached_thread_demo() {
    std::thread worker(worker_function, 999);
    worker.detach(); // 线程在后台运行

    std::cout << "Main thread continues..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}
```

### 1.2 线程安全的数据访问

#### 互斥锁 (Mutex)

#### Bitcoin Core中的应用
```cpp
// 来自 timeoffsets.h
class TimeOffsets {
private:
    mutable Mutex m_mutex;                                    // 互斥锁
    std::deque<std::chrono::seconds> m_offsets;              // 时间偏移队列
    node::Warnings& m_warnings;                               // 警告管理器
};
```

#### 实践练习
```cpp
#include <mutex>
#include <vector>
#include <thread>

// 线程安全的计数器
class ThreadSafeCounter {
public:
    void increment() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_count++;
    }

    int get() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

private:
    mutable std::mutex m_mutex;
    int m_count{0};
};

// 练习1: 基本互斥锁使用
void mutex_basic_demo() {
    ThreadSafeCounter counter;
    std::vector<std::thread> threads;

    // 创建多个线程同时增加计数器
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 1000; ++j) {
                counter.increment();
            }
        });
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Final count: " << counter.get() << std::endl;
}

// 练习2: 递归互斥锁
class RecursiveCounter {
public:
    void increment() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_count++;
        if (m_count % 10 == 0) {
            printCount(); // 递归调用
        }
    }

    void printCount() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        std::cout << "Count: " << m_count << std::endl;
    }

private:
    mutable std::recursive_mutex m_mutex;
    int m_count{0};
};
```

### 1.3 条件变量 (Condition Variables)

#### 基本概念
条件变量用于线程间的同步，允许线程等待特定条件满足。

#### 实践练习
```cpp
#include <condition_variable>
#include <queue>

// 线程安全的队列
template<typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(value));
        m_cv.notify_one();
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty(); });
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_cv;
};

// 生产者-消费者模式
void producer_consumer_demo() {
    ThreadSafeQueue<int> queue;

    // 生产者线程
    std::thread producer([&queue]() {
        for (int i = 0; i < 10; ++i) {
            queue.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // 消费者线程
    std::thread consumer([&queue]() {
        for (int i = 0; i < 10; ++i) {
            int value;
            queue.pop(value);
            std::cout << "Consumed: " << value << std::endl;
        }
    });

    producer.join();
    consumer.join();
}
```

## 2. 原子操作

### 2.1 原子变量

#### Bitcoin Core中的应用
```cpp
// 来自 context.h
struct NodeContext {
    std::atomic<int> exit_status{EXIT_SUCCESS};  // 原子退出状态
};
```

#### 实践练习
```cpp
#include <atomic>
#include <thread>
#include <vector>

// 原子计数器
class AtomicCounter {
public:
    void increment() {
        m_count.fetch_add(1, std::memory_order_relaxed);
    }

    int get() const {
        return m_count.load(std::memory_order_relaxed);
    }

    void reset() {
        m_count.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<int> m_count{0};
};

// 练习1: 原子操作性能对比
void atomic_performance_demo() {
    AtomicCounter atomic_counter;
    ThreadSafeCounter mutex_counter;

    std::vector<std::thread> threads;

    // 测试原子操作
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&atomic_counter]() {
            for (int j = 0; j < 100000; ++j) {
                atomic_counter.increment();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto atomic_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Atomic counter: " << atomic_counter.get()
              << " in " << atomic_duration.count() << "ms" << std::endl;
}

// 练习2: 内存序 (Memory Ordering)
class MemoryOrderDemo {
public:
    void set_flag() {
        m_flag.store(true, std::memory_order_release);
    }

    bool check_flag() {
        return m_flag.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> m_flag{false};
};
```

### 2.2 内存屏障

#### 实践练习
```cpp
#include <atomic>

// 双重检查锁定模式
class Singleton {
public:
    static Singleton* getInstance() {
        Singleton* instance = s_instance.load(std::memory_order_acquire);
        if (!instance) {
            std::lock_guard<std::mutex> lock(s_mutex);
            instance = s_instance.load(std::memory_order_relaxed);
            if (!instance) {
                instance = new Singleton();
                s_instance.store(instance, std::memory_order_release);
            }
        }
        return instance;
    }

private:
    Singleton() = default;

    static std::atomic<Singleton*> s_instance;
    static std::mutex s_mutex;
};

std::atomic<Singleton*> Singleton::s_instance{nullptr};
std::mutex Singleton::s_mutex;
```

## 3. 异步编程

### 3.1 std::async 和 std::future

#### 基本概念
`std::async` 提供了一种简单的方式来异步执行任务并获取结果。

#### 实践练习
```cpp
#include <future>
#include <vector>
#include <algorithm>

// 异步计算斐波那契数列
int fibonacci(int n) {
    if (n < 2) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 练习1: 基本异步任务
void async_basic_demo() {
    // 启动异步任务
    std::future<int> future = std::async(std::launch::async, fibonacci, 10);

    // 做其他工作
    std::cout << "Doing other work..." << std::endl;

    // 获取结果
    int result = future.get();
    std::cout << "Fibonacci(10) = " << result << std::endl;
}

// 练习2: 多个异步任务
void multiple_async_demo() {
    std::vector<std::future<int>> futures;

    // 启动多个异步任务
    for (int i = 0; i < 5; ++i) {
        futures.push_back(std::async(std::launch::async, fibonacci, i + 10));
    }

    // 收集所有结果
    for (auto& future : futures) {
        std::cout << "Result: " << future.get() << std::endl;
    }
}

// 练习3: 异步任务链
std::future<int> async_chain_demo() {
    auto future1 = std::async(std::launch::async, []() { return 42; });
    auto future2 = std::async(std::launch::async, [](int x) { return x * 2; }, future1.get());
    return std::async(std::launch::async, [](int x) { return x + 10; }, future2.get());
}
```

### 3.2 std::promise 和 std::future

#### 实践练习
```cpp
#include <promise>

// 异步任务执行器
class AsyncExecutor {
public:
    template<typename Func, typename... Args>
    std::future<typename std::result_of<Func(Args...)>::type>
    execute(Func&& func, Args&&... args) {
        using ReturnType = typename std::result_of<Func(Args...)>::type;

        std::promise<ReturnType> promise;
        std::future<ReturnType> future = promise.get_future();

        std::thread([promise = std::move(promise),
                     func = std::forward<Func>(func),
                     ...args = std::forward<Args>(args)]() mutable {
            try {
                promise.set_value(func(args...));
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        }).detach();

        return future;
    }
};

// 使用示例
void promise_future_demo() {
    AsyncExecutor executor;

    auto future = executor.execute([](int x, int y) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        return x + y;
    }, 10, 20);

    std::cout << "Waiting for result..." << std::endl;
    int result = future.get();
    std::cout << "Result: " << result << std::endl;
}
```

## 4. 线程池

### 4.1 基本线程池实现

#### 实践练习
```cpp
#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : m_stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            m_workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_cv.wait(lock, [this] {
                            return m_stop || !m_tasks.empty();
                        });

                        if (m_stop && m_tasks.empty()) {
                            return;
                        }

                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using ReturnType = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_cv.notify_one();
        return result;
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto& worker : m_workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop;
};

// 使用示例
void thread_pool_demo() {
    ThreadPool pool(4);
    std::vector<std::future<int>> results;

    // 提交任务
    for (int i = 0; i < 8; ++i) {
        results.emplace_back(pool.enqueue([i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return i * i;
        }));
    }

    // 获取结果
    for (auto& result : results) {
        std::cout << "Result: " << result.get() << std::endl;
    }
}
```

## 5. 锁的层次结构

### 5.1 避免死锁

#### 实践练习
```cpp
#include <mutex>
#include <thread>

// 层次锁
class HierarchicalMutex {
public:
    explicit HierarchicalMutex(unsigned long value) : m_hierarchy_value(value) {}

    void lock() {
        check_for_hierarchy_violation();
        m_internal_mutex.lock();
        update_hierarchy_value();
    }

    void unlock() {
        m_this_thread_hierarchy_value = m_previous_hierarchy_value;
        m_internal_mutex.unlock();
    }

    bool try_lock() {
        check_for_hierarchy_violation();
        if (!m_internal_mutex.try_lock()) {
            return false;
        }
        update_hierarchy_value();
        return true;
    }

private:
    void check_for_hierarchy_violation() {
        if (m_this_thread_hierarchy_value <= m_hierarchy_value) {
            throw std::logic_error("mutex hierarchy violated");
        }
    }

    void update_hierarchy_value() {
        m_previous_hierarchy_value = m_this_thread_hierarchy_value;
        m_this_thread_hierarchy_value = m_hierarchy_value;
    }

    std::mutex m_internal_mutex;
    unsigned long const m_hierarchy_value;
    unsigned long m_previous_hierarchy_value{0};

    static thread_local unsigned long m_this_thread_hierarchy_value;
};

thread_local unsigned long HierarchicalMutex::m_this_thread_hierarchy_value{ULONG_MAX};

// 使用示例
void hierarchical_mutex_demo() {
    HierarchicalMutex high_level_mutex(10000);
    HierarchicalMutex low_level_mutex(5000);

    std::thread([&]() {
        std::lock_guard<HierarchicalMutex> high_lock(high_level_mutex);
        std::lock_guard<HierarchicalMutex> low_lock(low_level_mutex);
        // 正确的锁顺序
    }).join();

    std::thread([&]() {
        try {
            std::lock_guard<HierarchicalMutex> low_lock(low_level_mutex);
            std::lock_guard<HierarchicalMutex> high_lock(high_level_mutex);
            // 错误的锁顺序，会抛出异常
        } catch (const std::logic_error& e) {
            std::cout << "Caught exception: " << e.what() << std::endl;
        }
    }).join();
}
```

## 6. 实践项目

### 项目1: 并发缓存系统

```cpp
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <thread>

template<typename K, typename V>
class ConcurrentCache {
public:
    // 读取操作（共享锁）
    std::optional<V> get(const K& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // 写入操作（独占锁）
    void put(const K& key, const V& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_cache[key] = value;
    }

    // 删除操作（独占锁）
    bool remove(const K& key) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        return m_cache.erase(key) > 0;
    }

    // 清空缓存（独占锁）
    void clear() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_cache.clear();
    }

    // 获取大小（共享锁）
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_cache.size();
    }

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<K, V> m_cache;
};

// 使用示例
void concurrent_cache_demo() {
    ConcurrentCache<std::string, int> cache;
    std::vector<std::thread> threads;

    // 写入线程
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&cache, i]() {
            for (int j = 0; j < 100; ++j) {
                cache.put("key" + std::to_string(i * 100 + j), i * 100 + j);
            }
        });
    }

    // 读取线程
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&cache]() {
            for (int j = 0; j < 100; ++j) {
                auto value = cache.get("key" + std::to_string(j));
                if (value) {
                    // 处理值
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Cache size: " << cache.size() << std::endl;
}
```

### 项目2: 异步任务调度器

```cpp
#include <functional>
#include <chrono>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>

class AsyncScheduler {
public:
    using Task = std::function<void()>;
    using TimePoint = std::chrono::steady_clock::time_point;

    AsyncScheduler() : m_stop(false) {
        m_scheduler_thread = std::thread([this] { run_scheduler(); });
    }

    ~AsyncScheduler() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_one();
        if (m_scheduler_thread.joinable()) {
            m_scheduler_thread.join();
        }
    }

    // 延迟执行任务
    void schedule_after(std::chrono::milliseconds delay, Task task) {
        auto execution_time = std::chrono::steady_clock::now() + delay;
        schedule_at(execution_time, std::move(task));
    }

    // 在指定时间执行任务
    void schedule_at(TimePoint time, Task task) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.emplace(time, std::move(task));
        m_cv.notify_one();
    }

private:
    void run_scheduler() {
        while (!m_stop) {
            std::unique_lock<std::mutex> lock(m_mutex);

            if (m_tasks.empty()) {
                m_cv.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                continue;
            }

            auto now = std::chrono::steady_clock::now();
            auto next_task = m_tasks.begin();

            if (next_task->first <= now) {
                // 执行任务
                Task task = std::move(next_task->second);
                m_tasks.erase(next_task);
                lock.unlock();

                task();
            } else {
                // 等待下一个任务
                m_cv.wait_until(lock, next_task->first);
            }
        }
    }

    std::thread m_scheduler_thread;
    std::multimap<TimePoint, Task> m_tasks;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop;
};

// 使用示例
void scheduler_demo() {
    AsyncScheduler scheduler;

    // 延迟执行任务
    scheduler.schedule_after(std::chrono::milliseconds(1000), []() {
        std::cout << "Task executed after 1 second" << std::endl;
    });

    scheduler.schedule_after(std::chrono::milliseconds(2000), []() {
        std::cout << "Task executed after 2 seconds" << std::endl;
    });

    // 等待任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}
```

## 总结

本教程涵盖了C++并发编程的核心概念：

1. **线程基础**: 线程创建、管理和同步
2. **互斥锁**: 保护共享数据，避免竞态条件
3. **条件变量**: 线程间通信和同步
4. **原子操作**: 无锁编程，提高性能
5. **异步编程**: 非阻塞任务执行
6. **线程池**: 高效的任务调度
7. **锁层次**: 避免死锁的设计模式

这些技术在Bitcoin Core中得到了广泛应用，是高性能并发系统的重要基础。