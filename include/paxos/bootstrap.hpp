#ifndef __BOOTSTRAP_HPP_INCLUDED__
#define __BOOTSTRAP_HPP_INCLUDED__

#include <fstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/shared_ptr.hpp>

#include "paxos/file.hpp"
#include "paxos/replicaset.hpp"
#include "paxos/sender.hpp"
#include "paxos/serialization.hpp"
#include "paxos/server.hpp"


namespace paxos
{


class Listener
{
};


template<typename Server>
class BootstrapListener : public Listener
{
public:

    BootstrapListener(
        std::shared_ptr<ReplicaSet>& legislators,
        std::string address,
        short port)
        : server(boost::make_shared<Server>(address, port))
    {
        server->RegisterAction([this, &legislators](std::string content){
            // write out file
            BootstrapFile bootstrap = Deserialize<BootstrapFile>(content);
            std::fstream file(
                bootstrap.name,
                std::ios::out | std::ios::trunc | std::ios::binary);
            file << bootstrap.content;
            if (boost::algorithm::ends_with(bootstrap.name, ReplicasetFilename))
            {
                legislators =  LoadReplicaSet(
                    std::stringstream(bootstrap.content));
            }
        });
        server->Start();
    }

private:

    boost::shared_ptr<Server> server;
};


template<typename Transport>
void SendBootstrap(
    Replica replica,
    std::string local_directory,
    std::string remote_directory)
{
    for (auto& entry : boost::make_iterator_range(
         boost::filesystem::directory_iterator(local_directory), {}))
    {
        NetworkFileSender<Transport> sender;

        // 1. serialize file
        std::ifstream filestream(entry.path().native());
        std::stringstream buffer;
        buffer << filestream.rdbuf();

        // 2. send bootstrap file
        BootstrapFile file;
        boost::filesystem::path remotepath(remote_directory);
        remotepath /= entry.path().filename();
        file.name = remotepath.native();
        file.content = buffer.str();
        sender.SendFile(replica, file);
    }
}


}


#endif
