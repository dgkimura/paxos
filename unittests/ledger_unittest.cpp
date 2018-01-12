#include "gtest/gtest.h"

#include "paxos/ledger.hpp"


class LedgerUnitTest: public testing::Test
{
    virtual void SetUp()
    {
        paxos::DisableLogging();
    }
};


int GetQueueSize(const std::shared_ptr<paxos::RolloverQueue<paxos::Decree>>& queue)
{
    int size = 0;
    for (auto d : *queue)
    {
        size += 1;
    }
    return size;
}


TEST_F(LedgerUnitTest, testAppendIncrementsTheSize)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);

    ASSERT_EQ(GetQueueSize(queue), 0);

    ledger.Append(paxos::Decree(paxos::Replica("an_author"), 1, "decree_contents", paxos::DecreeType::UserDecree));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LedgerUnitTest, testAppendSystemDecreeIncrementsTheSize)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);

    ASSERT_EQ(GetQueueSize(queue), 0);

    ledger.Append(paxos::Decree(paxos::Replica("an_author"), 1, "decree_contents", paxos::DecreeType::AddReplicaDecree));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LedgerUnitTest, testRemoveDecrementsTheSize)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);

    ledger.Append(paxos::Decree(paxos::Replica("an_author"), 1, "decree_contents", paxos::DecreeType::UserDecree));
    ledger.Remove();

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LedgerUnitTest, testLedgerAppendChangesIsEmptyStatus)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);

    ASSERT_TRUE(ledger.IsEmpty());

    ledger.Append(paxos::Decree(paxos::Replica("an_author"), 1, "decree_contents", paxos::DecreeType::AddReplicaDecree));

    ASSERT_FALSE(ledger.IsEmpty());
}


TEST_F(LedgerUnitTest, testEmptyHeadReturnsDefaultDecree)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    paxos::Decree expected, actual = ledger.Head();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testEmptyTailReturnsDefaultDecree)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    paxos::Decree expected, actual = ledger.Tail();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testEmptyNextWithFutureDecree)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    paxos::Decree expected, actual = ledger.Next(paxos::Decree(paxos::Replica("b_author"), 2, "b_content", paxos::DecreeType::UserDecree));

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testHeadWithMultipleDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    ledger.Append(paxos::Decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree));
    ledger.Append(paxos::Decree(paxos::Replica("b_author"), 2, "b_content", paxos::DecreeType::UserDecree));

    paxos::Decree expected = paxos::Decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree), actual = ledger.Head();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testTailWithMultipleDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    ledger.Append(paxos::Decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree));
    ledger.Append(paxos::Decree(paxos::Replica("b_author"), 2, "b_content", paxos::DecreeType::UserDecree));

    paxos::Decree expected = paxos::Decree(paxos::Replica("b_author"), 2, "b_content", paxos::DecreeType::UserDecree), actual = ledger.Tail();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testNextWithMultipleDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    ledger.Append(paxos::Decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree));
    ledger.Append(paxos::Decree(paxos::Replica("b_author"), 2, "b_content", paxos::DecreeType::UserDecree));
    ledger.Append(paxos::Decree(paxos::Replica("c_author"), 3, "c_content", paxos::DecreeType::UserDecree));

    paxos::Decree expected = paxos::Decree(paxos::Replica("b_author"), 2, "b_content", paxos::DecreeType::UserDecree),
           actual = ledger.Next(paxos::Decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree));

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testAppendIgnoresOutOfOrderDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    ledger.Append(paxos::Decree(paxos::Replica("b_author"), 2, "b_content", paxos::DecreeType::UserDecree));
    ledger.Append(paxos::Decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LedgerUnitTest, testAppendWritesOutOfOrderDecreeWithOrderedRoot)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    ledger.Append(paxos::Decree(paxos::Replica("b_author"), 2, "b_content", paxos::DecreeType::UserDecree));

    paxos::Decree next_decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree);
    next_decree.root_number = 3;
    ledger.Append(next_decree);

    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST_F(LedgerUnitTest, testAppendIgnoresDuplicateDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    ledger.Append(paxos::Decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree));
    ledger.Append(paxos::Decree(paxos::Replica("a_author"), 1, "a_content", paxos::DecreeType::UserDecree));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LedgerUnitTest, testDecreeHandlerOnAppend)
{
    std::string concatenated_content;
    auto handler = [&](std::string entry) { concatenated_content += entry; };

    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(
        queue,
        std::make_shared<paxos::CompositeHandler>(handler));
    ledger.Append(paxos::Decree(paxos::Replica("a_author"), 1, "AAAAA", paxos::DecreeType::UserDecree));
    ledger.Append(paxos::Decree(paxos::Replica("b_author"), 2, "BBBBB", paxos::DecreeType::UserDecree));

    ASSERT_EQ(concatenated_content, "AAAAABBBBB");
}


TEST_F(LedgerUnitTest, testRegisteredDecreeHandlerExecuted)
{
    class CustomHandler : public paxos::DecreeHandler
    {
    public:

        CustomHandler()
            : is_executed(false)
        {
        }

        virtual void operator()(std::string entry) override
        {
            is_executed = true;
        }

        bool is_executed;
    };

    auto handler = std::make_shared<CustomHandler>();

    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    paxos::Ledger ledger(queue);
    ledger.RegisterHandler(paxos::DecreeType::AddReplicaDecree, handler);
    ledger.Append(paxos::Decree(paxos::Replica("a_author"), 1, "AAAAA", paxos::DecreeType::AddReplicaDecree));

    ASSERT_TRUE(handler->is_executed);
}
