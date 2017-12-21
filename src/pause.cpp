#include <chrono>
#include <random>
#include <thread>

#include "paxos/pause.hpp"


void
NoPause::Start(std::function<void(void)> callback)
{
    callback();
}


RandomPause::RandomPause(
    std::chrono::milliseconds max)
    : max(max),
      io_service()
{
}


void
RandomPause::Start(std::function<void(void)> callback)
{
    std::random_device r;
    std::mt19937 generator(r());
    std::uniform_int_distribution<> distribution(0, max.count());

    std::thread t([&distribution, &generator, &callback]()
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(distribution(generator)));
        callback();
    });
    t.join();
}
