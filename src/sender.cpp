#include "paxos/logging.hpp"
#include "paxos/sender.hpp"

#include <boost/asio/io_service.hpp>


BoostTransport::BoostTransport(std::string hostname, short port)
    : io_service_(),
      socket_(io_service_),
      resolver_(io_service_)
{
    endpoint_ = resolver_.resolve({hostname, std::to_string(port)});
    boost::system::error_code ec;
    socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    socket_.set_option(boost::asio::socket_base::keep_alive(true), ec);
}


BoostTransport::~BoostTransport()
{
    socket_.close();
}


void
BoostTransport::Write(std::string content)
{
    try
    {
        boost::asio::connect(socket_, endpoint_);

        boost::asio::write(socket_, boost::asio::buffer(content.c_str(), content.size()));
        socket_.close();
    }
    catch (const boost::system::system_error& e)
    {
        LOG(LogLevel::Warning) << "Could not write to transport - " << e.what();
        socket_.close();
    }
}
