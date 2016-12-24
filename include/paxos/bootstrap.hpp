#ifndef __BOOTSTRAP_HPP_INCLUDED__
#define __BOOTSTRAP_HPP_INCLUDED__

#include <thread>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

#include "paxos/replicaset.hpp"


class BootstrapListener
{
public:

    BootstrapListener(std::string address, short port);

private:

    void do_accept();

    boost::asio::io_service io_service;

    boost::asio::ip::tcp::acceptor acceptor;

    boost::asio::ip::tcp::socket socket;

    class BootstrapSession : public boost::enable_shared_from_this<BootstrapSession>
    {
    public:
        BootstrapSession(boost::asio::ip::tcp::socket socket);

        void Start();

    private:

        void handle_read_message(const boost::system::error_code& err);

        boost::asio::ip::tcp::socket socket;

        boost::asio::streambuf response;
    };
};


void SendBootstrap(Replica replica, std::string location);


#endif
