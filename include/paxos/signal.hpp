#ifndef __SIGNAL_HPP_INCLUDED__
#define __SIGNAL_HPP_INCLUDED__

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>


namespace paxos
{


class Signal
{
public:

    Signal(
        std::function<void(void)> retry,
        std::chrono::milliseconds retry_interval=std::chrono::milliseconds(0));

    void Set(bool success);

    bool Wait();

private:
    std::mutex mutex;

    std::condition_variable condition;

    bool flag;

    std::function<void()> retry;

    std::chrono::milliseconds retry_interval;

    bool success;
};


}


#endif
