#include "gtest/gtest.h"

#include "paxos/lru_map.hpp"


TEST(LruMapTest, testContainsInsertedElement)
{
    paxos::lru_map<int, int> lruset(10);

    lruset[3] = 42;
    ASSERT_EQ(42, lruset[3]);
}


TEST(LruMapTest, testInsertExceedingCapacityRemovesLeastRecentlyUsedElement)
{
    paxos::lru_map<int, int> lruset(5);

    lruset[1] = 1;
    lruset[2] = 2;
    lruset[3] = 3;
    lruset[4] = 4;
    lruset[5] = 5;
    lruset[6] = 6;

    // First element should be evicted
    ASSERT_EQ(lruset.end(), lruset.find(1));
    ASSERT_NE(lruset.end(), lruset.find(2));
    ASSERT_NE(lruset.end(), lruset.find(3));
    ASSERT_NE(lruset.end(), lruset.find(4));
    ASSERT_NE(lruset.end(), lruset.find(5));
    ASSERT_NE(lruset.end(), lruset.find(6));
}


TEST(LruMapTest, testInsertSameElementDoesNotEvictElements)
{
    paxos::lru_map<int, int> lruset(2);

    lruset[1] = 1;
    lruset[2] = 2;
    lruset[2] = 2;
    ASSERT_NE(lruset.end(), lruset.find(1));
}


TEST(LruMapTest, testEraseCreatesSpaceForNextElementWithoutEvicting)
{
    paxos::lru_map<int, int> lruset(2);

    lruset[1] = 1;
    lruset[2] = 2;
    lruset.erase(2);
    lruset[3] = 3;
    ASSERT_NE(lruset.end(), lruset.find(1));
}
