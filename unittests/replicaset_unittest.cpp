#include "gtest/gtest.h"

#include "replicaset.hpp"


TEST(ReplicaSetUnittest, testReplicaSetSizeInitializedToZero)
{
    ReplicaSet set;

    ASSERT_EQ(set.GetSize(), 0);
}


TEST(ReplicaSetUnittest, testAddReplicaIncrementsReplicaSetSize)
{
    ReplicaSet set;
    Replica replica;
    set.Add(replica);

    ASSERT_EQ(set.GetSize(), 1);
}


TEST(ReplicaSetUnittest, testRemoveReplicaDecrementsReplicaSetSize)
{
    ReplicaSet set;
    Replica replica;

    set.Add(replica);
    ASSERT_EQ(set.GetSize(), 1);

    set.Remove(replica);
    ASSERT_EQ(set.GetSize(), 0);
}


TEST(ReplicaSetUnittest, testIterateReplicaSet)
{
    ReplicaSet set;
    Replica first = {"myhost"};
    set.Add(first);

    Replica second = {"yourhost"};
    set.Add(second);

    Replica third = {"theirhost"};
    set.Add(third);

    std::set<std::string> expected = {"myhost", "yourhost", "theirhost"};

    for (Replica r : set)
    {
        ASSERT_TRUE(expected.count(r.hostname) > 0);
    }
    ASSERT_EQ(set.GetSize(), 3);
}
