#include "CServer.hpp"
#include "const.h"
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include"configMgr.hpp"
#include "RedisMgr.hpp"
void TestRedisMgr() {
    assert(RedisMgr::GetInstance()->Set("blogwebsite","llfc.club"));
    std::string value="";
    assert(RedisMgr::GetInstance()->Get("blogwebsite", value) );
    assert(RedisMgr::GetInstance()->Get("nonekey", value) == false);
    assert(RedisMgr::GetInstance()->HSet("bloginfo","blogwebsite", "llfc.club"));
    assert(RedisMgr::GetInstance()->HGet("bloginfo","blogwebsite") != "");
    assert(RedisMgr::GetInstance()->ExistsKey("bloginfo"));
    assert(RedisMgr::GetInstance()->Del("bloginfo"));
    assert(RedisMgr::GetInstance()->Del("bloginfo"));
    assert(RedisMgr::GetInstance()->ExistsKey("bloginfo") == false);
    assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue1"));
    assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue2"));
    assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue3"));
    assert(RedisMgr::GetInstance()->RPop("lpushkey1", value));
    assert(RedisMgr::GetInstance()->RPop("lpushkey1", value));
    assert(RedisMgr::GetInstance()->LPop("lpushkey1", value));
    assert(RedisMgr::GetInstance()->LPop("lpushkey2", value)==false);
    RedisMgr::GetInstance()->Close();
}
int main() {
    ConfigMgr &gCfgMgr = ConfigMgr::GetInstance();
    std::string gate_port_str  = gCfgMgr["GateServer"]["Port"]; 
    unsigned short gate_port = atoi(gate_port_str.c_str());
    try {
        unsigned short prot = gate_port;
        net::io_context ioc{1};
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait(
            [&ioc](const boost::system::error_code &ec, int sigNum) {
                if (ec)
                    return;
                ioc.stop();
            });
        std::make_shared<CServer>(ioc, prot)->Start();
        ioc.run();
    } catch (std::exception &e) {
        std::cerr << __FUNCTION__ << " failed " << e.what() << "at "
                  << __FILE__ << ":" << __LINE__ << std::endl;
        return EXIT_FAILURE;
    }
}