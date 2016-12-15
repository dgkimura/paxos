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

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents"));

    ASSERT_EQ(ledger.Size(), 1);
}


TEST_F(LedgerUnitTest, testRemoveDecrementsTheSize)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());

    ledger.Append(Decree(Replica("an_author"), 1, "decree_contents"));
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
    Decree expected, actual = ledger.Next(Decree(Replica("b_author"), 2, "b_content"));

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testHeadWithMultipleDecrees)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    ledger.Append(Decree(Replica("a_author"), 1, "a_content"));
    ledger.Append(Decree(Replica("b_author"), 2, "b_content"));

    Decree expected = Decree(Replica("a_author"), 1, "a_content"), actual = ledger.Head();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testTailWithMultipleDecrees)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    ledger.Append(Decree(Replica("a_author"), 1, "a_content"));
    ledger.Append(Decree(Replica("b_author"), 2, "b_content"));

    Decree expected = Decree(Replica("b_author"), 2, "b_content"), actual = ledger.Tail();

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testNextWithMultipleDecrees)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    ledger.Append(Decree(Replica("a_author"), 1, "a_content"));
    ledger.Append(Decree(Replica("b_author"), 2, "b_content"));
    ledger.Append(Decree(Replica("c_author"), 3, "c_content"));

    Decree expected = Decree(Replica("b_author"), 2, "b_content"),
           actual = ledger.Next(Decree(Replica("a_author"), 1, "a_content"));

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST_F(LedgerUnitTest, testAppendIgnoresOutOfOrderDecrees)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    ledger.Append(Decree(Replica("b_author"), 2, "b_content"));
    ledger.Append(Decree(Replica("a_author"), 1, "a_content"));

    ASSERT_EQ(ledger.Size(), 1);
}


TEST_F(LedgerUnitTest, testAppendIgnoresDuplicateDecrees)
{
    Ledger ledger(std::make_shared<VolatileQueue<Decree>>());
    ledger.Append(Decree(Replica("a_author"), 1, "a_content"));
    ledger.Append(Decree(Replica("a_author"), 1, "a_content"));

    ASSERT_EQ(ledger.Size(), 1);
}


TEST_F(LedgerUnitTest, testDecreeHandlerOnAppend)
{
    std::string concatenated_content;
    auto handler = [&](std::string entry) { concatenated_content += entry; };

    Ledger ledger(std::make_shared<VolatileQueue<Decree>>(), DecreeHandler(handler));
    ledger.Append(Decree(Replica("a_author"), 1, "AAAAA"));
    ledger.Append(Decree(Replica("b_author"), 2, "BBBBB"));

    ASSERT_EQ(concatenated_content, "AAAAABBBBB");
}
