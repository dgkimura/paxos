#ifndef __BOOTSTRAP_HPP_INCLUDED__
#define __BOOTSTRAP_HPP_INCLUDED__

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/shared_ptr.hpp>

#include "paxos/file.hpp"
#include "paxos/replicaset.hpp"
#include "paxos/sender.hpp"
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
        : server(boost::make_shared<Server>(address, port))
    {
        server->RegisterAction([this](std::string content){
            // write out file
            BootstrapFile bootstrap = Deserialize<BootstrapFile>(content);
            std::fstream file(
                bootstrap.name,
                std::ios::out | std::ios::trunc | std::ios::binary);
            file << bootstrap.content;
        });
        server->Start();
    }

private:

    boost::shared_ptr<Server> server;
};


template<typename Transport>
void SendBootstrap(Replica replica, std::string location)
{
    for (auto& entry : boost::make_iterator_range(
         boost::filesystem::directory_iterator(location), {}))
    {
        NetworkSender<Transport> sender;

        // 1. serialize file
        std::ifstream filestream(entry.path().native());
        std::stringstream buffer;
        buffer << filestream.rdbuf();

        // 2. send bootstrap file
        BootstrapFile file;
        file.name = entry.path().native();
        file.content = buffer.str();
        sender.SendFile(replica, file);
    }
}


#endif
