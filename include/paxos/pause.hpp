#ifndef __PAUSE_HPP_INCLUDED__
#define __PAUSE_HPP_INCLUDED__

#include <chrono>
#include <functional>

#include <boost/asio.hpp>


namespace paxos
{


class Pause
{
public:

    virtual void Start(std::function<void(void)> callback) = 0;
};


class NoPause : public Pause
{
public:

    virtual void Start(std::function<void(void)> callback) override;
};


class RandomPause : public Pause
{
public:

    RandomPause(std::chrono::milliseconds max);

    virtual void Start(std::function<void(void)> callback) override;

private:

    std::chrono::milliseconds max;

    boost::asio::io_service io_service;
};


}


#endif
