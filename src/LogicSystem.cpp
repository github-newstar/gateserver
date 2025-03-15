#include "LogicSystem.hpp"
#include "const.h"
#include "MysqlMgr.hpp"
#include "HttpConnection.hpp"
#include <json/json.h>
#include <json/reader.h>
#include "VarifyGrpcClient.hpp"
#include "RedisMgr.hpp"
#include "src/MysqlDao.hpp"
#include "StatusGrpcClient.hpp"

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
            auto name = src_root["user"].asString();
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
            
            // 接入mysql判断是否存在该用户
            int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd);
            if(uid == 0 || uid == -1){
                std::cout << "reg user failed" << std::endl;
                root["error"] = ErrorCodes::UserExist;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            
            root["error"] = ErrorCodes::Success;
            root["email"] = src_root["email"];
            root["uid"] = uid;
            root ["user"]= src_root["user"].asString();
            root["passwd"] = src_root["passwd"].asString();
            root["confirm"] = src_root["confirm"].asString();
            root["varifycode"] = src_root["varifycode"].asString();
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
    });
    
    RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection){
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
            auto name = src_root["user"].asString();
            auto pwd = src_root["passwd"].asString();
            
            //在redis中检查email对应的验证码是否合理
            std::string varifyCode;
            bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varifyCode);
            if(!b_get_varify){
                std::cout << "get varify code expired" << std::endl;
                root["error"] = ErrorCodes::VarifyExpired;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            
            //查数据库判断用户名和邮箱是否匹配
            bool emailValid = MysqlMgr::GetInstance()->CheckEmail(name ,email);
            if(!emailValid){
                std::cout << "email not match" << std::endl;
                root["error"] = ErrorCodes::EmailNotMatch;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            
            //更新密码
            bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
            if(!b_up){
                std::cout << "update passwd failed" << std::endl;
                root["error"] = ErrorCodes::PasswdUpFailed;
                std::string jsonStr = root.toStyledString();
                beast::ostream(connection->response_.body()) << jsonStr;
                return true;
            }
            
            std::cout << "success to update passwd" << std::endl;
            root["error"] = ErrorCodes::Success;
            root["email"] = email;
            root["user"] = name;
            root["passwd"] = pwd;
            root["varifycode"] = src_root["varifycode"].asString();
            
            std::string jsonstr= root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;

        });
    RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
        auto bodyStr =
            boost::beast::buffers_to_string(connection->request_.body().data());
        std::cout << "receive body is " << bodyStr << std::endl;
        connection->response_.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parseSuccess = reader.parse(bodyStr, src_root);
        if (!parseSuccess) {
            std::cout << "parse json failed" << std::endl;
            root["error"] = ErrorCodes::ErrorJson;
            std::string jsonStr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonStr;
            return true;
        }
        
        auto email = src_root["email"].asString();
        auto pwd = src_root["passwd"].asString();
        UerInfo userInfo;
        //在mysql中检查邮箱和密码是否匹配
        bool pwdValid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
        if(!pwdValid){
            std::cout << "passwd not match" << std::endl;
            root["error"] = ErrorCodes::PasswdInvalid;
            std::string jsonStr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonStr;
            return true;
        }
        //查询statusserver寻找合适的连接
         auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
         if(reply.error()){
             std::cout << "get chat server failed" << std::endl;
             root["error"] = ErrorCodes::RPCFailed;
             std::string jsonStr = root.toStyledString();
             beast::ostream(connection->response_.body()) << jsonStr;
             return true;
         }
         std::cout << "success to load uesrinfo is " << userInfo.uid << std::endl;
         root["user"] = userInfo.name;
         root["error"] = ErrorCodes::Success;
         root["email"] = email;
         root["uid"] = userInfo.uid;
         root["token"] = reply.token();
         //host映射
         std::string gate_host = src_root["gate_host"].asString();
         if(gate_host != "127.0.0.1" || gate_host != "localhost"){
             root["host"] = gate_host;
         }else{
             root["host"] = reply.host();
         }
         root["port"] = reply.port();
         std::cerr<< "port is" << reply.port() << std::endl;
         std::string jsonstr = root.toStyledString();
         beast::ostream(connection->response_.body()) << jsonstr;
        return true;
        
        
    });
}
LogicSystem::~LogicSystem(){};
