#pragma once
#include<grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
// #include "const.h"
#include "Singleton.hpp"
#include <memory>
#include<queue>
#include<mutex>
#include<condition_variable>
//模板化了grpcpool
#include"GrpcPool.hpp"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::VarifyService;
using message::GetVarifyReq;
using message::GetVarifyRsp;


class VarifyGrpcClient : public Singleton<VarifyGrpcClient>
{
    friend class Singleton<VarifyGrpcClient>;
public:
    GetVarifyRsp GetVarifyCode(std::string email);
    
private:
    VarifyGrpcClient();
    // std::unique_ptr<RPCconPool> pool_;
    std::unique_ptr<GrpcPool<VarifyService, VarifyService::Stub>> pool_;
};
