#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/iterator_range.hpp>

#include "paxos/bootstrap.hpp"


void
SendBootstrap(Replica replica, std::string location)
{
    for (auto& entry : boost::make_iterator_range(
                           boost::filesystem::directory_iterator(location), {}))
    {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::socket socket(io_service);
        boost::asio::ip::tcp::resolver resolver(io_service);
        auto endpoint = resolver.resolve(
                            {
                                replica.hostname,
                                std::to_string(replica.port)
                            });
        boost::asio::connect(socket, endpoint);

        // 1. serialize bootstrap
        std::ifstream filestream(location);
        std::stringstream buffer;
        buffer << filestream.rdbuf();

        BootstrapFile bootstrap;
        bootstrap.content = buffer.str();
        std::string bootstrap_str = Serialize(bootstrap);

        // 2. write bootstrap
        boost::asio::write(socket, boost::asio::buffer(bootstrap_str.c_str(),
                                                       bootstrap_str.size()));

        // 3. close socket signals to server end of bootstrap
        socket.close();
    }
}
