#include <functional>

#include "gtest/gtest.h"

#include "paxos/queue.hpp"


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


TEST(QueueTest, testThatPersistentQueueCanIterateOverContents)
{
    std::stringstream file;

    PersistentQueue<std::string> queue(file);

    queue.Enqueue("narf");

    for (auto e : queue)
    {
        ASSERT_EQ(e, "narf");
    }
}


TEST(QueueTest, testThatPersistentQueueCanGetSize)
{
    std::stringstream file;

    PersistentQueue<std::string> queue(file);

    queue.Enqueue("narf");
    queue.Enqueue("zort");

    ASSERT_EQ(queue.Size(), 2);
}


TEST(QueueTest, testThatPersistentQueueCanDequeue)
{
    std::stringstream file;

    PersistentQueue<std::string> queue(file);

    queue.Enqueue("narf");
    queue.Enqueue("zort");
    queue.Enqueue("poit");
    queue.Dequeue();

    ASSERT_EQ(queue.Size(), 2);
}


TEST(QueueTest, testThatPersistentQueueIsCanRehydrateFromAPrevousPersistentQueue)
{
    std::stringstream file;

    PersistentQueue<std::string> queue(file);

    queue.Enqueue("narf");
    queue.Enqueue("zort");
    queue.Enqueue("poit");
    queue.Dequeue();

    PersistentQueue<std::string> next_queue(file);

    ASSERT_EQ(next_queue.Size(), 2);

    next_queue.Dequeue();
    PersistentQueue<std::string> final_queue(file);

    ASSERT_EQ(final_queue.Size(), 1);
}
