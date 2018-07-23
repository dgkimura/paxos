#ifndef __DECREE_HPP_INCLUDED__
#define __DECREE_HPP_INCLUDED__

#include <string>

#include "paxos/replicaset.hpp"


namespace paxos
{


enum class DecreeType
{
    //
    // User defined decree.
    //
    UserDecree,

    //
    // Add node decree.
    //
    AddReplicaDecree,

    //
    // Remove node decree.
    //
    RemoveReplicaDecree
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
    // Number identifying a round of paxos that stays constant during reties.
    //
    int root_number;

    //
    // Content is the entry which would be added to every ledger if the decree
    // comes to pass.
    //
    std::string content;

    //
    // Type defines the type of decree.
    //
    DecreeType type;

    Decree()
        : author(), number(), root_number(), content(), type()
    {
    }

    Decree(Replica a, int n, std::string c, DecreeType dtype)
        : author(a), number(n), root_number(n), content(c), type(dtype)
    {
    }
};

int CompareDecrees(Decree lhs, Decree rhs);

int CompareRootDecrees(Decree lhs, Decree rhs);

bool IsDecreeHigher(Decree lhs, Decree rhs);

bool IsDecreeHigherOrEqual(Decree lhs, Decree rhs);

bool IsDecreeEqual(Decree lhs, Decree rhs);

bool IsDecreeIdentical(Decree lhs, Decree rhs);

bool IsDecreeLower(Decree lhs, Decree rhs);

bool IsDecreeLowerOrEqual(Decree lhs, Decree rhs);

bool IsDecreeOrdered(Decree lhs, Decree rhs);

bool IsRootDecreeOrdered(Decree lhs, Decree rhs);

bool IsRootDecreeEqual(Decree lhs, Decree rhs);

bool IsRootDecreeHigher(Decree lhs, Decree rhs);

bool IsRootDecreeLower(Decree lhs, Decree rhs);

bool IsRootDecreeHigherOrEqual(Decree lhs, Decree rhs);

struct compare_decree
{
    bool operator()(const Decree& lhs, const Decree& rhs) const
    {
        return IsDecreeLower(lhs, rhs);
    }
};

struct compare_root_decree
{
    bool operator()(const Decree& lhs, const Decree& rhs) const
    {
        return lhs.root_number - rhs.root_number < 0;
    }
};

struct compare_map_decree
{
    bool operator()(const Decree& lhs, const Decree& rhs) const
    {
        if (CompareDecrees(lhs, rhs) != 0)
        {
               return CompareDecrees(lhs, rhs) < 0;
        }
        if (lhs.author.hostname != rhs.author.hostname)
        {
            return lhs.author.hostname < rhs.author.hostname;
        }
        return lhs.author.port < rhs.author.port;
    }
};

struct ascending_decree
{
    bool operator()(const Decree& lhs, const Decree& rhs) const
    {
        return IsDecreeHigher(lhs, rhs);
    }
};


struct UpdateReplicaSetDecree
{
    //
    // Author of decree.
    //
    Replica author;

    //
    // Replica to add or remove.
    //
    Replica replica;

    //
    // Remote directory storing replicated files.
    //
    std::string remote_directory;
};


}


#endif
