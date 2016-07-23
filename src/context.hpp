#ifndef __CONTEXT_HPP_INCLUDED__
#define __CONTEXT_HPP_INCLUDED__


#include <atomic>
#include <map>
#include <memory>
#include <vector>

#include <decree.hpp>
#include <fields.hpp>
#include <ledger.hpp>
#include <replicaset.hpp>


struct Context
{
};


struct ProposerContext : public Context
{
    std::atomic<int> current_decree_number;
    Decree highest_proposed_decree;
    std::shared_ptr<ReplicaSet> replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> promise_map;
    std::vector<std::string> requested_values;

    ProposerContext()
        : ProposerContext(std::make_shared<ReplicaSet>(), 1)
    {
    }

    ProposerContext(std::shared_ptr<ReplicaSet> replicaset_, int current_decree_number_)
        : current_decree_number(current_decree_number_),
          highest_proposed_decree(),
          replicaset(replicaset_),
          promise_map(),
          requested_values()
    {
    }
};


struct AcceptorContext : public Context
{
    Field<Decree> promised_decree;
    Field<Decree> accepted_decree;

    AcceptorContext()
        : AcceptorContext(
            std::make_shared<PersistentDecree>("promised_decree"),
            std::make_shared<PersistentDecree>("accepted_decree")
          )
    {
    }

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
    std::shared_ptr<ReplicaSet> replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> accepted_map;
    std::shared_ptr<Ledger> ledger;
    std::vector<Decree> tracked_future_decrees;

    LearnerContext()
        : LearnerContext(
            std::make_shared<ReplicaSet>(),
            std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>())
          )
    {
    }

    LearnerContext(
        std::shared_ptr<ReplicaSet> replicaset_,
        std::shared_ptr<Ledger> ledger_
    )
        : replicaset(replicaset_),
          accepted_map(),
          ledger(ledger_)
    {
    }
};


struct UpdaterContext : public Context
{
    std::shared_ptr<Ledger> ledger;

    UpdaterContext()
        : UpdaterContext(
            std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>())
          )
    {
    }

    UpdaterContext(
        std::shared_ptr<Ledger> ledger_
    )
        : ledger(ledger_)
    {
    }
};


#endif
