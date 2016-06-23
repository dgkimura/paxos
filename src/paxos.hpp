#ifndef __PAXOS_HPP_INCLUDED__
#define __PAXOS_HPP_INCLUDED__


#include <memory>

#include <replicaset.hpp>
#include <roles.hpp>
#include <sender.hpp>


class Parliament
{
public:

    Parliament();

    void AddLegislator(Replica replica);

    void RemoveLegislator(Replica replica);

    void CreateProposal(std::string entry);

private:

    std::shared_ptr<ReplicaSet> legislators;

    std::shared_ptr<Receiver> receiver;

    std::shared_ptr<Sender> sender;

    std::shared_ptr<LedgerType> ledger;

    std::shared_ptr<ProposerContext> proposer;

    std::shared_ptr<AcceptorContext> acceptor;

    std::shared_ptr<LearnerContext> learner;

    std::shared_ptr<UpdaterContext> updater;
};


#endif
