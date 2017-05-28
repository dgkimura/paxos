#ifndef __CONTEXT_HPP_INCLUDED__
#define __CONTEXT_HPP_INCLUDED__


#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <tuple>
#include <vector>

#include "paxos/decree.hpp"
#include "paxos/fields.hpp"
#include "paxos/ledger.hpp"
#include "paxos/pause.hpp"
#include "paxos/replicaset.hpp"


struct Context
{
};


struct ProposerContext : public Context
{
    std::shared_ptr<Ledger>& ledger;
    std::atomic_flag in_progress;
    Field<Decree> highest_proposed_decree;
    std::shared_ptr<ReplicaSet>& replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> promise_map;
    std::vector<std::tuple<std::string, DecreeType>> requested_values;
    std::function<void(std::string)> ignore_handler;

    std::mutex mutex;
    Decree highest_nacked_decree;
    std::shared_ptr<Pause> pause;

    ProposerContext(
        std::shared_ptr<ReplicaSet>& replicaset_,
        std::shared_ptr<Ledger>& ledger_,
        std::shared_ptr<Storage<Decree>> highest_proposed_decree_,
        std::function<void(std::string)> ignore_handler,
        std::shared_ptr<Pause> pause
    )
        : ledger(ledger_),
          highest_proposed_decree(highest_proposed_decree_),
          replicaset(replicaset_),
          promise_map(),
          requested_values(),
          ignore_handler(ignore_handler),
          mutex(),
          highest_nacked_decree(Replica(""), -1, "first", DecreeType::UserDecree),
          pause(pause)
    {
        std::atomic_flag_clear(&in_progress);
    }
};


struct AcceptorContext : public Context
{
    Field<Decree> promised_decree;
    Field<Decree> accepted_decree;

    AcceptorContext(
        std::shared_ptr<Storage<Decree>> promised_decree_,
        std::shared_ptr<Storage<Decree>> accepted_decree_
    )
        : promised_decree(promised_decree_),
          accepted_decree(accepted_decree_)
    {
    }
};


struct LearnerContext : public Context
{
    std::shared_ptr<ReplicaSet>& replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> accepted_map;
    std::shared_ptr<Ledger>& ledger;
    std::priority_queue<Decree, std::vector<Decree>, ascending_decree> tracked_future_decrees;
    bool is_observer;

    // Used to guard concurrent access of iterable/mutable data structures
    std::mutex mutex;

    LearnerContext(
        std::shared_ptr<ReplicaSet>& replicaset_,
        std::shared_ptr<Ledger>& ledger_,
        bool is_observer=false
    )
        : replicaset(replicaset_),
          accepted_map(),
          ledger(ledger_),
          tracked_future_decrees(),
          is_observer(is_observer),
          mutex()
    {
    }
};


struct UpdaterContext : public Context
{
    std::shared_ptr<Ledger>& ledger;

    UpdaterContext(
        std::shared_ptr<Ledger>& ledger_
    )
        : ledger(ledger_)
    {
    }
};


#endif
