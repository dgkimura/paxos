#include <boost/make_shared.hpp>

#include "paxos/server.hpp"


using boost::asio::ip::tcp;


BoostServer::BoostServer(std::string address, short port)
    : io_service(),
      acceptor(
          io_service,
          tcp::endpoint(boost::asio::ip::address::from_string(address), port)),
      socket(io_service)
{
    do_accept();
}


BoostServer::~BoostServer()
{
    io_service.stop();
    acceptor.close();
}


void
BoostServer::Start()
{
    // Updating shared reference and asio run must happen outside of the
    // constructor because at that point the object isn't guaranteed to be
    // fully instatiated yet. As such, start is expected to be called shortly
    // after construction has finished.
    auto self(shared_from_this());
    std::thread([this, self]() { io_service.run(); }).detach();
}


void
BoostServer::RegisterAction(std::function<void(std::string content)> action_)
{
    action = action_;
}


void
BoostServer::do_accept()
{
    acceptor.async_accept(socket,
        [this](boost::system::error_code ec_accept)
        {
            if (!ec_accept)
            {
                boost::make_shared<Session>(std::move(socket), action)->Start();
            }
            do_accept();
        });
}


BoostServer::Session::Session(
    boost::asio::ip::tcp::socket socket,
    std::function<void(std::string content)> action
)
    : socket(std::move(socket)),
      action(action)
{
}


void
BoostServer::Session::Start()
{
    boost::asio::async_read(socket, response,
        boost::asio::transfer_at_least(1),
        boost::bind(&Session::handle_read_message, shared_from_this(),
            boost::asio::placeholders::error));
}


void
BoostServer::Session::handle_read_message(
    const boost::system::error_code& err)
{
    if (!err)
    {
        boost::asio::async_read(socket, response,
            boost::asio::transfer_at_least(1),
            boost::bind(&Session::handle_read_message, shared_from_this(),
                boost::asio::placeholders::error));
    }
    if (err == boost::asio::error::eof)
    {
        boost::asio::streambuf::const_buffers_type bufs = response.data();
        std::string content(
            boost::asio::buffers_begin(bufs),
            boost::asio::buffers_begin(bufs) + response.size());
        action(content);
    }
}
