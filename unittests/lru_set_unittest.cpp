#include "gtest/gtest.h"

#include "paxos/lru_set.hpp"


TEST(LruSetTest, testContainsInsertedElement)
{
    paxos::lru_set<int> lruset(10);

    lruset.insert(42);
    ASSERT_TRUE(lruset.contains(42));
}


TEST(LruSetTest, testInsertExceedingCapacityRemovesLeastRecentlyUsedElement)
{
    paxos::lru_set<int> lruset(5);

    lruset.insert(1);
    lruset.insert(2);
    lruset.insert(3);
    lruset.insert(4);
    lruset.insert(5);
    lruset.insert(6);

    ASSERT_FALSE(lruset.contains(1));
    ASSERT_TRUE(lruset.contains(2));
    ASSERT_TRUE(lruset.contains(3));
    ASSERT_TRUE(lruset.contains(4));
    ASSERT_TRUE(lruset.contains(5));
    ASSERT_TRUE(lruset.contains(6));
}
