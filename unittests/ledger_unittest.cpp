#include "gtest/gtest.h"

#include "paxos/ledger.hpp"


class LedgerUnitTest: public testing::Test
{
    virtual void SetUp()
    {
        DisableLogging();
    }
};


int GetQueueSize(const std::shared_ptr<RolloverQueue<Decree>>& queue)
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
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);

    ASSERT_EQ(GetQueueSize(queue), 0);

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents", DecreeType::UserDecree));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LedgerUnitTest, testAppendSystemDecreeIncrementsTheSize)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);

    ASSERT_EQ(GetQueueSize(queue), 0);

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents", DecreeType::AddReplicaDecree));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LedgerUnitTest, testRemoveDecrementsTheSize)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents", DecreeType::UserDecree));
    ledger.Remove();

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LedgerUnitTest, testLedgerAppendChangesIsEmptyStatus)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);

    ASSERT_TRUE(ledger.IsEmpty());

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents", DecreeType::AddReplicaDecree));

    ASSERT_FALSE(ledger.IsEmpty());
}


TEST_F(LedgerUnitTest, testEmptyHeadReturnsDefaultDecree)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    Decree expected, actual = ledger.Head();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testEmptyTailReturnsDefaultDecree)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    Decree expected, actual = ledger.Tail();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testEmptyNextWithFutureDecree)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    Decree expected, actual = ledger.Next(Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree));

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testHeadWithMultipleDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree));

    Decree expected = Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree), actual = ledger.Head();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testTailWithMultipleDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree));

    Decree expected = Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree), actual = ledger.Tail();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testNextWithMultipleDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("c_author"), 3, "c_content", DecreeType::UserDecree));

    Decree expected = Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree),
           actual = ledger.Next(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testAppendIgnoresOutOfOrderDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    ledger.Append(Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LedgerUnitTest, testAppendIgnoresDuplicateDecrees)
{
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LedgerUnitTest, testDecreeHandlerOnAppend)
{
    std::string concatenated_content;
    auto handler = [&](std::string entry) { concatenated_content += entry; };

    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(
        queue,
        std::make_shared<CompositeHandler>(handler));
    ledger.Append(Decree(Replica("a_author"), 1, "AAAAA", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("b_author"), 2, "BBBBB", DecreeType::UserDecree));

    ASSERT_EQ(concatenated_content, "AAAAABBBBB");
}


TEST_F(LedgerUnitTest, testRegisteredDecreeHandlerExecuted)
{
    class CustomHandler : public DecreeHandler
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
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    Ledger ledger(queue);
    ledger.RegisterHandler(DecreeType::AddReplicaDecree, handler);
    ledger.Append(Decree(Replica("a_author"), 1, "AAAAA", DecreeType::AddReplicaDecree));

    ASSERT_TRUE(handler->is_executed);
}
