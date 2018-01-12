#include "paxos/decree.hpp"


namespace paxos
{


int
CompareDecrees(Decree lhs, Decree rhs)
{
    return lhs.number - rhs.number;
}


int
CompareRootDecrees(Decree lhs, Decree rhs)
{
    return lhs.root_number - rhs.root_number;
}


bool
IsDecreeHigher(Decree lhs, Decree rhs)
{
    return (IsRootDecreeEqual(lhs, rhs) && CompareDecrees(lhs, rhs) > 0) ||
            IsRootDecreeHigher(lhs, rhs);
}


bool
IsDecreeHigherOrEqual(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) >= 0;
}


bool
IsDecreeEqual(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) == 0 &&
           CompareRootDecrees(lhs, rhs) == 0;
}


bool
IsDecreeIdentical(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) == 0 &&
           CompareRootDecrees(lhs, rhs) == 0 &&
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
    return CompareRootDecrees(lhs, rhs) == -1;
}


bool
IsRootDecreeEqual(Decree lhs, Decree rhs)
{
    return CompareRootDecrees(lhs, rhs) == 0;
}


bool
IsRootDecreeHigher(Decree lhs, Decree rhs)
{
    return CompareRootDecrees(lhs, rhs) > 0;
}


}
