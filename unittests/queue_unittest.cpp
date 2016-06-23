#include <functional>

#include "gtest/gtest.h"

#include "queue.hpp"


TEST(QueueTest, testThatVolatileQueueCanEnqueueAnElement)
{
    VolatileQueue<std::string> queue;

    queue.Enqueue("narf");
    ASSERT_EQ(queue.Size(), 1);
}


TEST(QueueTest, testThatVolatileQueueCanDequeueAnElement)
{
    VolatileQueue<std::string> queue;

    queue.Enqueue("narf");
    queue.Dequeue();

    ASSERT_EQ(queue.Size(), 0);
}


TEST(QueueTest, testThatVolatileQueueCanIterateOverContents)
{
    VolatileQueue<std::string> queue;

    queue.Enqueue("narf");

    for (auto e : queue)
    {
        ASSERT_EQ(e, "narf");
    }
}
