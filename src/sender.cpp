#include "paxos/logging.hpp"
#include "paxos/sender.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>


namespace paxos
{


BoostTransport::BoostTransport(std::string hostname, short port)
    : io_service_(),
      socket_(io_service_),
      resolver_(io_service_),
      timer_(io_service_)
{
    endpoint_ = resolver_.resolve({hostname, std::to_string(port)});
    boost::system::error_code ec;
    socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    socket_.set_option(boost::asio::socket_base::keep_alive(false), ec);

    check_deadline();

    //
    // Set up the variable that receives the result of the asynchronous
    // operation. The error code is set to would_block to signal that the
    // operation is incomplete. Asio guarantees that its asynchronous
    // operations will never fail with would_block, so any other value in
    // ec indicates completion.
    //
    ec = boost::asio::error::would_block;
    boost::asio::async_connect(socket_, endpoint_,
                               boost::lambda::var(ec) = boost::lambda::_1);
    do io_service_.run_one(); while (ec == boost::asio::error::would_block);
}


BoostTransport::~BoostTransport()
{
    socket_.close();
}


void
BoostTransport::Write(std::string content)
{
    timer_.expires_from_now(boost::posix_time::seconds(1));

    try
    {
        if (socket_.is_open())
        {
            auto message_size = content.size();
            std::vector<uint8_t> header(4);
            header[0] = static_cast<uint8_t>((message_size >> 24) & 0xFF);
            header[1] = static_cast<uint8_t>((message_size >> 16) & 0xFF);
            header[2] = static_cast<uint8_t>((message_size >> 8) & 0xFF);
            header[3] = static_cast<uint8_t>((message_size >> 0) & 0xFF);
            boost::asio::write(socket_, boost::asio::buffer(header,
                                                            header.size()));

            boost::asio::write(socket_, boost::asio::buffer(content.c_str(),
                                                            content.size()));
        }
    }
    catch (const boost::system::system_error& e)
    {
        LOG(LogLevel::Warning) << "Could not write to transport - " << e.what();

        //
        // Try to re-establish a connection.
        //
        boost::system::error_code ec = boost::asio::error::would_block;
        boost::asio::async_connect(socket_, endpoint_,
                                   boost::lambda::var(ec) = boost::lambda::_1);
    }
}


void
BoostTransport::check_deadline()
{
    if (timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        //
        // The deadline has passed. The socket is closed so that any
        // outstanding asynchronous operations are cancelled. This allows the
        // blocked connect(), read_line() or write_line() functions to return.
        //
        boost::system::error_code ignored_ec;
        socket_.close(ignored_ec);

        //
        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is
        // set.
        //
        timer_.expires_at(boost::posix_time::pos_infin);
    }

    timer_.async_wait(
        boost::lambda::bind(&BoostTransport::check_deadline, this));
}


}
