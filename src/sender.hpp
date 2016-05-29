#ifndef __SENDER_HPP_INCLUDED__
#define __SENDER_HPP_INCLUDED__


#include <string>
#include <vector>

#include <boost/asio.hpp>

#include <messages.hpp>
#include <replicaset.hpp>

class Sender
{
public:

    virtual void Reply(Message message) = 0;

    virtual void ReplyAll(Message message) = 0;
};


class NetworkSender : public Sender
{
public:

    NetworkSender();

    ~NetworkSender();

    void Reply(Message message);

    void ReplyAll(Message message);

private:

    boost::asio::io_service io_service_;

    boost::asio::ip::tcp::socket socket_;

    std::shared_ptr<ReplicaSet> replicaset;
};


class FakeSender : public Sender
{
public:

    FakeSender()
        : sent_messages(),
          replicaset(std::shared_ptr<ReplicaSet>(new ReplicaSet()))
    {
    }

    FakeSender(std::shared_ptr<ReplicaSet> replicaset_)
        : sent_messages(),
          replicaset(replicaset_)
    {
    }

    ~FakeSender()
    {
    }

    void Reply(Message message)
    {
        sent_messages.push_back(message);
    }

    void ReplyAll(Message message)
    {
        for (auto r : *replicaset)
        {
            sent_messages.push_back(message);
        }
    }

    std::vector<Message> sentMessages()
    {
        return sent_messages;
    }

private:

    std::vector<Message> sent_messages;

    std::shared_ptr<ReplicaSet> replicaset;
};


#endif
