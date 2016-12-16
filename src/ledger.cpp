#include "paxos/ledger.hpp"


Ledger::Ledger(std::shared_ptr<BaseQueue<Decree>> decrees)
    : Ledger(decrees, DecreeHandler(), DecreeHandler())
{
}


Ledger::Ledger(std::shared_ptr<BaseQueue<Decree>> decrees,
               DecreeHandler user_decree_handler,
               DecreeHandler system_decree_handler)
    : decrees(decrees),
      user_decree_handler(user_decree_handler),
      system_decree_handler(system_decree_handler)
{
}


Ledger::~Ledger()
{
}


void
Ledger::Append(Decree decree)
{
    //
    // A lock must be acquired before executing decree_handler in order to
    // help prevent out of order decrees.
    //
    std::lock_guard<std::mutex> lock(mutex);

    Decree tail = Tail();
    if (tail.number < decree.number)
    {
        //
        // Execute handler before appending a decree thereby allowing the
        // handler to guard against ledger corruption. This can prevent
        // possible proliferation of a bad ledger to another replica.
        // It is safe because we require that the handler is idempotent.
        //
        if (decree.type == DecreeType::UserDecree)
        {
            user_decree_handler(decree.content);
        }
        else if (decree.type == DecreeType::SystemDecree)
        {
            system_decree_handler(decree.content);
        }
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
