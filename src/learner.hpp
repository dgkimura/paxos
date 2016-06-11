#ifndef __LEARNER_HPP_INCLUDED__
#define __LEARNER_HPP_INCLUDED__


#include <map>
#include <memory>

#include <callback.hpp>
#include <context.hpp>
#include <ledger.hpp>
#include <messages.hpp>
#include <receiver.hpp>
#include <replicaset.hpp>
#include <sender.hpp>


struct LearnerContext : public Context
{
    std::shared_ptr<ReplicaSet> replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> accepted_map;
    std::shared_ptr<Ledger> ledger;

    LearnerContext()
        : LearnerContext(std::shared_ptr<ReplicaSet>(new ReplicaSet()))
    {
    }

    LearnerContext(std::shared_ptr<ReplicaSet> replicaset_)
        : replicaset(replicaset_),
          accepted_map(),
          ledger()
    {
    }
};


void RegisterLearner(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<LearnerContext> context);


void HandleProclaim(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender);


void HandleUpdated(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender);


#endif
