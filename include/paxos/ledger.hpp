#ifndef __LEDGER_HPP_INCLUDED__
#define __LEDGER_HPP_INCLUDED__

#include <functional>
#include <mutex>
#include <unordered_map>

#include "paxos/customhash.hpp"
#include "paxos/decree.hpp"
#include "paxos/handler.hpp"
#include "paxos/logging.hpp"
#include "paxos/queue.hpp"


namespace paxos
{


class Ledger
{
public:

    Ledger(std::shared_ptr<RolloverQueue<Decree>> decrees);

    Ledger(std::shared_ptr<RolloverQueue<Decree>> decrees,
           std::shared_ptr<DecreeHandler> handler);

    ~Ledger();

    void RegisterHandler(DecreeType key,
                         std::shared_ptr<DecreeHandler> handler);

    void Append(Decree decree);

    void Remove();

    bool IsEmpty();

    Decree Head();

    Decree Tail();

    Decree Next(Decree previous);

private:

    std::shared_ptr<RolloverQueue<Decree>> decrees;

    std::recursive_mutex mutex;

    std::unordered_map<DecreeType, std::shared_ptr<DecreeHandler>> handlers;
};


}


#endif
