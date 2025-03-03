#include "VarifyGrpcClient.hpp"
#include "configMgr.hpp"
#include "const.h"
#include "src/GrpcPool.hpp"
#include "src/message.grpc.pb.h"


GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email) {
    ClientContext context;
    GetVarifyReq requset;
    GetVarifyRsp reply;
    requset.set_email(email);
    auto stub = pool_->getConnection();
    Status status = stub->GetVarifyCode(&context, requset, &reply);

    if (status.ok()) {
        pool_->returnConnection(std::move(stub));
        return reply;
    } else {
        pool_->returnConnection(std::move(stub));
        reply.set_error(ErrorCodes::RPCFailed);
        return reply;
    }
} 
VarifyGrpcClient::VarifyGrpcClient() {
    auto & gCfgMgr = ConfigMgr::GetInstance();
    std::string host = gCfgMgr["VarifyServer"]["Host"];
    std::string port = gCfgMgr["VarifyServer"]["Port"];
    std::cerr << "host is " << host << " port is " << port << std::endl;
    pool_.reset(new GrpcPool<VarifyService, VarifyService::Stub>(5, host, port));
}
