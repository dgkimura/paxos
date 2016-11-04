#ifndef __DECREE_HPP_INCLUDED__
#define __DECREE_HPP_INCLUDED__

#include <string>

#include "paxos/replicaset.hpp"


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

    Decree()
        : author(), number(), content()
    {
    }

    Decree(Replica a, int n, std::string c)
        : author(a), number(n), content(c)
    {
    }
};

int CompareDecrees(Decree lhs, Decree rhs);

bool IsDecreeHigher(Decree lhs, Decree rhs);

bool IsDecreeHigherOrEqual(Decree lhs, Decree rhs);

bool IsDecreeEqual(Decree lhs, Decree rhs);

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


#endif
