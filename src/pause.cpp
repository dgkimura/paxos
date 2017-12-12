#include <random>
#include <thread>

#include <boost/date_time/posix_time/posix_time.hpp>

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

    auto interval = boost::posix_time::milliseconds(distribution(generator));
    boost::asio::deadline_timer timer(io_service, interval);
    timer.async_wait(
        [&callback](const boost::system::error_code&){ callback(); }
    );
}
