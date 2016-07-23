#include <ledger.hpp>


Ledger::Ledger(std::shared_ptr<BaseQueue<Decree>> decrees_)
    : decrees(decrees_)
{
}


Ledger::~Ledger()
{
}


void
Ledger::Append(Decree decree)
{
    Decree tail = Tail();
    if (tail.number < decree.number)
    {
        decrees->Enqueue(decree);
    } else
    {
        LOG(LogLevel::Warning)
            << "Out of order decree. Ledger: "
            << tail.number
            << " Received: "
            << decree.number;
    }
}

void
Ledger::Remove()
{
    decrees->Dequeue();
}

int
Ledger::Size()
{
    return decrees->Size();
}


Decree
Ledger::Head()
{
    Decree head;
    for (Decree d : *decrees)
    {
        head = d;
        break;
    }
    return head;
}


Decree
Ledger::Tail()
{
    Decree tail;
    for (Decree d : *decrees)
    {
        tail = d;
    }
    return tail;
}


Decree
Ledger::Next(Decree previous)
{
    Decree next;
    for (Decree current : *decrees)
    {
        if (IsDecreeOrdered(previous, current))
        {
            next = current;
            break;
        }
    }
    return next;
}
