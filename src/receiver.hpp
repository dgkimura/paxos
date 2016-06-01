#ifndef __RECEIVER_HPP_INCLUDED__
#define __RECEIVER_HPP_INCLUDED__


#include <thread>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>

#include <callback.hpp>
#include <messages.hpp>


class Receiver
{
public:

    Receiver(short port=8081);

    void RegisterCallback(Callback&& callback, MessageType type);

private:

    void do_accept();

    boost::asio::io_service io_service_;

    boost::asio::ip::tcp::acceptor acceptor_;

    boost::asio::ip::tcp::socket socket_;

    std::unordered_map<MessageType, std::vector<Callback>, MessageTypeHash> registered_map;

    enum {max_length = 8192};

    char data_[max_length];
};


#endif
