#include <sender.hpp>

#include <boost/asio/io_service.hpp>


using boost::asio::ip::tcp;


NetworkSender::NetworkSender()
    : io_service_(),
      socket_(io_service_),
      replicaset()
{
}


NetworkSender::~NetworkSender()
{
    io_service_.post([this]() { socket_.close(); } );
}


void
NetworkSender::Reply(Message message)
{
    tcp::resolver resolver(io_service_);
    auto endpoint = resolver.resolve({message.to.hostname, std::to_string(message.to.port)});
    boost::asio::async_connect(socket_, endpoint,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
        {
            if (!ec)
            {
                // 1. serialize message
                // 2. write message
            }
        });
}


void
NetworkSender::ReplyAll(Message message)
{
    for (auto r : *replicaset)
    {
        Message m = message;
        m.to = r;
        Reply(m);
    }
}
