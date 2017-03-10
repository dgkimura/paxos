#include "gtest/gtest.h"

#include "paxos/ledger.hpp"


class LedgerUnitTest: public testing::Test
{
    virtual void SetUp()
    {
        DisableLogging();
    }
};


TEST_F(LedgerUnitTest, testAppendIncrementsTheSize)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());

    ASSERT_EQ(ledger.Size(), 0);

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents", DecreeType::UserDecree));

    ASSERT_EQ(ledger.Size(), 1);
}


TEST_F(LedgerUnitTest, testAppendSystemDecreeIncrementsTheSize)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());

    ASSERT_EQ(ledger.Size(), 0);

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents", DecreeType::AddReplicaDecree));

    ASSERT_EQ(ledger.Size(), 1);
}


TEST_F(LedgerUnitTest, testRemoveDecrementsTheSize)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents", DecreeType::UserDecree));
    ledger.Remove();

    ASSERT_EQ(ledger.Size(), 0);
}


TEST_F(LedgerUnitTest, testEmptyHeadReturnsDefaultDecree)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    Decree expected, actual = ledger.Head();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testEmptyTailReturnsDefaultDecree)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    Decree expected, actual = ledger.Tail();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testEmptyNextWithFutureDecree)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    Decree expected, actual = ledger.Next(Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree));

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testHeadWithMultipleDecrees)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
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
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
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
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
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
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    ledger.Append(Decree(Replica("b_author"), 2, "b_content", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));

    ASSERT_EQ(ledger.Size(), 1);
}


TEST_F(LedgerUnitTest, testAppendIgnoresDuplicateDecrees)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));
    ledger.Append(Decree(Replica("a_author"), 1, "a_content", DecreeType::UserDecree));

    ASSERT_EQ(ledger.Size(), 1);
}


TEST_F(LedgerUnitTest, testDecreeHandlerOnAppend)
{
    std::string concatenated_content;
    auto handler = [&](std::string entry) { concatenated_content += entry; };

    Ledger ledger(
        std::make_shared<VolatileQueue<Decree>>(),
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

    Ledger ledger(
        std::make_shared<VolatileQueue<Decree>>());
    ledger.RegisterHandler(DecreeType::AddReplicaDecree, handler);
    ledger.Append(Decree(Replica("a_author"), 1, "AAAAA", DecreeType::AddReplicaDecree));

    ASSERT_TRUE(handler->is_executed);
}
