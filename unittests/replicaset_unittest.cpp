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
