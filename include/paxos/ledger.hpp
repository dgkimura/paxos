#ifndef __LEDGER_HPP_INCLUDED__
#define __LEDGER_HPP_INCLUDED__

#include <functional>
#include <mutex>

#include "paxos/decree.hpp"
#include "paxos/logging.hpp"
#include "paxos/queue.hpp"


//
// Handler that will be executed after a decree has been accepted. It
// is expected to be idempotent.
//
using DecreeHandler = std::function<void(std::string entry)>;


class Ledger
{
public:

    Ledger(std::shared_ptr<BaseQueue<Decree>> decrees_);

    Ledger(std::shared_ptr<BaseQueue<Decree>> decrees_,
           DecreeHandler decree_handler_);

    ~Ledger();

    void Append(Decree decree);

    void Remove();

    int Size() const;

    Decree Head() const;

    Decree Tail() const;

    Decree Next(Decree previous) const;

private:

    std::shared_ptr<BaseQueue<Decree>> decrees;

    DecreeHandler decree_handler;

    std::mutex mutex;
};


#endif
