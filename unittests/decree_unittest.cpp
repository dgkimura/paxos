#include "gtest/gtest.h"

#include "decree.hpp"


TEST(DecreeUnitTest, testCompareDecreesWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_TRUE(CompareDecrees(lower, higher) < 0);
}


TEST(DecreeUnitTest, testCompareDecreesWithHigherDecreeAuthorOnRightHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_TRUE(CompareDecrees(lower, higher) < 0);
}


TEST(DecreeUnitTest, testCompareDecreesWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_TRUE(CompareDecrees(higher, lower) > 0);
}


TEST(DecreeUnitTest, testCompareDecreesWithHigherDecreeAuthorOnLeftHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_TRUE(CompareDecrees(higher, lower) > 0);
}


TEST(DecreeUnitTest, testCompareDecreesWithIdenticalDecrees)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_1", 1};

    ASSERT_TRUE(CompareDecrees(higher, lower) == 0);
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_TRUE(IsDecreeLower(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeAuthorOnRightHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_TRUE(IsDecreeLower(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_FALSE(IsDecreeLower(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithHigherDecreeAuthorOnLeftHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_FALSE(IsDecreeLower(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerWithIdenticalDecrees)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_1", 1};

    ASSERT_FALSE(IsDecreeLower(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_FALSE(IsDecreeEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithHigherDecreeAuthorOnRightHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_FALSE(IsDecreeEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_FALSE(IsDecreeEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithHigherDecreeAuthorOnLeftHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_FALSE(IsDecreeEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeEqualWithIdenticalDecrees)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_1", 1};

    ASSERT_TRUE(IsDecreeEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_FALSE(IsDecreeHigher(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherDecreeAuthorOnRightHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_FALSE(IsDecreeHigher(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_TRUE(IsDecreeHigher(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithHigherDecreeAuthorOnLeftHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_TRUE(IsDecreeHigher(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherWithIdenticalDecrees)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_1", 1};

    ASSERT_FALSE(IsDecreeHigher(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_FALSE(IsDecreeHigherOrEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithHigherDecreeAuthorOnRightHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_FALSE(IsDecreeHigherOrEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_TRUE(IsDecreeHigherOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithHigherDecreeAuthorOnLeftHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_TRUE(IsDecreeHigherOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeHigherOrEqualWithIdenticalDecrees)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_1", 1};

    ASSERT_TRUE(IsDecreeHigherOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithHigherDecreeNumberOnRightHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_TRUE(IsDecreeLowerOrEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithHigherDecreeAuthorOnRightHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_TRUE(IsDecreeLowerOrEqual(lower, higher));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithHigherDecreeNumberOnLeftHandSide)
{
    Decree lower{"an_author", 1};
    Decree higher{"an_author", 2};

    ASSERT_FALSE(IsDecreeLowerOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithHigherDecreeAuthorOnLeftHandSide)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_2", 1};

    ASSERT_FALSE(IsDecreeLowerOrEqual(higher, lower));
}


TEST(DecreeUnitTest, testIsDecreeLowerOrEqualWithIdenticalDecrees)
{
    Decree lower{"an_author_1", 1};
    Decree higher{"an_author_1", 1};

    ASSERT_TRUE(IsDecreeLowerOrEqual(higher, lower));
}
