#include "LogicSystem.hpp"
#include "const.h"
#include "HttpConnection.hpp"
#include <json/json.h>
#include <json/reader.h>
#include "VarifyGrpcClient.hpp"
#include "RedisMgr.hpp"

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
    if(getHandlers_.find(path) == getHandlers_.end()){
        return false;
    }
    getHandlers_[path](con);
    return true;
}
bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
    if(postHandlers_.find(path) == postHandlers_.end()){
        return false;
        }
    postHandlers_[path](con);
    return true;
}
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
    getHandlers_[url] = handler;
}
void LogicSystem::RegPost(std::string url, HttpHandler handler) {
    postHandlers_[url] = handler;
}
LogicSystem::LogicSystem() {
    RegGet("/get_test",[](std::shared_ptr<HttpConnection> connection){
        beast::ostream(connection->response_.body()) << "receive get_test req";
    int i = 0;
    for(auto &elem : connection->getParams_){
        i++;
        beast::ostream(connection->response_.body()) << "param " << 
        i << " key is " << elem.first;
        beast::ostream(connection->response_.body()) << " value is " << elem.second << std::endl;
    }
    });
    
    RegPost("/get_code", [](std::shared_ptr<HttpConnection> connection){
            auto bodyStr = boost::beast::buffers_to_string(connection->request_.body().data());
            
            std::cout << "receive body is " << bodyStr << std::endl;
            
            connection->response_.set(http::field::content_type, "text/json");
            
            Json::Value root;
            Json::Reader reader;
            Json::Value src_root;
            
            bool parseSuccess = reader.parse(bodyStr, src_root);
            if(!parseSuccess){
                std::cout << "parse json failed" << std::endl;
                root["error"] = ErrorCodes::ErrorJson;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            
            auto email = src_root["email"].asString();
            message::GetVarifyRsp rsp = VarifyGrpcClient::GetInstance()->GetVarifyCode(email);
            std::cout <<  "email is " << email << std::endl;
            root["error"] = rsp.error();
            root["email"] = src_root["email"];
            
            std::string jsonStr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonStr;
            return true;
    });
    RegPost("/ug", [](std::shared_ptr<HttpConnection> connection){
            auto bodyStr = boost::beast::buffers_to_string(connection->request_.body().data());
            std::cout << "receive body is " << bodyStr << std::endl;
            connection->response_.set(http::field::content_type, "text/json");
            
            Json::Value root;
            Json::Reader reader;
            Json::Value src_root;
            bool parseSuccess = reader.parse(bodyStr, src_root);
            if(!parseSuccess){
                std::cout << "parse json failed" << std::endl;
                root["error"] = ErrorCodes::ErrorJson;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            auto email = src_root["email"].asString();
            auto user = src_root["user"].asString();
            auto pwd = src_root["passwd"].asString();
            auto confirm = src_root["confirm"].asString();
            //二次密码校验
            if(pwd != confirm ){
                std::cout << "passwd err " << std::endl;
                root["error"] = ErrorCodes::PasswdErr;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            //先查找Redis中的email的验证码是否合理
            std::string varifyCode;
            bool b_get_varify =RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varifyCode);
            if(!b_get_varify){
                std::cout << "get varify code expired" << std::endl;
                root["error"] = ErrorCodes::VarifyExpired;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            
            if(varifyCode != src_root["varifycode"].asString()){
                std::cout << "varify code not match" << std::endl;
                root["error"] = ErrorCodes::VarifyErr;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            
            // TODO: 接入mysql判断是否存在该用户
            
            root["error"] = ErrorCodes::Success;
            root["email"] = src_root["email"];
            root ["user"]= src_root["user"].asString();
            root["passwd"] = src_root["passwd"].asString();
            root["confirm"] = src_root["confirm"].asString();
            root["varifycode"] = src_root["varifycode"].asString();
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
    });
    

}
LogicSystem::~LogicSystem(){};
