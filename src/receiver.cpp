#include <receiver.hpp>


using boost::asio::ip::tcp;


Receiver::Receiver(short port)
    : io_service_(),
      acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)),
      socket_(io_service_),
      registered_map()
{


    do_accept();

    io_service_.run();
}


void
Receiver::RegisterCallback(Callback&& callback, MessageType type)
{
    if (registered_map.find(type) == registered_map.end())
    {
        registered_map[type] = std::vector<Callback> { std::move(callback) };
    }
    else
    {
        registered_map[type].push_back(std::move(callback));
    }
}


void
Receiver::do_accept()
{
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec_accept)
        {
            if (!ec_accept)
            {
                socket_.async_read_some(boost::asio::buffer(data_, max_length),
                    [this](boost::system::error_code ec_read, std::size_t length)
                    {
                        if (!ec_read)
                        {
                            // 1. Deserialize data_ into Message
                            // 2. Callback registered functions with Message.
                        }
                    });
            }
            do_accept();
        });
}
