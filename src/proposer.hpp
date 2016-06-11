#ifndef __PROPOSER_HPP_INCLUDED__
#define __PROPOSER_HPP_INCLUDED__


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
    std::shared_ptr<ReplicaSet> replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> promise_map;

    ProposerContext()
        : ProposerContext(std::shared_ptr<ReplicaSet>(new ReplicaSet()))
    {
    }

    ProposerContext(std::shared_ptr<ReplicaSet> replicaset_)
        : highest_promised_decree(),
          replicaset(replicaset_),
          promise_map()
    {
    }
};


void RegisterProposer(
    std::shared_ptr<Receiver> receiver,
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


#endif
