#include "CServer.hpp"
#include "const.h"
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include"configMgr.hpp"
#include "RedisMgr.hpp"

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