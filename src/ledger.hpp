#ifndef __LEDGER_HPP_INCLUDED__
#define __LEDGER_HPP_INCLUDED__

#include <decree.hpp>
#include <logging.hpp>
#include <queue.hpp>


class LedgerType
{
public:

    virtual void Append(Decree decree) = 0;

    virtual void Remove() = 0;

    virtual int Size() = 0;

    virtual Decree Head() = 0;

    virtual Decree Tail() = 0;

    virtual Decree Next(Decree previous) = 0;
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
        Decree tail = Tail();
        if (tail.number < decree.number)
        {
            decrees.Enqueue(decree);
        } else
        {
            LOG(LogLevel::Warning) << "Out of order decree. Ledger: "
                << tail.number << " Received: " << decree.number;
        }
    }

    void Remove()
    {
        decrees.Dequeue();
    }

    int Size()
    {
        return decrees.Size();
    }

    Decree Head()
    {
        Decree head;
        for (Decree d : decrees)
        {
            head = d;
            break;
        }
        return head;
    }

    Decree Tail()
    {
        Decree tail;
        for (Decree d : decrees)
        {
            tail = d;
        }
        return tail;
    }

    Decree Next(Decree previous)
    {
        Decree next;
        for (Decree current : decrees)
        {
            if (IsDecreeOrdered(previous, current))
            {
                next = current;
                break;
            }
        }
        return next;
    }

private:

    T decrees;
};


using VolatileLedger = Ledger<VolatileQueue<Decree>>;


using PersistentLedger = Ledger<PersistentQueue<Decree>>;


#endif
