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
};


void RegisterLearner(
    Receiver receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<LearnerContext> context);


void HandleProclaim(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender);
