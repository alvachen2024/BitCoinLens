# C++设计模式教程 - 基于Bitcoin Core实践

## 1. PIMPL模式 (Pointer to Implementation)

### 1.1 模式概述

PIMPL模式（也称为"桥接模式"）通过将实现细节隐藏在私有指针后面，实现接口与实现的分离。

### 1.2 Bitcoin Core中的应用

#### 实际代码示例
```cpp
// 来自 txreconciliation.h
class TxReconciliationTracker {
private:
    class Impl;                    // 前向声明
    const std::unique_ptr<Impl> m_impl; // 实现指针

public:
    // 公共接口
    uint64_t PreRegisterPeer(NodeId peer_id);
    ReconciliationRegisterResult RegisterPeer(NodeId peer_id, bool is_peer_inbound,
                                              uint32_t peer_recon_version, uint64_t remote_salt);
    void ForgetPeer(NodeId peer_id);
    bool IsPeerRegistered(NodeId peer_id) const;
};
```

### 1.3 实现示例

```cpp
// 头文件: widget.h
class Widget {
public:
    Widget();
    ~Widget();

    // 公共接口
    void doSomething();
    int getValue() const;

    // 禁用拷贝
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    // 允许移动
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;

private:
    class Impl;                    // 前向声明
    std::unique_ptr<Impl> pImpl;   // 实现指针
};

// 实现文件: widget.cpp
#include "widget.h"
#include <vector>
#include <string>

// 实现类的定义
class Widget::Impl {
public:
    Impl() : value(0) {}

    void doSomething() {
        // 复杂的实现逻辑
        data.push_back("something");
        value++;
    }

    int getValue() const {
        return value;
    }

private:
    std::vector<std::string> data;
    int value;
};

// Widget类的实现
Widget::Widget() : pImpl(std::make_unique<Impl>()) {}

Widget::~Widget() = default;

Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;

void Widget::doSomething() {
    pImpl->doSomething();
}

int Widget::getValue() const {
    return pImpl->getValue();
}
```

### 1.4 优势分析

1. **编译隔离**: 实现变化不影响客户端代码
2. **编译速度**: 减少头文件依赖
3. **二进制兼容**: 实现变化不影响二进制接口
4. **封装性**: 完全隐藏实现细节

### 1.5 实践练习

```cpp
// 练习: 实现一个简单的数据库连接类
class DatabaseConnection {
public:
    DatabaseConnection(const std::string& connection_string);
    ~DatabaseConnection();

    bool connect();
    void disconnect();
    bool execute(const std::string& query);

    // 禁用拷贝
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    // 允许移动
    DatabaseConnection(DatabaseConnection&&) noexcept;
    DatabaseConnection& operator=(DatabaseConnection&&) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
```

## 2. RAII模式 (Resource Acquisition Is Initialization)

### 2.1 模式概述

RAII确保资源在对象构造时获取，在对象析构时自动释放，提供异常安全的资源管理。

### 2.2 Bitcoin Core中的应用

#### 智能指针管理
```cpp
// 来自 context.h
struct NodeContext {
    // 使用智能指针管理资源
    std::unique_ptr<kernel::Context> kernel;
    std::unique_ptr<ECC_Context> ecc_context;
    std::unique_ptr<AddrMan> addrman;
    std::unique_ptr<CConnman> connman;
    // ...
};
```

### 2.3 实现示例

```cpp
#include <memory>
#include <iostream>

// 资源类
class FileHandle {
public:
    FileHandle(const std::string& filename) {
        file = fopen(filename.c_str(), "r");
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        std::cout << "File opened: " << filename << std::endl;
    }

    ~FileHandle() {
        if (file) {
            fclose(file);
            std::cout << "File closed" << std::endl;
        }
    }

    // 禁用拷贝
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // 允许移动
    FileHandle(FileHandle&& other) noexcept : file(other.file) {
        other.file = nullptr;
    }

    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            if (file) fclose(file);
            file = other.file;
            other.file = nullptr;
        }
        return *this;
    }

    FILE* get() const { return file; }

private:
    FILE* file;
};

// 使用示例
void processFile(const std::string& filename) {
    try {
        FileHandle file(filename);
        // 使用文件
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file.get())) {
            // 处理数据
        }
        // 文件自动关闭
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

### 2.4 实践练习

```cpp
// 练习: 实现一个互斥锁RAII包装器
class ScopedLock {
public:
    explicit ScopedLock(std::mutex& mutex) : m_mutex(mutex) {
        m_mutex.lock();
        std::cout << "Lock acquired" << std::endl;
    }

    ~ScopedLock() {
        m_mutex.unlock();
        std::cout << "Lock released" << std::endl;
    }

    // 禁用拷贝和移动
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
    ScopedLock(ScopedLock&&) = delete;
    ScopedLock& operator=(ScopedLock&&) = delete;

private:
    std::mutex& m_mutex;
};

// 使用示例
std::mutex global_mutex;
int shared_data = 0;

void safeIncrement() {
    ScopedLock lock(global_mutex);
    shared_data++;
    // 锁在作用域结束时自动释放
}
```

## 3. 策略模式 (Strategy Pattern)

### 3.1 模式概述

策略模式定义一系列算法，将每个算法封装起来，并使它们可以互换。

### 3.2 Bitcoin Core中的应用

#### 连接类型策略
```cpp
// 来自 connection_types.h
enum class ConnectionType {
    INBOUND,              // 入站连接策略
    OUTBOUND_FULL_RELAY,  // 出站全中继策略
    MANUAL,               // 手动连接策略
    FEELER,               // 探测连接策略
    BLOCK_RELAY,          // 仅区块中继策略
    ADDR_FETCH,           // 地址获取策略
};
```

### 3.3 实现示例

```cpp
#include <memory>
#include <vector>
#include <algorithm>

// 策略接口
class SortingStrategy {
public:
    virtual ~SortingStrategy() = default;
    virtual void sort(std::vector<int>& data) = 0;
};

// 具体策略: 快速排序
class QuickSortStrategy : public SortingStrategy {
public:
    void sort(std::vector<int>& data) override {
        std::sort(data.begin(), data.end());
        std::cout << "Quick sort applied" << std::endl;
    }
};

// 具体策略: 冒泡排序
class BubbleSortStrategy : public SortingStrategy {
public:
    void sort(std::vector<int>& data) override {
        for (size_t i = 0; i < data.size(); ++i) {
            for (size_t j = 0; j < data.size() - i - 1; ++j) {
                if (data[j] > data[j + 1]) {
                    std::swap(data[j], data[j + 1]);
                }
            }
        }
        std::cout << "Bubble sort applied" << std::endl;
    }
};

// 上下文类
class Sorter {
public:
    explicit Sorter(std::unique_ptr<SortingStrategy> strategy)
        : m_strategy(std::move(strategy)) {}

    void setStrategy(std::unique_ptr<SortingStrategy> strategy) {
        m_strategy = std::move(strategy);
    }

    void sort(std::vector<int>& data) {
        if (m_strategy) {
            m_strategy->sort(data);
        }
    }

private:
    std::unique_ptr<SortingStrategy> m_strategy;
};

// 使用示例
void strategy_pattern_demo() {
    std::vector<int> data = {3, 1, 4, 1, 5, 9, 2, 6};

    Sorter sorter(std::make_unique<QuickSortStrategy>());
    sorter.sort(data);

    // 切换策略
    sorter.setStrategy(std::make_unique<BubbleSortStrategy>());
    sorter.sort(data);
}
```

### 3.4 实践练习

```cpp
// 练习: 实现不同的压缩策略
class CompressionStrategy {
public:
    virtual ~CompressionStrategy() = default;
    virtual std::vector<uint8_t> compress(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) = 0;
};

class GzipCompression : public CompressionStrategy {
public:
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) override {
        // Gzip压缩实现
        return data; // 简化实现
    }

    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) override {
        // Gzip解压实现
        return compressed; // 简化实现
    }
};

class LZ4Compression : public CompressionStrategy {
public:
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) override {
        // LZ4压缩实现
        return data; // 简化实现
    }

    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) override {
        // LZ4解压实现
        return compressed; // 简化实现
    }
};
```

## 4. 观察者模式 (Observer Pattern)

### 4.1 模式概述

观察者模式定义对象间的一对多依赖关系，当一个对象状态改变时，所有依赖者都会得到通知。

### 4.2 Bitcoin Core中的应用

#### 内核通知系统
```cpp
// 来自 kernel_notifications.h
class KernelNotifications {
public:
    virtual ~KernelNotifications() = default;

    virtual void blockTip(SynchronizationState state,
                         interfaces::BlockTip tip,
                         double verification_progress) = 0;

    virtual void headerTip(SynchronizationState state,
                          int64_t height,
                          int64_t timestamp,
                          bool presync) = 0;

    virtual void progress(const bilingual_str& title,
                         int progress_percent,
                         bool resume_possible) = 0;
};
```

### 4.3 实现示例

```cpp
#include <vector>
#include <memory>
#include <algorithm>

// 观察者接口
class Observer {
public:
    virtual ~Observer() = default;
    virtual void update(const std::string& message) = 0;
};

// 具体观察者
class ConsoleObserver : public Observer {
public:
    void update(const std::string& message) override {
        std::cout << "Console: " << message << std::endl;
    }
};

class FileObserver : public Observer {
public:
    void update(const std::string& message) override {
        // 写入文件
        std::cout << "File: " << message << std::endl;
    }
};

// 被观察者
class Subject {
public:
    void attach(std::shared_ptr<Observer> observer) {
        m_observers.push_back(observer);
    }

    void detach(std::shared_ptr<Observer> observer) {
        m_observers.erase(
            std::remove(m_observers.begin(), m_observers.end(), observer),
            m_observers.end()
        );
    }

    void notify(const std::string& message) {
        for (const auto& observer : m_observers) {
            observer->update(message);
        }
    }

private:
    std::vector<std::shared_ptr<Observer>> m_observers;
};

// 使用示例
void observer_pattern_demo() {
    auto subject = std::make_unique<Subject>();

    auto console_observer = std::make_shared<ConsoleObserver>();
    auto file_observer = std::make_shared<FileObserver>();

    subject->attach(console_observer);
    subject->attach(file_observer);

    subject->notify("System started");
    subject->notify("Processing data");

    subject->detach(console_observer);
    subject->notify("Console observer removed");
}
```

### 4.4 实践练习

```cpp
// 练习: 实现一个股票价格观察者系统
class StockPriceObserver {
public:
    virtual ~StockPriceObserver() = default;
    virtual void onPriceChanged(const std::string& symbol, double price) = 0;
};

class StockMarket {
public:
    void addObserver(std::shared_ptr<StockPriceObserver> observer) {
        m_observers.push_back(observer);
    }

    void setPrice(const std::string& symbol, double price) {
        m_prices[symbol] = price;
        notifyObservers(symbol, price);
    }

private:
    void notifyObservers(const std::string& symbol, double price) {
        for (const auto& observer : m_observers) {
            observer->onPriceChanged(symbol, price);
        }
    }

    std::vector<std::shared_ptr<StockPriceObserver>> m_observers;
    std::map<std::string, double> m_prices;
};
```

## 5. 单例模式 (Singleton Pattern)

### 5.1 模式概述

单例模式确保一个类只有一个实例，并提供全局访问点。

### 5.2 线程安全实现

```cpp
#include <mutex>
#include <memory>

class Singleton {
public:
    static Singleton& getInstance() {
        std::call_once(s_once_flag, []() {
            s_instance.reset(new Singleton());
        });
        return *s_instance;
    }

    void doSomething() {
        std::cout << "Singleton doing something" << std::endl;
    }

    // 禁用拷贝和移动
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

private:
    Singleton() = default;
    ~Singleton() = default;

    static std::unique_ptr<Singleton> s_instance;
    static std::once_flag s_once_flag;
};

std::unique_ptr<Singleton> Singleton::s_instance;
std::once_flag Singleton::s_once_flag;

// 使用示例
void singleton_demo() {
    auto& instance1 = Singleton::getInstance();
    auto& instance2 = Singleton::getInstance();

    // instance1 和 instance2 是同一个对象
    std::cout << "Same instance: " << (&instance1 == &instance2) << std::endl;
}
```

## 6. 工厂模式 (Factory Pattern)

### 6.1 模式概述

工厂模式提供创建对象的接口，但由子类决定要实例化的类是哪一个。

### 6.2 实现示例

```cpp
#include <memory>
#include <string>

// 产品接口
class Connection {
public:
    virtual ~Connection() = default;
    virtual void connect() = 0;
    virtual void disconnect() = 0;
};

// 具体产品
class TCPConnection : public Connection {
public:
    void connect() override {
        std::cout << "TCP connection established" << std::endl;
    }

    void disconnect() override {
        std::cout << "TCP connection closed" << std::endl;
    }
};

class UDPConnection : public Connection {
public:
    void connect() override {
        std::cout << "UDP connection established" << std::endl;
    }

    void disconnect() override {
        std::cout << "UDP connection closed" << std::endl;
    }
};

// 工厂接口
class ConnectionFactory {
public:
    virtual ~ConnectionFactory() = default;
    virtual std::unique_ptr<Connection> createConnection() = 0;
};

// 具体工厂
class TCPConnectionFactory : public ConnectionFactory {
public:
    std::unique_ptr<Connection> createConnection() override {
        return std::make_unique<TCPConnection>();
    }
};

class UDPConnectionFactory : public ConnectionFactory {
public:
    std::unique_ptr<Connection> createConnection() override {
        return std::make_unique<UDPConnection>();
    }
};

// 使用示例
void factory_pattern_demo() {
    auto tcp_factory = std::make_unique<TCPConnectionFactory>();
    auto udp_factory = std::make_unique<UDPConnectionFactory>();

    auto tcp_conn = tcp_factory->createConnection();
    auto udp_conn = udp_factory->createConnection();

    tcp_conn->connect();
    udp_conn->connect();
}
```

## 7. 实践项目

### 项目1: 简单的日志系统

```cpp
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>

// 日志级别
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// 日志观察者接口
class LogObserver {
public:
    virtual ~LogObserver() = default;
    virtual void log(LogLevel level, const std::string& message) = 0;
};

// 控制台日志观察者
class ConsoleLogObserver : public LogObserver {
public:
    void log(LogLevel level, const std::string& message) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "[" << getLevelString(level) << "] " << message << std::endl;
    }

private:
    std::string getLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    mutable std::mutex m_mutex;
};

// 文件日志观察者
class FileLogObserver : public LogObserver {
public:
    explicit FileLogObserver(const std::string& filename)
        : m_file(filename, std::ios::app) {}

    void log(LogLevel level, const std::string& message) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file << "[" << getLevelString(level) << "] " << message << std::endl;
        }
    }

private:
    std::string getLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    std::ofstream m_file;
    mutable std::mutex m_mutex;
};

// 日志管理器（单例）
class LogManager {
public:
    static LogManager& getInstance() {
        static LogManager instance;
        return instance;
    }

    void addObserver(std::shared_ptr<LogObserver> observer) {
        m_observers.push_back(observer);
    }

    void log(LogLevel level, const std::string& message) {
        for (const auto& observer : m_observers) {
            observer->log(level, message);
        }
    }

    void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
    void info(const std::string& message) { log(LogLevel::INFO, message); }
    void warning(const std::string& message) { log(LogLevel::WARNING, message); }
    void error(const std::string& message) { log(LogLevel::ERROR, message); }

private:
    LogManager() = default;
    std::vector<std::shared_ptr<LogObserver>> m_observers;
};

// 使用示例
int main() {
    auto& logger = LogManager::getInstance();

    // 添加观察者
    logger.addObserver(std::make_shared<ConsoleLogObserver>());
    logger.addObserver(std::make_shared<FileLogObserver>("app.log"));

    // 记录日志
    logger.info("Application started");
    logger.warning("Low memory warning");
    logger.error("Critical error occurred");

    return 0;
}
```

## 总结

本教程涵盖了C++中常用的设计模式：

1. **PIMPL模式**: 接口与实现分离，提高编译效率
2. **RAII模式**: 自动资源管理，确保异常安全
3. **策略模式**: 算法封装，支持运行时切换
4. **观察者模式**: 事件通知，松耦合设计
5. **单例模式**: 全局唯一实例，线程安全
6. **工厂模式**: 对象创建封装，支持扩展

这些模式在Bitcoin Core中得到了广泛应用，是高质量C++代码的重要基础。