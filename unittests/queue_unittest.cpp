#include <functional>

#include "gtest/gtest.h"

#include "paxos/queue.hpp"


int GetQueueSize(std::shared_ptr<BaseQueue<std::string>> queue)
{
    int size = 0;
    for (auto d : *queue)
    {
        size += 1;
    }
    return size;
}


TEST(QueueTest, testThatVolatileQueueCanEnqueueAnElement)
{
    auto queue = std::make_shared<VolatileQueue<std::string>>();

    queue->Enqueue("narf");
    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST(QueueTest, testThatVolatileQueueCanDequeueAnElement)
{
    auto queue = std::make_shared<VolatileQueue<std::string>>();

    queue->Enqueue("narf");
    queue->Dequeue();

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST(QueueTest, testThatVolatileQueueCanIterateOverContents)
{
    auto queue = std::make_shared<VolatileQueue<std::string>>();

    queue->Enqueue("narf");

    for (auto e : *queue)
    {
        ASSERT_EQ(e, "narf");
    }
}


TEST(QueueTest, testThatPersistentQueueCanIterateOverContents)
{
    std::stringstream file;

    auto queue = std::make_shared<PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");

    for (auto e : *queue)
    {
        ASSERT_EQ(e, "narf");
    }
}


TEST(QueueTest, testThatPersistentQueueSizeAfterEnqueue)
{
    std::stringstream file;

    auto queue = std::make_shared<PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");

    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST(QueueTest, testThatPersistentQueueCanDequeue)
{
    std::stringstream file;

    auto queue = std::make_shared<PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");
    queue->Enqueue("poit");
    queue->Dequeue();

    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST(QueueTest, testThatPersistentQueueIsCanRehydrateFromAPrevousPersistentQueue)
{
    std::stringstream file;

    auto queue = std::make_shared<PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");
    queue->Enqueue("poit");
    queue->Dequeue();

    auto next_queue = std::make_shared<PersistentQueue<std::string>>(file);

    ASSERT_EQ(GetQueueSize(next_queue), 2);

    next_queue->Dequeue();
    auto final_queue = std::make_shared<PersistentQueue<std::string>>(file);

    ASSERT_EQ(GetQueueSize(final_queue), 1);
}


TEST(QueueTest, testThatPersistentQueueGetLastElementOnEmptyQueue)
{
    std::stringstream file;

    auto queue = std::make_shared<PersistentQueue<std::string>>(file);

    ASSERT_EQ(queue->Last(), "");
}


TEST(QueueTest, testThatPersistentQueueGetLastElement)
{
    std::stringstream file;

    auto queue = std::make_shared<PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");
    queue->Enqueue("poit");

    ASSERT_EQ(queue->Last(), "poit");
}
