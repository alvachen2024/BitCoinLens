# 高级C++特性教程 - 基于Bitcoin Core实践

## 1. 模板元编程 (Template Metaprogramming)

### 1.1 类型特征 (Type Traits)

#### Bitcoin Core中的应用
```cpp
// 来自 types.h - 类型特征的使用
template<typename T>
struct is_transaction_type {
    static constexpr bool value = false;
};

template<>
struct is_transaction_type<CTransaction> {
    static constexpr bool value = true;
};
```

#### 实践实现
```cpp
#include <type_traits>
#include <iostream>

// 自定义类型特征
template<typename T>
struct is_container {
    static constexpr bool value = false;
};

template<typename T>
struct is_container<std::vector<T>> {
    static constexpr bool value = true;
};

template<typename T>
struct is_container<std::list<T>> {
    static constexpr bool value = true;
};

// 条件编译
template<typename T>
class SmartContainer {
public:
    void add(const T& item) {
        if constexpr (is_container<T>::value) {
            // T是容器类型
            m_data.insert(m_data.end(), item.begin(), item.end());
        } else {
            // T是普通类型
            m_data.push_back(item);
        }
    }

    size_t size() const {
        return m_data.size();
    }

private:
    std::vector<typename std::conditional<
        is_container<T>::value,
        typename T::value_type,
        T
    >::type> m_data;
};

// 类型特征工具
template<typename T>
struct remove_const_ref {
    using type = std::remove_const_t<std::remove_reference_t<T>>;
};

template<typename T>
using remove_const_ref_t = typename remove_const_ref<T>::type;

// 使用示例
void type_traits_demo() {
    SmartContainer<int> int_container;
    SmartContainer<std::vector<int>> vector_container;

    int_container.add(42);
    vector_container.add({1, 2, 3, 4, 5});

    std::cout << "Int container size: " << int_container.size() << std::endl;
    std::cout << "Vector container size: " << vector_container.size() << std::endl;
}
```

### 1.2 SFINAE (Substitution Failure Is Not An Error)

#### 实践实现
```cpp
#include <type_traits>

// SFINAE示例
template<typename T>
auto has_size_method(const T& obj) -> decltype(obj.size(), std::true_type{}) {
    return std::true_type{};
}

template<typename T>
auto has_size_method(...) -> std::false_type {
    return std::false_type{};
}

// 使用SFINAE的函数重载
template<typename T>
auto getSize(const T& obj) -> decltype(obj.size()) {
    return obj.size();
}

template<typename T>
auto getSize(const T& obj) -> decltype(sizeof(obj)) {
    return sizeof(obj);
}

// 更现代的SFINAE写法
template<typename T>
auto modern_getSize(const T& obj) {
    if constexpr (requires { obj.size(); }) {
        return obj.size();
    } else {
        return sizeof(obj);
    }
}

// 概念（C++20）
template<typename T>
concept HasSize = requires(T t) {
    { t.size() } -> std::convertible_to<size_t>;
};

template<HasSize T>
size_t concept_getSize(const T& obj) {
    return obj.size();
}
```

## 2. 完美转发和通用引用

### 2.1 完美转发

#### Bitcoin Core中的应用
```cpp
// 来自 context.cpp - 完美转发的使用
template<typename... Args>
auto make_unique(Args&&... args) {
    return std::make_unique<ChainstateManager>(std::forward<Args>(args)...);
}
```

#### 实践实现
```cpp
#include <utility>
#include <memory>

// 完美转发包装器
template<typename T>
class PerfectForwardWrapper {
public:
    template<typename U>
    PerfectForwardWrapper(U&& u) : m_value(std::forward<U>(u)) {}

    T& get() { return m_value; }
    const T& get() const { return m_value; }

private:
    T m_value;
};

// 通用工厂函数
template<typename T, typename... Args>
std::unique_ptr<T> make_unique_perfect(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 可变参数完美转发
template<typename... Args>
void log_all(Args&&... args) {
    (std::cout << ... << std::forward<Args>(args)) << std::endl;
}

// 使用示例
void perfect_forwarding_demo() {
    // 左值
    std::string str = "Hello";
    PerfectForwardWrapper<std::string> w1(str);

    // 右值
    PerfectForwardWrapper<std::string> w2(std::string("World"));

    // 临时对象
    PerfectForwardWrapper<std::string> w3("Temporary");

    // 可变参数完美转发
    log_all("Hello", " ", "World", " ", 42);
}
```

### 2.2 通用引用

#### 实践实现
```cpp
#include <vector>
#include <string>

// 通用引用模板
template<typename T>
class UniversalReferenceExample {
public:
    // 通用引用构造函数
    template<typename U>
    UniversalReferenceExample(U&& u) : m_value(std::forward<U>(u)) {}

    // 通用引用赋值运算符
    template<typename U>
    UniversalReferenceExample& operator=(U&& u) {
        m_value = std::forward<U>(u);
        return *this;
    }

    const T& get() const { return m_value; }

private:
    T m_value;
};

// 通用引用函数
template<typename T>
void processValue(T&& value) {
    if constexpr (std::is_lvalue_reference_v<T>) {
        std::cout << "Processing lvalue reference\n";
    } else {
        std::cout << "Processing rvalue reference\n";
    }

    // 使用完美转发
    useValue(std::forward<T>(value));
}

template<typename T>
void useValue(T&& value) {
    // 处理值
    std::cout << "Using value\n";
}
```

## 3. 移动语义和右值引用

### 3.1 移动语义

#### Bitcoin Core中的应用
```cpp
// 来自 transaction.h - 移动语义的使用
CTransactionRef GetTransaction(const CBlockIndex* const block_index,
                              const CTxMemPool* const mempool,
                              const uint256& hash,
                              uint256& hashBlock,
                              const BlockManager& blockman);
```

#### 实践实现
```cpp
#include <vector>
#include <string>

// 移动语义类
class MoveSemanticsExample {
public:
    // 默认构造函数
    MoveSemanticsExample() : m_data(1000, 'A') {
        std::cout << "Default constructor\n";
    }

    // 拷贝构造函数
    MoveSemanticsExample(const MoveSemanticsExample& other)
        : m_data(other.m_data) {
        std::cout << "Copy constructor\n";
    }

    // 移动构造函数
    MoveSemanticsExample(MoveSemanticsExample&& other) noexcept
        : m_data(std::move(other.m_data)) {
        std::cout << "Move constructor\n";
    }

    // 拷贝赋值运算符
    MoveSemanticsExample& operator=(const MoveSemanticsExample& other) {
        if (this != &other) {
            m_data = other.m_data;
            std::cout << "Copy assignment\n";
        }
        return *this;
    }

    // 移动赋值运算符
    MoveSemanticsExample& operator=(MoveSemanticsExample&& other) noexcept {
        if (this != &other) {
            m_data = std::move(other.m_data);
            std::cout << "Move assignment\n";
        }
        return *this;
    }

    // 析构函数
    ~MoveSemanticsExample() {
        std::cout << "Destructor\n";
    }

private:
    std::vector<char> m_data;
};

// 移动语义函数
MoveSemanticsExample createObject() {
    return MoveSemanticsExample{}; // 返回值优化(RVO)
}

void move_semantics_demo() {
    std::cout << "Creating object 1:\n";
    auto obj1 = createObject();

    std::cout << "\nCreating object 2:\n";
    auto obj2 = std::move(obj1); // 移动构造

    std::cout << "\nCreating object 3:\n";
    MoveSemanticsExample obj3;
    obj3 = std::move(obj2); // 移动赋值
}
```

### 3.2 右值引用

#### 实践实现
```cpp
#include <utility>

// 右值引用函数
void processRvalueReference(std::string&& str) {
    std::cout << "Processing rvalue: " << str << std::endl;
    // str在这里被移动
}

void processLvalueReference(const std::string& str) {
    std::cout << "Processing lvalue: " << str << std::endl;
    // str在这里被拷贝
}

// 通用处理函数
template<typename T>
void processUniversal(T&& value) {
    if constexpr (std::is_lvalue_reference_v<T>) {
        processLvalueReference(value);
    } else {
        processRvalueReference(std::move(value));
    }
}

// 移动迭代器
void move_iterator_demo() {
    std::vector<std::string> source = {"Hello", "World", "C++"};
    std::vector<std::string> destination;

    // 使用移动迭代器
    destination.assign(
        std::make_move_iterator(source.begin()),
        std::make_move_iterator(source.end())
    );

    std::cout << "Source size: " << source.size() << std::endl;
    std::cout << "Destination size: " << destination.size() << std::endl;
}
```

## 4. Lambda表达式和函数对象

### 4.1 高级Lambda表达式

#### Bitcoin Core中的应用
```cpp
// 来自 timeoffsets.cpp - Lambda表达式的使用
std::sort(vEvictionCandidates.begin(), vEvictionCandidates.end(),
    [](const NodeEvictionCandidate& a, const NodeEvictionCandidate& b) {
        return a.m_connected < b.m_connected;
    });
```

#### 实践实现
```cpp
#include <vector>
#include <algorithm>
#include <functional>

// 泛型Lambda (C++14)
void generic_lambda_demo() {
    auto print = [](const auto& item) {
        std::cout << item << " ";
    };

    std::vector<int> ints = {1, 2, 3, 4, 5};
    std::vector<std::string> strings = {"Hello", "World"};

    std::for_each(ints.begin(), ints.end(), print);
    std::for_each(strings.begin(), strings.end(), print);
    std::cout << std::endl;
}

// 捕获this的Lambda
class LambdaCaptureExample {
public:
    LambdaCaptureExample(int value) : m_value(value) {}

    void processData(const std::vector<int>& data) {
        // 捕获this
        std::for_each(data.begin(), data.end(),
            [this](int item) {
                m_value += item;
            });
    }

    int getValue() const { return m_value; }

private:
    int m_value;
};

// 递归Lambda
void recursive_lambda_demo() {
    std::function<int(int)> factorial = [&factorial](int n) {
        return n <= 1 ? 1 : n * factorial(n - 1);
    };

    std::cout << "Factorial of 5: " << factorial(5) << std::endl;
}

// Lambda作为返回值
auto createMultiplier(int factor) {
    return [factor](int value) { return value * factor; };
}

void lambda_return_demo() {
    auto doubleIt = createMultiplier(2);
    auto tripleIt = createMultiplier(3);

    std::cout << "Double 5: " << doubleIt(5) << std::endl;
    std::cout << "Triple 5: " << tripleIt(5) << std::endl;
}
```

### 4.2 函数对象

#### 实践实现
```cpp
#include <functional>

// 自定义函数对象
class CustomFunctionObject {
public:
    explicit CustomFunctionObject(int threshold) : m_threshold(threshold) {}

    bool operator()(int value) const {
        return value > m_threshold;
    }

private:
    int m_threshold;
};

// 函数对象模板
template<typename T>
class Comparator {
public:
    bool operator()(const T& a, const T& b) const {
        return a < b;
    }
};

// 状态函数对象
class StatefulFunctionObject {
public:
    StatefulFunctionObject() : m_count(0) {}

    int operator()(int value) {
        m_count++;
        return value + m_count;
    }

    int getCount() const { return m_count; }

private:
    int m_count;
};

void function_object_demo() {
    std::vector<int> numbers = {1, 5, 3, 8, 2, 9, 4};

    // 使用自定义函数对象
    CustomFunctionObject greaterThan5(5);
    auto it = std::find_if(numbers.begin(), numbers.end(), greaterThan5);

    if (it != numbers.end()) {
        std::cout << "First number greater than 5: " << *it << std::endl;
    }

    // 使用状态函数对象
    StatefulFunctionObject stateful;
    std::vector<int> transformed;
    std::transform(numbers.begin(), numbers.end(),
                   std::back_inserter(transformed), stateful);

    std::cout << "Transformed count: " << stateful.getCount() << std::endl;
}
```

## 5. 智能指针高级用法

### 5.1 自定义删除器

#### 实践实现
```cpp
#include <memory>
#include <iostream>

// 自定义删除器
struct FileDeleter {
    void operator()(FILE* file) const {
        if (file) {
            fclose(file);
            std::cout << "File closed by custom deleter\n";
        }
    }
};

struct ArrayDeleter {
    void operator()(int* ptr) const {
        delete[] ptr;
        std::cout << "Array deleted by custom deleter\n";
    }
};

// 使用自定义删除器
void custom_deleter_demo() {
    // 文件句柄
    std::unique_ptr<FILE, FileDeleter> file(fopen("test.txt", "w"));
    if (file) {
        fprintf(file.get(), "Hello, World!\n");
    }

    // 数组
    std::unique_ptr<int[], ArrayDeleter> array(new int[10]);
    for (int i = 0; i < 10; ++i) {
        array[i] = i;
    }
}

// Lambda删除器
void lambda_deleter_demo() {
    auto lambda_deleter = [](int* ptr) {
        delete[] ptr;
        std::cout << "Deleted by lambda deleter\n";
    };

    std::unique_ptr<int, decltype(lambda_deleter)> ptr(new int[5], lambda_deleter);
}
```

### 5.2 智能指针的转换

#### 实践实现
```cpp
#include <memory>

// 基类和派生类
class Base {
public:
    virtual ~Base() = default;
    virtual void print() const { std::cout << "Base\n"; }
};

class Derived : public Base {
public:
    void print() const override { std::cout << "Derived\n"; }
};

void smart_pointer_conversion_demo() {
    // 创建派生类对象
    auto derived = std::make_unique<Derived>();

    // 转换为基类指针
    std::unique_ptr<Base> base = std::move(derived);
    base->print();

    // shared_ptr的转换
    auto shared_derived = std::make_shared<Derived>();
    std::shared_ptr<Base> shared_base = shared_derived;

    // 动态转换
    if (auto* ptr = dynamic_cast<Derived*>(shared_base.get())) {
        ptr->print();
    }
}
```

## 6. 实践项目

### 项目1: 高级模板容器

```cpp
#include <vector>
#include <memory>
#include <type_traits>

// 高级模板容器
template<typename T, typename Allocator = std::allocator<T>>
class AdvancedContainer {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = typename std::allocator_traits<Allocator>::size_type;

    // 构造函数
    AdvancedContainer() = default;

    explicit AdvancedContainer(const Allocator& alloc) : m_alloc(alloc) {}

    AdvancedContainer(size_type count, const T& value, const Allocator& alloc = Allocator())
        : m_alloc(alloc) {
        m_data.resize(count, value);
    }

    // 移动构造函数
    AdvancedContainer(AdvancedContainer&& other) noexcept
        : m_data(std::move(other.m_data)), m_alloc(std::move(other.m_alloc)) {}

    // 移动赋值运算符
    AdvancedContainer& operator=(AdvancedContainer&& other) noexcept {
        if (this != &other) {
            m_data = std::move(other.m_data);
            m_alloc = std::move(other.m_alloc);
        }
        return *this;
    }

    // 元素访问
    T& operator[](size_type index) {
        return m_data[index];
    }

    const T& operator[](size_type index) const {
        return m_data[index];
    }

    // 容量操作
    void reserve(size_type size) {
        m_data.reserve(size);
    }

    size_type capacity() const {
        return m_data.capacity();
    }

    size_type size() const {
        return m_data.size();
    }

    bool empty() const {
        return m_data.empty();
    }

    // 修改器
    void push_back(const T& value) {
        m_data.push_back(value);
    }

    void push_back(T&& value) {
        m_data.push_back(std::move(value));
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        m_data.emplace_back(std::forward<Args>(args)...);
    }

    void clear() {
        m_data.clear();
    }

    // 迭代器
    auto begin() { return m_data.begin(); }
    auto end() { return m_data.end(); }
    auto begin() const { return m_data.begin(); }
    auto end() const { return m_data.end(); }

    // 算法支持
    template<typename Predicate>
    void remove_if(Predicate&& pred) {
        m_data.erase(
            std::remove_if(m_data.begin(), m_data.end(), std::forward<Predicate>(pred)),
            m_data.end()
        );
    }

    template<typename Compare>
    void sort(Compare&& comp) {
        std::sort(m_data.begin(), m_data.end(), std::forward<Compare>(comp));
    }

private:
    std::vector<T, Allocator> m_data;
    Allocator m_alloc;
};

// 使用示例
void advanced_container_demo() {
    AdvancedContainer<int> container;

    // 添加元素
    container.push_back(3);
    container.push_back(1);
    container.push_back(4);
    container.push_back(1);
    container.push_back(5);

    // 排序
    container.sort([](int a, int b) { return a < b; });

    // 移除元素
    container.remove_if([](int x) { return x == 1; });

    // 遍历
    for (int value : container) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
}
```

### 项目2: 类型安全的回调系统

```cpp
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <memory>

// 类型安全的回调系统
class TypeSafeCallbackSystem {
public:
    // 注册回调
    template<typename T>
    void registerCallback(const std::string& name, std::function<void(const T&)> callback) {
        m_callbacks[std::type_index(typeid(T))][name] =
            [callback](const void* data) {
                callback(*static_cast<const T*>(data));
            };
    }

    // 触发回调
    template<typename T>
    void triggerCallback(const std::string& name, const T& data) {
        auto type_it = m_callbacks.find(std::type_index(typeid(T)));
        if (type_it != m_callbacks.end()) {
            auto callback_it = type_it->second.find(name);
            if (callback_it != type_it->second.end()) {
                callback_it->second(&data);
            }
        }
    }

    // 移除回调
    template<typename T>
    void removeCallback(const std::string& name) {
        auto type_it = m_callbacks.find(std::type_index(typeid(T)));
        if (type_it != m_callbacks.end()) {
            type_it->second.erase(name);
        }
    }

    // 检查回调是否存在
    template<typename T>
    bool hasCallback(const std::string& name) const {
        auto type_it = m_callbacks.find(std::type_index(typeid(T)));
        if (type_it != m_callbacks.end()) {
            return type_it->second.find(name) != type_it->second.end();
        }
        return false;
    }

private:
    using CallbackFunction = std::function<void(const void*)>;
    using CallbackMap = std::unordered_map<std::string, CallbackFunction>;
    using TypeCallbackMap = std::unordered_map<std::type_index, CallbackMap>;

    TypeCallbackMap m_callbacks;
};

// 使用示例
void type_safe_callback_demo() {
    TypeSafeCallbackSystem callbackSystem;

    // 注册不同类型的回调
    callbackSystem.registerCallback<int>("print_int", [](const int& value) {
        std::cout << "Integer: " << value << std::endl;
    });

    callbackSystem.registerCallback<std::string>("print_string", [](const std::string& value) {
        std::cout << "String: " << value << std::endl;
    });

    // 触发回调
    callbackSystem.triggerCallback("print_int", 42);
    callbackSystem.triggerCallback("print_string", "Hello, World!");

    // 检查回调是否存在
    std::cout << "Has int callback: " << callbackSystem.hasCallback<int>("print_int") << std::endl;
    std::cout << "Has string callback: " << callbackSystem.hasCallback<std::string>("print_string") << std::endl;
}
```

## 总结

本教程涵盖了高级C++特性的核心概念：

1. **模板元编程**: 类型特征、SFINAE、概念
2. **完美转发**: 通用引用、std::forward
3. **移动语义**: 右值引用、移动构造函数
4. **Lambda表达式**: 泛型Lambda、捕获、递归
5. **智能指针**: 自定义删除器、类型转换
6. **实践项目**: 高级模板容器、类型安全回调系统

这些特性在Bitcoin Core中得到了广泛应用，是现代C++开发的重要基础。