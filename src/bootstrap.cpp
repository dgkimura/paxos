#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include "paxos/bootstrap.hpp"
#include "paxos/serialization.hpp"


BootstrapListener::BootstrapListener(std::string address, short port)
    : io_service(),
      acceptor(
          io_service,
          boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(address), port)),
      socket(io_service)
{
    do_accept();
}


void
BootstrapListener::do_accept()
{
    acceptor.async_accept(socket,
        [this](boost::system::error_code ec_accept)
        {
            if (!ec_accept)
            {
                boost::make_shared<BootstrapSession>(std::move(socket))->Start();
            }
            do_accept();
        });
}


BootstrapListener::BootstrapSession::BootstrapSession(
    boost::asio::ip::tcp::socket socket
)
    : socket(std::move(socket))
{
}


void
BootstrapListener::BootstrapSession::Start()
{
    boost::asio::async_read(socket, response,
        boost::asio::transfer_at_least(1),
        boost::bind(&BootstrapSession::handle_read_message, shared_from_this(),
            boost::asio::placeholders::error));
}


void
BootstrapListener::BootstrapSession::handle_read_message(
    const boost::system::error_code& err)
{
    if (!err)
    {
        boost::asio::async_read(socket, response,
            boost::asio::transfer_at_least(1),
            boost::bind(&BootstrapSession::handle_read_message, shared_from_this(),
                boost::asio::placeholders::error));
    }
    if (err == boost::asio::error::eof)
    {
        boost::asio::streambuf::const_buffers_type bufs = response.data();
        std::string message_str(
            boost::asio::buffers_begin(bufs),
            boost::asio::buffers_begin(bufs) + response.size());

        // write out file
        BootstrapFile bootstrap = Deserialize<BootstrapFile>(
            std::string(
                boost::asio::buffers_begin(bufs),
                boost::asio::buffers_begin(bufs) + response.size()));
    }
}


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
