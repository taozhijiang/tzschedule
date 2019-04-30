/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __TZSERIAL_INSANE_BIND_H__
#define __TZSERIAL_INSANE_BIND_H__

// 对于一些服务来说，即使不需要绑定端口，但是为了同ZooKeeper合作提供自己的实例
// 名字，还是需要使用到一个端口的

// 这里提供port()和acceptor()接口，应用程序还是可以使用他来做网络服务使用的

#include "ConstructException.h"
#include <boost/asio.hpp>


namespace tzrpc {

using namespace boost::asio;

class InsaneBind {

public:

    InsaneBind() :
        port_(0),
        io_service_(),
	    ep_(),
        acceptor_(io_service_, ep_.protocol())
    {
        do_internal_bind();
    }

    explicit InsaneBind(uint16_t port) :
        port_(port),
        io_service_(),
	    ep_(),
        acceptor_(io_service_, ep_.protocol()) {
        do_internal_bind();
    }

    ~InsaneBind() {
    }

    uint16_t port() {
        return port_;
    }

    boost::asio::ip::tcp::acceptor& acceptor() {
        return acceptor_;
    }
    
    // 禁止拷贝
    InsaneBind(const InsaneBind&) = delete;
    InsaneBind& operator=(const InsaneBind&) = delete;

private:

    void do_internal_bind() {

        uint16_t n_port = 0;
        try {

            ep_ = ip::tcp::endpoint(ip::address_v4::any(), port_);
            acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));

            acceptor_.bind(ep_);
            // ("bind address error: %d, %s", ec.value(), ec.message().c_str());
            
            acceptor_.listen();
            
            ip::tcp::endpoint le = acceptor_.local_endpoint(); 
            n_port = le.port();

        } catch (std::exception& e) {
            throw ConstructException("construct boost::asio failed.");
        }

        if(n_port == 0) {
            throw ConstructException("request port info failed.");
        }

        if(port_ != 0 && n_port != port_) {
            throw ConstructException("port mistach found.");
        }

        port_ = n_port;
    }

private:

    uint16_t port_;

    boost::asio::io_service io_service_;

    ip::tcp::endpoint ep_;
    ip::tcp::acceptor acceptor_;
};

} // end namespace tzrpc


#endif // __TZSERIAL_INSANE_BIND_H__