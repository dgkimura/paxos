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

    Signal();

    void Set(bool success);

    bool Wait();

private:
    std::mutex mutex;

    std::condition_variable condition;

    bool flag;

    bool success;
};


}


#endif
