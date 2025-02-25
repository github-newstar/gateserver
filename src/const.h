#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <sw/redis++/redis++.h>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
                                  // 
enum ErrorCodes{
    Success = 0,
    ErrorJson = 1001,
    RPCFailed = 1002,
    VarifyExpired = 1003, // 验证码已过期
    VarifyErr = 1004, // 验证码错误
    UserExist = 1005, // 用户已存在
    PasswdErr = 1006, // 密码错误
    EamilNotMatch = 1007, // 邮箱不匹配
    PasswdUpFailed = 1008, // 密码更新失败
    PasswdInalid = 1009, // 密码无效

};

class ConfigMgr;
extern ConfigMgr gCfgMgr;
#define CODEPREFIX "code_"