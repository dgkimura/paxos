#ifndef __DECREE_HPP_INCLUDED__
#define __DECREE_HPP_INCLUDED__

#include <string>

#include "paxos/replicaset.hpp"


enum class DecreeType
{
    //
    // User defined decree.
    //
    UserDecree,

    //
    // System defined decree.
    //
    SystemDecree
};


/*
 * Decree contains information about a proposal. Each decree is uniquely
 * definied by the author and number.
 */

struct Decree
{

    //
    // Author defines the origin of the proposal. It identifies the legislator.
    //
    Replica author;

    //
    // Number is a monotimically increasing value that can be used to identify
    // each round of paxos.
    //
    int number;

    //
    // Content is the entry which would be added to every ledger if the decree
    // comes to pass.
    //
    std::string content;

    DecreeType type;

    Decree()
        : author(), number(), content(), type()
    {
    }

    Decree(Replica a, int n, std::string c, DecreeType dtype)
        : author(a), number(n), content(c), type(dtype)
    {
    }
};

int CompareDecrees(Decree lhs, Decree rhs);

bool IsDecreeHigher(Decree lhs, Decree rhs);

bool IsDecreeHigherOrEqual(Decree lhs, Decree rhs);

bool IsDecreeEqual(Decree lhs, Decree rhs);

bool IsDecreeIdentical(Decree lhs, Decree rhs);

bool IsDecreeLower(Decree lhs, Decree rhs);

bool IsDecreeLowerOrEqual(Decree lhs, Decree rhs);

bool IsDecreeOrdered(Decree lhs, Decree rhs);

struct compare_decree
{
    bool operator()(const Decree& lhs, const Decree& rhs) const
    {
        return IsDecreeLower(lhs, rhs);
    }
};

struct ascending_decree
{
    bool operator()(const Decree& lhs, const Decree& rhs) const
    {
        return IsDecreeHigher(lhs, rhs);
    }
};


#endif
