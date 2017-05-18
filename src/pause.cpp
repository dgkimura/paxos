#include <random>
#include <thread>

#include "paxos/pause.hpp"


void
NoPause::Start()
{
}


RandomPause::RandomPause(std::chrono::milliseconds max)
    : max(max)
{
}


void
RandomPause::Start()
{
    std::random_device r;
    std::mt19937 generator(r());
    std::uniform_int_distribution<> distribution(0, max.count());

    auto sleep_interval = std::chrono::milliseconds(distribution(generator));
    std::this_thread::sleep_for(sleep_interval);
}
