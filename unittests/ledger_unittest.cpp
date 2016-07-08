#include "gtest/gtest.h"

#include "ledger.hpp"


TEST(LedgerUnitTest, testAppendIncrementsTheSize)
{
    VolatileLedger ledger;

    ASSERT_EQ(ledger.Size(), 0);

    ledger.Append(Decree("an_author", 1, "decree_contents"));

    ASSERT_EQ(ledger.Size(), 1);
}


TEST(LedgerUnitTest, testRemoveDecrementsTheSize)
{
    VolatileLedger ledger;

    ledger.Append(Decree("an_author", 1, "decree_contents"));
    ledger.Remove();

    ASSERT_EQ(ledger.Size(), 0);
}


TEST(LedgerUnitTest, testEmptyHeadReturnsDefaultDecree)
{
    VolatileLedger ledger;
    Decree expected, actual = ledger.Head();

    ASSERT_EQ(expected.author, actual.author);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST(LedgerUnitTest, testEmptyTailReturnsDefaultDecree)
{
    VolatileLedger ledger;
    Decree expected, actual = ledger.Tail();

    ASSERT_EQ(expected.author, actual.author);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST(LedgerUnitTest, testHeadWithMultipleDecrees)
{
    VolatileLedger ledger;
    ledger.Append(Decree("a_author", 1, "a_content"));
    ledger.Append(Decree("b_author", 2, "b_content"));

    Decree expected = Decree("a_author", 1, "a_content"), actual = ledger.Head();

    ASSERT_EQ(expected.author, actual.author);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}


TEST(LedgerUnitTest, testTailWithMultipleDecrees)
{
    VolatileLedger ledger;
    ledger.Append(Decree("a_author", 1, "a_content"));
    ledger.Append(Decree("b_author", 2, "b_content"));

    Decree expected = Decree("b_author", 2, "b_content"), actual = ledger.Tail();

    ASSERT_EQ(expected.author, actual.author);
    ASSERT_EQ(expected.number, actual.number);
    ASSERT_EQ(expected.content, actual.content);
}
