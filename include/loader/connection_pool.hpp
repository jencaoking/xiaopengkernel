#pragma once

#include "types.hpp"
#include "error.hpp"
#include "url.hpp"
#include "http_message.hpp"
#include "http_client.hpp"
#include <mutex>
#include <condition_variable>
#include <deque>

namespace xiaopeng {
namespace loader {

struct ConnectionKey {
    std::string host;
    std::uint16_t port;
    bool secure;
    
    bool operator==(const ConnectionKey& other) const {
        return host == other.host && port == other.port && secure == other.secure;
    }
};

struct ConnectionKeyHash {
    size_t operator()(const ConnectionKey& key) const {
        size_t h = std::hash<std::string>{}(key.host);
        h ^= std::hash<std::uint16_t>{}(key.port) << 1;
        h ^= std::hash<bool>{}(key.secure) << 2;
        return h;
    }
};

struct Connection {
    ConnectionKey key;
    TimePoint lastUsed;
    bool inUse = false;
    int id = 0;
    
    Connection(const ConnectionKey& k, int connectionId) 
        : key(k), lastUsed(std::chrono::steady_clock::now()), id(connectionId) {}
};

class ConnectionPool {
public:
    struct Config {
        size_t maxConnectionsPerHost = 6;
        size_t maxTotalConnections = 100;
        std::chrono::seconds idleTimeout{60};
        std::chrono::seconds connectTimeout{10};
    };
    
    ConnectionPool() = default;
    explicit ConnectionPool(const Config& config) : config_(config) {}
    
    Ptr<Connection> acquire(const Url& url) {
        ConnectionKey key{
            url.host(),
            url.port(),
            url.isSecure()
        };
        
        std::unique_lock<std::mutex> lock(mutex_);
        
        while (true) {
            auto& hostPool = pools_[key];
            
            for (auto& conn : hostPool) {
                if (!conn->inUse) {
                    conn->inUse = true;
                    conn->lastUsed = std::chrono::steady_clock::now();
                    return conn;
                }
            }
            
            if (hostPool.size() < config_.maxConnectionsPerHost && 
                totalConnections_ < config_.maxTotalConnections) {
                auto conn = std::make_shared<Connection>(key, nextId_++);
                conn->inUse = true;
                hostPool.push_back(conn);
                ++totalConnections_;
                return conn;
            }
            
            if (hostPool.size() >= config_.maxConnectionsPerHost) {
                cv_.wait(lock);
                continue;
            }
            
            cleanupIdleConnectionsLocked();
            
            if (totalConnections_ < config_.maxTotalConnections) {
                auto conn = std::make_shared<Connection>(key, nextId_++);
                conn->inUse = true;
                hostPool.push_back(conn);
                ++totalConnections_;
                return conn;
            }
            
            cv_.wait(lock);
        }
    }
    
    void release(Ptr<Connection> connection) {
        if (!connection) return;
        
        std::unique_lock<std::mutex> lock(mutex_);
        connection->inUse = false;
        connection->lastUsed = std::chrono::steady_clock::now();
        cv_.notify_one();
    }
    
    void cleanupIdleConnections() {
        std::unique_lock<std::mutex> lock(mutex_);
        cleanupIdleConnectionsLocked();
    }
    
    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        pools_.clear();
        totalConnections_ = 0;
    }
    
    size_t totalConnections() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return totalConnections_;
    }
    
    size_t activeConnections() const {
        std::unique_lock<std::mutex> lock(mutex_);
        size_t count = 0;
        for (const auto& [key, pool] : pools_) {
            for (const auto& conn : pool) {
                if (conn->inUse) ++count;
            }
        }
        return count;
    }
    
    size_t idleConnections() const {
        std::unique_lock<std::mutex> lock(mutex_);
        size_t count = 0;
        for (const auto& [key, pool] : pools_) {
            for (const auto& conn : pool) {
                if (!conn->inUse) ++count;
            }
        }
        return count;
    }
    
    const Config& config() const { return config_; }
    
    void setConfig(const Config& config) {
        std::unique_lock<std::mutex> lock(mutex_);
        config_ = config;
    }

private:
    void cleanupIdleConnectionsLocked() {
        auto now = std::chrono::steady_clock::now();
        
        for (auto it = pools_.begin(); it != pools_.end(); ) {
            auto& pool = it->second;
            
            for (auto connIt = pool.begin(); connIt != pool.end(); ) {
                if (!(*connIt)->inUse) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        now - (*connIt)->lastUsed);
                    if (elapsed > config_.idleTimeout) {
                        connIt = pool.erase(connIt);
                        --totalConnections_;
                        continue;
                    }
                }
                ++connIt;
            }
            
            if (pool.empty()) {
                it = pools_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    Config config_;
    HashMap<ConnectionKey, std::vector<Ptr<Connection>>, ConnectionKeyHash> pools_;
    size_t totalConnections_ = 0;
    int nextId_ = 1;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

}
}
