#include <memory>

#include "paxos/ledger.hpp"


Ledger::Ledger(std::shared_ptr<BaseQueue<Decree>> decrees)
    : Ledger(decrees, std::make_shared<EmptyDecreeHandler>())
{
}


Ledger::Ledger(std::shared_ptr<BaseQueue<Decree>> decrees,
               std::shared_ptr<DecreeHandler> handler)
    : decrees(decrees)
{
    handlers[DecreeType::UserDecree] = handler;
}


Ledger::~Ledger()
{
}


void
Ledger::RegisterHandler(DecreeType key, std::shared_ptr<DecreeHandler> handler)
{
    handlers[key] = handler;
}


void
Ledger::Append(Decree decree)
{
    //
    // A lock must be acquired before executing decree_handler in order to
    // help prevent out of order decrees.
    //
    std::lock_guard<std::recursive_mutex> lock(mutex);

    Decree tail = Tail();
    if (tail.number < decree.number)
    {
        //
        // Append a system decree before executing handler so that post-
        // processing handlers have a full ledger including current decree.
        //
        decrees->Enqueue(decree);
        if (handlers.count(decree.type) > 0)
        {
            (*handlers[decree.type])(decree.content);
        }
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
    std::lock_guard<std::recursive_mutex> lock(mutex);

    decrees->Dequeue();
}


int
Ledger::Size()
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    return decrees->Size();
}


bool
Ledger::IsEmpty()
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    Decree empty;
    return decrees->Last().number == empty.number;
}


Decree
Ledger::Head()
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

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
    std::lock_guard<std::recursive_mutex> lock(mutex);

    return decrees->Last();
}


Decree
Ledger::Next(Decree previous)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

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
