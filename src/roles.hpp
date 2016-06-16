#ifndef __ROLES_HPP_INCLUDED__
#define __ROLES_HPP_INCLUDED__


#include <map>
#include <memory>

#include <callback.hpp>
#include <context.hpp>
#include <ledger.hpp>
#include <messages.hpp>
#include <receiver.hpp>
#include <replicaset.hpp>
#include <sender.hpp>


struct ProposerContext : public Context
{
    Decree highest_promised_decree;
    std::shared_ptr<ReplicaSet> replicaset;
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> promise_map;
    std::shared_ptr<Ledger> ledger;

    ProposerContext()
        : ProposerContext(std::make_shared<ReplicaSet>(), std::make_shared<PersistentLedger>())
    {
    }

    ProposerContext(std::shared_ptr<ReplicaSet> replicaset_, std::shared_ptr<Ledger> ledger_)
        : highest_promised_decree(),
          replicaset(replicaset_),
          promise_map(),
          ledger(ledger_)
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
    std::shared_ptr<Ledger> ledger;

    LearnerContext()
        : LearnerContext(
            std::make_shared<ReplicaSet>(),
            std::make_shared<PersistentLedger>()
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
};


void RegisterProposer(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<ProposerContext> context);


void RegisterAcceptor(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<AcceptorContext> context);


void RegisterLearner(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<LearnerContext> context);


void RegisterUpdater(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<UpdaterContext> context);


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


void HandlePrepare(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender);


void HandleAccept(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender);


void HandleProclaim(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender);


void HandleUpdated(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender);


void HandleUpdate(
    Message message,
    std::shared_ptr<UpdaterContext> context,
    std::shared_ptr<Sender> sender);


#endif
