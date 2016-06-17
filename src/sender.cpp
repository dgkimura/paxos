#include <sender.hpp>
#include <serialization.hpp>

#include <boost/asio/io_service.hpp>


using boost::asio::ip::tcp;


NetworkSender::NetworkSender(std::shared_ptr<ReplicaSet> replicaset_)
    : io_service_(),
      socket_(io_service_),
      replicaset(replicaset_)
{
}


NetworkSender::NetworkSender()
    : io_service_(),
      socket_(io_service_),
      replicaset(new ReplicaSet())
{
}


NetworkSender::~NetworkSender()
{
    socket_.close();
}


void
NetworkSender::Reply(Message message)
{
    tcp::resolver resolver(io_service_);
    auto endpoint = resolver.resolve(
                        {
                            message.to.hostname,
                            std::to_string(message.to.port)
                        });
    std::lock_guard<std::mutex> guard(mutex);
    boost::asio::connect(socket_, endpoint);

    // 1. serialize message
    std::string message_str = Serialize(message);

    // 2. write message
    boost::asio::write(socket_, boost::asio::buffer(message_str.c_str(), message_str.size()));
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
