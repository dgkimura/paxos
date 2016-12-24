#include "paxos/bootstrap.hpp"


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
    }
}


void
SendBootstrap(Replica replica, std::string location)
{
}
