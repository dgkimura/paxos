#include <boost/make_shared.hpp>

#include "paxos/server.hpp"


namespace paxos
{


using boost::asio::ip::tcp;


SynchronousServer::SynchronousServer(std::string address, short port)
    : io_service(),
      acceptor(
          io_service,
          boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(address), port))
{
}


void
SynchronousServer::RegisterAction(
    std::function<void(const std::string& content)> action_)
{
    action = action_;
}


void
SynchronousServer::Start()
{
    auto self(shared_from_this());
    std::thread([this, self]() { do_accept(); }).detach();
}


void
SynchronousServer::do_accept()
{
    for (;;)
    {
        boost::asio::ip::tcp::socket socket(io_service);
        acceptor.accept(socket);

        std::vector<uint8_t> read_buffer(HEADER_SIZE);
        boost::asio::read(socket, boost::asio::buffer(read_buffer),
                          boost::asio::transfer_exactly(HEADER_SIZE));

        int message_size = 0;
        for (int i=0; i<HEADER_SIZE; i++)
        {
            message_size = message_size * 256 +
                           (static_cast<uint8_t>(read_buffer[i]) & 0xFF);
        }

        std::string content;
        read_buffer.resize(message_size);
        while (message_size > 0)
        {
            read_buffer.resize(message_size >= 1024 ? 1024 : message_size);

            boost::system::error_code ec;
            int read_bytes = boost::asio::read(
                socket,
                boost::asio::buffer(read_buffer),
                boost::asio::transfer_exactly(message_size),
                ec);
            content += std::string(read_buffer.begin(), read_buffer.end());
            message_size -= read_bytes;
        }

        action(content);

        // signal done, syn-ack...
        std::vector<uint8_t> a_byte{1};
        boost::asio::write(socket, boost::asio::buffer(a_byte,
                                                       a_byte.size()));
    }
}


AsynchronousServer::AsynchronousServer(std::string address, short port)
    : io_service(),
      acceptor(
          io_service,
          tcp::endpoint(boost::asio::ip::address::from_string(address), port)),
      socket(io_service)
{
    boost::system::error_code ec;
    socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
    socket.set_option(boost::asio::socket_base::keep_alive(false), ec);
    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.set_option(boost::asio::ip::tcp::acceptor::keep_alive(false));
    do_accept();
}


AsynchronousServer::~AsynchronousServer()
{
    io_service.stop();
    acceptor.close();
}


void
AsynchronousServer::Start()
{
    // Updating shared reference and asio run must happen after calling the
    // constructor because before that point the object isn't guaranteed to be
    // fully instatiated yet. As such, start is expected to be called shortly
    // after construction has finished.
    auto self(shared_from_this());
    std::thread([this, self]() { io_service.run(); }).detach();
}


void
AsynchronousServer::RegisterAction(
    std::function<void(const std::string& content)> action_)
{
    action = action_;
}


void
AsynchronousServer::do_accept()
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


AsynchronousServer::Session::Session(
    boost::asio::ip::tcp::socket socket,
    std::function<void(const std::string& content)> action
)
    : socket(std::move(socket)),
      action(action)
{
}


void
AsynchronousServer::Session::Start()
{
    readbuf.resize(HEADER_SIZE);
    boost::asio::async_read(socket, boost::asio::buffer(readbuf),
        boost::bind(&Session::handle_read_header, shared_from_this(),
            boost::asio::placeholders::error));
}


void
AsynchronousServer::Session::handle_read_header(
    const boost::system::error_code& err)
{
    if (!err)
    {
        unsigned int message_size = 0;
        for (int i=0; i<HEADER_SIZE; i++)
        {
            message_size = message_size * 256 +
                           (static_cast<uint8_t>(readbuf[i]) & 0xFF);
        }
        handle_read_message(message_size);
    }
}


void
AsynchronousServer::Session::handle_read_message(
    unsigned int message_size)
{
    readbuf.resize(HEADER_SIZE + message_size);
    boost::asio::mutable_buffers_1 buf = boost::asio::buffer(
        &readbuf[HEADER_SIZE], message_size);

    boost::asio::async_read(socket, buf,
        boost::bind(&Session::handle_process_message, shared_from_this(),
            boost::asio::placeholders::error));
}


void
AsynchronousServer::Session::handle_process_message(
    const boost::system::error_code& err)
{
    if (!err)
    {
        std::string content(readbuf.begin() + HEADER_SIZE, readbuf.end());

        action(content);
        Start();
    }
}


}
