#ifndef __PAXOS_HPP_INCLUDED__
#define __PAXOS_HPP_INCLUDED__

#include <functional>
#include <map>
#include <memory>

#include <paxos/decree.hpp>
#include <paxos/replicaset.hpp>
#include <paxos/roles.hpp>
#include <paxos/sender.hpp>


//
// Handler that will be executed after a decree has been accepted. It
// is expected to be idempotent.
//
using DecreeHandler = std::function<void(std::string entry)>;


//
// Map of a decree ballots.
//
using AbsenteeBallots = std::map<Decree, std::shared_ptr<ReplicaSet>,
                                 compare_decree>;


class Parliament
{
public:

    Parliament(std::string location=".",
               DecreeHandler decree_handler=[](std::string entry){});

    void AddLegislator(std::string address, short port);

    void RemoveLegislator(std::string address, short port);

    void SendProposal(std::string entry);

    AbsenteeBallots GetAbsenteeBallots(int max_ballots);

private:

    std::shared_ptr<ReplicaSet> legislators;

    std::shared_ptr<Receiver> receiver;

    std::shared_ptr<Sender> sender;

    std::shared_ptr<Ledger> ledger;

    std::shared_ptr<ProposerContext> proposer;

    std::shared_ptr<AcceptorContext> acceptor;

    std::shared_ptr<LearnerContext> learner;

    std::shared_ptr<UpdaterContext> updater;

    std::string location;
};


#endif
