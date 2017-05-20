#include "paxos/logging.hpp"
#include "paxos/sender.hpp"

#include <boost/asio/io_service.hpp>


using boost::asio::ip::tcp;


BoostTransport::BoostTransport()
    : io_service_(),
      socket_(io_service_)
{
}


BoostTransport::~BoostTransport()
{
    socket_.close();
}


void
BoostTransport::Connect(std::string hostname, short port)
{
    tcp::resolver resolver(io_service_);
    auto endpoint = resolver.resolve({hostname, std::to_string(port)});
    try
    {
        boost::asio::connect(socket_, endpoint);
    }
    catch (const boost::system::system_error& e)
    {
        LOG(LogLevel::Warning) << "Could not connect to " << hostname << ":" << port;
        socket_.close();
    }
}


void
BoostTransport::Write(std::string content)
{
    try
    {
        boost::asio::write(socket_, boost::asio::buffer(content.c_str(), content.size()));
    }
    catch (const boost::system::system_error& e)
    {
        LOG(LogLevel::Warning) << "Could not write to transport";
        socket_.close();
    }
}
