# 现代C++基础教程 - 基于Bitcoin Core实践

## 1. 智能指针 (Smart Pointers)

### 1.1 unique_ptr - 独占所有权

#### 基本概念
`std::unique_ptr` 是C++11引入的智能指针，提供独占所有权语义，确保资源在对象销毁时自动释放。

#### Bitcoin Core中的应用
```cpp
// 来自 context.h
struct NodeContext {
    // 内核组件 - 独占所有权
    std::unique_ptr<kernel::Context> kernel;
    std::unique_ptr<ECC_Context> ecc_context;

    // 网络组件 - 独占所有权
    std::unique_ptr<AddrMan> addrman;
    std::unique_ptr<CConnman> connman;
    std::unique_ptr<PeerManager> peerman;
    std::unique_ptr<BanMan> banman;
};
```

#### 实践练习
```cpp
#include <memory>
#include <iostream>

class Resource {
public:
    Resource() { std::cout << "Resource created\n"; }
    ~Resource() { std::cout << "Resource destroyed\n"; }
    void use() { std::cout << "Resource used\n"; }
};

// 练习1: 基本使用
void basic_unique_ptr_demo() {
    // 创建unique_ptr
    std::unique_ptr<Resource> ptr1 = std::make_unique<Resource>();

    // 使用资源
    ptr1->use();

    // 自动释放（作用域结束）
}

// 练习2: 转移所有权
void transfer_ownership_demo() {
    std::unique_ptr<Resource> ptr1 = std::make_unique<Resource>();

    // 转移所有权
    std::unique_ptr<Resource> ptr2 = std::move(ptr1);

    // ptr1现在是nullptr
    if (!ptr1) {
        std::cout << "ptr1 is now nullptr\n";
    }

    // ptr2拥有资源
    ptr2->use();
}

// 练习3: 函数返回
std::unique_ptr<Resource> create_resource() {
    return std::make_unique<Resource>();
}

void return_unique_ptr_demo() {
    auto resource = create_resource();
    resource->use();
}
```

### 1.2 shared_ptr - 共享所有权

#### 基本概念
`std::shared_ptr` 允许多个指针共享同一个对象，使用引用计数管理生命周期。

#### Bitcoin Core中的应用
```cpp
// 来自 transaction.h
CTransactionRef GetTransaction(const CBlockIndex* const block_index,
                              const CTxMemPool* const mempool,
                              const uint256& hash,
                              uint256& hashBlock,
                              const BlockManager& blockman);
```

#### 实践练习
```cpp
#include <memory>
#include <vector>

class SharedResource {
public:
    SharedResource(int id) : m_id(id) {
        std::cout << "SharedResource " << m_id << " created\n";
    }
    ~SharedResource() {
        std::cout << "SharedResource " << m_id << " destroyed\n";
    }
    int getId() const { return m_id; }
private:
    int m_id;
};

// 练习1: 基本共享
void basic_shared_ptr_demo() {
    // 创建shared_ptr
    std::shared_ptr<SharedResource> ptr1 = std::make_shared<SharedResource>(1);

    // 复制shared_ptr
    std::shared_ptr<SharedResource> ptr2 = ptr1;
    std::shared_ptr<SharedResource> ptr3 = ptr1;

    std::cout << "Reference count: " << ptr1.use_count() << std::endl; // 3

    // 使用资源
    ptr1->getId();
    ptr2->getId();
    ptr3->getId();
}

// 练习2: 在容器中使用
void shared_ptr_in_container() {
    std::vector<std::shared_ptr<SharedResource>> resources;

    // 添加资源
    resources.push_back(std::make_shared<SharedResource>(1));
    resources.push_back(std::make_shared<SharedResource>(2));

    // 共享资源
    auto shared_resource = resources[0];
    resources.push_back(shared_resource);

    std::cout << "Resource 1 reference count: " << shared_resource.use_count() << std::endl;
}
```

### 1.3 weak_ptr - 弱引用

#### 基本概念
`std::weak_ptr` 提供对shared_ptr管理对象的弱引用，不增加引用计数，避免循环引用。

#### 实践练习
```cpp
#include <memory>

class Node {
public:
    std::string name;
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev; // 使用weak_ptr避免循环引用

    Node(const std::string& n) : name(n) {}
};

void weak_ptr_demo() {
    auto node1 = std::make_shared<Node>("Node1");
    auto node2 = std::make_shared<Node>("Node2");

    // 建立双向链接
    node1->next = node2;
    node2->prev = node1;

    // 检查weak_ptr是否有效
    if (auto prev = node2->prev.lock()) {
        std::cout << "Previous node: " << prev->name << std::endl;
    }
}
```

## 2. 移动语义 (Move Semantics)

### 2.1 右值引用

#### 基本概念
右值引用(`&&`)允许我们"移动"而不是"复制"资源，提高性能。

#### Bitcoin Core中的应用
```cpp
// 来自 eviction.h
std::optional<NodeId> SelectNodeToEvict(std::vector<NodeEvictionCandidate>&& vEvictionCandidates);
```

#### 实践练习
```cpp
#include <vector>
#include <string>

class LargeObject {
public:
    LargeObject(size_t size) : m_data(size, 'A') {
        std::cout << "LargeObject constructed with size " << size << std::endl;
    }

    // 移动构造函数
    LargeObject(LargeObject&& other) noexcept
        : m_data(std::move(other.m_data)) {
        std::cout << "LargeObject moved" << std::endl;
    }

    // 移动赋值运算符
    LargeObject& operator=(LargeObject&& other) noexcept {
        if (this != &other) {
            m_data = std::move(other.m_data);
            std::cout << "LargeObject move assigned" << std::endl;
        }
        return *this;
    }

    // 禁用拷贝
    LargeObject(const LargeObject&) = delete;
    LargeObject& operator=(const LargeObject&) = delete;

private:
    std::vector<char> m_data;
};

// 练习1: 移动语义基础
void move_semantics_basic() {
    LargeObject obj1(1000000);

    // 移动构造
    LargeObject obj2 = std::move(obj1);

    // 移动赋值
    LargeObject obj3(500000);
    obj3 = std::move(obj2);
}

// 练习2: 在函数中使用移动语义
LargeObject create_large_object() {
    return LargeObject(1000000); // 返回值优化(RVO)
}

void move_semantics_function() {
    auto obj = create_large_object(); // 移动构造
}
```

### 2.2 完美转发 (Perfect Forwarding)

#### 基本概念
`std::forward` 保持参数的值类别（左值/右值），实现完美转发。

#### 实践练习
```cpp
#include <utility>

// 通用引用模板
template<typename T>
class Wrapper {
public:
    T value;

    // 完美转发构造函数
    template<typename U>
    Wrapper(U&& u) : value(std::forward<U>(u)) {}
};

// 练习: 完美转发
void perfect_forwarding_demo() {
    std::string str = "Hello";

    // 左值
    Wrapper<std::string> w1(str);

    // 右值
    Wrapper<std::string> w2(std::string("World"));

    // 临时对象
    Wrapper<std::string> w3("Temporary");
}
```

## 3. Lambda表达式

### 3.1 基本语法

#### Bitcoin Core中的应用
```cpp
// 来自 timeoffsets.cpp
std::sort(vEvictionCandidates.begin(), vEvictionCandidates.end(),
    [](const NodeEvictionCandidate& a, const NodeEvictionCandidate& b) {
        return a.m_connected < b.m_connected;
    });
```

#### 实践练习
```cpp
#include <vector>
#include <algorithm>
#include <iostream>

// 练习1: 基本lambda
void basic_lambda_demo() {
    std::vector<int> numbers = {3, 1, 4, 1, 5, 9, 2, 6};

    // 排序
    std::sort(numbers.begin(), numbers.end(),
        [](int a, int b) { return a < b; });

    // 打印
    std::for_each(numbers.begin(), numbers.end(),
        [](int n) { std::cout << n << " "; });
    std::cout << std::endl;
}

// 练习2: 捕获变量
void lambda_capture_demo() {
    int multiplier = 10;

    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // 值捕获
    std::for_each(numbers.begin(), numbers.end(),
        [multiplier](int n) {
            std::cout << n * multiplier << " ";
        });
    std::cout << std::endl;

    // 引用捕获
    int sum = 0;
    std::for_each(numbers.begin(), numbers.end(),
        [&sum](int n) { sum += n; });

    std::cout << "Sum: " << sum << std::endl;
}

// 练习3: 泛型lambda (C++14)
void generic_lambda_demo() {
    auto print = [](const auto& item) {
        std::cout << item << " ";
    };

    std::vector<int> ints = {1, 2, 3};
    std::vector<std::string> strings = {"hello", "world"};

    std::for_each(ints.begin(), ints.end(), print);
    std::for_each(strings.begin(), strings.end(), print);
}
```

## 4. 类型推导 (Type Deduction)

### 4.1 auto关键字

#### Bitcoin Core中的应用
```cpp
// 来自 context.cpp
auto chainman = std::make_unique<ChainstateManager>(...);
auto mempool = std::make_unique<CTxMemPool>(...);
```

#### 实践练习
```cpp
#include <vector>
#include <map>
#include <string>

// 练习1: 基本auto使用
void auto_basic_demo() {
    // 自动推导类型
    auto i = 42;                    // int
    auto d = 3.14;                  // double
    auto s = "hello";               // const char*
    auto str = std::string("world"); // std::string

    // 在循环中使用
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    for (auto& num : numbers) {
        num *= 2;
    }
}

// 练习2: auto与容器
void auto_container_demo() {
    std::map<std::string, int> scores = {
        {"Alice", 100},
        {"Bob", 85},
        {"Charlie", 92}
    };

    // 自动推导迭代器类型
    for (const auto& pair : scores) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    // 使用结构化绑定 (C++17)
    for (const auto& [name, score] : scores) {
        std::cout << name << ": " << score << std::endl;
    }
}
```

### 4.2 decltype关键字

#### 实践练习
```cpp
#include <type_traits>

// 练习1: 基本decltype
void decltype_basic_demo() {
    int x = 42;
    decltype(x) y = x; // y的类型与x相同

    std::vector<int> vec = {1, 2, 3};
    decltype(vec)::value_type element = 10; // int
}

// 练习2: decltype(auto)
template<typename T>
decltype(auto) get_value(T& container) {
    return container[0]; // 保持返回类型的引用性质
}

void decltype_auto_demo() {
    std::vector<int> vec = {1, 2, 3};
    decltype(auto) value = get_value(vec); // int&
}
```

## 5. 统一初始化 (Uniform Initialization)

### 5.1 列表初始化

#### Bitcoin Core中的应用
```cpp
// 来自 types.h
struct BlockCreateOptions {
    bool use_mempool{true};                                    // 成员初始化
    size_t block_reserved_weight{DEFAULT_BLOCK_RESERVED_WEIGHT};
    size_t coinbase_output_max_additional_sigops{400};
    CScript coinbase_output_script{CScript() << OP_TRUE};
};
```

#### 实践练习
```cpp
#include <vector>
#include <map>
#include <string>

// 练习1: 基本统一初始化
void uniform_init_basic() {
    // 各种类型的统一初始化
    int x{42};
    double d{3.14};
    std::string s{"hello"};

    // 容器初始化
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::map<std::string, int> map{{"a", 1}, {"b", 2}};

    // 数组初始化
    int arr[]{1, 2, 3, 4, 5};
}

// 练习2: 类成员初始化
class Point {
public:
    int x{0};  // 成员初始化
    int y{0};

    Point() = default;
    Point(int x, int y) : x{x}, y{y} {}
};

void class_init_demo() {
    Point p1{};           // 默认初始化
    Point p2{10, 20};     // 构造函数初始化
}
```

## 6. 实践项目

### 项目1: 简单的资源管理器

```cpp
#include <memory>
#include <vector>
#include <iostream>

// 资源类
class Resource {
public:
    Resource(int id) : m_id(id) {
        std::cout << "Resource " << m_id << " created\n";
    }
    ~Resource() {
        std::cout << "Resource " << m_id << " destroyed\n";
    }
    int getId() const { return m_id; }
private:
    int m_id;
};

// 资源管理器
class ResourceManager {
public:
    // 添加资源
    void addResource(int id) {
        m_resources.push_back(std::make_unique<Resource>(id));
    }

    // 获取资源数量
    size_t getResourceCount() const {
        return m_resources.size();
    }

    // 使用所有资源
    void useAllResources() {
        for (const auto& resource : m_resources) {
            std::cout << "Using resource " << resource->getId() << std::endl;
        }
    }

private:
    std::vector<std::unique_ptr<Resource>> m_resources;
};

// 主函数
int main() {
    ResourceManager manager;

    // 添加资源
    manager.addResource(1);
    manager.addResource(2);
    manager.addResource(3);

    std::cout << "Resource count: " << manager.getResourceCount() << std::endl;

    // 使用资源
    manager.useAllResources();

    return 0;
}
```

### 项目2: 线程安全的缓存

```cpp
#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>
#include <iostream>

template<typename K, typename V>
class ThreadSafeCache {
public:
    // 获取值
    std::shared_ptr<V> get(const K& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second;
        }
        return nullptr;
    }

    // 设置值
    void set(const K& key, std::shared_ptr<V> value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache[key] = value;
    }

    // 移除值
    void remove(const K& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.erase(key);
    }

    // 获取大小
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.size();
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<K, std::shared_ptr<V>> m_cache;
};

// 使用示例
int main() {
    ThreadSafeCache<std::string, int> cache;

    // 设置值
    cache.set("key1", std::make_shared<int>(42));
    cache.set("key2", std::make_shared<int>(100));

    // 获取值
    if (auto value = cache.get("key1")) {
        std::cout << "key1: " << *value << std::endl;
    }

    std::cout << "Cache size: " << cache.size() << std::endl;

    return 0;
}
```

## 总结

本教程涵盖了现代C++的核心特性：

1. **智能指针**: 自动内存管理，避免内存泄漏
2. **移动语义**: 提高性能，减少不必要的拷贝
3. **Lambda表达式**: 函数式编程，简化代码
4. **类型推导**: 减少冗余代码，提高可读性
5. **统一初始化**: 一致的初始化语法

这些特性在Bitcoin Core中得到了广泛应用，是现代C++开发的重要基础。