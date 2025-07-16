# C++性能优化教程 - 基于Bitcoin Core实践

## 1. 内存管理优化

### 1.1 智能指针性能

#### 基本概念
智能指针虽然提供了自动内存管理，但也会带来一定的性能开销。

#### Bitcoin Core中的应用
```cpp
// 来自 context.h - 使用智能指针管理资源
struct NodeContext {
    std::unique_ptr<kernel::Context> kernel;
    std::unique_ptr<ECC_Context> ecc_context;
    std::unique_ptr<AddrMan> addrman;
    std::unique_ptr<CConnman> connman;
    // ...
};
```

#### 性能对比实践
```cpp
#include <memory>
#include <chrono>
#include <iostream>
#include <vector>

// 性能测试类
class PerformanceTest {
public:
    PerformanceTest() { std::cout << "PerformanceTest constructed\n"; }
    ~PerformanceTest() { std::cout << "PerformanceTest destroyed\n"; }

    void doWork() {
        // 模拟一些工作
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            sum += i;
        }
    }
};

// 测试1: 原始指针
void raw_pointer_test(int count) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < count; ++i) {
        PerformanceTest* ptr = new PerformanceTest();
        ptr->doWork();
        delete ptr;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Raw pointer: " << duration.count() << " microseconds\n";
}

// 测试2: unique_ptr
void unique_ptr_test(int count) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < count; ++i) {
        auto ptr = std::make_unique<PerformanceTest>();
        ptr->doWork();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Unique pointer: " << duration.count() << " microseconds\n";
}

// 测试3: shared_ptr
void shared_ptr_test(int count) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < count; ++i) {
        auto ptr = std::make_shared<PerformanceTest>();
        ptr->doWork();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Shared pointer: " << duration.count() << " microseconds\n";
}

void smart_pointer_performance_demo() {
    const int count = 10000;
    std::cout << "Performance test with " << count << " objects:\n";

    raw_pointer_test(count);
    unique_ptr_test(count);
    shared_ptr_test(count);
}
```

### 1.2 内存池

#### 基本概念
内存池通过预分配内存块来减少动态内存分配的开销。

#### 实践实现
```cpp
#include <vector>
#include <mutex>
#include <cstddef>

template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t initial_size = 10) {
        expand(initial_size);
    }

    T* acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_free_list.empty()) {
            expand(m_pool.size()); // 双倍扩展
        }

        T* obj = m_free_list.back();
        m_free_list.pop_back();
        return obj;
    }

    void release(T* obj) {
        std::lock_guard<std::mutex> lock(m_mutex);
        obj->~T(); // 调用析构函数
        m_free_list.push_back(obj);
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.size();
    }

    size_t free_count() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_free_list.size();
    }

private:
    void expand(size_t count) {
        size_t old_size = m_pool.size();
        m_pool.resize(old_size + count);

        // 将新分配的对象添加到空闲列表
        for (size_t i = old_size; i < m_pool.size(); ++i) {
            m_free_list.push_back(&m_pool[i]);
        }
    }

    mutable std::mutex m_mutex;
    std::vector<T> m_pool;
    std::vector<T*> m_free_list;
};

// 使用示例
class ExpensiveObject {
public:
    ExpensiveObject() {
        // 模拟昂贵的构造
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    ~ExpensiveObject() {
        // 模拟昂贵的析构
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    void doWork() {
        // 模拟工作
        volatile int sum = 0;
        for (int i = 0; i < 100; ++i) {
            sum += i;
        }
    }
};

void object_pool_performance_demo() {
    const int iterations = 1000;

    // 测试1: 直接分配
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto* obj = new ExpensiveObject();
        obj->doWork();
        delete obj;
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // 测试2: 对象池
    ObjectPool<ExpensiveObject> pool;
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto* obj = pool.acquire();
        obj->doWork();
        pool.release(obj);
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "Direct allocation: " << duration1.count() << "ms\n";
    std::cout << "Object pool: " << duration2.count() << "ms\n";
    std::cout << "Speedup: " << (double)duration1.count() / duration2.count() << "x\n";
}
```

## 2. 缓存优化

### 2.1 多级缓存

#### Bitcoin Core中的应用
```cpp
// 来自 caches.h
struct CacheSizes {
    IndexCacheSizes index;   // 索引缓存
    kernel::CacheSizes kernel; // 内核缓存
};
```

#### 实践实现
```cpp
#include <unordered_map>
#include <list>
#include <memory>

// LRU缓存实现
template<typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : m_capacity(capacity) {}

    V* get(const K& key) {
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return nullptr;
        }

        // 移动到列表前端
        m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.second);
        return &it->second.first;
    }

    void put(const K& key, const V& value) {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // 更新现有值
            it->second.first = value;
            m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.second);
            return;
        }

        // 检查容量
        if (m_cache.size() >= m_capacity) {
            // 移除最久未使用的项
            auto last = m_lru_list.back();
            m_cache.erase(last);
            m_lru_list.pop_back();
        }

        // 添加新项
        m_lru_list.push_front(key);
        m_cache[key] = std::make_pair(value, m_lru_list.begin());
    }

    size_t size() const {
        return m_cache.size();
    }

private:
    size_t m_capacity;
    std::list<K> m_lru_list;
    std::unordered_map<K, std::pair<V, typename std::list<K>::iterator>> m_cache;
};

// 多级缓存系统
template<typename K, typename V>
class MultiLevelCache {
public:
    MultiLevelCache(size_t l1_size, size_t l2_size)
        : m_l1_cache(l1_size), m_l2_cache(l2_size) {}

    V* get(const K& key) {
        // 检查L1缓存
        if (auto* value = m_l1_cache.get(key)) {
            return value;
        }

        // 检查L2缓存
        if (auto* value = m_l2_cache.get(key)) {
            // 提升到L1缓存
            m_l1_cache.put(key, *value);
            return value;
        }

        return nullptr;
    }

    void put(const K& key, const V& value) {
        m_l1_cache.put(key, value);
        m_l2_cache.put(key, value);
    }

private:
    LRUCache<K, V> m_l1_cache;
    LRUCache<K, V> m_l2_cache;
};

// 性能测试
void cache_performance_demo() {
    const int iterations = 100000;

    // 测试1: 无缓存
    auto start1 = std::chrono::high_resolution_clock::now();
    std::unordered_map<int, int> no_cache;
    for (int i = 0; i < iterations; ++i) {
        no_cache[i] = i * i;
        volatile int value = no_cache[i % 1000]; // 模拟查找
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // 测试2: LRU缓存
    auto start2 = std::chrono::high_resolution_clock::now();
    LRUCache<int, int> cache(1000);
    for (int i = 0; i < iterations; ++i) {
        cache.put(i, i * i);
        volatile int* value = cache.get(i % 1000);
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "No cache: " << duration1.count() << "ms\n";
    std::cout << "LRU cache: " << duration2.count() << "ms\n";
    std::cout << "Speedup: " << (double)duration1.count() / duration2.count() << "x\n";
}
```

### 2.2 缓存友好的数据结构

#### 实践实现
```cpp
#include <vector>
#include <array>

// 缓存友好的矩阵类
template<typename T, size_t N>
class CacheFriendlyMatrix {
public:
    // 使用行主序存储
    T& operator()(size_t row, size_t col) {
        return m_data[row * N + col];
    }

    const T& operator()(size_t row, size_t col) const {
        return m_data[row * N + col];
    }

    // 行遍历（缓存友好）
    void processByRow() {
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                m_data[i * N + j] *= 2;
            }
        }
    }

    // 列遍历（缓存不友好）
    void processByColumn() {
        for (size_t j = 0; j < N; ++j) {
            for (size_t i = 0; i < N; ++i) {
                m_data[i * N + j] *= 2;
            }
        }
    }

private:
    std::array<T, N * N> m_data{};
};

// 性能测试
void cache_friendly_performance_demo() {
    const size_t size = 1000;
    CacheFriendlyMatrix<int, size> matrix;

    // 初始化矩阵
    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < size; ++j) {
            matrix(i, j) = i + j;
        }
    }

    // 测试行遍历
    auto start1 = std::chrono::high_resolution_clock::now();
    matrix.processByRow();
    auto end1 = std::chrono::high_resolution_clock::now();

    // 测试列遍历
    auto start2 = std::chrono::high_resolution_clock::now();
    matrix.processByColumn();
    auto end2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "Row traversal: " << duration1.count() << "ms\n";
    std::cout << "Column traversal: " << duration2.count() << "ms\n";
    std::cout << "Speedup: " << (double)duration2.count() / duration1.count() << "x\n";
}
```

## 3. 算法优化

### 3.1 批量处理

#### Bitcoin Core中的应用
```cpp
// 来自 transaction.cpp - 批量处理交易
void processTransactions(const std::vector<CTransaction>& transactions) {
    // 批量验证
    for (const auto& tx : transactions) {
        // 批量处理逻辑
    }
}
```

#### 实践实现
```cpp
#include <vector>
#include <algorithm>
#include <chrono>

// 批量处理器
template<typename T>
class BatchProcessor {
public:
    explicit BatchProcessor(size_t batch_size) : m_batch_size(batch_size) {}

    void process(const std::vector<T>& items) {
        for (size_t i = 0; i < items.size(); i += m_batch_size) {
            size_t end = std::min(i + m_batch_size, items.size());
            processBatch(items.begin() + i, items.begin() + end);
        }
    }

private:
    template<typename Iterator>
    void processBatch(Iterator begin, Iterator end) {
        // 批量处理逻辑
        for (auto it = begin; it != end; ++it) {
            processItem(*it);
        }
    }

    void processItem(const T& item) {
        // 模拟处理单个项目
        volatile int sum = 0;
        for (int i = 0; i < 100; ++i) {
            sum += i;
        }
    }

    size_t m_batch_size;
};

// 性能测试
void batch_processing_demo() {
    const size_t total_items = 100000;
    const size_t batch_size = 1000;

    std::vector<int> items(total_items);
    std::generate(items.begin(), items.end(), []() { return rand(); });

    BatchProcessor<int> processor(batch_size);

    auto start = std::chrono::high_resolution_clock::now();
    processor.process(items);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Batch processing: " << duration.count() << "ms\n";
}
```

### 3.2 并行算法

#### 实践实现
```cpp
#include <execution>
#include <algorithm>
#include <vector>
#include <thread>

// 并行排序
void parallel_sort_demo() {
    const size_t size = 1000000;
    std::vector<int> data(size);
    std::generate(data.begin(), data.end(), []() { return rand(); });

    // 串行排序
    auto data1 = data;
    auto start1 = std::chrono::high_resolution_clock::now();
    std::sort(data1.begin(), data1.end());
    auto end1 = std::chrono::high_resolution_clock::now();

    // 并行排序
    auto data2 = data;
    auto start2 = std::chrono::high_resolution_clock::now();
    std::sort(std::execution::par, data2.begin(), data2.end());
    auto end2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "Sequential sort: " << duration1.count() << "ms\n";
    std::cout << "Parallel sort: " << duration2.count() << "ms\n";
    std::cout << "Speedup: " << (double)duration1.count() / duration2.count() << "x\n";
}

// 并行归约
template<typename T>
T parallel_reduce(const std::vector<T>& data) {
    return std::reduce(std::execution::par, data.begin(), data.end());
}

void parallel_reduce_demo() {
    const size_t size = 10000000;
    std::vector<int> data(size);
    std::generate(data.begin(), data.end(), []() { return rand() % 100; });

    // 串行归约
    auto start1 = std::chrono::high_resolution_clock::now();
    int sum1 = std::accumulate(data.begin(), data.end(), 0);
    auto end1 = std::chrono::high_resolution_clock::now();

    // 并行归约
    auto start2 = std::chrono::high_resolution_clock::now();
    int sum2 = parallel_reduce(data);
    auto end2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "Sequential reduce: " << duration1.count() << "ms, sum = " << sum1 << "\n";
    std::cout << "Parallel reduce: " << duration2.count() << "ms, sum = " << sum2 << "\n";
    std::cout << "Speedup: " << (double)duration1.count() / duration2.count() << "x\n";
}
```

## 4. 编译器优化

### 4.1 内联函数

#### 实践实现
```cpp
#include <chrono>

// 内联函数
inline int add(int a, int b) {
    return a + b;
}

// 非内联函数
int add_noinline(int a, int b) {
    return a + b;
}

// 性能测试
void inline_performance_demo() {
    const int iterations = 100000000;

    // 测试内联函数
    auto start1 = std::chrono::high_resolution_clock::now();
    volatile int sum1 = 0;
    for (int i = 0; i < iterations; ++i) {
        sum1 += add(i, i + 1);
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // 测试非内联函数
    auto start2 = std::chrono::high_resolution_clock::now();
    volatile int sum2 = 0;
    for (int i = 0; i < iterations; ++i) {
        sum2 += add_noinline(i, i + 1);
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "Inline function: " << duration1.count() << "ms\n";
    std::cout << "Non-inline function: " << duration2.count() << "ms\n";
    std::cout << "Speedup: " << (double)duration2.count() / duration1.count() << "x\n";
}
```

### 4.2 循环优化

#### 实践实现
```cpp
#include <vector>

// 循环展开
void loop_unrolling_demo() {
    const size_t size = 1000000;
    std::vector<int> data(size);

    // 普通循环
    auto start1 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < size; ++i) {
        data[i] = i * 2;
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // 循环展开
    auto start2 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < size; i += 4) {
        if (i + 3 < size) {
            data[i] = i * 2;
            data[i + 1] = (i + 1) * 2;
            data[i + 2] = (i + 2) * 2;
            data[i + 3] = (i + 3) * 2;
        } else {
            // 处理剩余元素
            for (size_t j = i; j < size; ++j) {
                data[j] = j * 2;
            }
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);

    std::cout << "Normal loop: " << duration1.count() << " microseconds\n";
    std::cout << "Unrolled loop: " << duration2.count() << " microseconds\n";
    std::cout << "Speedup: " << (double)duration1.count() / duration2.count() << "x\n";
}
```

## 5. 实践项目

### 项目1: 高性能缓存系统

```cpp
#include <unordered_map>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <thread>

template<typename K, typename V>
class HighPerformanceCache {
public:
    explicit HighPerformanceCache(size_t capacity) : m_capacity(capacity) {}

    // 读取操作（使用共享锁）
    std::optional<V> get(const K& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // 更新LRU列表
            m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.second);
            return it->second.first;
        }
        return std::nullopt;
    }

    // 写入操作（使用独占锁）
    void put(const K& key, const V& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // 更新现有值
            it->second.first = value;
            m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.second);
            return;
        }

        // 检查容量
        if (m_cache.size() >= m_capacity) {
            // 移除最久未使用的项
            auto last = m_lru_list.back();
            m_cache.erase(last);
            m_lru_list.pop_back();
        }

        // 添加新项
        m_lru_list.push_front(key);
        m_cache[key] = std::make_pair(value, m_lru_list.begin());
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_cache.size();
    }

private:
    size_t m_capacity;
    mutable std::shared_mutex m_mutex;
    std::list<K> m_lru_list;
    std::unordered_map<K, std::pair<V, typename std::list<K>::iterator>> m_cache;
};

// 性能测试
void high_performance_cache_demo() {
    HighPerformanceCache<int, std::string> cache(1000);
    std::vector<std::thread> threads;

    // 写入线程
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&cache, i]() {
            for (int j = 0; j < 1000; ++j) {
                cache.put(i * 1000 + j, "value" + std::to_string(i * 1000 + j));
            }
        });
    }

    // 读取线程
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&cache]() {
            for (int j = 0; j < 1000; ++j) {
                auto value = cache.get(j);
                if (value) {
                    // 处理值
                    volatile int sum = 0;
                    for (int k = 0; k < 10; ++k) {
                        sum += k;
                    }
                }
            }
        });
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "High performance cache test completed in " << duration.count() << "ms\n";
    std::cout << "Final cache size: " << cache.size() << "\n";
}
```

### 项目2: 内存池管理器

```cpp
#include <vector>
#include <mutex>
#include <cstddef>
#include <memory>

class MemoryPoolManager {
public:
    explicit MemoryPoolManager(size_t block_size, size_t initial_blocks = 10)
        : m_block_size(block_size) {
        expand(initial_blocks);
    }

    void* allocate() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_free_blocks.empty()) {
            expand(m_pools.size()); // 双倍扩展
        }

        void* block = m_free_blocks.back();
        m_free_blocks.pop_back();
        return block;
    }

    void deallocate(void* block) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_free_blocks.push_back(block);
    }

    size_t total_blocks() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t total = 0;
        for (const auto& pool : m_pools) {
            total += pool.size();
        }
        return total;
    }

    size_t free_blocks() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_free_blocks.size();
    }

private:
    void expand(size_t count) {
        m_pools.emplace_back(m_block_size, count);
        auto& new_pool = m_pools.back();

        // 将新池中的块添加到空闲列表
        for (size_t i = 0; i < count; ++i) {
            m_free_blocks.push_back(new_pool.get_block(i));
        }
    }

    class MemoryPool {
    public:
        MemoryPool(size_t block_size, size_t block_count)
            : m_block_size(block_size), m_block_count(block_count) {
            m_data = std::make_unique<uint8_t[]>(block_size * block_count);
        }

        void* get_block(size_t index) {
            return m_data.get() + index * m_block_size;
        }

        size_t size() const { return m_block_count; }

    private:
        size_t m_block_size;
        size_t m_block_count;
        std::unique_ptr<uint8_t[]> m_data;
    };

    size_t m_block_size;
    mutable std::mutex m_mutex;
    std::vector<MemoryPool> m_pools;
    std::vector<void*> m_free_blocks;
};

// 使用示例
void memory_pool_manager_demo() {
    const size_t block_size = 1024;
    const size_t total_allocations = 10000;

    MemoryPoolManager pool(block_size);
    std::vector<void*> allocations;

    auto start = std::chrono::high_resolution_clock::now();

    // 分配内存
    for (size_t i = 0; i < total_allocations; ++i) {
        allocations.push_back(pool.allocate());
    }

    // 释放内存
    for (void* ptr : allocations) {
        pool.deallocate(ptr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Memory pool manager: " << duration.count() << " microseconds\n";
    std::cout << "Total blocks: " << pool.total_blocks() << "\n";
    std::cout << "Free blocks: " << pool.free_blocks() << "\n";
}
```

## 总结

本教程涵盖了C++性能优化的核心概念：

1. **内存管理优化**: 智能指针性能、内存池
2. **缓存优化**: 多级缓存、缓存友好的数据结构
3. **算法优化**: 批量处理、并行算法
4. **编译器优化**: 内联函数、循环优化
5. **实践项目**: 高性能缓存系统、内存池管理器

这些优化技术在Bitcoin Core中得到了广泛应用，是高性能C++系统的重要基础。