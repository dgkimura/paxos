#ifndef __BOOTSTRAP_HPP_INCLUDED__
#define __BOOTSTRAP_HPP_INCLUDED__

#include "paxos/file.hpp"
#include "paxos/replicaset.hpp"
#include "paxos/serialization.hpp"
#include "paxos/server.hpp"


class Listener
{
};


template<typename Server>
class BootstrapListener : public Listener
{
public:

    BootstrapListener(std::string address, short port)
        : server(address, port)
    {
        server.RegisterAction([this](std::string content){
            // write out file
            BootstrapFile bootstrap = Deserialize<BootstrapFile>(content);
        });
    }

private:

    Server server;
};


void SendBootstrap(Replica replica, std::string location);


#endif
