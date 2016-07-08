#ifndef __LEDGER_HPP_INCLUDED__
#define __LEDGER_HPP_INCLUDED__

#include <decree.hpp>
#include <queue.hpp>


class LedgerType
{
public:

    virtual void Append(Decree decree) = 0;

    virtual int Size() = 0;
};


template <typename T>
class Ledger : public LedgerType
{
public:

    Ledger(std::string name)
        : decrees(name)
    {
    }

    Ledger()
        : Ledger("paxos.ledger")
    {
    }

    ~Ledger()
    {
    }

    void Append(Decree decree)
    {
        decrees.Enqueue(decree);
    }

    int Size()
    {
        return decrees.Size();
    }

private:

    T decrees;
};


using VolatileLedger = Ledger<VolatileQueue<Decree>>;


using PersistentLedger = Ledger<PersistentQueue<Decree>>;


#endif
