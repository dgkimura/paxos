#include <map>
#include <memory>

#include <callback.hpp>
#include <context.hpp>
#include <messages.hpp>
#include <receiver.hpp>
#include <replicaset.hpp>
#include <sender.hpp>


struct ProposerContext : public Context
{
    Decree highest_promised_decree;
    ReplicaSet full_replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> promise_map;
};


void RegisterProposer(
    Receiver receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<ProposerContext> context);


void HandleRequest(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender);


void HandlePromise(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender);


void HandleAccepted(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender);
