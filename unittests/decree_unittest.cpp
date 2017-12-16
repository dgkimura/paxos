#include "gtest/gtest.h"

#include "paxos/decree.hpp"


TEST(DecreeUnitTest, testDecreeDefaultConstructor)
{
    Decree decree;

    ASSERT_EQ(decree.author.hostname, "");
    ASSERT_EQ(decree.author.port, 0);
    ASSERT_EQ(decree.number, 0);
    ASSERT_EQ(decree.content, "");
}


TEST(DecreeUnitTest, testCompareDecreesWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_TRUE(CompareDecrees(lower, higher) < 0);
}


TEST(DecreeUnitTest, testCompareDecreesWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_TRUE(CompareDecrees(higher, lower) > 0);
}


TEST(DecreeUnitTest, testCompareDecreesWithIdenticalDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);

    ASSERT_TRUE(CompareDecrees(higher, lower) == 0);
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_TRUE(IsDecreeLower(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeLower(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithIdenticalDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeLower(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithIdenticalDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);

    ASSERT_TRUE(IsDecreeEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeHigher(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_TRUE(IsDecreeHigher(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithIdenticalDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeHigher(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeHigherOrEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_TRUE(IsDecreeHigherOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithIdenticalDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);

    ASSERT_TRUE(IsDecreeHigherOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_TRUE(IsDecreeLowerOrEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeLowerOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithIdenticalDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);

    ASSERT_TRUE(IsDecreeLowerOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeOrderedWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_TRUE(IsDecreeOrdered(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeOrderedWithMuchHigherDecreeNumberOnRightHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 99, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeOrdered(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeOrderedWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower(Replica("an_author"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author"), 2, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeOrdered(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeOrderedWithIdenticalDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);

    ASSERT_FALSE(IsDecreeOrdered(higher, lower));
}


TEST(DecreeUnitTest, testIsRootDecreeOrderedWithIdenticalRootDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    lower.root_number = 1;
    Decree higher(Replica("an_author_1"), 2, "", DecreeType::UserDecree);
    higher.root_number = 1;

    ASSERT_FALSE(IsRootDecreeOrdered(lower, higher));
}


TEST(DecreeUnitTest, testIsRootDecreeOrderedWithOrderedRootDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    lower.root_number = 1;
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    higher.root_number = 2;

    ASSERT_TRUE(IsRootDecreeOrdered(lower, higher));
}


TEST(DecreeUnitTest, testIsRootDecreeEqualWithIdenticalRootDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    lower.root_number = 1;
    Decree higher(Replica("an_author_1"), 2, "", DecreeType::UserDecree);
    higher.root_number = 1;

    ASSERT_TRUE(IsRootDecreeEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsRootDecreeEqualWithOrderedRootDecrees)
{
    Decree lower(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    lower.root_number = 1;
    Decree higher(Replica("an_author_1"), 1, "", DecreeType::UserDecree);
    higher.root_number = 2;

    ASSERT_FALSE(IsRootDecreeEqual(lower, higher));
}
