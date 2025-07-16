# C++错误处理和安全编程教程 - 基于Bitcoin Core实践

## 1. 异常安全保证

### 1.1 异常安全级别

#### 基本概念
C++中的异常安全有三个级别：基本保证、强保证和不抛异常保证。

#### Bitcoin Core中的应用
```cpp
// 来自 transaction.h - 异常安全的交易处理
[[nodiscard]] TransactionError BroadcastTransaction(NodeContext& node,
                                                   CTransactionRef tx,
                                                   std::string& err_string,
                                                   const CAmount& max_tx_fee,
                                                   bool relay,
                                                   bool wait_callback);
```

#### 实践实现
```cpp
#include <memory>
#include <vector>
#include <stdexcept>

// 基本保证 - 资源不会泄漏
class BasicGuarantee {
private:
    std::unique_ptr<int[]> m_data;
    size_t m_size;

public:
    BasicGuarantee(size_t size) : m_size(size) {
        m_data = std::make_unique<int[]>(size);
        // 如果这里抛出异常，m_data会自动清理
    }

    ~BasicGuarantee() = default;

    // 基本保证：如果抛出异常，对象仍然处于有效状态
    void modify(size_t index, int value) {
        if (index >= m_size) {
            throw std::out_of_range("Index out of range");
        }
        m_data[index] = value;
    }
};

// 强保证 - 操作要么完全成功，要么完全失败
class StrongGuarantee {
private:
    std::vector<int> m_data;

public:
    // 强保证：如果抛出异常，对象状态不变
    void addElement(int value) {
        std::vector<int> new_data = m_data; // 创建副本
        new_data.push_back(value);          // 在副本上操作
        m_data = std::move(new_data);       // 原子交换
    }

    // 另一种实现方式：使用RAII
    void addElementRAII(int value) {
        auto old_size = m_data.size();
        m_data.push_back(value);

        // 如果后续操作失败，可以回滚
        try {
            validateData(); // 可能抛出异常的操作
        } catch (...) {
            m_data.resize(old_size); // 回滚
            throw;
        }
    }

private:
    void validateData() {
        // 模拟验证操作
        if (m_data.size() > 1000) {
            throw std::runtime_error("Data too large");
        }
    }
};

// 不抛异常保证 - 操作永远不会抛出异常
class NoThrowGuarantee {
public:
    // 不抛异常保证
    void swap(NoThrowGuarantee& other) noexcept {
        std::swap(m_data, other.m_data);
    }

    // 移动构造函数通常不抛异常
    NoThrowGuarantee(NoThrowGuarantee&& other) noexcept
        : m_data(std::move(other.m_data)) {}

    // 移动赋值运算符通常不抛异常
    NoThrowGuarantee& operator=(NoThrowGuarantee&& other) noexcept {
        if (this != &other) {
            m_data = std::move(other.m_data);
        }
        return *this;
    }

private:
    std::vector<int> m_data;
};
```

### 1.2 RAII和异常安全

#### 实践实现
```cpp
#include <fstream>
#include <memory>

// RAII文件句柄
class FileHandle {
public:
    explicit FileHandle(const std::string& filename) {
        m_file = fopen(filename.c_str(), "r");
        if (!m_file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
    }

    ~FileHandle() {
        if (m_file) {
            fclose(m_file);
        }
    }

    // 禁用拷贝
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // 允许移动
    FileHandle(FileHandle&& other) noexcept : m_file(other.m_file) {
        other.m_file = nullptr;
    }

    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            if (m_file) fclose(m_file);
            m_file = other.m_file;
            other.m_file = nullptr;
        }
        return *this;
    }

    FILE* get() const { return m_file; }

private:
    FILE* m_file;
};

// 异常安全的资源管理器
class ResourceManager {
public:
    ResourceManager() = default;

    // 强异常安全保证
    void addResource(std::unique_ptr<FileHandle> resource) {
        std::vector<std::unique_ptr<FileHandle>> new_resources = m_resources;
        new_resources.push_back(std::move(resource));
        m_resources = std::move(new_resources); // 原子操作
    }

    // 基本异常安全保证
    void removeResource(size_t index) {
        if (index >= m_resources.size()) {
            throw std::out_of_range("Index out of range");
        }
        m_resources.erase(m_resources.begin() + index);
    }

    size_t size() const noexcept {
        return m_resources.size();
    }

private:
    std::vector<std::unique_ptr<FileHandle>> m_resources;
};
```

## 2. 错误处理策略

### 2.1 返回值错误处理

#### Bitcoin Core中的应用
```cpp
// 来自 types.h - 错误类型枚举
enum class TransactionError {
    OK,                    // 无错误
    MISSING_INPUTS,        // 缺少输入
    ALREADY_IN_UTXO_SET,   // 已在UTXO集合中
    MEMPOOL_REJECTED,      // 内存池拒绝
    MEMPOOL_ERROR,         // 内存池错误
    MAX_FEE_EXCEEDED,      // 超过最大费用
    MAX_BURN_EXCEEDED,     // 超过最大销毁量
    INVALID_PACKAGE,       // 无效包
};
```

#### 实践实现
```cpp
#include <optional>
#include <variant>
#include <string>

// 使用std::optional进行错误处理
class OptionalErrorHandler {
public:
    std::optional<int> divide(int a, int b) {
        if (b == 0) {
            return std::nullopt; // 表示错误
        }
        return a / b;
    }

    std::optional<std::string> readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return std::nullopt;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }
};

// 使用std::variant进行错误处理
class VariantErrorHandler {
public:
    using Result = std::variant<int, std::string>; // 成功值或错误消息

    Result divide(int a, int b) {
        if (b == 0) {
            return std::string("Division by zero");
        }
        return a / b;
    }

    Result parseNumber(const std::string& str) {
        try {
            return std::stoi(str);
        } catch (const std::exception& e) {
            return std::string("Parse error: ") + e.what();
        }
    }
};

// 使用错误码进行错误处理
class ErrorCodeHandler {
public:
    enum class ErrorCode {
        SUCCESS = 0,
        INVALID_INPUT,
        OUT_OF_MEMORY,
        NETWORK_ERROR,
        TIMEOUT
    };

    struct Result {
        ErrorCode code;
        std::string message;
        int value;
    };

    Result processData(const std::string& input) {
        if (input.empty()) {
            return {ErrorCode::INVALID_INPUT, "Empty input", 0};
        }

        try {
            int value = std::stoi(input);
            return {ErrorCode::SUCCESS, "", value};
        } catch (const std::exception& e) {
            return {ErrorCode::INVALID_INPUT, e.what(), 0};
        }
    }
};
```

### 2.2 异常处理

#### 实践实现
```cpp
#include <exception>
#include <stdexcept>

// 自定义异常类
class BitcoinException : public std::runtime_error {
public:
    explicit BitcoinException(const std::string& message)
        : std::runtime_error(message) {}
};

class TransactionException : public BitcoinException {
public:
    explicit TransactionException(const std::string& message)
        : BitcoinException("Transaction error: " + message) {}
};

class NetworkException : public BitcoinException {
public:
    explicit NetworkException(const std::string& message)
        : BitcoinException("Network error: " + message) {}
};

// 异常安全的函数
class ExceptionSafeFunctions {
public:
    // 强异常安全保证
    void processTransaction(const std::string& tx_data) {
        auto old_state = m_state; // 保存旧状态

        try {
            validateTransaction(tx_data);
            executeTransaction(tx_data);
            commitTransaction();
        } catch (...) {
            m_state = old_state; // 回滚状态
            throw;
        }
    }

    // 基本异常安全保证
    void processData(const std::vector<int>& data) {
        for (size_t i = 0; i < data.size(); ++i) {
            try {
                processItem(data[i]);
            } catch (const std::exception& e) {
                // 记录错误但继续处理
                logError("Error processing item " + std::to_string(i) + ": " + e.what());
            }
        }
    }

private:
    void validateTransaction(const std::string& tx_data) {
        if (tx_data.empty()) {
            throw TransactionException("Empty transaction data");
        }
        // 其他验证逻辑
    }

    void executeTransaction(const std::string& tx_data) {
        // 执行交易逻辑
        if (tx_data.length() > 1000) {
            throw TransactionException("Transaction too large");
        }
    }

    void commitTransaction() {
        // 提交交易
        m_state = "committed";
    }

    void processItem(int item) {
        if (item < 0) {
            throw std::invalid_argument("Negative item");
        }
        // 处理项目
    }

    void logError(const std::string& error) {
        // 记录错误
        std::cerr << "Error: " << error << std::endl;
    }

    std::string m_state = "initial";
};
```

## 3. 资源管理和安全性

### 3.1 智能指针安全使用

#### 实践实现
```cpp
#include <memory>
#include <vector>

// 安全的智能指针使用
class SmartPointerSafety {
public:
    // 使用unique_ptr管理独占资源
    void manageUniqueResource() {
        auto resource = std::make_unique<ExpensiveResource>();
        resource->initialize();

        // 资源在作用域结束时自动释放
        if (resource->isValid()) {
            resource->process();
        }
    }

    // 使用shared_ptr管理共享资源
    void manageSharedResource() {
        auto resource = std::make_shared<SharedResource>();

        // 多个对象可以共享同一个资源
        std::vector<std::shared_ptr<SharedResource>> users;
        for (int i = 0; i < 5; ++i) {
            users.push_back(resource);
        }

        // 当所有shared_ptr都被销毁时，资源自动释放
    }

    // 使用weak_ptr避免循环引用
    void avoidCircularReference() {
        auto parent = std::make_shared<Parent>();
        auto child = std::make_shared<Child>();

        parent->setChild(child);
        child->setParent(parent); // 使用weak_ptr

        // 没有循环引用，资源可以正确释放
    }

private:
    class ExpensiveResource {
    public:
        void initialize() { /* 初始化逻辑 */ }
        bool isValid() const { return true; }
        void process() { /* 处理逻辑 */ }
    };

    class SharedResource {
    public:
        SharedResource() { std::cout << "SharedResource created\n"; }
        ~SharedResource() { std::cout << "SharedResource destroyed\n"; }
    };

    class Parent;
    class Child;

    class Parent {
    public:
        void setChild(std::shared_ptr<Child> child) {
            m_child = child;
        }

    private:
        std::shared_ptr<Child> m_child;
    };

    class Child {
    public:
        void setParent(std::shared_ptr<Parent> parent) {
            m_parent = parent; // weak_ptr不会增加引用计数
        }

    private:
        std::weak_ptr<Parent> m_parent;
    };
};
```

### 3.2 线程安全

#### 实践实现
```cpp
#include <mutex>
#include <shared_mutex>
#include <atomic>

// 线程安全的类
class ThreadSafeClass {
public:
    // 线程安全的读取操作
    int getValue() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_value;
    }

    // 线程安全的写入操作
    void setValue(int value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_value = value;
    }

    // 原子操作
    void increment() {
        m_counter.fetch_add(1, std::memory_order_relaxed);
    }

    int getCounter() const {
        return m_counter.load(std::memory_order_relaxed);
    }

private:
    mutable std::shared_mutex m_mutex;
    int m_value{0};
    std::atomic<int> m_counter{0};
};

// 异常安全的锁
class ExceptionSafeLock {
public:
    void processWithLock() {
        std::lock_guard<std::mutex> lock(m_mutex);

        try {
            // 可能抛出异常的操作
            riskyOperation();
        } catch (...) {
            // 锁在作用域结束时自动释放
            throw;
        }
    }

private:
    void riskyOperation() {
        // 模拟可能失败的操作
        if (rand() % 10 == 0) {
            throw std::runtime_error("Random failure");
        }
    }

    mutable std::mutex m_mutex;
};
```

## 4. 输入验证和安全检查

### 4.1 参数验证

#### Bitcoin Core中的应用
```cpp
// 来自 transaction.h - 参数验证
static const CFeeRate DEFAULT_MAX_RAW_TX_FEE_RATE{COIN / 10}; // 默认最大原始交易费用率：0.1 BTC
static const CAmount DEFAULT_MAX_BURN_AMOUNT{0};               // 默认最大销毁金额：0（不允许销毁）
```

#### 实践实现
```cpp
#include <limits>
#include <regex>
#include <cassert>

// 参数验证类
class ParameterValidator {
public:
    // 验证整数范围
    static bool validateRange(int value, int min, int max) {
        return value >= min && value <= max;
    }

    // 验证字符串长度
    static bool validateStringLength(const std::string& str, size_t min, size_t max) {
        return str.length() >= min && str.length() <= max;
    }

    // 验证邮箱格式
    static bool validateEmail(const std::string& email) {
        std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        return std::regex_match(email, email_regex);
    }

    // 验证URL格式
    static bool validateURL(const std::string& url) {
        std::regex url_regex(R"(https?://[^\s/$.?#].[^\s]*)");
        return std::regex_match(url, url_regex);
    }

    // 验证十六进制字符串
    static bool validateHexString(const std::string& hex) {
        if (hex.length() % 2 != 0) return false;

        std::regex hex_regex(R"([0-9a-fA-F]+)");
        return std::regex_match(hex, hex_regex);
    }
};

// 安全的函数实现
class SafeFunctions {
public:
    // 安全的除法
    std::optional<double> safeDivide(double a, double b) {
        if (b == 0.0) {
            return std::nullopt;
        }

        if (std::isnan(a) || std::isnan(b)) {
            return std::nullopt;
        }

        double result = a / b;
        if (std::isnan(result) || std::isinf(result)) {
            return std::nullopt;
        }

        return result;
    }

    // 安全的字符串转换
    std::optional<int> safeStringToInt(const std::string& str) {
        if (str.empty()) {
            return std::nullopt;
        }

        try {
            size_t pos = 0;
            int value = std::stoi(str, &pos);

            // 检查是否完全转换
            if (pos != str.length()) {
                return std::nullopt;
            }

            return value;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    // 安全的数组访问
    template<typename T>
    std::optional<T> safeArrayAccess(const std::vector<T>& array, size_t index) {
        if (index >= array.size()) {
            return std::nullopt;
        }
        return array[index];
    }
};
```

### 4.2 断言和调试

#### 实践实现
```cpp
#include <cassert>
#include <iostream>

// 调试辅助类
class DebugHelper {
public:
    // 条件断言
    static void assertCondition(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "Assertion failed: " << message << std::endl;
            assert(false);
        }
    }

    // 范围检查
    template<typename T>
    static void assertRange(T value, T min, T max, const std::string& message) {
        assertCondition(value >= min && value <= max,
                       message + " (value: " + std::to_string(value) +
                       ", range: [" + std::to_string(min) + ", " + std::to_string(max) + "])");
    }

    // 空指针检查
    template<typename T>
    static void assertNotNull(T* ptr, const std::string& message) {
        assertCondition(ptr != nullptr, message + " (null pointer)");
    }

    // 调试日志
    static void debugLog(const std::string& message) {
#ifdef _DEBUG
        std::cout << "[DEBUG] " << message << std::endl;
#endif
    }
};

// 安全的容器类
template<typename T>
class SafeVector {
public:
    T& at(size_t index) {
        if (index >= m_data.size()) {
            throw std::out_of_range("Index out of range: " + std::to_string(index));
        }
        return m_data[index];
    }

    const T& at(size_t index) const {
        if (index >= m_data.size()) {
            throw std::out_of_range("Index out of range: " + std::to_string(index));
        }
        return m_data[index];
    }

    T& operator[](size_t index) {
        assert(index < m_data.size());
        return m_data[index];
    }

    const T& operator[](size_t index) const {
        assert(index < m_data.size());
        return m_data[index];
    }

    void push_back(const T& value) {
        m_data.push_back(value);
    }

    size_t size() const {
        return m_data.size();
    }

private:
    std::vector<T> m_data;
};
```

## 5. 实践项目

### 项目1: 安全的配置管理器

```cpp
#include <unordered_map>
#include <string>
#include <variant>
#include <mutex>
#include <regex>

class SafeConfigManager {
public:
    using ConfigValue = std::variant<int, double, std::string, bool>;

    // 设置配置值
    bool setConfig(const std::string& key, const ConfigValue& value) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 验证键名
        if (!validateKey(key)) {
            return false;
        }

        // 验证值
        if (!validateValue(key, value)) {
            return false;
        }

        m_config[key] = value;
        return true;
    }

    // 获取配置值
    std::optional<ConfigValue> getConfig(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_config.find(key);
        if (it != m_config.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // 删除配置
    bool removeConfig(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_config.erase(key) > 0;
    }

    // 清空配置
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_config.size();
    }

private:
    bool validateKey(const std::string& key) {
        // 键名不能为空
        if (key.empty()) {
            return false;
        }

        // 键名只能包含字母、数字和下划线
        std::regex key_regex(R"([a-zA-Z_][a-zA-Z0-9_]*)");
        return std::regex_match(key, key_regex);
    }

    bool validateValue(const std::string& key, const ConfigValue& value) {
        // 根据键名验证值的类型和范围
        if (key == "port") {
            if (auto* port = std::get_if<int>(&value)) {
                return *port >= 1 && *port <= 65535;
            }
            return false;
        }

        if (key == "timeout") {
            if (auto* timeout = std::get_if<double>(&value)) {
                return *timeout >= 0.0 && *timeout <= 3600.0;
            }
            return false;
        }

        if (key == "max_connections") {
            if (auto* max_conn = std::get_if<int>(&value)) {
                return *max_conn >= 1 && *max_conn <= 10000;
            }
            return false;
        }

        return true;
    }

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, ConfigValue> m_config;
};

// 使用示例
void safe_config_manager_demo() {
    SafeConfigManager config;

    // 设置有效配置
    assert(config.setConfig("port", 8080));
    assert(config.setConfig("timeout", 30.5));
    assert(config.setConfig("max_connections", 100));
    assert(config.setConfig("debug", true));
    assert(config.setConfig("host", std::string("localhost")));

    // 设置无效配置
    assert(!config.setConfig("", 123)); // 空键名
    assert(!config.setConfig("invalid-key", 123)); // 无效键名
    assert(!config.setConfig("port", 70000)); // 端口超出范围
    assert(!config.setConfig("timeout", -1.0)); // 负数超时

    // 获取配置
    auto port = config.getConfig("port");
    assert(port.has_value());
    assert(std::get<int>(*port) == 8080);

    std::cout << "Config manager test passed\n";
}
```

### 项目2: 异常安全的资源池

```cpp
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

template<typename T>
class ExceptionSafeResourcePool {
public:
    explicit ExceptionSafeResourcePool(size_t initial_size) {
        expand(initial_size);
    }

    // 获取资源
    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_free_resources.empty()) {
            expand(m_pool.size()); // 双倍扩展
        }

        auto resource = std::move(m_free_resources.back());
        m_free_resources.pop_back();

        // 尝试初始化资源
        try {
            resource->initialize();
        } catch (...) {
            // 如果初始化失败，将资源放回池中
            m_free_resources.push_back(std::move(resource));
            throw;
        }

        return resource;
    }

    // 释放资源
    void release(std::unique_ptr<T> resource) {
        if (!resource) {
            return;
        }

        // 清理资源
        try {
            resource->cleanup();
        } catch (...) {
            // 即使清理失败也要继续
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_free_resources.push_back(std::move(resource));
    }

    size_t totalResources() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.size();
    }

    size_t freeResources() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_free_resources.size();
    }

private:
    void expand(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            m_free_resources.push_back(std::make_unique<T>());
        }
    }

    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<T>> m_pool;
    std::vector<std::unique_ptr<T>> m_free_resources;
};

// 示例资源类
class DatabaseConnection {
public:
    void initialize() {
        // 模拟数据库连接初始化
        if (rand() % 10 == 0) {
            throw std::runtime_error("Database connection failed");
        }
        m_connected = true;
    }

    void cleanup() {
        // 模拟数据库连接清理
        m_connected = false;
    }

    void executeQuery(const std::string& query) {
        if (!m_connected) {
            throw std::runtime_error("Not connected to database");
        }
        // 执行查询
    }

private:
    bool m_connected = false;
};

// 使用示例
void exception_safe_resource_pool_demo() {
    ExceptionSafeResourcePool<DatabaseConnection> pool(5);

    std::vector<std::unique_ptr<DatabaseConnection>> connections;

    // 获取连接
    for (int i = 0; i < 10; ++i) {
        try {
            auto conn = pool.acquire();
            conn->executeQuery("SELECT * FROM users");
            connections.push_back(std::move(conn));
        } catch (const std::exception& e) {
            std::cout << "Failed to acquire connection: " << e.what() << std::endl;
        }
    }

    // 释放连接
    for (auto& conn : connections) {
        pool.release(std::move(conn));
    }

    std::cout << "Total resources: " << pool.totalResources() << std::endl;
    std::cout << "Free resources: " << pool.freeResources() << std::endl;
}
```

## 总结

本教程涵盖了C++错误处理和安全编程的核心概念：

1. **异常安全保证**: 基本保证、强保证、不抛异常保证
2. **错误处理策略**: 返回值、异常、错误码
3. **资源管理**: 智能指针、RAII、线程安全
4. **输入验证**: 参数验证、安全检查、断言
5. **实践项目**: 安全配置管理器、异常安全资源池

这些技术在Bitcoin Core中得到了广泛应用，是安全可靠的C++系统的重要基础。