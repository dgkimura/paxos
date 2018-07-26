#ifndef __CONTEXT_HPP_INCLUDED__
#define __CONTEXT_HPP_INCLUDED__


#include <atomic>
#include <chrono>
#include <deque>
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
#include "paxos/lru_set.hpp"
#include "paxos/pause.hpp"
#include "paxos/replicaset.hpp"
#include "paxos/signal.hpp"


namespace paxos
{


struct Context
{
};


struct ProposerContext : public Context
{
    std::shared_ptr<Ledger>& ledger;
    Field<Decree> highest_proposed_decree;
    std::shared_ptr<ReplicaSet>& replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_map_decree> promise_map;
    std::set<Decree, compare_decree> ntie_map;
    std::map<Decree, std::tuple<std::shared_ptr<ReplicaSet>, bool>, compare_map_decree> nprepare_map;
    paxos::lru_set<Decree, compare_root_decree> resume_map;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_map_decree> naccept_map;
    std::deque<std::tuple<std::string, DecreeType>> requested_values;

    std::mutex mutex;
    Decree highest_nacked_decree;
    Decree highest_nacktie_decree;
    std::chrono::high_resolution_clock::time_point nacktie_time;
    std::chrono::milliseconds interval;
    std::shared_ptr<Pause> pause;
    std::shared_ptr<Signal>& signal;

    ProposerContext(
        std::shared_ptr<ReplicaSet>& replicaset_,
        std::shared_ptr<Ledger>& ledger_,
        std::shared_ptr<Storage<Decree>> highest_proposed_decree_,
        std::shared_ptr<Pause> pause,
        std::shared_ptr<Signal>& signal
    )
        : ledger(ledger_),
          highest_proposed_decree(highest_proposed_decree_),
          replicaset(replicaset_),
          promise_map(),
          ntie_map(),
          nprepare_map(),
          naccept_map(),
          resume_map(256),
          requested_values(),
          mutex(),
          highest_nacked_decree(Replica(""), -1, "first", DecreeType::UserDecree),
          highest_nacktie_decree(Replica(""), -1, "first", DecreeType::UserDecree),
          nacktie_time(std::chrono::high_resolution_clock::now()),
          interval(std::chrono::milliseconds(1000)),
          pause(pause),
          signal(signal)
    {
    }
};


struct AcceptorContext : public Context
{
    Field<Decree> promised_decree;
    Field<Decree> accepted_decree;
    paxos::lru_set<Decree, compare_decree> accepted_set;
    std::chrono::high_resolution_clock::time_point accepted_time;
    std::chrono::milliseconds interval;
    std::mutex mutex;

    AcceptorContext(
        std::shared_ptr<Storage<Decree>> promised_decree_,
        std::shared_ptr<Storage<Decree>> accepted_decree_,
        std::chrono::milliseconds interval_
    )
        : promised_decree(promised_decree_),
          accepted_decree(accepted_decree_),
          accepted_set(256),
          accepted_time(std::chrono::high_resolution_clock::now()),
          interval(interval_),
          mutex()
    {
    }
};


struct LearnerContext : public Context
{
    std::shared_ptr<ReplicaSet>& replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_map_decree> accepted_map;
    std::shared_ptr<Ledger>& ledger;
    std::priority_queue<Decree, std::vector<Decree>, ascending_decree> tracked_future_decrees;
    bool is_observer;
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


}


#endif
