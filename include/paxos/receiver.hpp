#ifndef __RECEIVER_HPP_INCLUDED__
#define __RECEIVER_HPP_INCLUDED__


#include <thread>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "paxos/callback.hpp"
#include "paxos/messages.hpp"


class Receiver
{
public:

    virtual void RegisterCallback(Callback&& callback, MessageType type) = 0;
};

class NetworkReceiver : public Receiver
{
public:
    NetworkReceiver(std::string address, short port);

    ~NetworkReceiver();

    void RegisterCallback(Callback&& callback, MessageType type);

private:

    void do_accept();

    boost::asio::io_service io_service;

    boost::asio::ip::tcp::acceptor acceptor;

    boost::asio::ip::tcp::socket socket;

    std::unordered_map<MessageType, std::vector<Callback>, MessageTypeHash> registered_map;

    class Session : public boost::enable_shared_from_this<Session>
    {
    public:
        Session(
            boost::asio::ip::tcp::socket socket,
            std::unordered_map<MessageType, std::vector<Callback>, MessageTypeHash> registered_map
        );

        void Start();

    private:

        void handle_read_message(const boost::system::error_code& err);

        boost::asio::ip::tcp::socket socket;

        std::unordered_map<MessageType, std::vector<Callback>, MessageTypeHash> registered_map_;

        boost::asio::streambuf response;
    };
};


#endif
