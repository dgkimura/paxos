#ifndef __RECEIVER_HPP_INCLUDED__
#define __RECEIVER_HPP_INCLUDED__

#include <functional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include "paxos/callback.hpp"
#include "paxos/customhash.hpp"
#include "paxos/messages.hpp"
#include "paxos/server.hpp"


class Receiver
{
public:

    virtual void RegisterCallback(Callback&& callback, MessageType type) = 0;
};

template<typename Server>
class NetworkReceiver : public Receiver
{
public:

    NetworkReceiver(std::string address, short port)
        : server(boost::make_shared<Server>(address, port))
    {
        server->RegisterAction([this](std::string content){
            ProcessContent(content);
        });
        server->Start();
    }

    void ProcessContent(std::string content)
    {
        Message message = Deserialize<Message>(content);

        for (Callback callback : GetRegisteredCallbacks(message.type))
        {
            callback(message);
        }
    }

    void RegisterCallback(Callback&& callback, MessageType type) override
    {
        if (registered_map.find(type) == registered_map.end())
        {
            registered_map[type] = std::vector<Callback> { std::move(callback) };
        }
        else
        {
            registered_map[type].push_back(std::move(callback));
        }
    }

    std::vector<Callback> GetRegisteredCallbacks(MessageType type)
    {
        if (registered_map.find(type) != registered_map.end())
        {
            return registered_map[type];
        }
        else
        {
            return std::vector<Callback>();
        }
    }

private:

    boost::shared_ptr<Server> server;

    std::unordered_map<MessageType, std::vector<Callback>> registered_map;
};


#endif
