#include <decree.hpp>


int
CompareDecrees(Decree lhs, Decree rhs)
{
    return lhs.number != rhs.number ?
        lhs.number - rhs.number :
        lhs.author.compare(rhs.author);
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
IsDecreeLower(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) < 0;
}


bool
IsDecreeLowerOrEqual(Decree lhs, Decree rhs)
{
    return CompareDecrees(lhs, rhs) <= 0;
}
