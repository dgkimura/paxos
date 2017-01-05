#ifndef __SENDER_HPP_INCLUDED__
#define __SENDER_HPP_INCLUDED__


#include <mutex>
#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "paxos/messages.hpp"
#include "paxos/replicaset.hpp"

class Sender
{
public:

    virtual void Reply(Message message) = 0;

    virtual void ReplyAll(Message message) = 0;
};


class NetworkSender : public Sender
{
public:

    NetworkSender(std::shared_ptr<ReplicaSet> replicaset_);

    ~NetworkSender();

    void Reply(Message message);

    void ReplyAll(Message message);

private:

    boost::asio::io_service io_service_;

    boost::asio::ip::tcp::socket socket_;

    std::shared_ptr<ReplicaSet> replicaset;

    std::mutex mutex;
};


#endif
