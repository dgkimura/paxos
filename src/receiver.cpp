#include "paxos/receiver.hpp"
#include "paxos/serialization.hpp"


using boost::asio::ip::tcp;


NetworkReceiver::NetworkReceiver(std::string address, short port)
    : io_service(),
      acceptor(
          io_service,
          tcp::endpoint(boost::asio::ip::address::from_string(address), port)),
      socket(io_service),
      registered_map()
{
    do_accept();

    std::thread([&]() { io_service.run(); }).detach();
}


NetworkReceiver::~NetworkReceiver()
{
    io_service.stop();
    acceptor.close();
}


void
NetworkReceiver::RegisterCallback(Callback&& callback, MessageType type)
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
NetworkReceiver::do_accept()
{
    acceptor.async_accept(socket,
        [this](boost::system::error_code ec_accept)
        {
            if (!ec_accept)
            {
                boost::make_shared<Session>(std::move(socket), registered_map)->Start();
            }
            do_accept();
        });
}


NetworkReceiver::Session::Session(
    boost::asio::ip::tcp::socket socket,
    std::unordered_map<
        MessageType,
        std::vector<Callback>,
        MessageTypeHash> registered_map
)
    : socket(std::move(socket)),
      registered_map_(registered_map)
{
}


void
NetworkReceiver::Session::Start()
{
    boost::asio::async_read(socket, response,
        boost::asio::transfer_at_least(1),
        boost::bind(&Session::handle_read_message, shared_from_this(),
            boost::asio::placeholders::error));
}


void
NetworkReceiver::Session::handle_read_message(
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
        Message message = Deserialize<Message>(
            std::string(
                boost::asio::buffers_begin(bufs),
                boost::asio::buffers_begin(bufs) + response.size()));

        for (Callback callback : registered_map_[message.type])
        {
            callback(message);
        }
    }
}
