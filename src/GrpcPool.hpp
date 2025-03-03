#pragma once
#include <grpcpp/grpcpp.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <string>

// 模板化 GRPC 连接池 - 使用两个参数：Service 和 Stub 类型
template<typename ServiceType, typename ServiceStubType>
class GrpcPool {
public:
    GrpcPool(size_t poolSize, const std::string& host, const std::string& port)
        : bStop_(false), poolSize_(poolSize), host_(host), port_(port) {
        // 初始化连接池
        for (size_t i = 0; i < poolSize_; ++i) {
            std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
                host_ + ":" + port_,
                grpc::InsecureChannelCredentials()
            );
            // 使用 ServiceType::NewStub 创建 stub
            connections_.push(ServiceType::NewStub(channel));
        }
    }
    
    ~GrpcPool() {
        Close();
    }
    
    std::unique_ptr<ServiceStubType> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]{
            if(bStop_){
                return true;
            }
            return !connections_.empty();
        });
        
        if (bStop_) {
            return nullptr;
        }
        
        auto context = std::move(connections_.front());
        connections_.pop();
        return context;
    }
    
    void returnConnection(std::unique_ptr<ServiceStubType> context) {
        if (!context) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        if (bStop_) {
            return;
        }
        connections_.push(std::move(context));
        cond_.notify_one();
    }
    
    void Close() {
        bStop_ = true;
        cond_.notify_all();
    }
    
protected:
    std::atomic_bool bStop_;
    std::size_t poolSize_;
    std::string host_;
    std::string port_;
    std::queue<std::unique_ptr<ServiceStubType>> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};