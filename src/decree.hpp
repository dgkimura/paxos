#ifndef __DECREE_HPP_INCLUDED__
#define __DECREE_HPP_INCLUDED__

#include <string>


struct Decree
{
    std::string author;

    int number;

    std::string content;

    Decree()
        : author(), number(), content()
    {
    }

    Decree(std::string a, int n, std::string c)
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

struct compare_decree
{
    bool operator()(const Decree& lhs, const Decree& rhs) const
    {
        return IsDecreeLower(lhs, rhs);
    }
};


#endif
