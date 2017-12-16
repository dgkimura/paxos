#include "paxos/decree.hpp"


int
CompareDecrees(Decree lhs, Decree rhs)
{
    return lhs.number - rhs.number;
}


bool
IsDecreeHigher(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) > 0;
}


bool
IsDecreeHigherOrEqual(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) >= 0;
}


bool
IsDecreeEqual(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) == 0;
}


bool
IsDecreeIdentical(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) == 0 &&
           IsReplicaEqual(lhs.author, rhs.author);
}


bool
IsDecreeLower(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) < 0;
}


bool
IsDecreeLowerOrEqual(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) <= 0;
}


bool
IsDecreeOrdered(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) == -1;
}


bool
IsRootDecreeOrdered(Decree lhs, Decree rhs)
{
    return lhs.root_number - rhs.root_number == -1;
}


bool
IsRootDecreeEqual(Decree lhs, Decree rhs)
{
    return lhs.root_number == rhs.root_number;
}
