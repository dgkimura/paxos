#ifndef __SIGNAL_HPP_INCLUDED__
#define __SIGNAL_HPP_INCLUDED__

#include <condition_variable>
#include <mutex>


class Signal
{
public:

    Signal();

    void Set();

    void Wait();

private:
    std::mutex mutex;

    std::condition_variable condition;

    bool flag;
};


#endif