#include <thread>

#include "paxos/signal.hpp"


namespace paxos
{


Signal::Signal(
    std::function<void(void)> retry,
    std::chrono::milliseconds retry_interval)
    : flag(false),
      retry(retry),
      retry_interval(retry_interval)
{
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
    while (condition.wait_for(lock, retry_interval) == std::cv_status::timeout)
    {
        if (flag)
        {
            flag = false;
            return success;
        }
        else
        {
            retry();
        }
    }
    return success;
}


}
