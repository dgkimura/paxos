#ifndef __RECEIVER_HPP_INCLUDED__
#define __RECEIVER_HPP_INCLUDED__


#include <thread>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>

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

    boost::asio::io_service io_service_;

    boost::asio::ip::tcp::acceptor acceptor_;

    boost::asio::ip::tcp::socket socket_;

    std::unordered_map<MessageType, std::vector<Callback>, MessageTypeHash> registered_map;

    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        Session(
            boost::asio::ip::tcp::socket socket,
            std::unordered_map<MessageType, std::vector<Callback>, MessageTypeHash> registered_map
        );

        void Start();

    private:

        boost::asio::ip::tcp::socket socket_;

        std::unordered_map<MessageType, std::vector<Callback>, MessageTypeHash> registered_map_;

        enum {max_length = 8192};

        char data_[max_length];
    };
};


#endif
