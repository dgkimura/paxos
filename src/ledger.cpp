#include "paxos/ledger.hpp"


Ledger::Ledger(std::shared_ptr<BaseQueue<Decree>> decrees_)
    : Ledger(decrees_, [](std::string entry){})
{
}


Ledger::Ledger(std::shared_ptr<BaseQueue<Decree>> decrees_,
               std::function<void(std::string entry)> decree_handler_)
    : decrees(decrees_),
      decree_handler(decree_handler_)
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
        //
        // Execute handler before appending a decree thereby allowing the
        // handler to guard against ledger corruption. This can prevent
        // possible proliferation of a bad ledger to another replica.
        // It is safe because we require that the handler is idempotent.
        //
        decree_handler(decree.content);
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
Ledger::Size() const
{
    return decrees->Size();
}


Decree
Ledger::Head() const
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
Ledger::Tail() const
{
    Decree tail;
    for (Decree d : *decrees)
    {
        tail = d;
    }
    return tail;
}


Decree
Ledger::Next(Decree previous) const
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
