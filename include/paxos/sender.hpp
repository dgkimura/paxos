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


class BoostTransport
{
public:

    BoostTransport();

    ~BoostTransport();

    void Connect(std::string hostname, short port);

    void Write(std::string content);

private:

    boost::asio::io_service io_service_;

    boost::asio::ip::tcp::socket socket_;
};


template<typename Transport>
class NetworkSender : public Sender
{
public:

    NetworkSender(std::shared_ptr<ReplicaSet> replicaset_)
        : replicaset(replicaset_)
    {
    }

    NetworkSender()
        : replicaset(std::make_shared<ReplicaSet>())
    {
    }

    void Reply(Message message)
    {
        std::lock_guard<std::mutex> guard(mutex);

        Transport transport;
        transport.Connect(message.to.hostname, message.to.port);

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

    void SendFile(Replica replica, BootstrapFile file)
    {
        std::lock_guard<std::mutex> guard(mutex);

        Transport transport;
        transport.Connect(replica.hostname, replica.port + 1);

        // 1. serialize file
        std::string file_str = Serialize(file);

        // 2. write file
        transport.Write(file_str);
    }

private:

    std::shared_ptr<ReplicaSet> replicaset;

    std::mutex mutex;
};


#endif
