#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include "paxos/bootstrap.hpp"
#include "paxos/sender.hpp"


void
SendBootstrap(Replica replica, std::string location)
{
    for (auto& entry : boost::make_iterator_range(
         boost::filesystem::directory_iterator(location), {}))
    {
        NetworkSender<BoostTransport> sender;

        // 1. serialize file
        std::ifstream filestream(location);
        std::stringstream buffer;
        buffer << filestream.rdbuf();

        // 2. send bootstrap file
        BootstrapFile file;
        file.content = buffer.str();
        sender.SendFile(replica, file);
    }
}
