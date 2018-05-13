#include "gtest/gtest.h"

#include "paxos/decree.hpp"


TEST(DecreeUnitTest, testDecreeDefaultConstructor)
{
    paxos::Decree decree;

    ASSERT_EQ(decree.author.hostname, "");
    ASSERT_EQ(decree.author.port, 0);
    ASSERT_EQ(decree.number, 0);
    ASSERT_EQ(decree.content, "");
}


TEST(DecreeUnitTest, testCompareDecreesWithHigherDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::CompareDecrees(lower, higher) < 0);
}


TEST(DecreeUnitTest, testCompareDecreesWithHigherDecreeNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::CompareDecrees(higher, lower) > 0);
}


TEST(DecreeUnitTest, testCompareDecreesWithIdenticalDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::CompareDecrees(higher, lower) == 0);
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::IsDecreeLower(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeLower(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeRootNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    higher.root_number = 2;

    ASSERT_TRUE(paxos::IsDecreeLower(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeRootNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    higher.root_number = 2;

    ASSERT_FALSE(paxos::IsDecreeLower(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithIdenticalDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeLower(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithHigherDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithHigherDecreeNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithIdenticalDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::IsDecreeEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherRootDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);

    higher.root_number = 2;

    ASSERT_FALSE(paxos::IsDecreeHigher(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherRootDecreeNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);

    lower.root_number = 2;

    ASSERT_FALSE(paxos::IsDecreeHigher(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeHigher(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherDecreeNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::IsDecreeHigher(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithIdenticalDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeHigher(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithHigherDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeHigherOrEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithHigherDecreeNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::IsDecreeHigherOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithIdenticalDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::IsDecreeHigherOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithHigherDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::IsDecreeLowerOrEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithHigherDecreeNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeLowerOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithIdenticalDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::IsDecreeLowerOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeOrderedWithHigherDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::IsDecreeOrdered(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeOrderedWithMuchHigherDecreeNumberOnRightHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 99, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeOrdered(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeOrderedWithHigherDecreeNumberOnLeftHandSide)
{
    paxos::Decree lower(paxos::Replica("an_author"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author"), 2, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeOrdered(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeOrderedWithIdenticalDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::IsDecreeOrdered(higher, lower));
}


TEST(DecreeUnitTest, testIsRootDecreeOrderedWithIdenticalRootDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    lower.root_number = 1;
    paxos::Decree higher(paxos::Replica("an_author_1"), 2, "", paxos::DecreeType::UserDecree);
    higher.root_number = 1;

    ASSERT_FALSE(paxos::IsRootDecreeOrdered(lower, higher));
}


TEST(DecreeUnitTest, testIsRootDecreeOrderedWithOrderedRootDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    lower.root_number = 1;
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    higher.root_number = 2;

    ASSERT_TRUE(paxos::IsRootDecreeOrdered(lower, higher));
}


TEST(DecreeUnitTest, testIsRootDecreeEqualWithIdenticalRootDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    lower.root_number = 1;
    paxos::Decree higher(paxos::Replica("an_author_1"), 2, "", paxos::DecreeType::UserDecree);
    higher.root_number = 1;

    ASSERT_TRUE(paxos::IsRootDecreeEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsRootDecreeEqualWithOrderedRootDecrees)
{
    paxos::Decree lower(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    lower.root_number = 1;
    paxos::Decree higher(paxos::Replica("an_author_1"), 1, "", paxos::DecreeType::UserDecree);
    higher.root_number = 2;

    ASSERT_FALSE(paxos::IsRootDecreeEqual(lower, higher));
}


TEST(DecreeUnitTest, testCompareMapDecreeComparesHostnameWhenDecreesAreEqual)
{
    paxos::Decree a_decree(paxos::Replica("author_a"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree another_decree(paxos::Replica("author_b"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::compare_map_decree()(a_decree, another_decree));
    ASSERT_FALSE(paxos::compare_map_decree()(another_decree, a_decree));
}


TEST(DecreeUnitTest, testCompareMapDecreeComparesPortWhenDecreesHostnamesAreEqual)
{
    paxos::Decree a_decree(paxos::Replica("author_a", 111), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree another_decree(paxos::Replica("author_a", 222), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_TRUE(paxos::compare_map_decree()(a_decree, another_decree));
    ASSERT_FALSE(paxos::compare_map_decree()(another_decree, a_decree));
}


TEST(DecreeUnitTest, testCompareDecreeDoesNotCompareHostnameWhenDecreesAreEqual)
{
    paxos::Decree a_decree(paxos::Replica("author_a"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree another_decree(paxos::Replica("author_b"), 1, "", paxos::DecreeType::UserDecree);

    ASSERT_FALSE(paxos::compare_decree()(a_decree, another_decree));
    ASSERT_FALSE(paxos::compare_decree()(another_decree, a_decree));
}


TEST(DecreeUnitTest, testCompareMapDecreeUsingKeyWithSameNumberButDiffeerntAuthorInStandardMap)
{
    std::map<paxos::Decree, int, paxos::compare_map_decree> compare_map;

    // Use 2 decrees with same number, but different authors
    paxos::Decree decree_with_author_a(paxos::Replica("author_a"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree decree_with_author_b(paxos::Replica("author_b"), 1, "", paxos::DecreeType::UserDecree);

    compare_map[decree_with_author_a] = 42;

    ASSERT_TRUE(compare_map.find(decree_with_author_a) != compare_map.end());
    ASSERT_TRUE(compare_map.find(decree_with_author_b) == compare_map.end());
}
