#include <functional>

#include "gtest/gtest.h"

#include "paxos/queue.hpp"


template<typename T>
int GetQueueSize(std::shared_ptr<T> queue)
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
    auto queue = std::make_shared<paxos::VolatileQueue<std::string>>();

    queue->Enqueue("narf");
    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST(QueueTest, testThatVolatileQueueCanDequeueAnElement)
{
    auto queue = std::make_shared<paxos::VolatileQueue<std::string>>();

    queue->Enqueue("narf");
    queue->Dequeue();

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST(QueueTest, testThatVolatileQueueCanIterateOverContents)
{
    auto queue = std::make_shared<paxos::VolatileQueue<std::string>>();

    queue->Enqueue("narf");

    for (auto e : *queue)
    {
        ASSERT_EQ(e, "narf");
    }
}


TEST(QueueTest, testThatPersistentQueueCanIterateOverContents)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");

    for (auto e : *queue)
    {
        ASSERT_EQ(e, "narf");
    }
}


TEST(QueueTest, testThatPersistentQueueSizeAfterEnqueue)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");

    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST(QueueTest, testThatPersistentQueueCanDequeue)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");
    queue->Enqueue("poit");
    queue->Dequeue();

    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST(QueueTest, testThatPersistentQueueIsCanRehydrateFromAPrevousPersistentQueue)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");
    queue->Enqueue("poit");
    queue->Dequeue();

    auto next_queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    ASSERT_EQ(GetQueueSize(next_queue), 2);

    next_queue->Dequeue();
    auto final_queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    ASSERT_EQ(GetQueueSize(final_queue), 1);
}


TEST(QueueTest, testThatPersistentQueueGetLastElementOnEmptyQueue)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    ASSERT_EQ(queue->Last(), "");
}


TEST(QueueTest, testThatPersistentQueueGetLastElement)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");
    queue->Enqueue("poit");

    ASSERT_EQ(queue->Last(), "poit");
}


TEST(QueueTest, testThatPersistentQueueEnqueueAfterDequeueReplacesElement)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::PersistentQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Dequeue();
    queue->Enqueue("zoit");

    ASSERT_EQ(queue->Last(), "zoit");
    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST(QueueTest, testThatRolloverQueueCanIterateOverContents)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::RolloverQueue<std::string>>(file);

    queue->Enqueue("narf");

    for (auto e : *queue)
    {
        ASSERT_EQ(e, "narf");
    }
}


TEST(QueueTest, testThatRolloverQueueSizeAfterEnqueue)
{
    std::stringstream file;

    std::string element("narf");

    auto queue = std::make_shared<paxos::RolloverQueue<std::string>>(file, paxos::Serialize(element).length());

    queue->Enqueue(element);
    queue->Enqueue("zort");

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST(QueueTest, testThatRolloverQueueGetLastLement)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::RolloverQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");
    queue->Enqueue("poit");

    ASSERT_EQ(queue->Last(), "poit");
}


TEST(QueueTest, testThatRolloverQueueCanDequeue)
{
    std::stringstream file;

    auto queue = std::make_shared<paxos::RolloverQueue<std::string>>(file);

    queue->Enqueue("narf");
    queue->Enqueue("zort");
    queue->Enqueue("poit");
    queue->Dequeue();

    ASSERT_EQ(GetQueueSize(queue), 2);
}
