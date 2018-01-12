#include "paxos/signal.hpp"


namespace paxos
{


Signal::Signal()
{
    flag = false;
}

void
Signal::Set(bool success_)
{
    std::unique_lock<std::mutex> lock(mutex);
    flag = true;
    success = success_;
    condition.notify_one();
}

bool
Signal::Wait()
{
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&] { return flag; });
    flag = false;
    return success;
}


}
