#ifndef __SERVER_HPP_INCLUDED__
#define __SERVER_HPP_INCLUDED__

#include <thread>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>


namespace paxos
{


class SynchronousServer : public boost::enable_shared_from_this<SynchronousServer>
{
public:

    SynchronousServer(std::string address, short port);

    void RegisterAction(std::function<void(std::string content)> action_);

    void Start();

private:

    static const unsigned int HEADER_SIZE = 4;

    void do_accept();

    boost::asio::io_service io_service;

    boost::asio::ip::tcp::acceptor acceptor;

    std::function<void(std::string content)> action;
};


class AsynchronousServer : public boost::enable_shared_from_this<AsynchronousServer>
{
public:

    AsynchronousServer(std::string address, short port);

    ~AsynchronousServer();

    void RegisterAction(std::function<void(std::string content)> action_);

    void Start();

private:

    static const unsigned int HEADER_SIZE = 4;

    void do_accept();

    boost::asio::io_service io_service;

    boost::asio::ip::tcp::acceptor acceptor;

    boost::asio::ip::tcp::socket socket;

    class Session : public boost::enable_shared_from_this<Session>
    {
    public:
        Session(
            boost::asio::ip::tcp::socket socket,
            std::function<void(std::string content)> action
        );

        void Start();

    private:

        void handle_read_header(const boost::system::error_code& err);

        void handle_read_message(unsigned int message_size);

        void handle_process_message(const boost::system::error_code& err);

        boost::asio::ip::tcp::socket socket;

        std::vector<uint8_t> readbuf;

        std::function<void(std::string content)> action;
    };

    std::function<void(std::string content)> action;
};


}


#endif
