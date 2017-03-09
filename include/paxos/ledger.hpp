#ifndef __LEDGER_HPP_INCLUDED__
#define __LEDGER_HPP_INCLUDED__

#include <functional>
#include <mutex>
#include <unordered_map>

#include "paxos/decree.hpp"
#include "paxos/handler.hpp"
#include "paxos/logging.hpp"
#include "paxos/queue.hpp"


struct DecreeTypeHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};


class Ledger
{
public:

    Ledger(std::shared_ptr<BaseQueue<Decree>> decrees);

    Ledger(std::shared_ptr<BaseQueue<Decree>> decrees,
           std::shared_ptr<DecreeHandler> handler);

    ~Ledger();

    void RegisterHandler(DecreeType key,
                         std::shared_ptr<DecreeHandler> handler);

    void Append(Decree decree);

    void Remove();

    int Size() const;

    Decree Head() const;

    Decree Tail() const;

    Decree Next(Decree previous) const;

private:

    std::shared_ptr<BaseQueue<Decree>> decrees;

    std::mutex mutex;

    std::unordered_map<DecreeType, std::shared_ptr<DecreeHandler>,
                       DecreeTypeHash> handlers;
};


#endif
