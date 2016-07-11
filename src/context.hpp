#ifndef __CONTEXT_HPP_INCLUDED__
#define __CONTEXT_HPP_INCLUDED__


#include <map>
#include <memory>
#include <vector>

#include <decree.hpp>
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
    Decree promised_decree;
    Decree accepted_decree;
};


struct LearnerContext : public Context
{
    std::shared_ptr<ReplicaSet> replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> accepted_map;
    std::shared_ptr<LedgerType> ledger;

    LearnerContext()
        : LearnerContext(
            std::make_shared<ReplicaSet>(),
            std::make_shared<PersistentLedger>()
          )
    {
    }

    LearnerContext(
        std::shared_ptr<ReplicaSet> replicaset_,
        std::shared_ptr<LedgerType> ledger_
    )
        : replicaset(replicaset_),
          accepted_map(),
          ledger(ledger_)
    {
    }
};


struct UpdaterContext : public Context
{
};


#endif
