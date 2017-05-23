#ifndef __SENDER_HPP_INCLUDED__
#define __SENDER_HPP_INCLUDED__


#include <mutex>
#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "paxos/messages.hpp"
#include "paxos/replicaset.hpp"
#include "paxos/serialization.hpp"

class Sender
{
public:

    virtual void Reply(Message message) = 0;

    virtual void ReplyAll(Message message) = 0;
};

class FileSender
{
public:

    virtual void SendFile(Replica replica, BootstrapFile file) = 0;
};


class BoostTransport
{
public:

    BoostTransport(std::string hostname, short port);

    ~BoostTransport();

    void Write(std::string content);

private:

    boost::asio::io_service io_service_;

    boost::asio::ip::tcp::socket socket_;

    boost::asio::ip::tcp::resolver resolver_;

    boost::asio::ip::basic_resolver_iterator<boost::asio::ip::tcp> endpoint_;
};


template<typename Transport>
class NetworkSender : public Sender
{
public:

    NetworkSender(std::shared_ptr<ReplicaSet>& replicaset_)
        : replicaset(replicaset_)
    {
    }

    void Reply(Message message)
    {
        std::lock_guard<std::mutex> guard(mutex);

        Transport transport(message.to.hostname, message.to.port);

        // 1. serialize message
        std::string message_str = Serialize(message);

        // 2. write message
        transport.Write(message_str);
    }

    void ReplyAll(Message message)
    {
        for (auto r : *replicaset)
        {
            Message m = message;
            m.to = r;
            Reply(m);
        }
    }

private:

    std::shared_ptr<ReplicaSet>& replicaset;

    std::mutex mutex;
};



template<typename Transport>
class NetworkFileSender : public FileSender
{
public:

    void SendFile(Replica replica, BootstrapFile file)
    {
        std::lock_guard<std::mutex> guard(mutex);

        Transport transport(replica.hostname, replica.port + 1);

        // 1. serialize file
        std::string file_str = Serialize(file);

        // 2. write file
        transport.Write(file_str);
    }

private:

    std::mutex mutex;
};


#endif
