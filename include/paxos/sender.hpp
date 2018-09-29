#ifndef __SENDER_HPP_INCLUDED__
#define __SENDER_HPP_INCLUDED__


#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "paxos/messages.hpp"
#include "paxos/replicaset.hpp"
#include "paxos/serialization.hpp"


namespace paxos
{


std::vector<uint8_t> CreateHeader(size_t message_size);


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

    void Read();

private:

    void check_deadline();

    boost::asio::io_service io_service_;

    boost::asio::ip::tcp::socket socket_;

    boost::asio::ip::tcp::resolver resolver_;

    boost::asio::ip::basic_resolver_iterator<boost::asio::ip::tcp> endpoint_;

    boost::asio::deadline_timer timer_;
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

        std::string key = message.to.hostname + ":" +
                          std::to_string(message.to.port);
        if (cached_transports.find(key) == std::end(cached_transports))
        {
            cached_transports[key] = std::unique_ptr<Transport>(
                                        new Transport(message.to.hostname,
                                                      message.to.port));
        }
        auto& transport = cached_transports[key];

        // 1. serialize message
        std::string message_str = Serialize(message);

        if (IsValidMessageString(message_str))
        {
            // 2. write message
            transport->Write(message_str);
        }
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

    std::unordered_map<std::string, std::unique_ptr<Transport>> cached_transports;

    std::mutex mutex;

    bool IsValidMessageString(const std::string& message_str)
    {
        return message_str.size() > 0;
    }
};



template<typename Transport>
class NetworkFileSender : public FileSender
{
public:

    void SendFile(Replica replica, BootstrapFile file)
    {
        std::lock_guard<std::mutex> guard(mutex);

        std::string key = replica.hostname + ":" +
                          std::to_string(replica.port + 1);
        auto transport = std::unique_ptr<Transport>(
                                    new Transport(replica.hostname,
                                                  replica.port + 1));

        // 1. serialize file
        std::string file_str = Serialize(file);

        // 2. write file
        transport->Write(file_str);

        // 3. block until file send completed
        transport->Read();
    }

private:

    std::unordered_map<std::string, std::unique_ptr<Transport>> cached_transports;

    std::mutex mutex;
};


}


#endif
