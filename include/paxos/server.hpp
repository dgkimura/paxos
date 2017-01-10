#ifndef __SERVER_HPP_INCLUDED__
#define __SERVER_HPP_INCLUDED__

#include <thread>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>


class BoostServer : public boost::enable_shared_from_this<BoostServer>
{
public:

    BoostServer(std::string address, short port);

    ~BoostServer();

    void RegisterAction(std::function<void(std::string content)> action_);

    void Start();

private:

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

        void handle_read_message(const boost::system::error_code& err);

        boost::asio::ip::tcp::socket socket;

        boost::asio::streambuf response;

        std::function<void(std::string content)> action;
    };

    std::function<void(std::string content)> action;
};


#endif
